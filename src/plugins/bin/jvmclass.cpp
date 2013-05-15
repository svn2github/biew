#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
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
#include <set>

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bin_util.h"
#include "colorset.h"
#include "codeguid.h"
#include "reg_form.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
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

    class JVM_Parser : public Binary_Parser {
	public:
	    JVM_Parser(binary_stream&,CodeGuider&);
	    virtual ~JVM_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F4();
	    virtual __filesize_t	action_F8();
	    virtual __filesize_t	action_F10();

	    virtual __filesize_t	show_header();
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	    virtual __filesize_t	get_public_symbol(std::string& str,unsigned& _class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,std::string& name,
							__filesize_t& start,__filesize_t& end,int& _class,int& bitness);
	private:
	    std::string			jvm_ReadPubName(binary_stream&b_cache,const symbolic_information& it);
	    void			decode_acc_flags(unsigned flags,char *str) const;
	    bool			jvm_read_pool(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			jvm_read_fields(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			jvm_read_methods(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			jvm_read_attributes(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			jvm_read_interfaces(binary_stream&handle,memArray *names,unsigned nnames);
	    void			skip_fields(unsigned nitems,int attr);
	    void			skip_attributes(binary_stream&handle,unsigned nitems);
	    std::string			get_class_name(binary_stream&handle,unsigned idx);
	    std::string			get_name(binary_stream&handle);
	    std::string			get_utf8(binary_stream&handle,unsigned nidx);
	    void			skip_constant_pool(binary_stream&handle,unsigned nitems);
	    unsigned			skip_constant(binary_stream&handle,unsigned char id);
	    __filesize_t		__ShowAttributes(const std::string& title);
	    void			jvm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));

	    inline uint16_t JVM_WORD(const uint16_t* cval,bool is_msbf) const { return FMT_WORD(*cval,is_msbf); }
	    inline uint32_t JVM_DWORD(const uint32_t* cval,bool is_msbf) const { return FMT_DWORD(*cval,is_msbf); }
	    inline uint64_t JVM_QWORD(const uint64_t* cval,bool is_msbf) const { return FMT_QWORD(*cval,is_msbf); }

	    binary_stream*	jvm_cache;
	    binary_stream*	pool_cache;
	    std::set<symbolic_information>	PubNames;

	    ClassFile_t		jvm_header;
	    binary_stream&	main_handle;
    };

static const char* txt[]={ "", "Import", "Code  ", "Data  ", "", "", "", "Pool  ", "", "Attrib" };
const char* JVM_Parser::prompt(unsigned idx) const { return txt[idx]; }

unsigned JVM_Parser::skip_constant(binary_stream& handle,unsigned char id)
{
    unsigned add;
    unsigned short sval;
    add=0;
    switch(id)
    {
	default:
	case CONSTANT_STRING:
	case CONSTANT_CLASS: handle.seek(2,binary_stream::Seek_Cur); break;
	case CONSTANT_INTEGER:
	case CONSTANT_FLOAT:
	case CONSTANT_FIELDREF:
	case CONSTANT_METHODREF:
	case CONSTANT_NAME_AND_TYPE:
	case CONSTANT_INTERFACEMETHODREF: handle.seek(4,binary_stream::Seek_Cur); break;
	case CONSTANT_LONG:
	case CONSTANT_DOUBLE: handle.seek(8,binary_stream::Seek_Cur); add=1; break;
	case CONSTANT_UTF8:
			sval=handle.read(type_word);
			sval=JVM_WORD(&sval,1);
			handle.seek(sval,binary_stream::Seek_Cur);
			break;
    }
    return add;
}

void JVM_Parser::skip_constant_pool(binary_stream& handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned char id;
	id=handle.read(type_byte);
	i+=skip_constant(handle,id);
    }
}

std::string JVM_Parser::get_utf8(binary_stream& handle,unsigned nidx)
{
    unsigned char id;
    handle.seek(jvm_header.constants_offset,binary_stream::Seek_Set);
    skip_constant_pool(handle,nidx-1);
    id=handle.read(type_byte);
    if(id==CONSTANT_UTF8)
    {
	nidx=handle.read(type_word);
	nidx=JVM_WORD((uint16_t*)&nidx,1);
	char str[nidx+1];
	handle.read(str,nidx);
	str[nidx]=0;
	return str;
    }
    return "";
}

std::string JVM_Parser::get_name(binary_stream& handle)
{
    unsigned short nidx;
    nidx=handle.read(type_word);
    nidx=JVM_WORD(&nidx,1);
    return get_utf8(handle,nidx);
}

std::string JVM_Parser::get_class_name(binary_stream& handle,unsigned idx)
{
    if(idx && idx<(unsigned)jvm_header.constant_pool_count-1)
    {
	unsigned char id;
	handle.seek(jvm_header.constants_offset,binary_stream::Seek_Set);
	skip_constant_pool(handle,idx-1);
	id=handle.read(type_byte);
	if(id==CONSTANT_CLASS) return get_name(handle);
    }
    return "";
}

void JVM_Parser::skip_attributes(binary_stream& handle,unsigned nitems)
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	uint32_t lval;
	handle.seek(2,binary_stream::Seek_Cur);
	lval=handle.read(type_dword);
	lval=JVM_DWORD(&lval,1);
	handle.seek(lval,binary_stream::Seek_Cur);
    }
}

