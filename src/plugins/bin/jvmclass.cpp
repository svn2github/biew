#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/jvmclass.c
 * @brief       This file contains implementation of decoder for Java's ClassFile
 *              file format.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2004
 * @note        Development, fixes and improvements
**/
#include <algorithm>

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bin_util.h"
#include "bmfile.h"
#include "colorset.h"
#include "codeguid.h"
#include "reg_form.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"

namespace beye {
enum {
    CONSTANT_UTF8		=1,
    CONSTANT_INTEGER		=3,
    CONSTANT_FLOAT		=4,
    CONSTANT_LONG		=5,
    CONSTANT_DOUBLE		=6,
    CONSTANT_CLASS		=7,
    CONSTANT_STRING		=8,
    CONSTANT_FIELDREF		=9,
    CONSTANT_METHODREF		=10,
    CONSTANT_INTERFACEMETHODREF	=11,
    CONSTANT_NAME_AND_TYPE	=12
};
typedef struct ClassFile_s
{
    uint32_t	magic;
    uint16_t	minor;
    uint16_t	major;
    uint16_t	constant_pool_count;
    /* constant_pool[] */
    uint16_t	access_flags;
    uint16_t	this_class;
    uint16_t	super_class;
    uint16_t	interfaces_count;
/*    u2 interfaces[interfaces_count]; */
    uint16_t	fields_count;
/*    field_info fields[fields_count]; */
    uint16_t	methods_count;
/*    method_info methods[methods_count]; */
    uint16_t	attributes_count;
/*    attribute_info attributes[attributes_count]; */
/* private data: */
    __filesize_t header_length;
    __filesize_t constants_offset;
    __filesize_t interfaces_offset;
    __filesize_t fields_offset;
    __filesize_t methods_offset;
    __filesize_t attributes_offset;
/* meta data: */
    __filesize_t data_offset;
    __filesize_t code_offset;
    __filesize_t attrcode_offset;
}ClassFile_t;

ClassFile_t jvm_header;

static BFile* jvm_cache;
static BFile* pool_cache;

inline uint16_t JVM_WORD(const uint16_t* cval,bool is_msbf) { return FMT_WORD(cval,is_msbf); }
inline uint32_t JVM_DWORD(const uint32_t* cval,bool is_msbf) { return FMT_DWORD(cval,is_msbf); }
inline uint64_t JVM_QWORD(const uint64_t* cval,bool is_msbf) { return FMT_DWORD(cval,is_msbf); }


static bool  __FASTCALL__ jvm_check_fmt()
{
  unsigned char id[4];
  bmReadBufferEx(id,sizeof(id),0,BFile::Seek_Set);
  /* Cafe babe !!! */
  return id[0]==0xCA && id[1]==0xFE && id[2]==0xBA && id[3]==0xBE && bmGetFLength()>=16;
}

static unsigned  __FASTCALL__ skip_constant(BFile& handle,unsigned char id)
{
    unsigned add;
    unsigned short sval;
    add=0;
    switch(id)
    {
	default:
	case CONSTANT_STRING:
	case CONSTANT_CLASS: handle.seek(2,BFile::Seek_Cur); break;
	case CONSTANT_INTEGER:
	case CONSTANT_FLOAT:
	case CONSTANT_FIELDREF:
	case CONSTANT_METHODREF:
	case CONSTANT_NAME_AND_TYPE:
	case CONSTANT_INTERFACEMETHODREF: handle.seek(4,BFile::Seek_Cur); break;
	case CONSTANT_LONG:
	case CONSTANT_DOUBLE: handle.seek(8,BFile::Seek_Cur); add=1; break;
	case CONSTANT_UTF8:
			sval=handle.read_word();
			sval=JVM_WORD(&sval,1);
			handle.seek(sval,BFile::Seek_Cur);
			break;
    }
    return add;
}

static void  __FASTCALL__ skip_constant_pool(BFile& handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned char id;
	id=handle.read_byte();
	i+=skip_constant(handle,id);
    }
}

