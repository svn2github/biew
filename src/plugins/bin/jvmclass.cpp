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
#include <sstream>
#include <iomanip>
#include <set>

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "udn.h"
#include "colorset.h"
#include "codeguid.h"
#include "listbox.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "libbeye/bswap.h"
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
	    JVM_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~JVM_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F4();
	    virtual __filesize_t	action_F8();
	    virtual __filesize_t	action_F10();

	    virtual __filesize_t	show_header() const;
	    virtual std::string		bind(const DisMode& _parent,__filesize_t shift,Bin_Format::bind_type flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual Bin_Format::bitness	query_bitness(__filesize_t) const;
	    virtual std::string		address_resolving(__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev);
	    virtual Object_Info		get_object_attribute(__filesize_t pa);
	private:
	    std::string			jvm_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    void			decode_acc_flags(unsigned flags,char *str) const;
	    std::vector<std::string>	jvm_read_pool(binary_stream&handle,size_t nnames) const;
	    std::vector<std::string>	jvm_read_fields(binary_stream&handle,size_t nnames) const;
	    std::vector<std::string>	jvm_read_methods(binary_stream&handle,size_t nnames) const;
	    std::vector<std::string>	jvm_read_attributes(binary_stream&handle,size_t nnames) const;
	    std::vector<std::string>	jvm_read_interfaces(binary_stream&handle,size_t nnames) const;
	    void			skip_fields(unsigned nitems,int attr);
	    void			skip_attributes(binary_stream&handle,unsigned nitems) const;
	    std::string			get_class_name(binary_stream&handle,unsigned idx) const;
	    std::string			get_name(binary_stream&handle) const;
	    std::string			get_utf8(binary_stream&handle,unsigned nidx) const;
	    void			skip_constant_pool(binary_stream&handle,unsigned nitems) const;
	    unsigned			skip_constant(binary_stream&handle,unsigned char id) const;
	    __filesize_t		__ShowAttributes(const std::string& title) const;
	    void			jvm_ReadPubNameList(binary_stream& handle);

#if __BYTE_ORDER == __BIG_ENDIAN
	    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) const __CONST_FUNC__ { return !is_big ? bswap_16(cval) : cval; }
	    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) const __CONST_FUNC__ { return !is_big ? bswap_32(cval) :cval; }
	    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) const __CONST_FUNC__ { return !is_big ? bswap_64(cval) :cval; }
#else
	    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) const __CONST_FUNC__ { return is_big ? bswap_16(cval) : cval; }
	    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) const __CONST_FUNC__ { return is_big ? bswap_32(cval) :cval; }
	    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) const __CONST_FUNC__ { return is_big ? bswap_64(cval) :cval; }
#endif
	    inline uint16_t JVM_WORD(const uint16_t* cval,bool is_msbf) const __PURE_FUNC__ { return FMT_WORD(*cval,is_msbf); }
	    inline uint32_t JVM_DWORD(const uint32_t* cval,bool is_msbf) const __PURE_FUNC__ { return FMT_DWORD(*cval,is_msbf); }
	    inline uint64_t JVM_QWORD(const uint64_t* cval,bool is_msbf) const __PURE_FUNC__ { return FMT_QWORD(*cval,is_msbf); }

	    binary_stream*	jvm_cache;
	    binary_stream*	pool_cache;
	    std::set<symbolic_information>	PubNames;

	    ClassFile_t		jvm_header;
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;
    };

static const char* txt[]={ "", "Import", "Code  ", "Data  ", "", "", "", "Pool  ", "", "Attrib" };
const char* JVM_Parser::prompt(unsigned idx) const { return txt[idx]; }

unsigned JVM_Parser::skip_constant(binary_stream& handle,unsigned char id) const
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

void JVM_Parser::skip_constant_pool(binary_stream& handle,unsigned nitems) const
{
    unsigned i;
    for(i=0;i<nitems;i++)
    {
	unsigned char id;
	id=handle.read(type_byte);
	i+=skip_constant(handle,id);
    }
}

std::string JVM_Parser::get_utf8(binary_stream& handle,unsigned nidx) const
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
	binary_packet bp=handle.read(nidx); memcpy(str,bp.data(),bp.size());
	str[nidx]=0;
	return str;
    }
    return "";
}

std::string JVM_Parser::get_name(binary_stream& handle) const
{
    unsigned short nidx;
    nidx=handle.read(type_word);
    nidx=JVM_WORD(&nidx,1);
    return get_utf8(handle,nidx);
}

std::string JVM_Parser::get_class_name(binary_stream& handle,unsigned idx) const
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

void JVM_Parser::skip_attributes(binary_stream& handle,unsigned nitems) const
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

