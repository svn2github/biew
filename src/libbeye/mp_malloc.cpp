#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;

#include <algorithm>
#include <iostream>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <malloc.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

namespace beye {

enum { Max_BackTraces=13 };

struct mp_slot_t {
    any_t*	page_ptr;
    size_t	size;
    size_t	ncalls;
    any_t*	calls[Max_BackTraces];
};

struct mp_slot_container_t {
    mp_slot_t*	slots;
    size_t	nslots;
    size_t	size;
};

struct priv_t {
    const char*			argv0;
    unsigned			rnd_limit;
    unsigned			every_nth_call;
    enum mp_malloc_e		flags;
    /* for randomizer */
    unsigned long long int	total_calls;
    /* backtrace and protector slots */
    mp_slot_container_t		mallocs;/* not freed mallocs */
    mp_slot_container_t		reallocs; /* suspect reallocs */
    mp_slot_container_t		frees;    /* suspect free */
};
static priv_t* priv=NULL;

static any_t* prot_page_align(any_t *ptr) { return (any_t*)(((unsigned long)ptr)&(~(__VM_PAGE_SIZE__-1))); }
static size_t app_fullsize(size_t size) {
    unsigned npages = size/__VM_PAGE_SIZE__;
    unsigned fullsize;
    if(size%__VM_PAGE_SIZE__) npages++;
    npages++;
    fullsize=npages*__VM_PAGE_SIZE__;
    return fullsize;
}
static any_t* prot_last_page(any_t* rp,size_t fullsize) { return reinterpret_cast<any_t*>(reinterpret_cast<long>(rp)+(fullsize-__VM_PAGE_SIZE__)); }
static void __prot_print_slots(mp_slot_container_t* c) {
    size_t i;
    for(i=0;i<c->nslots;i++) {
	std::cerr<<"slot["<<i<<"] address: "<<c->slots[i].page_ptr<<" size: "<<c->slots[i].size<<std::endl;
    }
}

static size_t	prot_find_slot_idx(mp_slot_container_t* c,any_t* ptr) {
    size_t i;
    for(i=0;i<c->nslots;i++) {
	if(c->slots[i].page_ptr==ptr) return i;
    }
    return UINT_MAX;
}

static mp_slot_t*	prot_find_slot(mp_slot_container_t* c,any_t* ptr) {
    size_t idx=prot_find_slot_idx(c,ptr);
    if(idx!=UINT_MAX) return &c->slots[idx];
    return NULL;
}

static mp_slot_t*	prot_append_slot(mp_slot_container_t* c,any_t*ptr,size_t size) {
    mp_slot_t* s;
    s=c->slots;
    if(!c->slots) {
	c->size=16;
	s=(mp_slot_t*)malloc(sizeof(mp_slot_t)*16);
    }
    else {
	if(c->nslots+1>c->size) {
	    c->size+=16;
	    s=(mp_slot_t*)realloc(c->slots,sizeof(mp_slot_t)*c->size);
	}
    }
    c->slots=s;
    ::memset(&c->slots[c->nslots],0,sizeof(mp_slot_t));
    c->slots[c->nslots].page_ptr=ptr;
    c->slots[c->nslots].size=size;
    c->nslots++;
    return &c->slots[c->nslots-1];
}

static void	prot_free_slot(mp_slot_container_t* c,any_t* ptr) {
    size_t idx=prot_find_slot_idx(c,ptr);
    if(idx!=UINT_MAX) {
	::memmove(&c->slots[idx],&c->slots[idx+1],sizeof(mp_slot_t)*(c->nslots-(idx+1)));
	c->nslots--;
	if(c->nslots<c->size-16) {
	    c->size-=16;
	    c->slots=(mp_slot_t*)realloc(c->slots,sizeof(mp_slot_t)*c->size);
	}
    } else printf("[prot_free_slot] Internal error! Can't find slot for address: %p\n",ptr);
}

/* ----------- append ------------ */

static any_t* __prot_malloc_append(size_t size) {
    any_t* rp;
    size_t fullsize=app_fullsize(size);
    rp=::memalign(__VM_PAGE_SIZE__,fullsize);
    if(rp) {
	prot_append_slot(&priv->mallocs,rp,size);
	// protect last page here
	::mprotect(prot_last_page(rp,fullsize),__VM_PAGE_SIZE__,MP_DENY_ALL);
	rp=reinterpret_cast<any_t*>(reinterpret_cast<long>(rp)+fullsize-__VM_PAGE_SIZE__-size);
    }
    return rp;
}

static any_t* __prot_memalign_append(size_t boundary,size_t size) { UNUSED(boundary);  return __prot_malloc_append(size); }

/* REPORT */
typedef struct bt_cache_entry_s {
    any_t*	addr;
    char*	str;
}bt_cache_entry_t;

typedef struct bt_cache_s {
    bt_cache_entry_t*	entry;
    unsigned		num_entries;
}bt_cache_t;

static bt_cache_t*	init_bt_cache(void) { return (bt_cache_t*)calloc(1,sizeof(bt_cache_t)); }

static void		uninit_bt_cache(bt_cache_t* cache) {
    unsigned i;
    for(i=0;i<cache->num_entries;i++) free(cache->entry[i].str);
    ::free(cache->entry);
    ::free(cache);
}

static char* bt_find_cache(bt_cache_t* cache,any_t* ptr) {
    unsigned i;
    for(i=0;i<cache->num_entries;i++) if(cache->entry[i].addr==ptr) return cache->entry[i].str;
    return NULL;
}

static bt_cache_entry_t* bt_append_cache(bt_cache_t* cache,any_t* ptr,const char *str) {
    if(!cache->entry)	cache->entry=(bt_cache_entry_t*)malloc(sizeof(bt_cache_entry_t));
    else		cache->entry=(bt_cache_entry_t*)realloc(cache->entry,sizeof(bt_cache_entry_t)*(cache->num_entries+1));
    cache->entry[cache->num_entries].addr=ptr;
    cache->entry[cache->num_entries].str=strdup(str);
    cache->num_entries++;
    return &cache->entry[cache->num_entries-1];
}

static char* exec_addr2line(any_t*ptr, char* result,unsigned len) {
    unsigned i;
    char ch,cmd[4096];
    ::sprintf(cmd,"addr2line -s -e %s %p\n",priv->argv0,ptr);
    FILE* in=::popen(cmd,"r");
    if(!in) return NULL;
    i=0;
    while(1) {
	ch=::fgetc(in);
	if(::feof(in)) break;
	if(ch=='\n') break;
	result[i++]=ch;
	if(i>=len-1) break;
    }
    result[i]='\0';
    ::pclose(in);
    return result;
}

static char* addr2line(bt_cache_t* cache,any_t*ptr) {
    char *rs;
    if(priv->argv0) {
	bt_cache_entry_t* centry;
	char result[4096];
	if((rs=bt_find_cache(cache,ptr))!=NULL) return rs;
	rs=exec_addr2line(ptr,result,sizeof(result));
	if(!rs) return NULL;
	centry=bt_append_cache(cache,ptr,result);
	return centry->str;
    }
    return NULL;
}

static __always_inline void __print_backtrace(unsigned num) {
    bt_cache_t* cache=init_bt_cache();
    any_t*	calls[num];
    unsigned	i,ncalls;
    ncalls=::backtrace(calls,num);
    std::cerr<<"*** Backtrace for suspect call ***"<<std::endl;
    for(i=0;i<ncalls;i++) {
	std::cerr<<"    "<<calls[i]<<" -> "<<addr2line(cache,calls[i])<<std::endl;
    }
    uninit_bt_cache(cache);
}

void print_backtrace(const std::string& why,any_t** stack,unsigned num) {
    char result[4096];
    unsigned	i;
    std::cerr<<(!why.empty()?why.c_str():"*** Backtrace for suspect call ***")<<std::endl;
    for(i=0;i<num;i++) {
	std::cerr<<"    "<<stack[i]<<" -> "<<exec_addr2line(stack[i],result,sizeof(result))<<std::endl;
    }
}

static void __prot_free_append(any_t*ptr) {
    any_t *page_ptr=prot_page_align(ptr);
    mp_slot_t* slot=prot_find_slot(&priv->mallocs,page_ptr);
    if(!slot) {
	::printf("[__prot_free_append] suspect call found! Can't find slot for address: %p [aligned: %p]\n",ptr,page_ptr);
	__prot_print_slots(&priv->mallocs);
	__print_backtrace(Max_BackTraces);
	::kill(::getpid(), SIGILL);
    }
    size_t fullsize=app_fullsize(slot->size);
    ::mprotect(prot_last_page(page_ptr,fullsize),__VM_PAGE_SIZE__,MP_PROT_READ|MP_PROT_WRITE);
    ::free(page_ptr);
    prot_free_slot(&priv->mallocs,page_ptr);
}

static any_t* __prot_realloc_append(any_t*ptr,size_t size) {
    any_t* rp;
    if((rp=__prot_malloc_append(size))!=NULL && ptr) {
	mp_slot_t* slot=prot_find_slot(&priv->mallocs,prot_page_align(ptr));
	if(!slot) {
	    ::printf("[__prot_realloc_append] suspect call found! Can't find slot for address: %p [aligned: %p]\n",ptr,prot_page_align(ptr));
	    __prot_print_slots(&priv->mallocs);
	    __print_backtrace(Max_BackTraces);
	    ::kill(::getpid(), SIGILL);
	}
	::memcpy(rp,ptr,std::min(slot->size,size));
	__prot_free_append(ptr);
    }
    return rp;
}

/* ----------- prepend ------------ */
static any_t* pre_page_align(any_t *ptr) { return (any_t*)((unsigned long)ptr-__VM_PAGE_SIZE__); }
static size_t pre_fullsize(size_t size) { return size+__VM_PAGE_SIZE__; }

static any_t* __prot_malloc_prepend(size_t size) {
    any_t* rp;
    size_t fullsize=pre_fullsize(size);
    rp=::memalign(__VM_PAGE_SIZE__,fullsize);
    if(rp) {
	prot_append_slot(&priv->mallocs,rp,size);
	// protect last page here
	::mprotect(rp,__VM_PAGE_SIZE__,MP_DENY_ALL);
	rp=reinterpret_cast<any_t*>(reinterpret_cast<long>(rp)+__VM_PAGE_SIZE__);
    }
    return rp;
}

static any_t* __prot_memalign_prepend(size_t boundary,size_t size) { UNUSED(boundary);  return __prot_malloc_prepend(size); }

static void __prot_free_prepend(any_t*ptr) {
    any_t *page_ptr=pre_page_align(ptr);
    mp_slot_t* slot=prot_find_slot(&priv->mallocs,page_ptr);
    if(!slot) {
	::printf("[__prot_free_prepend] suspect call found! Can't find slot for address: %p [aligned: %p]\n",ptr,page_ptr);
	__prot_print_slots(&priv->mallocs);
	__print_backtrace(Max_BackTraces);
	::kill(::getpid(), SIGILL);
    }
    ::mprotect(page_ptr,__VM_PAGE_SIZE__,MP_PROT_READ|MP_PROT_WRITE);
    ::free(page_ptr);
    prot_free_slot(&priv->mallocs,page_ptr);
}

static any_t* __prot_realloc_prepend(any_t*ptr,size_t size) {
    any_t* rp;
    if((rp=__prot_malloc_prepend(size))!=NULL && ptr) {
	mp_slot_t* slot=prot_find_slot(&priv->mallocs,pre_page_align(ptr));
	if(!slot) {
	    ::printf("[__prot_realloc_prepend] suspect call found! Can't find slot for address: %p [aligned: %p]\n",ptr,pre_page_align(ptr));
	    __prot_print_slots(&priv->mallocs);
	    __print_backtrace(Max_BackTraces);
	    ::kill(getpid(), SIGILL);
	}
	::memcpy(rp,ptr,std::min(slot->size,size));
	__prot_free_prepend(ptr);
    }
    return rp;
}

static any_t* prot_malloc(size_t size) {
    any_t* rp;
    if(priv->flags&MPA_FLG_BOUNDS_CHECK) rp=__prot_malloc_append(size);
    else				 rp=__prot_malloc_prepend(size);
    return rp;
}

static any_t* prot_memalign(size_t boundary,size_t size) {
    any_t* rp;
    if(priv->flags&MPA_FLG_BOUNDS_CHECK) rp=__prot_memalign_append(boundary,size);
    else				 rp=__prot_memalign_prepend(boundary,size);
    return rp;
}

static any_t* prot_realloc(any_t*ptr,size_t size) {
    any_t* rp;
    if(priv->flags&MPA_FLG_BOUNDS_CHECK) rp=__prot_realloc_append(ptr,size);
    else				 rp=__prot_realloc_prepend(ptr,size);
    return rp;
}

static void prot_free(any_t*ptr) {
    if(priv->flags&MPA_FLG_BOUNDS_CHECK) __prot_free_append(ptr);
    else				 __prot_free_prepend(ptr);
}

static __always_inline any_t* bt_malloc(size_t size) {
    any_t*rp;
    mp_slot_t* slot;
    rp=::malloc(size);
    if(rp) {
	slot=prot_append_slot(&priv->mallocs,rp,size);
	slot->ncalls=::backtrace(slot->calls,Max_BackTraces);
    }
    return rp;
}

static __always_inline any_t* bt_memalign(size_t boundary,size_t size) {
    any_t*rp;
    rp=::memalign(boundary,size);
    if(rp) {
	mp_slot_t* slot;
	slot=prot_append_slot(&priv->mallocs,rp,size);
	slot->ncalls=::backtrace(slot->calls,Max_BackTraces);
    }
    return rp;
}

static __always_inline any_t* bt_realloc(any_t*ptr,size_t size) {
    any_t* rp;
    mp_slot_t* slot;
    if(!ptr) return bt_malloc(size);
    rp=::realloc(ptr,size);
    if(rp) {
	slot=prot_find_slot(&priv->mallocs,ptr);
	if(!slot) {
	    std::cerr<<"[bt_realloc] suspect call found! Can't find slot for address: "<<ptr<<std::endl;
	    mp_slot_t* _slot;
	    _slot=prot_append_slot(&priv->reallocs,ptr,size);
	    _slot->ncalls=::backtrace(_slot->calls,Max_BackTraces);
	} else {
	    slot->page_ptr=rp; // update address after realloc
	    slot->size=size;
	}
    }
    return rp;
}

static __always_inline void bt_free(any_t*ptr) {
    mp_slot_t* slot=prot_find_slot(&priv->mallocs,ptr);
    if(!slot) {
	std::cerr<<"[bt_free] suspect call found! Can't find slot for address: "<<ptr<<std::endl;
	mp_slot_t* _slot;
	_slot=prot_append_slot(&priv->frees,ptr,0);
	_slot->ncalls=::backtrace(_slot->calls,Max_BackTraces);
	return;
    }
    prot_free_slot(&priv->mallocs,ptr);
    ::free(ptr);
}

/*======== STATISTICS =======*/

static void bt_print_slots(bt_cache_t* cache,mp_slot_container_t* c) {
    size_t i,j;
    for(i=0;i<c->nslots;i++) {
	char *s;
	int printable=1;
	std::cerr<<"address: "<<c->slots[i].page_ptr<<" size: "<<c->slots[i].size<<" dump: ";
	s=reinterpret_cast<char *>(c->slots[i].page_ptr);
	for(j=0;j<std::min(c->slots[i].size,size_t(20));j++) {
	    if(!::isprint(s[j])) {
		printable=0;
		break;
	    }
	}
	if(printable) std::cerr<<std::string(s).substr(0,20)<<std::endl;
	else for(j=0;j<std::min(c->slots[i].size,size_t(Max_BackTraces));j++) {
	    std::cerr<<(unsigned char)s[j]<<" ";
	}
	std::cerr<<std::endl;
	for(j=0;j<c->slots[i].ncalls;j++) {
	    std::cerr<<(j==0?"bt=>":"    ")<<c->slots[i].calls[j]<<" -> "<<addr2line(cache,c->slots[i].calls[j])<<std::endl;
	}
    }
}
/* ================== HEAD FUNCTIONS  ======================= */
void	mp_init_malloc(const std::string& argv0,unsigned rnd_limit,unsigned every_nth_call,enum mp_malloc_e flags)
{
    ::srand(::time(0));
    if(!priv) priv=(priv_t*)::malloc(sizeof(priv_t));
    ::memset(priv,0,sizeof(priv_t));
    priv->argv0=::strdup(argv0.c_str());
    priv->rnd_limit=rnd_limit;
    priv->every_nth_call=every_nth_call;
    priv->flags=flags;
}

void	mp_uninit_malloc(int verbose)
{
    int done=0;
    bt_cache_t* cache=init_bt_cache();
    if(priv->flags&MPA_FLG_BACKTRACE) {
	if(priv->mallocs.nslots) {
	    unsigned long total;
	    unsigned i;
	    total=0;
	    for(i=0;i<priv->mallocs.nslots;i++) total+=priv->mallocs.slots[i].size;
	    std::cerr<<"Warning! "<<priv->mallocs.nslots<<" slots were not freed. Totally "<<total<<" bytes was leaked"<<std::endl;
	}
	if(verbose) {
	    if(priv->mallocs.nslots) {
		std::cerr<<"****** List of malloced but not freed pointers *******"<<std::endl;
		bt_print_slots(cache,&priv->mallocs);
		done=1;
	    }
	    if(priv->reallocs.nslots) {
		std::cerr<<std::endl<<"****** List of suspect realloc() calls *******"<<std::endl;
		bt_print_slots(cache,&priv->reallocs);
		done=1;
	    }
	    if(priv->frees.nslots) {
		std::cerr<<std::endl<<"****** List of suspect free() calls *******"<<std::endl;
		bt_print_slots(cache,&priv->frees);
		done=1;
	    }
	} else {
	    if(priv->reallocs.nslots || priv->frees.nslots)
		std::cerr<<"*** Were found suspect calls of mp_realloc or mp_free  ***"<<std::endl;
		std::cerr<<"*** Most probably your copy of program contains viruses!!!"<<std::endl;
	}
    }
    if(done) std::cerr<<std::endl<<"For source lines you may also print in (gdb): list *0xADDRESS"<<std::endl;
    uninit_bt_cache(cache);
    if(priv->argv0) ::free((void*)priv->argv0);
    ::free(priv);
    priv=NULL;
}

any_t* mp_malloc(size_t __size)
{
    any_t* rb,*rnd_buff=NULL;
    if(!priv) mp_init_malloc("",1000,10,MPA_FLG_RANDOMIZER);
    if(priv->every_nth_call && priv->rnd_limit && !priv->flags) {
	if(priv->total_calls%priv->every_nth_call==0) {
	    rnd_buff=::malloc(::rand()%priv->rnd_limit);
	}
    }
    if(priv->flags&(MPA_FLG_BOUNDS_CHECK|MPA_FLG_BEFORE_CHECK)) rb=prot_malloc(__size);
    else if(priv->flags&MPA_FLG_BACKTRACE)			rb=bt_malloc(__size);
    else							rb=malloc(__size);
    if(rnd_buff) ::free(rnd_buff);
    priv->total_calls++;
    return rb;
}

/* randomizing of memalign is useless feature */
any_t*	__FASTCALL__ mp_memalign (size_t boundary, size_t __size)
{
    any_t* rb;
    if(!priv) mp_init_malloc("",1000,10,MPA_FLG_RANDOMIZER);
    if(priv->flags&(MPA_FLG_BOUNDS_CHECK|MPA_FLG_BEFORE_CHECK)) rb=prot_memalign(boundary,__size);
    else if(priv->flags&MPA_FLG_BACKTRACE)			rb=bt_memalign(boundary,__size);
    else							rb=memalign(boundary,__size);
    return rb;
}

any_t*	mp_realloc(any_t*__ptr, size_t __size) {
    any_t* rp;
    if(priv->flags&(MPA_FLG_BOUNDS_CHECK|MPA_FLG_BEFORE_CHECK)) rp=prot_realloc(__ptr,__size);
    else if(priv->flags&MPA_FLG_BACKTRACE)			rp=bt_realloc(__ptr,__size);
    else							rp=realloc(__ptr,__size);
    return rp;
}

void	mp_free(any_t*__ptr)
{
    // we really may have some number of pointers malloced before mp_init_malloc()
    // example: global constructors with using of overloaded operator new()
    if(priv) {
	if(__ptr) {
	    if(priv->flags&(MPA_FLG_BOUNDS_CHECK|MPA_FLG_BEFORE_CHECK))
		prot_free(__ptr);
	    else if(priv->flags&MPA_FLG_BACKTRACE)
		bt_free(__ptr);
	    else
		::free(__ptr);
	}
    } else ::free(__ptr);
}

/* ================ APPENDIX ==================== */

any_t*	mp_mallocz (size_t __size) {
    any_t* rp;
    rp=mp_malloc(__size);
    if(rp) ::memset(rp,0,__size);
    return rp;
}

char *	mp_strdup(const char *src) {
    char *rs=NULL;
    if(src) {
	unsigned len=strlen(src);
	rs=(char *)mp_malloc(len+1);
	if(rs) strcpy(rs,src);
    }
    return rs;
}

int __FASTCALL__ mp_mprotect(any_t* addr,size_t len,enum mp_prot_e flags)
{
    return ::mprotect(addr,len,flags);
}

} // namespace beye

