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
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "reg_form.h"
#include "colorset.h"
#include "bin_util.h"
#include "codeguid.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "plugins/bin/rdoff.h"
#include "libbeye/kbd_code.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
    struct rdoff_ImpName {
	unsigned short lsegno;
	unsigned short reserv;
	__filesize_t nameoff;
    };

    class RDOff_Parser : public Binary_Parser {
	public:
	    RDOff_Parser(CodeGuider&);
	    virtual ~RDOff_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F5();

	    virtual __filesize_t	show_header();
	    virtual bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(char *,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	    virtual __filesize_t	get_public_symbol(char *str,unsigned cb_str,unsigned *_class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
							__filesize_t *start,__filesize_t *end,int *_class,int *bitness);
	private:
	    void			rdoff_ReadPubName(binary_stream&b_cache,const struct PubName *it,char *buff,unsigned cb_buff);
	    void			rdoff_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
	    bool			rdoffBuildReferStr(const DisMode&parent,char *str,RDOFF_RELOC *reloc,__filesize_t ulShift,int flags);
	    void			BuildRelocRDOFF();
	    static tCompare		rdoff_compare_reloc(const any_t *e1,const any_t *e2);
	    bool			rdoff_skiprec(unsigned char type);
	    static tCompare		compare_impnames(const any_t *v1,const any_t *v2);
	    bool			FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
	    void			ReadImpNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
	    __filesize_t		rdoff_FindExport(const std::string& name);

	    unsigned long		rdoff_hdrlen,ds_len;
	    __filesize_t		cs_start,ds_start;
	    linearArray*		PubNames;
	    linearArray*		rdoffReloc;
	    linearArray*		rdoffImpNames;
	    unsigned char		__codelen;
	    CodeGuider&			code_guider;
    };
static const char* txt[]={ "RdHelp", "ModRef", "Export", "", "Import", "", "", "", "", "" };
const char* RDOff_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t RDOff_Parser::action_F1()
{
  hlpDisplay(10011);
  return beye_context().bm_file().tell();
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
	    beye_context().sc_bm_file().seek(8,binary_stream::Seek_Cur);
	    break;
    case 2: /** import symbol */
	    beye_context().sc_bm_file().seek(2,binary_stream::Seek_Cur);
	    for(i = 0;i < 32;i++)
	    {
	      nulch = beye_context().sc_bm_file().read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 3: /** export symbol */
	    beye_context().sc_bm_file().seek(5,binary_stream::Seek_Cur);
	    for(i = 0;i < 32;i++)
	    {
	      nulch = beye_context().sc_bm_file().read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 4: /** import library */
	    for(i = 0;i < 128;i++)
	    {
	      nulch = beye_context().sc_bm_file().read(type_byte);
	      if(!nulch) break;
	    }
	    break;
    case 5: /** reserve bss */
	    beye_context().sc_bm_file().seek(4,binary_stream::Seek_Cur);
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
  memArray * rdoff_et;
  fpos = beye_context().bm_file().tell();
  if(!(rdoff_et = ma_Build(0,true))) return fpos;
  beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
  while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
  {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 3)
    {
      char ch;
      segno = beye_context().sc_bm_file().read(type_byte);
      segoff = beye_context().sc_bm_file().read(type_dword);
      for(i = 0;i < 32;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	str[i] = ch;
	is_eof = beye_context().sc_bm_file().eof();
	if(!ch || is_eof) break;
      }
      str[i] = 0;
      abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      if(abs_off < FILESIZE_MAX) abs_off += segoff;
      sprintf(sout,"%-50s offset=%08lXH",str,(unsigned long)abs_off);
      if(!ma_AddString(rdoff_et,sout,true) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(beye_context().sc_bm_file().eof()) break;
    }
  }
  if(rdoff_et->nItems)
  {
    int ret;
    ret = ma_Display(rdoff_et,EXP_TABLE,LB_SELECTIVE,0);
    if(ret != -1)
    {
      char *rets;
      rets = ((char **)rdoff_et->data)[ret];
      rets = strstr(rets,"offset=");
      if(rets) fpos = strtoul(&rets[7],NULL,16);
    }
  }
  else                 beye_context().NotifyBox(NOT_ENTRY,EXP_TABLE);
  exit:
  ma_Destroy(rdoff_et);
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
  beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
  ret = 0L;
  while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
  {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 3)
    {
      char ch;
      segno = beye_context().sc_bm_file().read(type_byte);
      segoff = beye_context().sc_bm_file().read(type_dword);
      for(i = 0;i < 32;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	is_eof = beye_context().sc_bm_file().eof();
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
      if(beye_context().sc_bm_file().eof()) break;
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
  memArray * rdoff_mr;
  fpos = beye_context().bm_file().tell();
  if(!(rdoff_mr = ma_Build(0,true))) return fpos;
  beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
  while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
  {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 4)
    {
      char ch;
      for(i = 0;i < 128;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	is_eof = beye_context().sc_bm_file().eof();
	str[i] = ch;
	if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(!ma_AddString(rdoff_mr,str,true) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(beye_context().sc_bm_file().eof()) break;
    }
  }
  if(rdoff_mr->nItems) ma_Display(rdoff_mr,MOD_REFER,LB_SORTABLE,0);
  else                 beye_context().NotifyBox(NOT_ENTRY,MOD_REFER);
  exit:
  ma_Destroy(rdoff_mr);
  return fpos;
}

__filesize_t RDOff_Parser::action_F5()
{
  __filesize_t fpos;
  unsigned char rec;
  unsigned i;
  char str[33];
  memArray * rdoff_it;
  fpos = beye_context().bm_file().tell();
  if(!(rdoff_it = ma_Build(0,true))) return fpos;
  beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
  while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
  {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 2)
    {
      char ch;
      beye_context().sc_bm_file().seek(2,binary_stream::Seek_Cur);
      for(i = 0;i < 32;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	is_eof = beye_context().sc_bm_file().eof();
	str[i] = ch;
	if(!ch || is_eof) break;
      }
      str[i] = 0;
      if(!ma_AddString(rdoff_it,str,true) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(beye_context().sc_bm_file().eof()) break;
    }
  }
  if(rdoff_it->nItems) ma_Display(rdoff_it,IMPPROC_TABLE,LB_SORTABLE,0);
  else                 beye_context().NotifyBox(NOT_ENTRY,EXP_TABLE);
  exit:
  ma_Destroy(rdoff_it);
  return fpos;
}

__filesize_t RDOff_Parser::show_header()
{
  int endian;
  __filesize_t fpos,entry;
  unsigned long hs_len,cs_len;
  TWindow *w;
  fpos = beye_context().bm_file().tell();
  beye_context().sc_bm_file().seek(5,binary_stream::Seek_Set);
  endian = beye_context().sc_bm_file().read(type_byte);
  hs_len = beye_context().sc_bm_file().read(type_dword);
  beye_context().sc_bm_file().seek(hs_len,binary_stream::Seek_Cur);
  cs_len = beye_context().sc_bm_file().read(type_dword);
  beye_context().sc_bm_file().seek(cs_len,binary_stream::Seek_Cur);
  ds_len = beye_context().sc_bm_file().read(type_dword);
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

tCompare RDOff_Parser::rdoff_compare_reloc(const any_t*e1,const any_t*e2)
{
  const RDOFF_RELOC  *r1, *r2;
  r1 = reinterpret_cast<const RDOFF_RELOC*>(e1);
  r2 = reinterpret_cast<const RDOFF_RELOC*>(e2);
  return __CmpLong__(r1->offset,r2->offset);
}

void  RDOff_Parser::BuildRelocRDOFF()
{
  unsigned char rec;
  if(!(rdoffReloc = la_Build(0,sizeof(RDOFF_RELOC),MemOutBox))) return;
  beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
  while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
  {
    RDOFF_RELOC rdf_r;
    rec = beye_context().sc_bm_file().read(type_byte);
    if(rec == 1)
    {
      unsigned char segno;
      __filesize_t off;
      segno = beye_context().sc_bm_file().read(type_byte);
      rdf_r.is_rel = 0;
      if(segno >= 64)
      {
	rdf_r.is_rel = 1;
	segno -= 64;
      }
      rdf_r.offset = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      off = beye_context().sc_bm_file().read(type_dword);
      if(beye_context().sc_bm_file().eof()) break;
      rdf_r.offset += off;
      rdf_r.reflen = beye_context().sc_bm_file().read(type_byte);
      rdf_r.segto = beye_context().sc_bm_file().read(type_word);
      if(!la_AddData(rdoffReloc,&rdf_r,MemOutBox)) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) goto exit;
      if(beye_context().sc_bm_file().eof()) break;
    }
  }
  exit:
  la_Sort(rdoffReloc,rdoff_compare_reloc);
}

bool RDOff_Parser::rdoffBuildReferStr(const DisMode& parent,char *str,RDOFF_RELOC *reloc,__filesize_t ulShift,int flags)
{
   char name[400],ch,buff[400];
   const char *ptr_type;
   __filesize_t field_val,foff,base_seg_off;
   bool retrf;
   unsigned i;
   if(flags & APREF_USE_TYPE)
   {
     switch(reloc->reflen)
     {
       case 1: ptr_type = "(b) "; break;
       case 2: ptr_type = "off16 "; break;
       default:
       case 4: ptr_type = "off32 "; break;
     }
     strcat(str,ptr_type);
   }
   base_seg_off = FILESIZE_MAX;
   switch(reloc->segto)
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
		 struct rdoff_ImpName key,*ret;
		 key.lsegno = reloc->segto;
		 ret = (rdoff_ImpName*)la_Find(rdoffImpNames,&key,compare_impnames);
		 if(ret)
		 {
		   beye_context().sc_bm_file().seek(ret->nameoff,binary_stream::Seek_Set);
		   name[0] = 0;
		   for(i = 0;i < sizeof(name);i++)
		   {
		     ch = beye_context().sc_bm_file().read(type_byte);
		     name[i] = ch;
		     if(!ch || beye_context().sc_bm_file().eof()) break;
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
   if(reloc->segto >= 3) strcat(str,ptr_type); /** case extern symbol */
   if(reloc->segto < 3 || reloc->is_rel)
   {
     foff = beye_context().sc_bm_file().tell();
     beye_context().sc_bm_file().seek(reloc->offset,binary_stream::Seek_Set);
     switch(reloc->reflen)
     {
       default:
       case 4: field_val = beye_context().sc_bm_file().read(type_dword);
	       break;
       case 2: field_val = beye_context().sc_bm_file().read(type_word);
	       break;
       case 1: field_val = beye_context().sc_bm_file().read(type_byte);
	       break;
     }
     beye_context().sc_bm_file().seek(foff,binary_stream::Seek_Set);
     if(reloc->segto < 3)
     {
       if(base_seg_off < FILESIZE_MAX)
       {
	 base_seg_off += field_val;
	 if(FindPubName(buff,sizeof(buff),base_seg_off))
	 {
	   strcat(str,buff);
	 }
	 else goto unnamed;
       }
       else
       {
	 unnamed:
	 strcat(str,ptr_type);
	 strcat(str,Get8Digit(field_val));
	 if(!EditMode && !DumpMode && (flags & APREF_TRY_LABEL))
	   code_guider.add_go_address(parent,str,reloc->segto ? ds_start + field_val : cs_start + field_val);
	 retrf = true;
       }
     }
     if(reloc->is_rel)
     {
       __filesize_t seg_off;
       __fileoff_t fv;
       seg_off = ulShift < ds_start ? cs_start : ulShift < ds_start+ds_len ? ds_start : FILESIZE_MAX;
       fv = field_val;
       if(!(ulShift + fv + reloc->reflen == seg_off && (flags & APREF_TRY_LABEL)))
       {
	 sprintf(&str[strlen(str)],"(%s)",Get8SignDig(field_val));
       }
     }
   }
   return retrf;
}

bool RDOff_Parser::bind(const DisMode& parent,char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RDOFF_RELOC *rrdoff,key;
  bool ret;
  char buff[400];
  if(flags & APREF_TRY_PIC) return false;
  if(!rdoffReloc) BuildRelocRDOFF();
  if(!rdoffImpNames) ReadImpNameList(beye_context().sc_bm_file(),MemOutBox);
  if(!PubNames) rdoff_ReadPubNameList(beye_context().sc_bm_file(),MemOutBox);
  key.offset = ulShift;
  __codelen = codelen;
  rrdoff = (RDOFF_RELOC*)la_Find(rdoffReloc,&key,rdoff_compare_reloc);
  ret = rrdoff ? rdoffBuildReferStr(parent,str,rrdoff,ulShift,flags) : false;
  if(!ret && (flags & APREF_TRY_LABEL))
  {
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) code_guider.add_go_address(parent,str,r_sh);
       ret = true;
     }
  }
  return ret;
}

RDOff_Parser::RDOff_Parser(CodeGuider& _code_guider)
	    :Binary_Parser(_code_guider)
	    ,code_guider(_code_guider)
{
    unsigned long cs_len;
    beye_context().sc_bm_file().seek(6,binary_stream::Seek_Set);
    rdoff_hdrlen = beye_context().sc_bm_file().read(type_dword);
    beye_context().sc_bm_file().seek(rdoff_hdrlen,binary_stream::Seek_Cur);
    cs_len = beye_context().sc_bm_file().read(type_dword);
    beye_context().sc_bm_file().seek(cs_len,binary_stream::Seek_Cur);
    ds_len = beye_context().sc_bm_file().read(type_dword);
    cs_start = rdoff_hdrlen + 14;
    ds_start = cs_start + cs_len + 4;
}

RDOff_Parser::~RDOff_Parser()
{
  if(rdoffReloc) { la_Destroy(rdoffReloc); rdoffReloc = NULL; }
  if(rdoffImpNames) { la_Destroy(rdoffImpNames); rdoffImpNames = NULL; }
  if(PubNames) { la_Destroy(PubNames); PubNames = NULL; }
}

void RDOff_Parser::rdoff_ReadPubName(binary_stream& b_cache,const struct PubName *it,
		       char *buff,unsigned cb_buff)
{
    unsigned char ch;
    unsigned i;
    b_cache.seek(it->nameoff,binary_stream::Seek_Set);
    for(i = 0;i < cb_buff;i++)
    {
      ch = beye_context().sc_bm_file().read(type_byte);
      buff[i] = ch;
      if(!ch || beye_context().sc_bm_file().eof()) break;
    }
}

bool RDOff_Parser::FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  binary_stream& b_cache = beye_context().sc_bm_file();
  struct PubName *ret,key;
  key.pa = pa;
  if(!PubNames) rdoff_ReadPubNameList(b_cache,MemOutBox);
  ret = (PubName*)la_Find(PubNames,&key,fmtComparePubNames);
  if(ret)
  {
    rdoff_ReadPubName(b_cache,ret,buff,cb_buff);
    return true;
  }
  return udnFindName(pa,buff,cb_buff);
}

void RDOff_Parser::rdoff_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
 unsigned char segno,rec;
 __filesize_t segoff,abs_off;
 unsigned i;
 struct PubName rdf_pn;
 UNUSED(handle);
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
 beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
 while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
 {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 3)
    {
      char ch;
      segno = beye_context().sc_bm_file().read(type_byte);
      segoff = beye_context().sc_bm_file().read(type_dword);
      rdf_pn.nameoff = beye_context().sc_bm_file().tell();
      for(i = 0;i < 32;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	is_eof = beye_context().sc_bm_file().eof();
	if(!ch || is_eof) break;
      }
      abs_off = segno == 0 ? cs_start : segno == 1 ? ds_start : FILESIZE_MAX;
      if(abs_off < FILESIZE_MAX) abs_off += segoff;
      rdf_pn.pa = abs_off;
      rdf_pn.attr = SC_GLOBAL;
      if(!la_AddData(PubNames,&rdf_pn,MemOutBox) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(beye_context().sc_bm_file().eof()) break;
    }
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

tCompare RDOff_Parser::compare_impnames(const any_t*v1,const any_t*v2)
{
  const struct rdoff_ImpName  *pnam1, *pnam2;
  pnam1 = (const struct rdoff_ImpName  *)v1;
  pnam2 = (const struct rdoff_ImpName  *)v2;
  return __CmpLong__(pnam1->lsegno,pnam2->lsegno);
}

void  RDOff_Parser::ReadImpNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
 unsigned char rec;
 unsigned i;
 struct rdoff_ImpName rdf_in;
 UNUSED(handle);
 if(!rdoffImpNames)
   if(!(rdoffImpNames = la_Build(0,sizeof(struct rdoff_ImpName),mem_out))) return;
 beye_context().sc_bm_file().seek(10,binary_stream::Seek_Set);
 while(beye_context().sc_bm_file().tell() < rdoff_hdrlen + 5)
 {
    bool is_eof;
    rec = beye_context().sc_bm_file().read(type_byte);
    is_eof = false;
    if(rec == 2)
    {
      char ch;
      rdf_in.lsegno = beye_context().sc_bm_file().read(type_word);
      rdf_in.nameoff = beye_context().sc_bm_file().tell();
      for(i = 0;i < 32;i++)
      {
	ch = beye_context().sc_bm_file().read(type_byte);
	is_eof = beye_context().sc_bm_file().eof();
	if(!ch) break;
      }
      if(!la_AddData(rdoffImpNames,&rdf_in,MemOutBox) || is_eof) break;
    }
    else
    {
      if(!rdoff_skiprec(rec)) break;
      if(beye_context().sc_bm_file().eof()) break;
    }
 }
 if(rdoffImpNames->nItems) la_Sort(rdoffImpNames,compare_impnames);
}

/** bitness not declared, but we assign 32-bit as default */
int RDOff_Parser::query_bitness(__filesize_t pa) const
{
  UNUSED(pa);
  return DAB_USE32;
}

__filesize_t RDOff_Parser::get_public_symbol(char *str,unsigned cb_str,unsigned *func_class,
			   __filesize_t pa,bool as_prev)
{
    binary_stream& b_cache = beye_context().sc_bm_file();
    __filesize_t fpos;
    size_t idx;
    if(!PubNames) rdoff_ReadPubNameList(b_cache,NULL);
    fpos=fmtGetPubSym(*func_class,pa,as_prev,PubNames,idx);
    if(idx!=std::numeric_limits<size_t>::max()) {
	struct PubName *it;
	it = &((struct PubName  *)PubNames->data)[idx];
	rdoff_ReadPubName(b_cache,it,str,cb_str);
	str[cb_str-1] = 0;
    }
    return fpos;
}

unsigned RDOff_Parser::get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
			 __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = beye_context().sc_bm_file().flength();
  *_class = OC_NOOBJECT;
  *bitness = query_bitness(pa);
  name[0] = 0;
  if(pa < cs_start)
  {
    *end = cs_start;
    ret = 0;
  }
  else
    if(pa >= cs_start && pa < ds_start - 4)
    {
      *start = cs_start;
      *_class = OC_CODE;
      *end = ds_start-4;
      ret = 1;
    }
    else
    {
      if(pa >= ds_start - 4 && pa < ds_start)
      {
	*start = ds_start-4;
	*end = ds_start;
	*_class = OC_NOOBJECT;
	ret = 2;
      }
      else
      {
	if(pa >= ds_start && pa < ds_start + ds_len)
	{
	  *_class = OC_DATA;
	  *start = ds_start;
	  *end = ds_start + ds_len;
	  ret = 3;
	}
	else
	{
	  *start = ds_start+ds_len;
	  *_class = OC_NOOBJECT;
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

bool RDOff_Parser::address_resolving(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 bool bret = true;
 uint32_t res;
 if(cfpos < cs_start)
 {
    strcpy(addr,"RDFH:");
    strcpy(&addr[5],Get4Digit(cfpos));
 }
 else
   if((res=pa2va(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = false;
  return bret;
}

int RDOff_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe() {
  char rbuff[6];
    beye_context().sc_bm_file().seek(0,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(rbuff,sizeof(rbuff));
  return memcmp(rbuff,"RDOFF1",sizeof(rbuff)) == 0 ||
	 memcmp(rbuff,"RDOFF\x1",sizeof(rbuff)) == 0;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) RDOff_Parser(_parent); }
extern const Binary_Parser_Info rdoff_info = {
    "RDOFF (Relocatable Dynamic Object File Format)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