std::vector<std::string> JVM_Parser::jvm_read_interfaces(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    __filesize_t fpos;
    unsigned short id;
    std::string str;
    handle.seek(jvm_header.interfaces_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++) {
	id=handle.read(type_word);
	fpos=handle.tell();
	id=JVM_WORD(&id,1);
	str=get_class_name(handle,id);
	rc.push_back(str);
	handle.seek(fpos,binary_stream::Seek_Set);
    }
    return rc;
}

__filesize_t JVM_Parser::action_F2()
{
    __filesize_t fpos = bctx.tell();
    std::string title = " interfaces ";
    ssize_t nnames = jvm_header.interfaces_count;
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w = PleaseWaitWnd();
    std::vector<std::string> objs = jvm_read_interfaces(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return fpos;
}

std::vector<std::string> JVM_Parser::jvm_read_attributes(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    __filesize_t fpos;
    uint32_t len;
    std::ostringstream oss;
    std::string str;
    handle.seek(jvm_header.attributes_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++) {
	fpos=handle.tell();
	str=get_name(handle);
	handle.seek(fpos+2,binary_stream::Seek_Set);
	len=handle.read(type_dword);
	len=JVM_DWORD(&len,1);
	oss.str("");
	oss<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)len<<"H "<<str;
	rc.push_back(oss.str());
	handle.seek(len,binary_stream::Seek_Cur);
    }
    return rc;
}