using namespace beye;
#include <new>

any_t*	SECURE_NAME0(_mp_malloc)(size_t size) {
    any_t* ptr;
    ptr = mp_malloc(size);
    if(!ptr) {
	std::bad_alloc ba;
	throw ba;
    }
    return ptr;
}

any_t*	SECURE_NAME1(_mp_mallocz)(size_t size) {
    any_t* ptr;
    ptr = mp_mallocz(size);
    if(!ptr) {
	std::bad_alloc ba;
	throw ba;
    }
    return ptr;
}

any_t*	SECURE_NAME2(_mp_memalign)(size_t boundary,size_t size) {
    any_t* ptr;
    ptr = mp_memalign(boundary,size);
    if(!ptr) {
	std::bad_alloc ba;
	throw ba;
    }
    return ptr;
}

void	SECURE_NAME3(_mp_free)(any_t* ptr) {
    mp_free(ptr);
}

any_t* operator new(size_t size) throw(std::bad_alloc) { return SECURE_NAME0(_mp_malloc)(size); }
any_t* operator new(size_t size,const zeromemory_t&) { return SECURE_NAME1(_mp_mallocz)(size); }
any_t* operator new(size_t size,const alignedmemory_t&,size_t boundary) { return SECURE_NAME2(_mp_memalign)(boundary,size); }
any_t* operator new(size_t size,const std::nothrow_t&) { return mp_malloc(size); }
any_t* operator new[](size_t size) throw(std::bad_alloc) { return SECURE_NAME0(_mp_malloc)(size); }
any_t* operator new[](size_t size,const zeromemory_t&) { return SECURE_NAME1(_mp_mallocz)(size); }
any_t* operator new[](size_t size,const alignedmemory_t&,size_t boundary) { return SECURE_NAME2(_mp_memalign)(boundary,size); }
any_t* operator new[](size_t size,const std::nothrow_t&) { return mp_malloc(size); }
void   operator delete(any_t* p) throw() { SECURE_NAME3(_mp_free)(p); }
void   operator delete[](any_t* p) throw() { SECURE_NAME3(_mp_free)(p); }