static void  __FASTCALL__ get_utf8(BFile& handle,unsigned nidx,char *str,unsigned slen)
{
    unsigned char id;
    handle.seek(jvm_header.constants_offset,BFile::Seek_Set);
    skip_constant_pool(handle,nidx-1);
    id=handle.read_byte();
    if(id==CONSTANT_UTF8)
    {
	nidx=handle.read_word();
	nidx=JVM_WORD((uint16_t*)&nidx,1);
	nidx=std::min(nidx,slen-1);
	handle.read(str,nidx);
	str[nidx]=0;
    }
}

static void  __FASTCALL__ get_name(BFile& handle,char *str,unsigned slen)
{
    unsigned short nidx;
    nidx=handle.read_word();
    nidx=JVM_WORD(&nidx,1);
    get_utf8(handle,nidx,str,slen);
}

static char *  __FASTCALL__ get_class_name(BFile& handle,unsigned idx,char *str,unsigned slen)
{
    *str='\0';
    if(idx && idx<(unsigned)jvm_header.constant_pool_count-1)
    {
	unsigned char id;
	handle.seek(jvm_header.constants_offset,BFile::Seek_Set);
	skip_constant_pool(handle,idx-1);
	id=handle.read_byte();
	if(id==CONSTANT_CLASS) get_name(handle,str,slen);
    }
    return str;
}

static void  __FASTCALL__ skip_attributes(BFile& handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	uint32_t lval;
	handle.seek(2,BFile::Seek_Cur);
	lval=handle.read_dword();
	lval=JVM_DWORD(&lval,1);
	handle.seek(lval,BFile::Seek_Cur);
    }
}

static void  __FASTCALL__ skip_fields(unsigned nitems,int attr)
{
    unsigned i;
    __filesize_t fpos;
    for(i=0;i<nitems;i++)
    {
	unsigned short sval;
	bmSeek(6,BFile::Seek_Cur);
	sval=bmReadWord();
	sval=JVM_WORD(&sval,1);
	fpos=bmGetCurrFilePos();
	if(i==0)
	{
	    __filesize_t lval;
	    lval=sval?fpos+6:fpos;
	    if(attr) jvm_header.code_offset=lval;
	    else jvm_header.data_offset=lval;
	}
	skip_attributes(bmbioHandle(),sval);
    }
}

static bool __FASTCALL__ jvm_read_interfaces(BFile& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short id;
    char str[80];
    handle.seek(jvm_header.interfaces_offset,BFile::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	id=handle.read_word();
	fpos=handle.tell();
	id=JVM_WORD(&id,1);
	get_class_name(handle,id,str,sizeof(str));
	if(!ma_AddString(names,str,true)) break;
	handle.seek(fpos,BFile::Seek_Set);
    }
    return true;
}

static unsigned __FASTCALL__ jvm_get_num_interfaces(BFile& handle)
{
    UNUSED(handle);
    return jvm_header.interfaces_count;
}


static __filesize_t __FASTCALL__ ShowInterfaces()
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_interfaces,jvm_read_interfaces,
		    " interfaces ",
		    LB_SORTABLE,NULL);
  return fpos;
}

static bool __FASTCALL__ jvm_read_attributes(BFile& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    uint32_t len;
    char str[80],sout[100];
    handle.seek(jvm_header.attributes_offset,BFile::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	get_name(handle,str,sizeof(str));
	handle.seek(fpos+2,BFile::Seek_Set);
	len=handle.read_dword();
	len=JVM_DWORD(&len,1);
	sprintf(sout,"%08lXH %s",(long)len,str);
	if(!ma_AddString(names,sout,true)) break;
	handle.seek(len,BFile::Seek_Cur);
    }
    return true;
}

static unsigned __FASTCALL__ jvm_get_num_attributes(BFile& handle)
{
    UNUSED(handle);
    return jvm_header.attributes_count;
}