void JVM_Parser::skip_fields(unsigned nitems,int attr)
{
    unsigned i;
    __filesize_t fpos;
    for(i=0;i<nitems;i++)
    {
	unsigned short sval;
	main_handle.seek(6,binary_stream::Seek_Cur);
	sval=main_handle.read(type_word);
	sval=JVM_WORD(&sval,1);
	fpos=main_handle.tell();
	if(i==0)
	{
	    __filesize_t lval;
	    lval=sval?fpos+6:fpos;
	    if(attr) jvm_header.code_offset=lval;
	    else jvm_header.data_offset=lval;
	}
	skip_attributes(main_handle,sval);
    }
}

bool JVM_Parser::jvm_read_interfaces(binary_stream& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short id;
    std::string str;
    handle.seek(jvm_header.interfaces_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	id=handle.read(type_word);
	fpos=handle.tell();
	id=JVM_WORD(&id,1);
	str=get_class_name(handle,id);
	if(!ma_AddString(names,str.c_str(),true)) break;
	handle.seek(fpos,binary_stream::Seek_Set);
    }
    return true;
}

__filesize_t JVM_Parser::action_F2()
{
    __filesize_t fpos = beye_context().tell();
    std::string title = " interfaces ";
    ssize_t nnames = jvm_header.interfaces_count;
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = jvm_read_interfaces(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return fpos;
}

bool JVM_Parser::jvm_read_attributes(binary_stream& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    uint32_t len;
    char sout[100];
    std::string str;
    handle.seek(jvm_header.attributes_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	str=get_name(handle);
	handle.seek(fpos+2,binary_stream::Seek_Set);
	len=handle.read(type_dword);
	len=JVM_DWORD(&len,1);
	sprintf(sout,"%08lXH %s",(long)len,str.c_str());
	if(!ma_AddString(names,sout,true)) break;
	handle.seek(len,binary_stream::Seek_Cur);
    }
    return true;
}

__filesize_t  JVM_Parser::__ShowAttributes(const std::string& title)
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    ssize_t nnames = jvm_header.attributes_count;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = jvm_read_attributes(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret!=-1) {
	unsigned i;
	main_handle.seek(jvm_header.attributes_offset,binary_stream::Seek_Set);
	for(i=0;i<(unsigned)ret+1;i++) {
	    uint32_t len;
	    fpos=main_handle.tell();
	    main_handle.seek(fpos+2,binary_stream::Seek_Set);
	    len=main_handle.read(type_dword);
	    len=JVM_DWORD(&len,1);
	    fpos+=6;
	    main_handle.seek(len,binary_stream::Seek_Cur);
	}
    }
    return fpos;
}

__filesize_t JVM_Parser::action_F10()
{
    return __ShowAttributes(" length   attributes ");
}

