#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        bin_util.c
 * @brief       This file contains common functions of plugins/bin of BEYE project.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "beye.h"
#include "bin_util.h"
#include "reg_form.h"
#include "beyeutil.h"
#include "bmfile.h"
#include "bconsole.h"
#include "tstrings.h"
#include "plugins/disasm.h"

namespace beye {
linearArray *PubNames = NULL;

tCompare __FASTCALL__ fmtComparePubNames(const void  *v1,const void  *v2)
{
  const struct PubName  *pnam1, *pnam2;
  pnam1 = (const struct PubName  *)v1;
  pnam2 = (const struct PubName  *)v2;
  return __CmpLong__(pnam1->pa,pnam2->pa);
}

bool __FASTCALL__ fmtFindPubName(BFile& fmt_cache,char *buff,unsigned cb_buff,
		   __filesize_t pa,
		   ReadPubNameList fmt_readlist,
		   ReadPubName fmt_readpub)
{
  struct PubName *ret,key;
  key.pa = pa;
  if(!PubNames) (*fmt_readlist)(fmt_cache,MemOutBox);
  ret = (PubName*)la_Find(PubNames,&key,fmtComparePubNames);
  if(ret)
  {
    (*fmt_readpub)(fmt_cache,ret,buff,cb_buff);
    return true;
  }
  return udnFindName(pa,buff,cb_buff);
}

__filesize_t __FASTCALL__ fmtGetPubSym(BFile& fmt_cache,char *str,unsigned cb_str,
			   unsigned *func_class,__filesize_t pa,bool as_prev,
			   ReadPubNameList fmt_readlist,
			   ReadPubName fmt_readpub)
{
  __filesize_t cfpos,ret_addr,cur_addr;
  unsigned long i,idx,nitems;
  struct PubName key,*it;
  cfpos = bmGetCurrFilePos();
  if(!PubNames) (*fmt_readlist)(fmt_cache,NULL);
  if(!PubNames->nItems) return 0;
  ret_addr = 0L;
  idx = UINT_MAX;
  key.pa = pa;
  i = (unsigned)la_FindNearest(PubNames,&key,fmtComparePubNames);
  nitems = PubNames->nItems;
  if(as_prev) idx = i;
  else
  {
    static unsigned long multiref_i = 0;
    get_next:
    while((cur_addr = ((struct PubName  *)PubNames->data)[i].pa) <= pa)
    {
      i++;
      if((cur_addr == pa && i > multiref_i) || (i >= nitems - 1)) break;
    }
    idx = i;
    if(idx < PubNames->nItems) ret_addr = cur_addr;
    else ret_addr = 0L;
    if(ret_addr && ret_addr == pa)
    {
      if(idx <= multiref_i) { i = idx; goto get_next; }
      else multiref_i = idx;
    }
    else multiref_i = 0;
  }
  if(idx < PubNames->nItems)
  {
    ret_addr = ((struct PubName  *)PubNames->data)[idx].pa;
    *func_class = ((struct PubName  *)PubNames->data)[idx].attr;
    if(!idx && pa < ret_addr && as_prev)
    {
      ret_addr = 0;
    }
    else
    {
      it = &((struct PubName  *)PubNames->data)[idx];
      (*fmt_readpub)(fmt_cache,it,str,cb_str);
      str[cb_str-1] = 0;
    }
  }
  bmSeek(cfpos,BFile::Seek_Set);
  return ret_addr;
}

static BFile*  __FASTCALL__ ReopenSeek(__filesize_t dist)
{
 BFile* handle;
 handle = bmbioHandle().dup_ex(BBIO_SMALL_CACHE_SIZE);
 if(handle != &bNull) handle->seek(dist,BFile::Seek_Set);
 else                 errnoMessageBox(READ_FAIL,NULL,errno);
 return handle;
}

int __FASTCALL__ fmtShowList( GetNumItems gni,ReadItems ri,const char * title,int flags,unsigned * ordinal)
{
 int ret;
 bool bval;
 BFile* handle;
 unsigned nnames;
 memArray * obj;
 TWindow* w;
 ret = -1;
 if((handle = ReopenSeek(0)) == &bNull) return ret;
 nnames = gni ? (*gni)(*handle) : (unsigned)-1;
 if(!(obj = ma_Build(nnames,true))) goto exit;
 w = PleaseWaitWnd();
 bval = (*ri)(*handle,obj,nnames);
 CloseWnd(w);
 if(bval)
 {
   if(!obj->nItems) { NotifyBox(NOT_ENTRY,title); goto exit; }
   if(flags)
   {
     ret = ma_Display(obj,title,flags,-1);
     if(ordinal && ret != -1)
     {
       const char* cptr;
       char buff[40];
       cptr = strrchr((char*)obj->data[ret],LB_ORD_DELIMITER);
       cptr++;
       strcpy(buff,cptr);
       *ordinal = atoi(buff);
     }
   }
   else    { ret = -1; ma_Display(obj,title,LB_SORTABLE,-1); }
 }
 ma_Destroy(obj);
 exit:
 delete handle;
 return ret;
}

/* User Defined names (UDN) */

typedef struct tagUDN {
    char		name[256];
    __filesize_t	offset;
}udn;

static tCompare __FASTCALL__ udn_compare(const void  *e1,const void  *e2)
{
    const udn  *p1 = (const udn  *)e1;
    const udn  *p2 = (const udn  *)e2;
    return p1->offset<p2->offset?-1:p1->offset>p2->offset?1:0;
}

static linearArray *udn_list=NULL;
static bool udn_modified=false;
static char udn_fname[4096];

static bool __FASTCALL__ udnAddItem( void ) {
    __filesize_t off;
    udn item,*prev;
    char ud_name[256],prompt[256];
    off = BMGetCurrFilePos();
    sprintf(prompt," Name for %08X offset: ",off);
    prev=NULL;
    ud_name[0]='\0';
    if(udn_list) {
	item.name[255]='\0';
	item.offset=off;
	prev = (udn*)la_Find(udn_list,&item,udn_compare);
	if(prev) strcpy(ud_name,prev->name);
    }
    if(GetStringDlg(ud_name,prompt," [ENTER] - Proceed ",NAME_MSG))
    {
	if(!udn_list) udn_list=la_Build(0,sizeof(udn),NULL);
	if(udn_list) {
	    if(prev) strcpy(prev->name,ud_name);
	    else {
		strcpy(item.name,ud_name);
		item.offset=off;
		la_AddData(udn_list,&item,NULL);
	    }
	    la_Sort(udn_list,udn_compare);
	}
	udn_modified=true;
	return true;
    }
    return false;
}

static unsigned __FASTCALL__ udnGetNumItems(BFile& handle) {
    UNUSED(handle);
    return udn_list->nItems;
}

static bool    __FASTCALL__ udnReadItems(BFile& handle,memArray * names,unsigned nnames)
{
    char stmp[256];
    unsigned i;
    UNUSED(handle);
    for(i=0;i<nnames;i++) {
	sprintf(stmp,"%-40s %08lX"
		,((udn *)udn_list->data)[i].name
		,(unsigned long)((udn *)udn_list->data)[i].offset);
	if(!ma_AddString(names,stmp,true)) break;
    }
    return true;
}

static bool __FASTCALL__ udnDeleteItem( void ) {
  int rval=-1;
  if(udn_list) {
    rval = fmtShowList(udnGetNumItems,udnReadItems,
		    " User-defined Names (aka bookmarks) ",
		    LB_SELECTIVE,NULL);
    if(rval!=-1) {
	la_DeleteData(udn_list,rval);
	la_Sort(udn_list,udn_compare);
	udn_modified=true;
    }
  }
  else ErrMessageBox("UDN list is empty!",NULL);
  return rval==-1?false:true;
}

bool __FASTCALL__ udnSelectName(__filesize_t *off) {
  int rval=-1;
  if(udn_list) {
    rval = fmtShowList(udnGetNumItems,udnReadItems,
		    " User-defined Names (aka bookmarks) ",
		    LB_SELECTIVE,NULL);
    if(rval!=-1) *off = ((udn *)udn_list->data)[rval].offset;
  }
  else ErrMessageBox("UDN list is empty!",NULL);
  return rval==-1?false:true;
}

bool __FASTCALL__ udnFindName(__filesize_t pa,char *buff, unsigned cb_buff) {
    udn *item;
    udn key;
    if(udn_list) {
	key.name[0]='\0';
	key.offset = pa;
	item=(udn*)la_Find(udn_list,&key,udn_compare);
	if(item) {
	    strncpy(buff,item->name,cb_buff);
	    buff[cb_buff-1]='\0';
	    return true;
	}
    }
    return false;
}

bool __FASTCALL__ __udnSaveList( void )
{
    unsigned i;
    if(udn_list) {
	FILE *out;
	if((out = fopen(udn_fname,"wt"))!=NULL) {
	    fprintf(out,"; This is an automatically generated list of user-defined names\n"
			"; for: %s\n"
			"; by Beye-%s\n"
			,BMName()
			,BEYE_VERSION);
	    for(i=0;i<udn_list->nItems;i++)
		fprintf(out,"%016llX:%s\n"
		,((udn *)udn_list->data)[i].offset
		,((udn *)udn_list->data)[i].name);
	    fclose(out);
	    udn_modified=false;
	    return true;
	}
	else {
	    char stmp[256];
	    sprintf(stmp,"Can't open file: %s\n",strerror(errno));
	    ErrMessageBox(udn_fname,stmp);
	}
    }
    return false;
}


bool __FASTCALL__ udnSaveList( void ) {
    if(GetStringDlg(udn_fname," Please enter file name: "," [ENTER] - Proceed ",NAME_MSG))
    {
	if(udn_list)	return __udnSaveList();
	else		ErrMessageBox("UDN list is empty!",NULL);
    }
    return false;
}

bool __FASTCALL__  __udnLoadList( void ) {
    unsigned i;
    udn item;
    FILE *in;
    if((in = fopen(udn_fname,"rt"))!=NULL) {
	    char buff[4096],*brk;
	    unsigned blen;
	    i = 0;
	    while(!feof(in)) {
		buff[0]='\0';
		fgets(buff,sizeof(buff),in);
		i++;
		if(buff[0]==';'||buff[0]=='\0') continue;
		brk=strchr(buff,':');
		if(!brk) {
		    char stmp[256];
		    sprintf(stmp,"Can't recognize line: %u",i);
		    ErrMessageBox(stmp,NULL);
		    return true;
		}
		*brk='\0';
		sscanf(buff,"%016llX",&item.offset);
		strncpy(item.name,brk+1,sizeof(item.name));
		item.name[sizeof(item.name)-1]='\0';
		blen = strlen(item.name);
		while(item.name[blen-1]==10||item.name[blen-1]==13) {
		    item.name[blen-1]='\0'; blen--;
		}
		if(!udn_list) udn_list = la_Build(0,sizeof(udn),NULL);
		if(udn_list)  la_AddData(udn_list,&item,NULL);
		else break;
	    }
	    fclose(in);
	    if(udn_list) la_Sort(udn_list,udn_compare);
	    return true;
	}
	else {
	    char stmp[256];
	    sprintf(stmp,"Can't open file: %s\n",strerror(errno));
	    ErrMessageBox(udn_fname,stmp);
    }
    return false;
}

bool __FASTCALL__ udnLoadList( void ) {
    if(GetStringDlg(udn_fname," Please enter file name: "," [ENTER] - Proceed ",NAME_MSG))
    {
	if(udn_list)	return __udnLoadList();
	else		ErrMessageBox("UDN list is empty!",NULL);
    }
    return false;
}

static const char *udn_operations[] =
{
    "~Add item",
    "~Delete item",
    "~Load list from file",
    "~Save list to file"
};
typedef bool (__FASTCALL__ *udnFunc)( void );

static udnFunc udn_funcs[] =
{
    udnAddItem,
    udnDeleteItem,
    udnLoadList,
    udnSaveList
};

bool __FASTCALL__ udnUserNames( void ) {
  unsigned nModes;
  int i;
  nModes = sizeof(udn_operations)/sizeof(char *);
  i = 0;
  i = SelBoxA(const_cast<char**>(udn_operations),nModes," Select operation: ",i);
  if(i != -1)
  {
     int ret;
     TWindow * w;
     w = PleaseWaitWnd();
     ret = (*udn_funcs[i])();
     CloseWnd(w);
     return ret;
  }
  return false;
}

void __FASTCALL__ udnInit( hIniProfile *ini ) {
  udn_fname[0]='\0';
  if(beye_context().is_valid_ini_args())
  {
    beye_context().read_profile_string(ini,"Beye","Browser","udn_list","",udn_fname,sizeof(udn_fname));
    if(udn_fname[0]) __udnLoadList();
  }
}

void __FASTCALL__ udnTerm( hIniProfile *ini ) {
  if(udn_list) {
    if(udn_modified) {
	WarnMessageBox("User-defined list of names was not saved",NULL);
	udnSaveList();
    }
    la_Destroy(udn_list);
  }
  beye_context().write_profile_string(ini,"Beye","Browser","udn_list",udn_fname);
}
} // namespace beye