static __filesize_t  __FASTCALL__ __ShowAttributes(const std::string& title)
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_attributes,jvm_read_attributes,
		title,
		LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    unsigned i;
    bmSeek(jvm_header.attributes_offset,BFile::Seek_Set);
    for(i=0;i<(unsigned)ret+1;i++)
    {
	uint32_t len;
	fpos=bmGetCurrFilePos();
	bmSeek(fpos+2,BFile::Seek_Set);
	len=bmReadDWord();
	len=JVM_DWORD(&len,1);
	fpos+=6;
	bmSeek(len,BFile::Seek_Cur);
    }
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowAttributes()
{
    return __ShowAttributes(" length   attributes ");
}

static bool __FASTCALL__ jvm_read_methods(BFile& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char str[80],str2[80],sout[256];
    handle.seek(jvm_header.methods_offset,BFile::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	flg=handle.read_word();
	flg=JVM_WORD(&flg,1);
	get_name(handle,str,sizeof(str));
	handle.seek(fpos+4,BFile::Seek_Set);
	get_name(handle,str2,sizeof(str2));
	handle.seek(fpos+6,BFile::Seek_Set);
	sval=handle.read_word();
	acount=JVM_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str,str2);
	if(!ma_AddString(names,sout,true)) break;
    }
    return true;
}

static unsigned __FASTCALL__ jvm_get_num_methods(BFile& handle)
{
    UNUSED(handle);
    return jvm_header.methods_count;
}

static __filesize_t __FASTCALL__ ShowMethods()
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_methods,jvm_read_methods,
		    " length   attributes ",
		    LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    char str[80];
    unsigned i;
    unsigned short acount=0;
    bmSeek(jvm_header.methods_offset,BFile::Seek_Set);
    for(i=0;i<(unsigned)ret+1;i++)
    {
	fpos=bmGetCurrFilePos();
	bmSeek(2,BFile::Seek_Cur);
	get_name(bmbioHandle(),str,sizeof(str));
	bmSeek(fpos+6,BFile::Seek_Set);
	acount=bmReadWord();
	acount=JVM_WORD(&acount,1);
	skip_attributes(bmbioHandle(),acount);
    }
    fpos += 6;
    if(acount>1)
    {
	__filesize_t a_offset;
	unsigned short a_count;
	a_offset = jvm_header.attributes_offset;
	a_count = jvm_header.attributes_count;
	jvm_header.attributes_offset=fpos+2;
	jvm_header.attributes_count=acount;
	fpos=__ShowAttributes(str);
	jvm_header.attributes_offset=a_offset;
	jvm_header.attributes_count=a_count;
    }
    else fpos += acount?6:0;
  }
  return fpos;
}


static bool __FASTCALL__ jvm_read_fields(BFile& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char str[80],str2[80],sout[256];
    handle.seek(jvm_header.fields_offset,BFile::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	flg=handle.read_word();
	flg=JVM_WORD(&flg,1);
	get_name(handle,str,sizeof(str));
	handle.seek(fpos+4,BFile::Seek_Set);
	get_name(handle,str2,sizeof(str2));
	handle.seek(fpos+6,BFile::Seek_Set);
	sval=handle.read_word();
	acount=JVM_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str,str2);
	if(!ma_AddString(names,sout,true)) break;
    }
    return true;
}

static unsigned __FASTCALL__ jvm_get_num_fields(BFile& handle)
{
    UNUSED(handle);
    return jvm_header.fields_count;
}

