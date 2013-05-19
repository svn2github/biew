#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/rdoff.c
 * @brief       This file contains implementation of decoder for RDOFF v1 file format.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <set>

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "reg_form.h"
#include "colorset.h"
#include "udn.h"
#include "codeguid.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "plugins/bin/rdoff.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    struct rdoff_ImpName {
	unsigned short lsegno;
	unsigned short reserv;
	__filesize_t nameoff;

	bool operator<(const rdoff_ImpName& rhs) const { return lsegno<rhs.lsegno; }
    };

    class RDOff_Parser : public Binary_Parser {
	public:
	    RDOff_Parser(binary_stream&,CodeGuider&,udn&);
	    virtual ~RDOff_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F5();

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
	    std::string			rdoff_ReadPubName(binary_stream&b_cache,const symbolic_information& it);
	    void			rdoff_ReadPubNameList(binary_stream& handle);
	    bool			rdoffBuildReferStr(const DisMode&parent,std::string& str,const RDOFF_RELOC& reloc,__filesize_t ulShift,int flags);
	    void			BuildRelocRDOFF();
	    bool			rdoff_skiprec(unsigned char type);
	    bool			FindPubName(std::string& buff,__filesize_t pa);
	    void			ReadImpNameList(binary_stream& handle);
	    __filesize_t		rdoff_FindExport(const std::string& name);

	    unsigned long		rdoff_hdrlen,ds_len;
	    __filesize_t		cs_start,ds_start;
	    std::set<symbolic_information>	PubNames;
	    std::set<RDOFF_RELOC>	rdoffReloc;
	    std::set<rdoff_ImpName>	rdoffImpNames;
	    unsigned char		__codelen;
	    binary_stream&		main_handle;
	    CodeGuider&			code_guider;
	    udn&			_udn;
    };
static const char* txt[]={ "RdHelp", "ModRef", "Export", "", "Import", "", "", "", "", "" };
const char* RDOff_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t RDOff_Parser::action_F1()
{
  hlpDisplay(10011);
  return beye_context().tell();
}