__filesize_t  JVM_Parser::__ShowAttributes(const std::string& title) const
{
    __filesize_t fpos = bctx.tell();
    int ret;
    ssize_t nnames = jvm_header.attributes_count;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = jvm_read_attributes(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
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

std::vector<std::string> JVM_Parser::jvm_read_methods(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    std::ostringstream oss;
    std::string str,str2;
    handle.seek(jvm_header.methods_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++) {
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
	oss.str("");
	oss<<std::hex<<std::setfill('0')<<std::setw(4)<<acount<<"H "
	    <<std::hex<<std::setfill('0')<<std::setw(4)<<flg<<" "
	    <<str<<" "<<str2;
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t JVM_Parser::action_F3()
{
    __filesize_t fpos = bctx.tell();
    int ret;
    std::string title = " length   attributes ";
    ssize_t nnames = jvm_header.methods_count;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = jvm_read_methods(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
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

std::vector<std::string> JVM_Parser::jvm_read_fields(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    __filesize_t fpos;
    unsigned short flg,sval,acount;
    std::ostringstream oss;
    std::string str,str2;
    handle.seek(jvm_header.fields_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++) {
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
	oss.str("");
	oss<<std::hex<<std::setfill('0')<<std::setw(4)<<acount<<"H "
	    <<std::hex<<std::setfill('0')<<std::setw(4)<<flg<<" "
	    <<str<<" "<<str2;
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t JVM_Parser::action_F4()
{
    __filesize_t fpos = bctx.tell();
    int ret;
    std::string title = " length   attributes ";
    ssize_t nnames = jvm_header.fields_count;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = jvm_read_fields(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
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

std::vector<std::string> JVM_Parser::jvm_read_pool(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    __filesize_t fpos;
    uint32_t lval,lval2;
    unsigned i;
    unsigned short flg,sval,slen;
    unsigned char utag;
    std::ostringstream oss;
    std::string str,str2;
    binary_packet bp(1);
    handle.seek(jvm_header.constants_offset,binary_stream::Seek_Set);
    for(i=0;i<nnames;i++) {
	fpos=handle.tell();
	utag=handle.read(type_byte);
	oss.str("");
	switch(utag) {
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			fpos=handle.tell();
			str=get_name(handle);
			handle.seek(fpos+2,binary_stream::Seek_Set);
			oss<<(utag==CONSTANT_CLASS?"Class":"String")<<": "<<str;
			break;
	    case CONSTANT_FIELDREF:
	    case CONSTANT_METHODREF:
	    case CONSTANT_INTERFACEMETHODREF:
			flg=handle.read(type_word);
			flg=JVM_WORD(&flg,1);
			sval=handle.read(type_word);
			sval=JVM_WORD(&sval,1);
			oss<<(utag==CONSTANT_FIELDREF?"FieldRef":utag==CONSTANT_METHODREF?"MethodRef":"InterfaceMethodRef")
			    <<": class=#"<<std::hex<<std::setfill('0')<<std::setw(4)<<flg
			    <<"H name_type_idx="<<std::hex<<std::setfill('0')<<std::setw(4)<<sval;
			break;
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
			lval=handle.read(type_dword);
			lval=JVM_DWORD(&lval,1);
			oss<<(utag==CONSTANT_INTEGER?"Integer":"Float")
			    <<std::hex<<std::setfill('0')<<std::setw(8)<<lval;
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=handle.read(type_dword);
			lval=JVM_DWORD(&lval,1);
			lval2=handle.read(type_dword);
			lval2=JVM_DWORD(&lval2,1);
			oss<<(utag==CONSTANT_LONG?"Long":"Double")
			    <<" hi="<<std::hex<<std::setfill('0')<<std::setw(8)<<lval
			    <<" lo="<<std::hex<<std::setfill('0')<<std::setw(8)<<lval2;
			i++;
			break;
	    case CONSTANT_NAME_AND_TYPE:
			fpos=handle.tell();
			str=get_name(handle);
			handle.seek(fpos+2,binary_stream::Seek_Set);
			str2=get_name(handle);
			handle.seek(fpos+4,binary_stream::Seek_Set);
			oss<<"Name&Type: "<<str<<" "<<str2;
			break;
	    case CONSTANT_UTF8:
			sval=handle.read(type_word);
			sval=JVM_WORD(&sval,1);
			slen=std::min(sizeof(str)-1,size_t(sval));
			fpos=handle.tell();
			char stmp[slen+1];
			bp=handle.read(slen); memcpy(stmp,bp.data(),bp.size());
			handle.seek(fpos+sval,binary_stream::Seek_Set);
			stmp[slen]='\0';
			oss<<"UTF8: "<<stmp;
			break;
	    default:
			oss<<"Unknown: "<<utag;
			i=nnames;
			break;
	}
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t JVM_Parser::action_F8()
{
    __filesize_t fpos = bctx.tell();
    std::string title = " Constant pool ";
    ssize_t nnames = jvm_header.constant_pool_count;
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w = PleaseWaitWnd();
    std::vector<std::string> objs = jvm_read_pool(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return fpos;
}

JVM_Parser::JVM_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    __filesize_t fpos;
    unsigned short sval;
    unsigned char id[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    binary_packet bp=main_handle.read(sizeof(id)); memcpy(id,bp.data(),bp.size());
    /* Cafe babe !!! */
    if(!(id[0]==0xCA && id[1]==0xFE && id[2]==0xBA && id[3]==0xBE && main_handle.flength()>=16)) throw bad_format_exception();

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
    jvm_cache = bh.dup();
    pool_cache = bh.dup();
}

JVM_Parser::~JVM_Parser()
{
  binary_stream& bh=main_handle;
  if(jvm_cache != &bh) delete jvm_cache;
  if(pool_cache != &bh) delete pool_cache;
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

__filesize_t JVM_Parser::show_header() const
{
    __filesize_t entry;
    TWindow * hwnd;
    unsigned keycode;
    char sinfo[70];
    std::string sinfo2,sinfo3;
    entry=bctx.tell();
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

__filesize_t JVM_Parser::va2pa(__filesize_t va) const { return  va + jvm_header.code_offset; }
__filesize_t JVM_Parser::pa2va(__filesize_t pa) const { return pa >= jvm_header.code_offset ? pa - jvm_header.code_offset : 0L; }

std::string JVM_Parser::address_resolving(__filesize_t cfpos)
{
    std::ostringstream oss;
    if(cfpos >= jvm_header.methods_offset) oss<<"."<<std::hex<<std::setfill('0')<<std::setw(8)<<pa2va(cfpos);
    else if(cfpos >= jvm_header.fields_offset) oss<<"Data:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-jvm_header.fields_offset);
    else if(cfpos >= jvm_header.interfaces_offset) oss<<"Imp :"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-jvm_header.interfaces_offset);
    else if(cfpos >= jvm_header.constants_offset) oss<<"Pool:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-jvm_header.constants_offset);
    else oss<<"Hdr :"<<std::hex<<std::setfill('0')<<std::setw(4)<<cfpos;
    return oss.str();
}

std::string JVM_Parser::jvm_ReadPubName(binary_stream& b_cache,const symbolic_information& it) const
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

void JVM_Parser::jvm_ReadPubNameList(binary_stream& handle)
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
	jvm_pn.attr    = flg & 0x0008 ? Symbol_Info::Local : Symbol_Info::Global;
	PubNames.insert(jvm_pn);
	handle.seek(jlen,binary_stream::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? Symbol_Info::Local : Symbol_Info::Global;
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
	jvm_pn.attr    = flg & 0x0008 ? Symbol_Info::Local : Symbol_Info::Global;
	PubNames.insert(jvm_pn);
	handle.seek(jlen,binary_stream::Seek_Cur);
	if(handle.eof()) break;
    }
    if(!acount)
    {
	jvm_pn.pa      = handle.tell();
	jvm_pn.attr    = flg & 0x0008 ? Symbol_Info::Local : Symbol_Info::Global;
	PubNames.insert(jvm_pn);
    }
    if(handle.eof()) break;
 }
}

Symbol_Info JVM_Parser::get_public_symbol(__filesize_t pa,bool as_prev)
{
    Symbol_Info rc;
    binary_stream& b_cache = main_handle;
    if(PubNames.empty()) jvm_ReadPubNameList(b_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    rc=find_symbolic_information(PubNames,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	rc.name=jvm_ReadPubName(b_cache,*idx);
    }
    return rc;
}

Object_Info JVM_Parser::get_object_attribute(__filesize_t pa)
{
    Object_Info rc;
    rc.start = 0;
    rc.end = main_handle.flength();
    rc._class = Object_Info::NoObject;
    rc.bitness = Bin_Format::Use16;
    rc.number=0;
    if(pa < jvm_header.data_offset) {
	rc.end =jvm_header.data_offset;
	rc.number = 0;
    } else if(pa >= jvm_header.data_offset && pa < jvm_header.methods_offset) {
	rc._class = Object_Info::Data;
	rc.start = jvm_header.data_offset;
	rc.end = jvm_header.methods_offset;
	rc.number = 1;
    } else if(pa >= jvm_header.methods_offset && pa < jvm_header.code_offset) {
	rc._class = Object_Info::NoObject;
	rc.start = jvm_header.methods_offset;
	rc.end = jvm_header.code_offset;
	rc.number = 2;
    } else if(pa >= jvm_header.code_offset && pa < jvm_header.attributes_offset) {
	rc._class = Object_Info::Code;
	rc.start = jvm_header.code_offset;
	rc.end = jvm_header.attributes_offset;
	rc.number = 3;
    } else if(pa >= jvm_header.attributes_offset && pa < jvm_header.attrcode_offset) {
	rc._class = Object_Info::NoObject;
	rc.start = jvm_header.attributes_offset;
	rc.end = jvm_header.attrcode_offset;
	rc.number = 4;
    } else {
	__filesize_t fpos;
	uint32_t len;
	fpos=main_handle.tell();
	main_handle.seek(jvm_header.attributes_offset,binary_stream::Seek_Set);
	std::string stmp;
	stmp=get_name(main_handle);
	rc.name=stmp;
	main_handle.seek(fpos+2,binary_stream::Seek_Set);
	len=main_handle.read(type_dword);
	len=JVM_DWORD(&len,1);
	main_handle.seek(fpos,binary_stream::Seek_Set);
	rc._class = Object_Info::Code;
	rc.start = jvm_header.attributes_offset+6;
	rc.end = rc.start+len;
	rc.number = 5;
    }
    return rc;
}

Bin_Format::bitness JVM_Parser::query_bitness(__filesize_t off) const
{
    UNUSED(off);
    return Bin_Format::Use16;
}

std::string JVM_Parser::bind(const DisMode& parent,__filesize_t ulShift,Bin_Format::bind_type flags,int codelen,__filesize_t r_sh)
{
    std::string str;
    UNUSED(parent);
    UNUSED(r_sh);
    std::ostringstream oss;
    binary_packet bp(1);
    if((flags & Bin_Format::Try_Label)!=Bin_Format::Try_Label) {
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
	if(lidx<1 || lidx>jvm_header.constant_pool_count) goto bye;
	skip_constant_pool(*pool_cache,lidx-1);
	utag=pool_cache->read(type_byte);
	switch(utag) {
	    case CONSTANT_STRING:
	    case CONSTANT_CLASS:
			str=get_name(*pool_cache);
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
			str=get_class_name(*pool_cache,sval);
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
			oss<<((utag==CONSTANT_INTEGER)?"Integer":"Float")<<":"<<std::hex<<std::setfill('0')<<std::setw(8)<<lval;
			str=oss.str();
			break;
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
			lval=pool_cache->read(type_dword);
			lval=JVM_DWORD(&lval,1);
			lval2=pool_cache->read(type_dword);
			lval2=JVM_DWORD(&lval2,1);
			oss<<((utag==CONSTANT_INTEGER)?"Long":"Double")<<":"<<std::hex<<std::setfill('0')<<std::setw(8)<<lval<<std::hex<<std::setfill('0')<<std::setw(8)<<lval2;
			str=oss.str();
			break;
	    case CONSTANT_NAME_AND_TYPE:
	    name_type:
			fpos=pool_cache->tell();
			str=get_name(*pool_cache);
			pool_cache->seek(fpos+2,binary_stream::Seek_Set);
			str+=" ";
			str+=get_name(*pool_cache);
			break;
	    case CONSTANT_UTF8:
			sval=pool_cache->read(type_word);
			sval=JVM_WORD(&sval,1);
			char stmp[sval+1];
			fpos=pool_cache->tell();
			bp=pool_cache->read(sval); memcpy(stmp,bp.data(),bp.size());
			str=stmp;
			break;
	    default:	break;
	}
    }
bye:
    return str;
}

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) JVM_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info jvm_info = {
    "Java's ClassFile",	/**< plugin name */
    query_interface
};
} // namespace	usr