static __filesize_t __FASTCALL__ ShowFields()
{
  __filesize_t fpos;
  int ret;
  fpos = BMGetCurrFilePos();
  ret=fmtShowList(jvm_get_num_fields,jvm_read_fields,
		    " length   attributes ",
		    LB_SELECTIVE,NULL);
  if(ret!=-1)
  {
    char str[80];
    unsigned i;
    unsigned short acount=0;
    bmSeek(jvm_header.fields_offset,BFile::Seek_Set);
    for(i=0;i<(unsigned)ret+1;i++)
    {
	fpos=bmGetCurrFilePos();
	bmSeek(2,BFile::Seek_Cur);
	get_name(bmbioHandle(),str,sizeof(str));
	bmSeek(fpos+6,BFile::Seek_Set);
	acount=bmReadWord();
	acount=JVM_WORD(&acount,1);
	skip_attributes(bmbioHandle(),acount);
    }
    fpos += 6;
    if(acount>1)
    {
	__filesize_t a_offset;
	unsigned short a_count;
	a_offset = jvm_header.attributes_offset;
	a_count = jvm_header.attributes_count;
	jvm_header.attributes_offset=fpos+2;
	jvm_header.attributes_count=acount;
	fpos=__ShowAttributes(str);
	jvm_header.attributes_offset=a_offset;
	jvm_header.attributes_count=a_count;
    }
    else fpos += acount?6:0;
  }
  return fpos;
}

static bool __FASTCALL__ jvm_read_pool(BFile& handle,memArray * names,unsigned nnames)
{
    __filesize_t fpos;
    uint32_t lval,lval2;
    unsigned i;
    unsigned short flg,sval,slen;
    unsigned char utag;
    char str[80],str2[80],sout[256];
    handle.seek(jvm_header.constants_offset,BFile::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	utag=handle.read_byte();
	switch(utag)
	{
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			fpos=handle.tell();
			get_name(handle,str,sizeof(str));
			handle.seek(fpos+2,BFile::Seek_Set);
			sprintf(sout,"%s: %s",utag==CONSTANT_CLASS?"Class":"String",str);
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			flg=handle.read_word();
			flg=JVM_WORD(&flg,1);
			sval=handle.read_word();
			sval=JVM_WORD(&sval,1);
			sprintf(sout,"%s: class=#%04XH name_type_idx=#%04XH"
			,utag==CONSTANT_FIELDREF?"FieldRef":utag==CONSTANT_METHODREF?"MethodRef":"InterfaceMethodRef"
			,flg,sval);
			break;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=handle.read_dword();
			lval=JVM_DWORD(&lval,1);
			sprintf(sout,"%s: %08lXH",utag==CONSTANT_INTEGER?"Integer":"Float"
			,(long)lval);
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=handle.read_dword();
			lval=JVM_DWORD(&lval,1);
			lval2=handle.read_dword();
			lval2=JVM_DWORD(&lval2,1);
			sprintf(sout,"%s: hi=%08lXH lo=%08lXH",utag==CONSTANT_LONG?"Long":"Double"
			,(long)lval,(long)lval2);
			i++;
			break;
	    case CONSTANT_NAME_AND_TYPE:
			fpos=handle.tell();
			get_name(handle,str,sizeof(str));
			handle.seek(fpos+2,BFile::Seek_Set);
			get_name(handle,str2,sizeof(str2));
			handle.seek(fpos+4,BFile::Seek_Set);
			sprintf(sout,"Name&Type: %s %s",str,str2);
			break;
	    case CONSTANT_UTF8:
			sval=handle.read_word();
			sval=JVM_WORD(&sval,1);
			slen=std::min(sizeof(str)-1,size_t(sval));
			fpos=handle.tell();
			handle.read(str,slen);
			handle.seek(fpos+sval,BFile::Seek_Set);
			str[slen]='\0';
			sprintf(sout,"UTF8: %s",str);
			break;
	    default:
			sprintf(sout,"Unknown: %u",utag);
			i=nnames;
			break;
	}
	if(!ma_AddString(names,sout,true)) break;
    }
    return true;
}

static unsigned __FASTCALL__ jvm_get_num_pools(BFile& handle)
{
    UNUSED(handle);
    return jvm_header.constant_pool_count;
}

static __filesize_t __FASTCALL__ ShowPool()
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  fmtShowList(jvm_get_num_pools,jvm_read_pool,
		    " Constant pool ",
		    LB_SORTABLE,NULL);
  return fpos;
}