bool JVM_Parser::jvm_read_methods(binary_stream& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char sout[256];
    std::string str,str2;
    handle.seek(jvm_header.methods_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	flg=handle.read(type_word);
	flg=JVM_WORD(&flg,1);
	str=get_name(handle);
	handle.seek(fpos+4,binary_stream::Seek_Set);
	str2=get_name(handle);
	handle.seek(fpos+6,binary_stream::Seek_Set);
	sval=handle.read(type_word);
	acount=JVM_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str.c_str(),str2.c_str());
	if(!ma_AddString(names,sout,true)) break;
    }
    return true;
}

__filesize_t JVM_Parser::action_F3()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " length   attributes ";
    ssize_t nnames = jvm_header.methods_count;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = jvm_read_methods(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret!=-1) {
	std::string str;
	unsigned i;
	unsigned short acount=0;
	main_handle.seek(jvm_header.methods_offset,binary_stream::Seek_Set);
	for(i=0;i<(unsigned)ret+1;i++) {
	    fpos=main_handle.tell();
	    main_handle.seek(2,binary_stream::Seek_Cur);
	    str=get_name(main_handle);
	    main_handle.seek(fpos+6,binary_stream::Seek_Set);
	    acount=main_handle.read(type_word);
	    acount=JVM_WORD(&acount,1);
	    skip_attributes(main_handle,acount);
	}
	fpos += 6;
	if(acount>1) {
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

bool JVM_Parser::jvm_read_fields(binary_stream& handle,memArray * names,unsigned nnames)
{
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    char sout[256];
    std::string str,str2;
    handle.seek(jvm_header.fields_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	flg=handle.read(type_word);
	flg=JVM_WORD(&flg,1);
	str=get_name(handle);
	handle.seek(fpos+4,binary_stream::Seek_Set);
	str2=get_name(handle);
	handle.seek(fpos+6,binary_stream::Seek_Set);
	sval=handle.read(type_word);
	acount=JVM_WORD(&sval,1);
	skip_attributes(handle,acount);
	sprintf(sout,"%04XH %04XH %s %s",acount,flg,str.c_str(),str2.c_str());
	if(!ma_AddString(names,sout,true)) break;
    }
    return true;
}

__filesize_t JVM_Parser::action_F4()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " length   attributes ";
    ssize_t nnames = jvm_header.fields_count;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = jvm_read_fields(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret!=-1) {
	std::string str;
	unsigned i;
	unsigned short acount=0;
	main_handle.seek(jvm_header.fields_offset,binary_stream::Seek_Set);
	for(i=0;i<(unsigned)ret+1;i++) {
	    fpos=main_handle.tell();
	    main_handle.seek(2,binary_stream::Seek_Cur);
	    str=get_name(main_handle);
	    main_handle.seek(fpos+6,binary_stream::Seek_Set);
	    acount=main_handle.read(type_word);
	    acount=JVM_WORD(&acount,1);
	    skip_attributes(main_handle,acount);
	}
	fpos += 6;
	if(acount>1) {
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

bool JVM_Parser::jvm_read_pool(binary_stream& handle,memArray * names,unsigned nnames)
{
    __filesize_t fpos;
    uint32_t lval,lval2;
    unsigned i;
    unsigned short flg,sval,slen;
    unsigned char utag;
    char sout[256];
    std::string str,str2;
    handle.seek(jvm_header.constants_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++)
    {
	fpos=handle.tell();
	utag=handle.read(type_byte);
	switch(utag)
	{
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			fpos=handle.tell();
			str=get_name(handle);
			handle.seek(fpos+2,binary_stream::Seek_Set);
			sprintf(sout,"%s: %s",utag==CONSTANT_CLASS?"Class":"String",str.c_str());
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			flg=handle.read(type_word);
			flg=JVM_WORD(&flg,1);
			sval=handle.read(type_word);
			sval=JVM_WORD(&sval,1);
			sprintf(sout,"%s: class=#%04XH name_type_idx=#%04XH"
			,utag==CONSTANT_FIELDREF?"FieldRef":utag==CONSTANT_METHODREF?"MethodRef":"InterfaceMethodRef"
			,flg,sval);
			break;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=handle.read(type_dword);
			lval=JVM_DWORD(&lval,1);
			sprintf(sout,"%s: %08lXH",utag==CONSTANT_INTEGER?"Integer":"Float"
			,(long)lval);
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=handle.read(type_dword);
			lval=JVM_DWORD(&lval,1);
			lval2=handle.read(type_dword);
			lval2=JVM_DWORD(&lval2,1);
			sprintf(sout,"%s: hi=%08lXH lo=%08lXH",utag==CONSTANT_LONG?"Long":"Double"
			,(long)lval,(long)lval2);
			i++;
			break;
	    case CONSTANT_NAME_AND_TYPE:
			fpos=handle.tell();
			str=get_name(handle);
			handle.seek(fpos+2,binary_stream::Seek_Set);
			str2=get_name(handle);
			handle.seek(fpos+4,binary_stream::Seek_Set);
			sprintf(sout,"Name&Type: %s %s",str.c_str(),str2.c_str());
			break;
	    case CONSTANT_UTF8:
			sval=handle.read(type_word);
			sval=JVM_WORD(&sval,1);
			slen=std::min(sizeof(str)-1,size_t(sval));
			fpos=handle.tell();
			char stmp[slen+1];
			handle.read(stmp,slen);
			handle.seek(fpos+sval,binary_stream::Seek_Set);
			stmp[slen]='\0';
			sprintf(sout,"UTF8: %s",stmp);
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

__filesize_t JVM_Parser::action_F8()
{
    __filesize_t fpos = beye_context().tell();
    std::string title = " Constant pool ";
    ssize_t nnames = jvm_header.constant_pool_count;
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = jvm_read_pool(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return fpos;
}

JVM_Parser::JVM_Parser(binary_stream& h,CodeGuider& code_guider)
	    :Binary_Parser(h,code_guider)
	    ,main_handle(h)
{
    __filesize_t fpos;
    unsigned short sval;
    jvm_header.magic=0xCAFEBABE;
    jvm_header.attrcode_offset=-1;
    jvm_header.code_offset=-1;
    jvm_header.data_offset=-1;
    fpos=main_handle.tell();
    main_handle.seek(4,binary_stream::Seek_Set);
    sval=main_handle.read(type_word);
    jvm_header.minor=JVM_WORD(&sval,1);
    sval=main_handle.read(type_word);
    jvm_header.major=JVM_WORD(&sval,1);
    sval=main_handle.read(type_word);
    jvm_header.constant_pool_count=JVM_WORD(&sval,1);
    jvm_header.constants_offset=main_handle.tell();
    skip_constant_pool(main_handle,jvm_header.constant_pool_count-1);
    sval=main_handle.read(type_word);
    jvm_header.access_flags=JVM_WORD(&sval,1);
    sval=main_handle.read(type_word);
    jvm_header.this_class=JVM_WORD(&sval,1);
    sval=main_handle.read(type_word);
    jvm_header.super_class=JVM_WORD(&sval,1);
    sval=main_handle.read(type_word);
    jvm_header.interfaces_count=JVM_WORD(&sval,1);
    jvm_header.interfaces_offset=main_handle.tell();
    main_handle.seek(jvm_header.interfaces_count*2,binary_stream::Seek_Cur);
    sval=main_handle.read(type_word);
    jvm_header.fields_count=JVM_WORD(&sval,1);
    jvm_header.fields_offset=main_handle.tell();
    skip_fields(jvm_header.fields_count,0);
    sval=main_handle.read(type_word);
    jvm_header.methods_count=JVM_WORD(&sval,1);
    jvm_header.methods_offset=main_handle.tell();
    skip_fields(jvm_header.fields_count,1); /* methods have the same struct as fields */
    sval=main_handle.read(type_word);
    jvm_header.attrcode_offset=0;
    jvm_header.attributes_count=JVM_WORD(&sval,1);
    jvm_header.attributes_offset=main_handle.tell();
    if(jvm_header.attributes_count) jvm_header.attrcode_offset=jvm_header.attributes_offset;
    skip_attributes(main_handle,sval);
    jvm_header.header_length=main_handle.tell();
    main_handle.seek(fpos,binary_stream::Seek_Set);
    binary_stream& bh = main_handle;
    if((jvm_cache = bh.dup()) == &bNull) jvm_cache = &bh;
    if((pool_cache = bh.dup()) == &bNull) pool_cache = &bh;
}

JVM_Parser::~JVM_Parser()
{
  binary_stream& bh=main_handle;
  if(jvm_cache != &bNull && jvm_cache != &bh) delete jvm_cache;
  if(pool_cache != &bNull && pool_cache != &bh) delete pool_cache;
}

int JVM_Parser::query_platform() const { return DISASM_JAVA; }

void JVM_Parser::decode_acc_flags(unsigned flags, char *str) const
{
    if(flags & 0x0001) strcpy(str," PUBLIC");
    if(flags & 0x0010) strcat(str," FINAL");
    if(flags & 0x0020) strcat(str," SUPER");
    if(flags & 0x0200) strcat(str," INTERFACE");
    if(flags & 0x0400) strcat(str," ABSTRACT");
    strcat(str," ");
}

__filesize_t JVM_Parser::show_header()
{
    __filesize_t entry;
    TWindow * hwnd;
    unsigned keycode;
    char sinfo[70];
    std::string sinfo2,sinfo3;
    entry=beye_context().tell();
    hwnd = CrtDlgWndnls(" ClassFile Header ",78,11);
    hwnd->goto_xy(1,1);
    decode_acc_flags(jvm_header.access_flags,sinfo);
    sinfo2=get_class_name(main_handle,jvm_header.this_class);
    sinfo3=get_class_name(main_handle,jvm_header.super_class);
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
	     ,jvm_header.this_class,sinfo2.c_str()
	     ,jvm_header.super_class,sinfo3.c_str()
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
    delete hwnd;
    return entry;
}

__filesize_t JVM_Parser::va2pa(__filesize_t va)
{
  return  va + jvm_header.code_offset;
}

__filesize_t JVM_Parser::pa2va(__filesize_t pa)
{
  return pa >= jvm_header.code_offset ? pa - jvm_header.code_offset : 0L;
}

bool JVM_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
  bool bret = true;
  if(cfpos >= jvm_header.methods_offset)
  {
    addr=".";
    addr+=Get8Digit(pa2va(cfpos));
  }
  else
/*
  if(cfpos >= jvm_header.attributes_offset) sprintf(addr,"Attr:%s",Get4Digit(cfpos-jvm_header.attributes_offset));
  else
    if(cfpos >= jvm_header.methods_offset) sprintf(addr,"Code:%s",Get4Digit(cfpos-jvm_header.methods_offset));
    else
*/
	if(cfpos >= jvm_header.fields_offset) {
	    addr="Data:";
	    addr+=Get4Digit(cfpos-jvm_header.fields_offset);
	}
	else
	    if(cfpos >= jvm_header.interfaces_offset) {
		addr="Imp :";
		addr+=Get4Digit(cfpos-jvm_header.interfaces_offset);
	    }
	    else
		if(cfpos >= jvm_header.constants_offset) {
		    addr="Pool:";
		    addr+=Get4Digit(cfpos-jvm_header.constants_offset);
		}
		else {
		    addr="Hdr :";
		    addr+=Get4Digit(cfpos);
		}
  return bret;
}

std::string JVM_Parser::jvm_ReadPubName(binary_stream& b_cache,const symbolic_information& it)
{
    std::string buff;
    b_cache.seek(it.nameoff,binary_stream::Seek_Set);
    buff=get_name(b_cache);
    if(it.addinfo) {
	buff+=".";
	b_cache.seek(it.addinfo,binary_stream::Seek_Set);
	buff+=get_name(b_cache);
    }
    return buff;
}

void JVM_Parser::jvm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
 __filesize_t fpos;
 unsigned i;
 symbolic_information jvm_pn;
 unsigned short acount,flg;
/* Lookup fields */
 handle.seek(jvm_header.fields_offset,binary_stream::Seek_Set);
 for(i = 0;i < jvm_header.fields_count;i++)
 {
    fpos=handle.tell();
    flg=handle.read(type_word);
    flg=JVM_WORD(&flg,1);
    jvm_pn.nameoff = handle.tell();
    jvm_pn.addinfo=0;
    handle.seek(fpos+6,binary_stream::Seek_Set);
    acount=handle.read(type_word);
    acount=JVM_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	uint32_t jlen;
	fpos=handle.tell();
	jvm_pn.addinfo=fpos;
	handle.seek(fpos+2,binary_stream::Seek_Set);
	jlen=handle.read(type_dword);
	jlen=JVM_DWORD(&jlen,1);
	jvm_pn.pa = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	PubNames.insert(jvm_pn);
	handle.seek(jlen,binary_stream::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	PubNames.insert(jvm_pn);
    }
    if(handle.eof()) break;
 }
/* Lookup methods */
 handle.seek(jvm_header.methods_offset,binary_stream::Seek_Set);
 for(i=0;i<jvm_header.fields_count;i++)
 {
    fpos=handle.tell();
    flg=handle.read(type_word);
    flg=JVM_WORD(&flg,1);
    jvm_pn.nameoff = handle.tell();
    handle.seek(fpos+6,binary_stream::Seek_Set);
    acount=handle.read(type_word);
    acount=JVM_WORD(&acount,1);
    for(i=0;i<acount;i++)
    {
	uint32_t jlen;
	fpos=handle.tell();
	jvm_pn.addinfo=fpos;
	handle.seek(fpos+2,binary_stream::Seek_Set);
	jlen=handle.read(type_dword);
	jlen=JVM_DWORD(&jlen,1);
	jvm_pn.pa = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	PubNames.insert(jvm_pn);
	handle.seek(jlen,binary_stream::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? SC_LOCAL : SC_GLOBAL;
	PubNames.insert(jvm_pn);
    }
    if(handle.eof()) break;
 }
// if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

__filesize_t JVM_Parser::get_public_symbol(std::string& str,unsigned& func_class,
			   __filesize_t pa,bool as_prev)
{
    binary_stream& b_cache = main_handle;
    __filesize_t fpos;
    if(PubNames.empty()) jvm_ReadPubNameList(b_cache,NULL);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    fpos=find_symbolic_information(PubNames,func_class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	str=jvm_ReadPubName(b_cache,*idx);
    }
    return fpos;
}

unsigned JVM_Parser::get_object_attribute(__filesize_t pa,std::string& name,
		      __filesize_t& start,__filesize_t& end,int& _class,int& bitness)
{
  unsigned ret;
  start = 0;
  end = main_handle.flength();
  _class = OC_NOOBJECT;
  bitness = DAB_USE16;
  name[0] = 0;
  if(pa < jvm_header.data_offset)
  {
    end =jvm_header.data_offset;
    ret = 0;
  }
  else
    if(pa >= jvm_header.data_offset && pa < jvm_header.methods_offset)
    {
      _class = OC_DATA;
      start = jvm_header.data_offset;
      end = jvm_header.methods_offset;
      ret = 1;
    }
    else
    if(pa >= jvm_header.methods_offset && pa < jvm_header.code_offset)
    {
      _class = OC_NOOBJECT;
      start = jvm_header.methods_offset;
      end = jvm_header.code_offset;
      ret = 2;
    }
    else
    if(pa >= jvm_header.code_offset && pa < jvm_header.attributes_offset)
    {
      _class = OC_CODE;
      start = jvm_header.code_offset;
      end = jvm_header.attributes_offset;
      ret = 3;
    }
    else
    if(pa >= jvm_header.attributes_offset && pa < jvm_header.attrcode_offset)
    {
      _class = OC_NOOBJECT;
      start = jvm_header.attributes_offset;
      end = jvm_header.attrcode_offset;
      ret = 4;
    }
    else
    {
      __filesize_t fpos;
      uint32_t len;
      fpos=main_handle.tell();
      main_handle.seek(jvm_header.attributes_offset,binary_stream::Seek_Set);
      std::string stmp;
      stmp=get_name(main_handle);
      name=stmp;
      main_handle.seek(fpos+2,binary_stream::Seek_Set);
      len=main_handle.read(type_dword);
      len=JVM_DWORD(&len,1);
      main_handle.seek(fpos,binary_stream::Seek_Set);
      _class = OC_CODE;
      start = jvm_header.attributes_offset+6;
      end = start+len;
      ret = 5;
    }
  return ret;
}

int JVM_Parser::query_bitness(__filesize_t off) const
{
    UNUSED(off);
    return DAB_USE16;
}

bool JVM_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
    bool retrf = true;
    UNUSED(parent);
    UNUSED(r_sh);
    if((flags & APREF_TRY_LABEL)!=APREF_TRY_LABEL) {
	__filesize_t fpos;
	uint32_t lidx,lval,lval2;
	unsigned short sval,sval2;
	unsigned char utag;
	jvm_cache->seek(ulShift,binary_stream::Seek_Set);
	switch(codelen) {
	    case 4: lidx=jvm_cache->read(type_dword); lidx=JVM_DWORD(&lidx,1); break;
	    case 2: sval=jvm_cache->read(type_word); lidx=JVM_WORD(&sval,1); break;
	    default:
	    case 1: lidx=jvm_cache->read(type_byte); break;
	}
	pool_cache->seek(jvm_header.constants_offset,binary_stream::Seek_Set);
	if(lidx<1 || lidx>jvm_header.constant_pool_count) { retrf = false; goto bye; }
	skip_constant_pool(*pool_cache,lidx-1);
	utag=pool_cache->read(type_byte);
	switch(utag) {
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			str+=get_name(*pool_cache);
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			fpos=pool_cache->tell();
			sval=pool_cache->read(type_word);
			sval=JVM_WORD(&sval,1);
			sval2=pool_cache->read(type_word);
			sval2=JVM_WORD(&sval2,1);
			pool_cache->seek(jvm_header.constants_offset,binary_stream::Seek_Set);
			str+=get_class_name(*pool_cache,sval);
			str+=".";
			pool_cache->seek(jvm_header.constants_offset,binary_stream::Seek_Set);
			skip_constant_pool(*pool_cache,sval2-1);
			utag=pool_cache->read(type_byte);
			if(utag!=CONSTANT_NAME_AND_TYPE) break;
			goto name_type;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=pool_cache->read(type_dword);
			lval=JVM_DWORD(&lval,1);
			str+=(utag==CONSTANT_INTEGER)?"Integer":"Float";
			str+=":";
			str+=Get8Digit(lval);
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=pool_cache->read(type_dword);
			lval=JVM_DWORD(&lval,1);
			lval2=pool_cache->read(type_dword);
			lval2=JVM_DWORD(&lval2,1);
			str+=(utag==CONSTANT_INTEGER)?"Long":"Double";
			str+=":";
			str+=Get8Digit(lval);
			str+=Get8Digit(lval2);
			break;
	    case CONSTANT_NAME_AND_TYPE:
	    name_type:
			fpos=pool_cache->tell();
			str+=get_name(*pool_cache);
			pool_cache->seek(fpos+2,binary_stream::Seek_Set);
			str+=" ";
			str+=get_name(*pool_cache);
			break;
	    case CONSTANT_UTF8:
			sval=pool_cache->read(type_word);
			sval=JVM_WORD(&sval,1);
			char stmp[sval+1];
			fpos=pool_cache->tell();
			pool_cache->read(stmp,sval);
			str+=stmp;
			break;
	    default:	retrf = false;
			break;
	}
    }
    else retrf = false;
    bye:
    return retrf;
}

static bool probe(binary_stream& main_handle) {
  unsigned char id[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(id,sizeof(id));
  /* Cafe babe !!! */
  return id[0]==0xCA && id[1]==0xFE && id[2]==0xBA && id[3]==0xBE && main_handle.flength()>=16;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent) { return new(zeromem) JVM_Parser(h,_parent); }
extern const Binary_Parser_Info jvm_info = {
    "Java's ClassFile",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