/** return 0 if error */
bool  RDOff_Parser::rdoff_skiprec(unsigned char type)
{
  unsigned i;
  bool ret;
  unsigned char nulch;
  ret = true;
  switch(type)
  {
    case 1: /** reloc record */
	    main_handle.seek(8,binary_stream::Seek_Cur);
	    break;
    case 2: /** import symbol */
	    main_handle.seek(2,binary_stream::Seek_Cur);
	    for(i = 0;i < 32;i++)
	    {
	      nulch = main_handle.read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 3: /** export symbol */
	    main_handle.seek(5,binary_stream::Seek_Cur);
	    for(i = 0;i < 32;i++)
	    {
	      nulch = main_handle.read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 4: /** import library */
	    for(i = 0;i < 128;i++)
	    {
	      nulch = main_handle.read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 5: /** reserve bss */
	    main_handle.seek(4,binary_stream::Seek_Cur);
	    break;
    default: /** unknown ??? */
	    beye_context().ErrMessageBox("Broken RDOFF file","");
	    ret = false;
	    break;
  }
  return ret;
}

__filesize_t RDOff_Parser::action_F3()
{
    __filesize_t fpos;
    unsigned char rec;
    unsigned i;
    char str[33],sout[256];
    unsigned char segno;
    __filesize_t segoff;
    __filesize_t abs_off;
    std::vector<std::string> rdoff_et;
    fpos = beye_context().tell();
    main_handle.seek(10,binary_stream::Seek_Set);
    while(main_handle.tell() < rdoff_hdrlen + 5) {
	bool is_eof;
	rec = main_handle.read(type_byte);
	is_eof = false;
	if(rec == 3) {
	    char ch;
	    segno = main_handle.read(type_byte);
	    segoff = main_handle.read(type_dword);
	    for(i = 0;i < 32;i++) {
		ch = main_handle.read(type_byte);
		str[i] = ch;
		is_eof = main_handle.eof();
		if(!ch || is_eof) break;
	    }
	    str[i] = 0;
	    abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
	    if(abs_off < FILESIZE_MAX) abs_off += segoff;
	    sprintf(sout,"%-50s offset=%08lXH",str,(unsigned long)abs_off);
	    rdoff_et.push_back(sout);
	    if(is_eof) break;
	} else {
	    if(!rdoff_skiprec(rec)) goto exit;
	    if(main_handle.eof()) break;
	}
    }
    if(!rdoff_et.empty()) {
	int ret;
	ret = ListBox(rdoff_et,EXP_TABLE,LB_SELECTIVE,0);
	if(ret != -1) {
	    const char *rets;
	    rets = strstr(rdoff_et[ret].c_str(),"offset=");
	    if(rets) fpos = strtoul(&rets[7],NULL,16);
	}
    } else beye_context().NotifyBox(NOT_ENTRY,EXP_TABLE);
exit:
    return fpos;
}

__filesize_t RDOff_Parser::rdoff_FindExport(const std::string& name)
{
  __filesize_t ret;
  unsigned char rec;
  unsigned i;
  char str[33];
  unsigned char segno;
  __filesize_t segoff;
  __filesize_t abs_off;
  main_handle.seek(10,binary_stream::Seek_Set);
  ret = 0L;
  while(main_handle.tell() < rdoff_hdrlen + 5)
  {
    bool is_eof;
    rec = main_handle.read(type_byte);
    is_eof = false;
    if(rec == 3)
    {
      char ch;
      segno = main_handle.read(type_byte);
      segoff = main_handle.read(type_dword);
      for(i = 0;i < 32;i++)
      {
	ch = main_handle.read(type_byte);
	is_eof = main_handle.eof();
	str[i] = ch;
	if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(name==str)
      {
	abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : 0L;
	if(abs_off != 0L) abs_off += segoff;
	ret = abs_off;
	break;
      }
      if(is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(main_handle.eof()) break;
    }
  }
  return ret;
}

__filesize_t RDOff_Parser::action_F2()
{
    __filesize_t fpos;
    unsigned char rec;
    unsigned i;
    char str[129];
    std::vector<std::string> rdoff_mr;
    fpos = beye_context().tell();
    main_handle.seek(10,binary_stream::Seek_Set);
    while(main_handle.tell() < rdoff_hdrlen + 5) {
	bool is_eof;
	rec = main_handle.read(type_byte);
	is_eof = false;
	if(rec == 4) {
	    char ch;
	    for(i = 0;i < 128;i++) {
		ch = main_handle.read(type_byte);
		is_eof = main_handle.eof();
		str[i] = ch;
		if(!ch || is_eof) break;
	    }
	    str[i] = 0;
	    rdoff_mr.push_back(str);
	    if(is_eof) break;
	} else {
	    if(!rdoff_skiprec(rec)) goto exit;
	    if(main_handle.eof()) break;
	}
    }
    if(!rdoff_mr.empty()) ListBox(rdoff_mr,MOD_REFER,LB_SORTABLE,0);
    else                  beye_context().NotifyBox(NOT_ENTRY,MOD_REFER);
exit:
    return fpos;
}

__filesize_t RDOff_Parser::action_F5()
{
    __filesize_t fpos;
    unsigned char rec;
    unsigned i;
    char str[33];
    std::vector<std::string> rdoff_it;
    fpos = beye_context().tell();
    main_handle.seek(10,binary_stream::Seek_Set);
    while(main_handle.tell() < rdoff_hdrlen + 5) {
	bool is_eof;
	rec = main_handle.read(type_byte);
	is_eof = false;
	if(rec == 2) {
	    char ch;
	    main_handle.seek(2,binary_stream::Seek_Cur);
	    for(i = 0;i < 32;i++) {
		ch = main_handle.read(type_byte);
		is_eof = main_handle.eof();
		str[i] = ch;
		if(!ch || is_eof) break;
	    }
	    str[i] = 0;
	    rdoff_it.push_back(str);
	    if(is_eof) break;
	} else {
	    if(!rdoff_skiprec(rec)) goto exit;
	    if(main_handle.eof()) break;
	}
    }
    if(!rdoff_it.empty()) ListBox(rdoff_it,IMPPROC_TABLE,LB_SORTABLE,0);
    else                  beye_context().NotifyBox(NOT_ENTRY,EXP_TABLE);
exit:
    return fpos;
}

__filesize_t RDOff_Parser::show_header()
{
  int endian;
  __filesize_t fpos,entry;
  unsigned long hs_len,cs_len;
  TWindow *w;
  fpos = beye_context().tell();
  main_handle.seek(5,binary_stream::Seek_Set);
  endian = main_handle.read(type_byte);
  hs_len = main_handle.read(type_dword);
  main_handle.seek(hs_len,binary_stream::Seek_Cur);
  cs_len = main_handle.read(type_dword);
  main_handle.seek(cs_len,binary_stream::Seek_Cur);
  ds_len = main_handle.read(type_dword);
  cs_start = hs_len + 14;
  ds_start = cs_start + cs_len + 4;
  w = CrtDlgWndnls(endian == 0x01 ? " RDOFF big endian " : " RDOFF little endian ",54,6);
  w->goto_xy(1,1);
  w->printf(
	   "Length of header section    = %08lXH\n"
	   "Length of code section      = %08lXH\n"
	   "segment .code               = %08lXH\n"
	   "Length of data secion       = %08lXH\n"
	   "segment .data               = %08lXH\n"
	   ,hs_len
	   ,cs_len
	   ,cs_start
	   ,ds_len
	   ,ds_start);
  w->set_color(dialog_cset.entry);
  entry = rdoff_FindExport("_main");
  w->printf(
	   "Entry point                 = %08lXH"
	   ,entry);
  w->clreol();
  w->set_color(dialog_cset.main);
  while(1)
  {
    int keycode;
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    switch(keycode)
    {
      case KE_ENTER:      fpos = entry; goto exit;
      case KE_F(10):
      case KE_ESCAPE:     goto exit;
      default:            break;
    };
  }
  exit:
  delete w;
  return fpos;
}

void  RDOff_Parser::BuildRelocRDOFF()
{
  unsigned char rec;
//  if(!(rdoffReloc = la_Build(0,sizeof(RDOFF_RELOC),MemOutBox))) return;
  main_handle.seek(10,binary_stream::Seek_Set);
  while(main_handle.tell() < rdoff_hdrlen + 5)
  {
    RDOFF_RELOC rdf_r;
    rec = main_handle.read(type_byte);
    if(rec == 1)
    {
      unsigned char segno;
      __filesize_t off;
      segno = main_handle.read(type_byte);
      rdf_r.is_rel = 0;
      if(segno >= 64)
      {
	rdf_r.is_rel = 1;
	segno -= 64;
      }
      rdf_r.offset = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      off = main_handle.read(type_dword);
      if(main_handle.eof()) break;
      rdf_r.offset += off;
      rdf_r.reflen = main_handle.read(type_byte);
      rdf_r.segto = main_handle.read(type_word);
      rdoffReloc.insert(rdf_r);
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(main_handle.eof()) break;
    }
  }
  exit:;
//  la_Sort(rdoffReloc,rdoff_compare_reloc);
}

bool RDOff_Parser::rdoffBuildReferStr(const DisMode& parent,std::string& str,const RDOFF_RELOC& reloc,__filesize_t ulShift,int flags)
{
   char name[400],ch;
   std::string buff;
   const char *ptr_type;
   __filesize_t field_val,foff,base_seg_off;
   bool retrf;
   unsigned i;
   if(flags & APREF_USE_TYPE)
   {
     switch(reloc.reflen)
     {
       case 1: ptr_type = "(b) "; break;
       case 2: ptr_type = "off16 "; break;
       default:
       case 4: ptr_type = "off32 "; break;
     }
     str+=ptr_type;
   }
   base_seg_off = FILESIZE_MAX;
   switch(reloc.segto)
   {
     case 0:  ptr_type = "cs:";
	      base_seg_off = cs_start;
	      break;
     case 1:  ptr_type = "ds:";
	      base_seg_off = ds_start;
	      break;
     case 2:  ptr_type = "bss:";
	      break;
     default:
	      {
		 struct rdoff_ImpName key;
		 std::set<rdoff_ImpName>::const_iterator ret;
		 key.lsegno = reloc.segto;
		 ret = rdoffImpNames.find(key);
		 if(ret!=rdoffImpNames.end())
		 {
		   main_handle.seek((*ret).nameoff,binary_stream::Seek_Set);
		   name[0] = 0;
		   for(i = 0;i < sizeof(name);i++)
		   {
		     ch = main_handle.read(type_byte);
		     name[i] = ch;
		     if(!ch || main_handle.eof()) break;
		   }
		   name[i] = 0;
		   ptr_type = name;
		 }
		 else
		 {
		   ptr_type = "???";
		 }
	      }
   }
   retrf = true;
   if(reloc.segto >= 3) str+=ptr_type; /** case extern symbol */
   if(reloc.segto < 3 || reloc.is_rel)
   {
     foff = main_handle.tell();
     main_handle.seek(reloc.offset,binary_stream::Seek_Set);
     switch(reloc.reflen)
     {
       default:
       case 4: field_val = main_handle.read(type_dword);
	       break;
       case 2: field_val = main_handle.read(type_word);
	       break;
       case 1: field_val = main_handle.read(type_byte);
	       break;
     }
     main_handle.seek(foff,binary_stream::Seek_Set);
     if(reloc.segto < 3)
     {
       if(base_seg_off < FILESIZE_MAX)
       {
	 base_seg_off += field_val;
	 if(FindPubName(buff,base_seg_off))
	 {
	   str+=buff;
	 }
	 else goto unnamed;
       }
       else
       {
	 unnamed:
	 str+=ptr_type;
	 str+=Get8Digit(field_val);
	 if(!EditMode && !DumpMode && (flags & APREF_TRY_LABEL))
	   code_guider.add_go_address(parent,str,reloc.segto ? ds_start + field_val : cs_start + field_val);
	 retrf = true;
       }
     }
     if(reloc.is_rel)
     {
       __filesize_t seg_off;
       __fileoff_t fv;
       seg_off = ulShift < ds_start ? cs_start : ulShift < ds_start+ds_len ? ds_start : FILESIZE_MAX;
       fv = field_val;
       if(!(ulShift + fv + reloc.reflen == seg_off && (flags & APREF_TRY_LABEL)))
       {
        str+="(";
	str+=Get8SignDig(field_val);
	str+=")";
       }
     }
   }
   return retrf;
}

bool RDOff_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RDOFF_RELOC key;
  std::set<RDOFF_RELOC>::const_iterator rrdoff;
  bool ret;
  std::string buff;
  if(flags & APREF_TRY_PIC) return false;
  if(rdoffReloc.empty()) BuildRelocRDOFF();
  if(rdoffImpNames.empty()) ReadImpNameList(main_handle);
  if(PubNames.empty()) rdoff_ReadPubNameList(main_handle);
  key.offset = ulShift;
  __codelen = codelen;
  rrdoff = rdoffReloc.find(key);
  ret = (rrdoff!=rdoffReloc.end())? rdoffBuildReferStr(parent,str,*rrdoff,ulShift,flags) : false;
  if(!ret && (flags & APREF_TRY_LABEL))
  {
     if(FindPubName(buff,r_sh))
     {
       str+=buff;
       if(!DumpMode && !EditMode) code_guider.add_go_address(parent,str,r_sh);
       ret = true;
     }
  }
  return ret;
}

RDOff_Parser::RDOff_Parser(binary_stream& h,CodeGuider& _code_guider,udn& u)
	    :Binary_Parser(h,_code_guider,u)
	    ,main_handle(h)
	    ,code_guider(_code_guider)
	    ,_udn(u)
{
    unsigned long cs_len;
    main_handle.seek(6,binary_stream::Seek_Set);
    rdoff_hdrlen = main_handle.read(type_dword);
    main_handle.seek(rdoff_hdrlen,binary_stream::Seek_Cur);
    cs_len = main_handle.read(type_dword);
    main_handle.seek(cs_len,binary_stream::Seek_Cur);
    ds_len = main_handle.read(type_dword);
    cs_start = rdoff_hdrlen + 14;
    ds_start = cs_start + cs_len + 4;
}

RDOff_Parser::~RDOff_Parser()
{
//  if(rdoffReloc) { la_Destroy(rdoffReloc); rdoffReloc = NULL; }
//  if(rdoffImpNames) { la_Destroy(rdoffImpNames); rdoffImpNames = NULL; }
//  if(PubNames) { la_Destroy(PubNames); PubNames = NULL; }
}

std::string RDOff_Parser::rdoff_ReadPubName(binary_stream& b_cache,const symbolic_information& it)
{
    std::string rc;
    unsigned char ch;
    b_cache.seek(it.nameoff,binary_stream::Seek_Set);
    for(;;)
    {
      ch = main_handle.read(type_byte);
      if(!ch || main_handle.eof()) break;
      rc+=ch;
    }
    return rc;
}

bool RDOff_Parser::FindPubName(std::string& buff,__filesize_t pa)
{
  binary_stream& b_cache = main_handle;
  symbolic_information key;
  std::set<symbolic_information>::const_iterator ret;
  key.pa = pa;
  if(PubNames.empty()) rdoff_ReadPubNameList(b_cache);
  ret = PubNames.find(key);
  if(ret!=PubNames.end()) {
    buff=rdoff_ReadPubName(b_cache,*ret);
    return true;
  }
  return _udn.find(pa,buff);
}

void RDOff_Parser::rdoff_ReadPubNameList(binary_stream& handle)
{
 unsigned char segno,rec;
 __filesize_t segoff,abs_off;
 unsigned i;
 symbolic_information rdf_pn;
 UNUSED(handle);
// if(!PubNames)
 main_handle.seek(10,binary_stream::Seek_Set);
 while(main_handle.tell() < rdoff_hdrlen + 5)
 {
    bool is_eof;
    rec = main_handle.read(type_byte);
    is_eof = false;
    if(rec == 3)
    {
      char ch;
      segno = main_handle.read(type_byte);
      segoff = main_handle.read(type_dword);
      rdf_pn.nameoff = main_handle.tell();
      for(i = 0;i < 32;i++)
      {
	ch = main_handle.read(type_byte);
	is_eof = main_handle.eof();
	if(!ch || is_eof) break;
      }
      abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      if(abs_off < FILESIZE_MAX) abs_off += segoff;
      rdf_pn.pa = abs_off;
      rdf_pn.attr = SC_GLOBAL;
      PubNames.insert(rdf_pn);
      if(is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(main_handle.eof()) break;
    }
 }
// if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

void  RDOff_Parser::ReadImpNameList(binary_stream& handle)
{
 unsigned char rec;
 unsigned i;
 rdoff_ImpName rdf_in;
 UNUSED(handle);
 main_handle.seek(10,binary_stream::Seek_Set);
 while(main_handle.tell() < rdoff_hdrlen + 5)
 {
    bool is_eof;
    rec = main_handle.read(type_byte);
    is_eof = false;
    if(rec == 2)
    {
      char ch;
      rdf_in.lsegno = main_handle.read(type_word);
      rdf_in.nameoff = main_handle.tell();
      for(i = 0;i < 32;i++)
      {
	ch = main_handle.read(type_byte);
	is_eof = main_handle.eof();
	if(!ch) break;
      }
      rdoffImpNames.insert(rdf_in);
      if(is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(main_handle.eof()) break;
    }
 }
// if(rdoffImpNames->nItems) la_Sort(rdoffImpNames,compare_impnames);
}

/** bitness not declared, but we assign 32-bit as default */
int RDOff_Parser::query_bitness(__filesize_t pa) const
{
  UNUSED(pa);
  return DAB_USE32;
}

__filesize_t RDOff_Parser::get_public_symbol(std::string& str,unsigned& func_class,
			   __filesize_t pa,bool as_prev)
{
    binary_stream& b_cache = main_handle;
    __filesize_t fpos;
    if(PubNames.empty()) rdoff_ReadPubNameList(b_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    fpos=find_symbolic_information(PubNames,func_class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	str=rdoff_ReadPubName(b_cache,*idx);
    }
    return fpos;
}

unsigned RDOff_Parser::get_object_attribute(__filesize_t pa,std::string& name,
			 __filesize_t& start,__filesize_t& end,int& _class,int& bitness)
{
  unsigned ret;
  start = 0;
  end = main_handle.flength();
  _class = OC_NOOBJECT;
  bitness = query_bitness(pa);
  name.clear();
  if(pa < cs_start)
  {
    end = cs_start;
    ret = 0;
  }
  else
    if(pa >= cs_start && pa < ds_start - 4)
    {
      start = cs_start;
      _class = OC_CODE;
      end = ds_start-4;
      ret = 1;
    }
    else
    {
      if(pa >= ds_start - 4 && pa < ds_start)
      {
	start = ds_start-4;
	end = ds_start;
	_class = OC_NOOBJECT;
	ret = 2;
      }
      else
      {
	if(pa >= ds_start && pa < ds_start + ds_len)
	{
	  _class = OC_DATA;
	  start = ds_start;
	  end = ds_start + ds_len;
	  ret = 3;
	}
	else
	{
	  start = ds_start+ds_len;
	  _class = OC_NOOBJECT;
	  ret = 4;
	}
      }
    }
  return ret;
}

__filesize_t RDOff_Parser::va2pa(__filesize_t va)
{
  return va + cs_start;
}

__filesize_t RDOff_Parser::pa2va(__filesize_t pa)
{
  return pa > cs_start ? pa - cs_start : 0L;
}

bool RDOff_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 bool bret = true;
 uint32_t res;
 if(cfpos < cs_start)
 {
    addr="RDFH:";
    addr+=Get4Digit(cfpos);
 }
 else
   if((res=pa2va(cfpos))!=0)
   {
     addr = ".";
     addr+=Get8Digit(res);
   }
   else bret = false;
  return bret;
}

int RDOff_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe(binary_stream& main_handle) {
    char rbuff[6];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(rbuff,sizeof(rbuff));
    return  memcmp(rbuff,"RDOFF1",sizeof(rbuff)) == 0 ||
	    memcmp(rbuff,"RDOFF\x1",sizeof(rbuff)) == 0;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) RDOff_Parser(h,_parent,u); }
extern const Binary_Parser_Info rdoff_info = {
    "RDOFF (Relocatable Dynamic Object File Format)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