static void __FASTCALL__ jvm_init_fmt(CodeGuider& code_guider)
{
    UNUSED(code_guider);
    __filesize_t fpos;
    unsigned short sval;
    jvm_header.magic=0xCAFEBABE;
    jvm_header.attrcode_offset=-1;
    jvm_header.code_offset=-1;
    jvm_header.data_offset=-1;
    fpos=bmGetCurrFilePos();
    bmSeek(4,BFile::Seek_Set);
    sval=bmReadWord();
    jvm_header.minor=JVM_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.major=JVM_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.constant_pool_count=JVM_WORD(&sval,1);
    jvm_header.constants_offset=bmGetCurrFilePos();
    skip_constant_pool(bmbioHandle(),jvm_header.constant_pool_count-1);
    sval=bmReadWord();
    jvm_header.access_flags=JVM_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.this_class=JVM_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.super_class=JVM_WORD(&sval,1);
    sval=bmReadWord();
    jvm_header.interfaces_count=JVM_WORD(&sval,1);
    jvm_header.interfaces_offset=bmGetCurrFilePos();
    bmSeek(jvm_header.interfaces_count*2,BFile::Seek_Cur);
    sval=bmReadWord();
    jvm_header.fields_count=JVM_WORD(&sval,1);
    jvm_header.fields_offset=bmGetCurrFilePos();
    skip_fields(jvm_header.fields_count,0);
    sval=bmReadWord();
    jvm_header.methods_count=JVM_WORD(&sval,1);
    jvm_header.methods_offset=bmGetCurrFilePos();
    skip_fields(jvm_header.fields_count,1); /* methods have the same struct as fields */
    sval=bmReadWord();
    jvm_header.attrcode_offset=0;
    jvm_header.attributes_count=JVM_WORD(&sval,1);
    jvm_header.attributes_offset=bmGetCurrFilePos();
    if(jvm_header.attributes_count) jvm_header.attrcode_offset=jvm_header.attributes_offset;
    skip_attributes(bmbioHandle(),sval);
    jvm_header.header_length=bmGetCurrFilePos();
    bmSeek(fpos,BFile::Seek_Set);
    BFile& bh = bmbioHandle();
    if((jvm_cache = bh.dup(BBIO_SMALL_CACHE_SIZE)) == &bNull) jvm_cache = &bh;
    if((pool_cache = bh.dup(BBIO_SMALL_CACHE_SIZE)) == &bNull) pool_cache = &bh;
}

static void __FASTCALL__ jvm_destroy_fmt()
{
  BFile& bh=bmbioHandle();
  if(jvm_cache != &bNull && jvm_cache != &bh) delete jvm_cache;
  if(pool_cache != &bNull && pool_cache != &bh) delete pool_cache;
}

static int  __FASTCALL__ jvm_platform() { return DISASM_JAVA; }

static void  __FASTCALL__ decode_acc_flags(unsigned flags, char *str)
{
    if(flags & 0x0001) strcpy(str," PUBLIC");
    if(flags & 0x0010) strcat(str," FINAL");
    if(flags & 0x0020) strcat(str," SUPER");
    if(flags & 0x0200) strcat(str," INTERFACE");
    if(flags & 0x0400) strcat(str," ABSTRACT");
    strcat(str," ");
}

static __filesize_t __FASTCALL__ ShowJvmHeader()
{
    __filesize_t entry;
    TWindow * hwnd;
    unsigned keycode;
    char sinfo[70],sinfo2[70],sinfo3[70];
    entry=BMGetCurrFilePos();
    hwnd = CrtDlgWndnls(" ClassFile Header ",78,11);
    hwnd->goto_xy(1,1);
    decode_acc_flags(jvm_header.access_flags,sinfo);
    get_class_name(bmbioHandle(),jvm_header.this_class,sinfo2,sizeof(sinfo2));
    get_class_name(bmbioHandle(),jvm_header.super_class,sinfo3,sizeof(sinfo3));
    hwnd->printf(
	     "Signature     = 'CAFEBABE'\n"
	     "Version       = %u.%u\n"
	     "# Constants   = %u\n"
	     "Access flags  = %04X (%s)\n"
	     "This class    = #%u '%s'\n"
	     "Super class   = #%u '%s'\n"
	     "# interfaces  = %u (at %08lXH)\n"
	     "# fields      = %u (at %08lXH)\n"
	     "# methods     = %u (at %08lXH)\n"
	     "# attributes  = %u (at %08lXH)\n"
	     "Header length = %lu\n"
	     ,jvm_header.major,jvm_header.minor
	     ,jvm_header.constant_pool_count-1
	     ,jvm_header.access_flags, sinfo
	     ,jvm_header.this_class,sinfo2
	     ,jvm_header.super_class,sinfo3
	     ,jvm_header.interfaces_count,jvm_header.interfaces_offset
	     ,jvm_header.fields_count,jvm_header.fields_offset
	     ,jvm_header.methods_count,jvm_header.methods_offset
	     ,jvm_header.attributes_count,jvm_header.attributes_offset
	     ,jvm_header.header_length
	     );
    while(1)
    {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { entry = entry; break; }
	else
	if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    CloseWnd(hwnd);
    return entry;
}

static __filesize_t __FASTCALL__ jvm_VA2PA(__filesize_t va)
{
  return  va + jvm_header.code_offset;
}

static __filesize_t __FASTCALL__ jvm_PA2VA(__filesize_t pa)
{
  return pa >= jvm_header.code_offset ? pa - jvm_header.code_offset : 0L;
}

static bool __FASTCALL__ jvm_AddressResolv(char *addr,__filesize_t cfpos)
{
  bool bret = true;
  if(cfpos >= jvm_header.methods_offset)
  {
    addr[0]='.';
    strcpy(&addr[1],Get8Digit(jvm_PA2VA(cfpos)));
  }
  else
/*
  if(cfpos >= jvm_header.attributes_offset) sprintf(addr,"Attr:%s",Get4Digit(cfpos-jvm_header.attributes_offset));
  else
    if(cfpos >= jvm_header.methods_offset) sprintf(addr,"Code:%s",Get4Digit(cfpos-jvm_header.methods_offset));
    else
*/
	if(cfpos >= jvm_header.fields_offset) sprintf(addr,"Data:%s",Get4Digit(cfpos-jvm_header.fields_offset));
	else
	    if(cfpos >= jvm_header.interfaces_offset) sprintf(addr,"Imp :%s",Get4Digit(cfpos-jvm_header.interfaces_offset));
	    else
		if(cfpos >= jvm_header.constants_offset) sprintf(addr,"Pool:%s",Get4Digit(cfpos-jvm_header.constants_offset));
		else sprintf(addr,"Hdr :%s",Get4Digit(cfpos));
  return bret;
}

static void __FASTCALL__ jvm_ReadPubName(BFile& b_cache,const struct PubName *it,
			    char *buff,unsigned cb_buff)
{
    b_cache.seek(it->nameoff,BFile::Seek_Set);
    get_name(b_cache,buff,cb_buff);
    if(it->addinfo)
    {
	char *s_end;
	strcat(buff,".");
	s_end=buff+strlen(buff);
	b_cache.seek(it->addinfo,BFile::Seek_Set);
	get_name(b_cache,s_end,cb_buff-(s_end-buff));
    }
}

static void __FASTCALL__ jvm_ReadPubNameList(BFile& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
 __filesize_t fpos;
 unsigned i;
 struct PubName jvm_pn;
 unsigned short acount,flg;
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
/* Lookup fields */
 handle.seek(jvm_header.fields_offset,BFile::Seek_Set);
 for(i = 0;i < jvm_header.fields_count;i++)
 {
    fpos=handle.tell();
    flg=handle.read_word();
    flg=JVM_WORD(&flg,1);
    jvm_pn.nameoff = handle.tell();
    jvm_pn.addinfo=0;
    handle.seek(fpos+6,BFile::Seek_Set);
    acount=handle.read_word();
    acount=JVM_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	uint32_t jlen;
	fpos=handle.tell();
	jvm_pn.addinfo=fpos;
	handle.seek(fpos+2,BFile::Seek_Set);
	jlen=handle.read_dword();
	jlen=JVM_DWORD(&jlen,1);
	jvm_pn.pa = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
	handle.seek(jlen,BFile::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
    }
    if(handle.eof()) break;
 }
/* Lookup methods */
 handle.seek(jvm_header.methods_offset,BFile::Seek_Set);
 for(i=0;i<jvm_header.fields_count;i++)
 {
    fpos=handle.tell();
    flg=handle.read_word();
    flg=JVM_WORD(&flg,1);
    jvm_pn.nameoff = handle.tell();
    handle.seek(fpos+6,BFile::Seek_Set);
    acount=handle.read_word();
    acount=JVM_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	uint32_t jlen;
	fpos=handle.tell();
	jvm_pn.addinfo=fpos;
	handle.seek(fpos+2,BFile::Seek_Set);
	jlen=handle.read_dword();
	jlen=JVM_DWORD(&jlen,1);
	jvm_pn.pa = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
	handle.seek(jlen,BFile::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	if(!la_AddData(PubNames,&jvm_pn,mem_out)) break;
    }
    if(handle.eof()) break;
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ jvm_GetPubSym(char *str,unsigned cb_str,unsigned *func_class,
			   __filesize_t pa,bool as_prev)
{
  return fmtGetPubSym(bmbioHandle(),str,cb_str,func_class,pa,as_prev,
		      jvm_ReadPubNameList,
		      jvm_ReadPubName);
}

static unsigned __FASTCALL__ jvm_GetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
		      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = DAB_USE16;
  name[0] = 0;
  if(pa < jvm_header.data_offset)
  {
    *end =jvm_header.data_offset;
    ret = 0;
  }
  else
    if(pa >= jvm_header.data_offset && pa < jvm_header.methods_offset)
    {
      *_class = OC_DATA;
      *start = jvm_header.data_offset;
      *end = jvm_header.methods_offset;
      ret = 1;
    }
    else
    if(pa >= jvm_header.methods_offset && pa < jvm_header.code_offset)
    {
      *_class = OC_NOOBJECT;
      *start = jvm_header.methods_offset;
      *end = jvm_header.code_offset;
      ret = 2;
    }
    else
    if(pa >= jvm_header.code_offset && pa < jvm_header.attributes_offset)
    {
      *_class = OC_CODE;
      *start = jvm_header.code_offset;
      *end = jvm_header.attributes_offset;
      ret = 3;
    }
    else
    if(pa >= jvm_header.attributes_offset && pa < jvm_header.attrcode_offset)
    {
      *_class = OC_NOOBJECT;
      *start = jvm_header.attributes_offset;
      *end = jvm_header.attrcode_offset;
      ret = 4;
    }
    else
    {
      __filesize_t fpos;
      uint32_t len;
      fpos=bmGetCurrFilePos();
      bmSeek(jvm_header.attributes_offset,BFile::Seek_Set);
      get_name(bmbioHandle(),name,cb_name);
      bmSeek(fpos+2,BFile::Seek_Set);
      len=bmReadDWord();
      len=JVM_DWORD(&len,1);
      bmSeek(fpos,BFile::Seek_Set);
      *_class = OC_CODE;
      *start = jvm_header.attributes_offset+6;
      *end = *start+len;
      ret = 5;
    }
  return ret;
}

static int __FASTCALL__ jvm_bitness(__filesize_t off)
{
    UNUSED(off);
    return DAB_USE16;
}

static bool __FASTCALL__ jvm_AppendRef(const DisMode& parent,char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
 bool retrf = true;
 unsigned slen=1000; /* According on disasm/java/java.c */
 UNUSED(r_sh);
    if((flags & APREF_TRY_LABEL)!=APREF_TRY_LABEL)
    {
	__filesize_t fpos;
	uint32_t lidx,lval,lval2;
	unsigned sl;
	unsigned short sval,sval2;
	unsigned char utag;
	jvm_cache->seek(ulShift,BFile::Seek_Set);
	switch(codelen)
	{
	    case 4: lidx=jvm_cache->read_dword(); lidx=JVM_DWORD(&lidx,1); break;
	    case 2: sval=jvm_cache->read_word(); lidx=JVM_WORD(&sval,1); break;
	    default:
	    case 1: lidx=jvm_cache->read_byte(); break;
	}
	pool_cache->seek(jvm_header.constants_offset,BFile::Seek_Set);
	if(lidx<1 || lidx>jvm_header.constant_pool_count) { retrf = false; goto bye; }
	skip_constant_pool(*pool_cache,lidx-1);
	utag=pool_cache->read_byte();
	str=&str[strlen(str)];
	switch(utag)
	{
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			get_name(*pool_cache,str,slen);
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			fpos=pool_cache->tell();
			sval=pool_cache->read_word();
			sval=JVM_WORD(&sval,1);
			sval2=pool_cache->read_word();
			sval2=JVM_WORD(&sval2,1);
			pool_cache->seek(jvm_header.constants_offset,BFile::Seek_Set);
			get_class_name(*pool_cache,sval,str,slen);
			strcat(str,".");
			sl=strlen(str);
			slen-=sl;
			str+=sl;
			pool_cache->seek(jvm_header.constants_offset,BFile::Seek_Set);
			skip_constant_pool(*pool_cache,sval2-1);
			utag=pool_cache->read_byte();
			if(utag!=CONSTANT_NAME_AND_TYPE) break;
			goto name_type;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=pool_cache->read_dword();
			lval=JVM_DWORD(&lval,1);
			strcpy(str,utag==CONSTANT_INTEGER?"Integer":"Float");
			strcat(str,":");
			strcat(str,Get8Digit(lval));
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=pool_cache->read_dword();
			lval=JVM_DWORD(&lval,1);
			lval2=pool_cache->read_dword();
			lval2=JVM_DWORD(&lval2,1);
			strcpy(str,utag==CONSTANT_INTEGER?"Long":"Double");
			strcat(str,":");
			strcat(str,Get8Digit(lval));
			strcat(str,Get8Digit(lval2));
			break;
	    case CONSTANT_NAME_AND_TYPE:
	    name_type:
			fpos=pool_cache->tell();
			get_name(*pool_cache,str,slen);
			pool_cache->seek(fpos+2,BFile::Seek_Set);
			strcat(str," ");
			sl=strlen(str);
			slen-=sl;
			str+=sl;
			get_name(*pool_cache,str,slen);
			break;
	    case CONSTANT_UTF8:
			sval=pool_cache->read_word();
			sval=JVM_WORD(&sval,1);
			sl=std::min(slen,unsigned(sval));
			fpos=pool_cache->tell();
			pool_cache->read(str,sl);
			str[sl]='\0';
			break;
	    default:	retrf = false;
			break;
	}
    }
    else retrf = false;
    bye:
    return retrf;
}

extern const REGISTRY_BIN jvmTable =
{
  "Java's ClassFile",
  { NULL, "Import", "Code  ", "Data  ", NULL, NULL, NULL, "Pool  ", NULL, "Attrib" },
  { NULL, ShowInterfaces, ShowMethods, ShowFields, NULL, NULL, NULL, ShowPool, NULL, ShowAttributes },
  jvm_check_fmt,
  jvm_init_fmt,
  jvm_destroy_fmt,
  ShowJvmHeader,
  jvm_AppendRef,
  jvm_platform,
  jvm_bitness,
  NULL,
  jvm_AddressResolv,
  jvm_VA2PA,
  jvm_PA2VA,
  jvm_GetPubSym,
  jvm_GetObjAttr
};
} // namespace beye
