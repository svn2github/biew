#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/coff386.c
 * @brief       This file contains implementation of COFF-i386 file format decoder.
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
#include <algorithm>
#include <set>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "reg_form.h"
#include "tstrings.h"
#include "codeguid.h"
#include "colorset.h"
#include "bconsole.h"
#include "udn.h"
#include "beyehelp.h"
#include "plugins/bin/coff.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    struct RELOC_COFF386 {
	unsigned long offset;
	unsigned long nameoff;
	unsigned      type;
	bool operator<(const RELOC_COFF386& rhs) const { return offset<rhs.offset; }
    };
    class Coff_Parser : public Binary_Parser {
	public:
	    Coff_Parser(binary_stream&,CodeGuider&,udn&);
	    virtual ~Coff_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F7();
	    virtual __filesize_t	action_F10();

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev);
	    virtual Object_Info		get_object_attribute(__filesize_t pa);
	private:
	    Object_Info			_get_object_attribute(__filesize_t pa) const;
	    __filesize_t		BuildReferStrCoff386(const DisMode&parent,std::string& str,const RELOC_COFF386& rne,int flags) const;
	    bool			coffSymTabReadItemsIdx(binary_stream&handle,unsigned long idx,std::string& name,unsigned& secnum,__filesize_t& offset) const;
	    void			BuildRelocCoff386();
	    __filesize_t		CalcEntryCoff(unsigned long idx,bool display_msg) const;
	    std::vector<std::string>	coffSymTabReadItems(binary_stream&handle,size_t nnames) const;
	    const char*			coffEncodeClass(unsigned _class) const;
	    const char*			coffEncodeType(unsigned type) const;
	    const char*			coff386_encode_hdr(unsigned info) const;
	    std::vector<SCNHDR>		__coffReadObjects(binary_stream& handle,size_t n) const;
	    void			paint_pages(TWindow& win,const std::vector<SCNHDR>& names,unsigned start) const;
	    std::string			coffReadLongName(binary_stream&handle,__filesize_t offset) const;
	    bool			FindPubName(std::string& buff,__filesize_t pa) const;
	    std::string			coff_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    void			coff_ReadPubNameList(binary_stream& handle);

	    inline uint16_t		COFF_WORD(const uint8_t* cval) const { return (uint16_t)(*(const uint16_t *)(const uint8_t *)cval); }
	    inline uint32_t		COFF_DWORD(const uint8_t* cval) const { return (uint32_t)(*(const uint32_t *)(const uint8_t *)cval); }

	    struct external_filehdr	coff386hdr;
	    AOUTHDR		coff386ahdr;
	    SCNHDR*		coff386so;
	    uint_fast16_t	nsections;
	    binary_stream*	coff_cache;
	    __filesize_t	strings_ptr;
	    std::set<symbolic_information>	PubNames;
	    char		__codelen;
	    std::set<RELOC_COFF386>	RelocCoff386;

	    binary_stream&	main_handle;
	    CodeGuider&		code_guider;
	    udn&		_udn;
    };
static const char* txt[]={ "CofHlp", "", "", "", "", "", "SymTab", "", "", "Objects" };
const char* Coff_Parser::prompt(unsigned idx) const { return txt[idx]; }

bool Coff_Parser::FindPubName(std::string& buff,__filesize_t pa) const
{
  symbolic_information key;
  std::set<symbolic_information>::const_iterator ret;
  key.pa = pa;
  ret = PubNames.find(key);
  if(ret!=PubNames.end()) {
    buff=coff_ReadPubName(*coff_cache,*ret);
    return true;
  }
  return _udn.find(pa,buff);
}

std::string Coff_Parser::coffReadLongName(binary_stream& handle,__filesize_t offset) const
{
    std::string rc;
  unsigned i;
  __filesize_t fpos;
   fpos = handle.tell();
   handle.seek(offset+strings_ptr,binary_stream::Seek_Set);
   for(i = 0;;i++) {
     char ch;
     ch = handle.read(type_byte);
     if(ch && !handle.eof()) rc+=ch;
     else  break;
   }
   handle.seek(fpos,binary_stream::Seek_Set);
   return rc;
}

__filesize_t Coff_Parser::va2pa(__filesize_t va) const
{
    uint_fast16_t i;
    for(i = 0;i < nsections;i++) {
	if(va >= COFF_DWORD(coff386so[i].s_vaddr) && va <= COFF_DWORD(coff386so[i].s_vaddr)+COFF_DWORD(coff386so[i].s_size)) {
	    return COFF_DWORD(coff386so[i].s_scnptr) + (va - COFF_DWORD(coff386so[i].s_vaddr));
	}
    }
    return 0L;
}

__filesize_t Coff_Parser::pa2va(__filesize_t pa) const
{
    uint_fast16_t i;
    for(i = 0;i < nsections;i++) {
	if(pa >= COFF_DWORD(coff386so[i].s_scnptr) && pa <= COFF_DWORD(coff386so[i].s_scnptr)+COFF_DWORD(coff386so[i].s_size)) {
	    return COFF_DWORD(coff386so[i].s_vaddr) + (pa - COFF_DWORD(coff386so[i].s_scnptr));
	}
    }
    return 0L;
}

void Coff_Parser::paint_pages(TWindow& win,const std::vector<SCNHDR>& objs,unsigned start) const
{
    char buffer[81];
    win.freeze();
    win.clear();
    sprintf(buffer," Object Table [ %u / %u ] ",start + 1,objs.size());
    win.set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    win.goto_xy(1,1);
    win.printf(
	  "Object Name                    = %8s\n"
	  "Physical address               = %08lXH\n"
	  "Virtual address                = %08lXH\n"
	  "Section size                   = %08lXH\n"
	  "File pointer to raw data       = %08lXH\n"
	  "File pointer to relocations    = %08lXH\n"
	  "File pointer to line numbers   = %08lXH\n"
	  "Number of relocations          = %04XH\n"
	  "Number of line numbers         = %04XH\n"
	  "Flags                          = %08lXH\n"
	  "  [%c] - section type is text\n"
	  "  [%c] - section type is data\n"
	  "  [%c] - section type is bss"
	  ,objs[start].s_name
	  ,COFF_DWORD(objs[start].s_paddr)
	  ,COFF_DWORD(objs[start].s_vaddr)
	  ,COFF_DWORD(objs[start].s_size)
	  ,COFF_DWORD(objs[start].s_scnptr)
	  ,COFF_DWORD(objs[start].s_relptr)
	  ,COFF_DWORD(objs[start].s_lnnoptr)
	  ,COFF_WORD(objs[start].s_nreloc)
	  ,COFF_WORD(objs[start].s_nlnno)
	  ,COFF_DWORD(objs[start].s_flags)
	  ,Gebool((COFF_DWORD(objs[start].s_flags) & STYP_TEXT) == STYP_TEXT)
	  ,Gebool((COFF_DWORD(objs[start].s_flags) & STYP_DATA) == STYP_DATA)
	  ,Gebool((COFF_DWORD(objs[start].s_flags) & STYP_BSS) == STYP_BSS)
	  );
    win.refresh_full();
}

std::vector<SCNHDR> Coff_Parser::__coffReadObjects(binary_stream& handle,size_t n) const
{
    std::vector<SCNHDR> rc;
    size_t i;
    for(i = 0;i < n;i++) {
	SCNHDR po;
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(&po,sizeof(SCNHDR));
	rc.push_back(po);
    }
    return rc;
}

__filesize_t Coff_Parser::action_F10()
{
    binary_stream* handle;
    unsigned nnames;
    __filesize_t fpos,off;
    fpos = beye_context().tell();
    nnames = COFF_WORD(coff386hdr.f_nscns);
    if(!nnames) { beye_context().NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
    handle = coff_cache;
    off = sizeof(coff386hdr);
    if(COFF_WORD(coff386hdr.f_opthdr)) off += COFF_WORD(coff386hdr.f_opthdr);
    handle->seek(off,binary_stream::Seek_Set);
    std::vector<SCNHDR> objs = __coffReadObjects(*handle,nnames);
    if(!objs.empty()) {
	int ret;
	ret = PageBox(70,13,objs,*this,&Coff_Parser::paint_pages);
	if(ret != -1)  fpos = COFF_DWORD(objs[ret].s_scnptr);
    }
    return fpos;
}

const char* Coff_Parser::coff386_encode_hdr(unsigned info) const
{
   switch(info)
   {
     case I386MAGIC: return " i386 coff ";
     case I386PTXMAGIC: return " i386 PTX coff ";
     case I386AIXMAGIC: return " i386 AIX coff ";
     case LYNXCOFFMAGIC: return " Lynx coff ";
     default:    return " Unknown coff i386 format ";
   }
}

__filesize_t Coff_Parser::show_header() const
{
  __filesize_t fpos,entry,v_entry;
  unsigned keycode;
  TWindow * w;
  fpos = beye_context().tell();
  v_entry = entry = 0L;
  if(*(unsigned short *)coff386ahdr.magic == ZMAGIC)
  {
    v_entry = entry = *(__filesize_t *)&coff386ahdr.entry;
    entry = va2pa(v_entry);
  }
  w = CrtDlgWndnls(coff386_encode_hdr(COFF_WORD(coff386hdr.f_magic)),54,12);
  w->goto_xy(1,1);
  w->printf(
	   "Number of sections          = %04XH\n"
	   "Time & date stamp           = %08lXH\n"
	   "File pointer to symtab      = %08lXH\n"
	   "Number of symtab entries    = %08lXH\n"
	   "Size of optional header     = %04XH\n"
	   "COFF flags                  = %04XH\n"
	   "  [%c] - Rel. info stripped\n"
	   "  [%c] - File is executable (no unresolv. refs.)\n"
	   "  [%c] - Line numbers stripped\n"
	   "  [%c] - Local symbols stripped\n"
	   "  [%c] - File is 32-bits little endian\n"
	   ,COFF_WORD(coff386hdr.f_nscns)
	   ,COFF_DWORD(coff386hdr.f_timdat)
	   ,COFF_DWORD(coff386hdr.f_symptr)
	   ,COFF_DWORD(coff386hdr.f_nsyms)
	   ,COFF_WORD(coff386hdr.f_opthdr)
	   ,COFF_WORD(coff386hdr.f_flags)
	   ,Gebool((COFF_WORD(coff386hdr.f_flags) & F_RELFLG) == F_RELFLG)
	   ,Gebool((COFF_WORD(coff386hdr.f_flags) & F_EXEC) == F_EXEC)
	   ,Gebool((COFF_WORD(coff386hdr.f_flags) & F_LNNO) == F_LNNO)
	   ,Gebool((COFF_WORD(coff386hdr.f_flags) & F_LSYMS) == F_LSYMS)
	   ,Gebool((COFF_WORD(coff386hdr.f_flags) & F_AR32WR) == F_AR32WR));
  w->set_color(dialog_cset.entry);
  w->printf("Entry point                 = %08lXH (VA=%08lXH)"
	   ,entry,v_entry);
  w->clreol();
  w->set_color(dialog_cset.main);
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = entry; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  delete w;
  return fpos;
}

const char* Coff_Parser::coffEncodeType(unsigned type) const
{
  const char *ret;
  switch(type)
  {
    case T_NULL: ret = "T_NULL"; break;
    case T_VOID: ret = "T_VOID"; break;
    case T_CHAR: ret = "T_CHAR"; break;
    case T_SHORT: ret = "T_SHORT"; break;
    case T_INT: ret = "T_INT"; break;
    case T_LONG: ret = "T_LONG"; break;
    case T_FLOAT: ret = "T_FLOAT"; break;
    case T_DOUBLE: ret = "T_DOUBLE"; break;
    case T_STRUCT: ret = "T_STRUCT"; break;
    case T_UNION: ret = "T_UNION"; break;
    case T_ENUM: ret = "T_ENUM"; break;
    case T_MOE: ret = "T_MOE"; break;
    case T_UCHAR: ret = "T_UCHAR"; break;
    case T_USHORT: ret = "T_USHORT"; break;
    case T_UINT: ret = "T_UINT"; break;
    case T_ULONG: ret = "T_ULONG"; break;
    case T_LNGDBL: ret = "T_LNGDBL"; break;
    default: ret = "T_UNK"; break;
  }
  return ret;
}

const char* Coff_Parser::coffEncodeClass(unsigned _class) const
{
  const char *ret;
  switch(_class)
  {
    case C_EFCN: ret = "C_EFCN"; break;
    case C_NULL: ret = "C_NULL"; break;
    case C_AUTO: ret = "C_AUTO"; break;
    case C_EXT: ret = "C_EXT"; break;
    case C_STAT: ret = "C_STAT"; break;
    case C_REG: ret = "C_REG"; break;
    case C_EXTDEF: ret = "C_EXTDEF"; break;
    case C_LABEL: ret = "C_LABEL"; break;
    case C_ULABEL: ret = "C_ULABEL"; break;
    case C_MOS: ret = "C_MOS"; break;
    case C_ARG: ret = "C_ARG"; break;
    case C_STRTAG: ret = "C_STRTAG"; break;
    case C_MOU: ret = "C_MOU"; break;
    case C_UNTAG: ret = "C_UNTAG"; break;
    case C_TPDEF: ret = "C_TPDEF"; break;
    case C_USTATIC: ret = "C_USTATIC"; break;
    case C_ENTAG: ret = "C_ENTAG"; break;
    case C_MOE: ret = "C_MOE"; break;
    case C_REGPARM: ret = "C_REGPARM"; break;
    case C_FIELD: ret = "C_FIELD"; break;
    case C_AUTOARG: ret = "C_AUTOARG"; break;
    case C_LASTENT: ret = "C_LASTENT"; break;
    case C_BLOCK: ret = "C_BLOCK"; break;
    case C_FCN: ret = "C_FCN"; break;
    case C_EOS: ret = "C_EOS"; break;
    case C_FILE: ret = "C_FILE"; break;
    case C_LINE: ret = "C_LINE"; break;
    case C_ALIAS: ret = "C_ALIAS"; break;
    case C_HIDDEN: ret = "C_HIDDEN"; break;
    default: ret = "C_UNK"; break;
  }
  return ret;
}

std::vector<std::string> Coff_Parser::coffSymTabReadItems(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    unsigned length;
    handle.seek(COFF_DWORD(coff386hdr.f_symptr),binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	struct external_syment cse;
	std::string stmp;
	handle.read(&cse,sizeof(struct external_syment));
	if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
	    COFF_DWORD(cse.e.e.e_offset) >= 4)
	    stmp=coffReadLongName(handle,COFF_DWORD(cse.e.e.e_offset));
	else stmp.assign((char*)cse.e.e_name,E_SYMNMLEN);
	if(IsKbdTerminate() || handle.eof()) break;
	length = stmp.length();
	if(length > 38) stmp.replace(38,std::string::npos,">>>");
//   else  {  memset(&stmp[length],' ',38-length);  stmp[38] = 0; }
	char sbuf;
	sprintf(&sbuf,"(%04hX.%08lX) %s.%s"
	   ,COFF_WORD(cse.e_scnum)
	   ,(unsigned long)COFF_DWORD(cse.e_value)
	   ,coffEncodeClass(cse.e_sclass[0])
	   ,coffEncodeType(COFF_WORD(cse.e_type)));
	stmp+=sbuf;
	rc.push_back(stmp);
    }
    return rc;
}

__filesize_t Coff_Parser::CalcEntryCoff(unsigned long idx,bool display_msg) const
{
  struct external_syment cse;
  uint_fast16_t sec_num;
  __filesize_t fpos;
  fpos = 0L;
  main_handle.seek(COFF_DWORD(coff386hdr.f_symptr)+idx*sizeof(struct external_syment),binary_stream::Seek_Set);
  main_handle.read(&cse,sizeof(struct external_syment));
  sec_num = COFF_WORD(cse.e_scnum);
  if(sec_num && sec_num <= COFF_WORD(coff386hdr.f_nscns) &&
    ((cse.e_sclass[0] == C_EXT ||
      cse.e_sclass[0] == C_STAT ||
      cse.e_sclass[0] == C_HIDDEN) ||
     (cse.e_sclass[0] == C_LABEL &&
      COFF_DWORD(cse.e_value))))
  {
    fpos = COFF_DWORD(coff386so[sec_num-1].s_scnptr)+COFF_DWORD(cse.e_value);
  }
  else
    if(display_msg)
      beye_context().ErrMessageBox(NO_ENTRY,"");
  return fpos;
}

__filesize_t Coff_Parser::action_F7()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = "Symbol Table";
    ssize_t nnames = COFF_DWORD(coff386hdr.f_nsyms);
    int flags = LB_SELECTIVE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = coffSymTabReadItems(main_handle,nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
exit:
    if(ret != -1) fpos = CalcEntryCoff(ret,true);
    return fpos;
}

/***************************************************************************/
/*********************  REFS COFF386  **************************************/
/***************************************************************************/
void Coff_Parser::BuildRelocCoff386()
{
  TWindow * w;
  size_t j,segcount, nr;
  RELOC_COFF386 rel;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(PubNames.empty()) coff_ReadPubNameList(main_handle);
  w->goto_xy(1,1);
  w->puts(BUILD_REFS);
  for(segcount = 0;segcount < COFF_WORD(coff386hdr.f_nscns);segcount++)
  {
    bool is_eof;
    is_eof = false;
    main_handle.seek(COFF_DWORD(coff386so[segcount].s_relptr),binary_stream::Seek_Set);
    nr = COFF_WORD(coff386so[segcount].s_nreloc);
    for(j = 0;j < nr;j++)
    {
      struct external_reloc er;
      main_handle.read(&er,sizeof(struct external_reloc));
      if((is_eof = main_handle.eof()) != 0) break;
      rel.offset = er.r_vaddr + COFF_DWORD(coff386so[segcount].s_scnptr);
      rel.nameoff = er.r_symndx;
      rel.type = er.r_type;
      RelocCoff386.insert(rel);
    }
    if(is_eof) break;
  }
  delete w;
}

bool Coff_Parser::coffSymTabReadItemsIdx(binary_stream& handle,unsigned long idx,
					std::string& name,
					unsigned& secnum,
					__filesize_t& offset) const
{
    struct external_syment cse;
    if(idx >= COFF_DWORD(coff386hdr.f_nsyms)) return false;
    handle.seek(COFF_DWORD(coff386hdr.f_symptr) + idx*sizeof(struct external_syment),binary_stream::Seek_Set);
    handle.read(&cse,sizeof(struct external_syment));
    if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
	COFF_DWORD(cse.e.e.e_offset) >= 4)
	name=coffReadLongName(handle,COFF_DWORD(cse.e.e.e_offset));
    else name.assign((char*)cse.e.e_name,E_SYMNMLEN);
    secnum = COFF_WORD(cse.e_scnum);
    offset = COFF_DWORD(cse.e_value);
    return true;
}

__filesize_t Coff_Parser::BuildReferStrCoff386(const DisMode& parent,std::string& str,const RELOC_COFF386& rne,int flags) const
{
    __filesize_t offset,e;
    bool retval;
    unsigned long val;
    unsigned secnum=0;
    bool is_idx,val_assigned;
    std::string secname;
    std::string name,pubname;
    retval = true;
    main_handle.seek(rne.offset,binary_stream::Seek_Set);
    val = main_handle.read(type_dword);
  /* rne->nameoff it's only pointer to name descriptor */
    is_idx = coffSymTabReadItemsIdx(*coff_cache,rne.nameoff,name,secnum,offset);
    val_assigned = false;
    if(is_idx) {
	if(!offset && secnum) {
	/*
	    try resolve some relocations (like .text+1234) with names from
	    public name list
	*/
	    if(FindPubName(pubname,COFF_DWORD(coff386so[secnum-1].s_scnptr) + val)) {
		str+=pubname;
		val_assigned = true;
	    } else goto def_val;
	} else {
def_val:
	    str+=name;
	}
    } else str+=".?bad?";
    if(val && !val_assigned) {
	str+="+";
	str+=Get8Digit(val);
	if(secnum) retval = (COFF_DWORD(coff386so[secnum-1].s_scnptr)+val)?true:false;
	else       retval = false;
    }
    if(rne.type == RELOC_REL32 && (flags & APREF_TRY_LABEL) != APREF_TRY_LABEL) {
	Object_Info rc = _get_object_attribute(rne.offset);
	str+="-";
	str+=rc.name;
    }
    e = CalcEntryCoff(rne.nameoff,false);
    if(!DumpMode && !EditMode && e && (flags & APREF_TRY_LABEL) == APREF_TRY_LABEL)
						code_guider.add_go_address(parent,str,e);
    return retval;
}

bool Coff_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  std::set<RELOC_COFF386>::const_iterator rcoff386;
  RELOC_COFF386 key;
  bool ret;
  std::string buff;
  ret = false;
  if(flags & APREF_TRY_PIC) return ret;
  if(PubNames.empty()) coff_ReadPubNameList(main_handle);
  if((COFF_WORD(coff386hdr.f_flags) & F_RELFLG) == F_RELFLG) goto try_pub;
  if(RelocCoff386.empty()) BuildRelocCoff386();
  key.offset = ulShift;
  __codelen = codelen;
  rcoff386 = RelocCoff386.find(key);
  if(rcoff386!=RelocCoff386.end()) ret = BuildReferStrCoff386(parent,str,(*rcoff386),flags);
  try_pub:
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

Coff_Parser::Coff_Parser(binary_stream& h,CodeGuider& _code_guider,udn& u)
	    :Binary_Parser(h,_code_guider,u)
	    ,main_handle(h)
	    ,code_guider(_code_guider)
	    ,_udn(u)
{
    __filesize_t s_off = sizeof(coff386hdr);
    uint_fast16_t i;
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&coff386hdr,sizeof(struct external_filehdr));
    if(COFF_WORD(coff386hdr.f_opthdr)) main_handle.read(&coff386ahdr,sizeof(AOUTHDR));
    nsections = COFF_WORD(coff386hdr.f_nscns);
    coff386so = new SCNHDR[nsections];
    coff_cache = main_handle.dup();
    if(COFF_WORD(coff386hdr.f_opthdr)) s_off += COFF_WORD(coff386hdr.f_opthdr);
    coff_cache->seek(s_off,binary_stream::Seek_Set);
    for(i = 0;i < nsections;i++) coff_cache->read(&coff386so[i],sizeof(SCNHDR));
    strings_ptr = COFF_DWORD(coff386hdr.f_symptr)+COFF_DWORD(coff386hdr.f_nsyms)*sizeof(struct external_syment);
}

Coff_Parser::~Coff_Parser()
{
  delete coff386so;
  if(coff_cache != &main_handle) delete coff_cache;
}

int Coff_Parser::query_bitness(__filesize_t off) const
{
  UNUSED(off);
  return DAB_USE32;
}

bool Coff_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos < sizeof(struct external_filehdr))
  {
    addr="coffih:";
    addr+=Get2Digit(cfpos);
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

__filesize_t Coff_Parser::action_F1()
{
  hlpDisplay(10002);
  return beye_context().tell();
}

std::string Coff_Parser::coff_ReadPubName(binary_stream& b_cache,const symbolic_information& it) const
{
    if(!it.addinfo)
      return coffReadLongName(b_cache,it.nameoff);
    else {
	b_cache.seek(it.nameoff,binary_stream::Seek_Set);
	char buff[it.addinfo+1];
	b_cache.read(buff,it.addinfo);
	buff[it.addinfo] = 0;
	return buff;
    }
}

void Coff_Parser::coff_ReadPubNameList(binary_stream& handle)
{
 unsigned i,nnames;
 symbolic_information pn;
 handle.seek(COFF_DWORD(coff386hdr.f_symptr),binary_stream::Seek_Set);
 nnames = COFF_DWORD(coff386hdr.f_nsyms);
 for(i = 0;i < nnames;i++)
 {
   struct external_syment cse;
   __filesize_t where;
   uint_fast16_t sec_num;
   where = handle.tell();
   handle.read(&cse,sizeof(struct external_syment));
   sec_num = COFF_WORD(cse.e_scnum);
   if(sec_num && sec_num <= COFF_WORD(coff386hdr.f_nscns) &&
     (cse.e_sclass[0] == C_EXT ||
      cse.e_sclass[0] == C_STAT ||
      cse.e_sclass[0] == C_HIDDEN ||
      cse.e_sclass[0] == C_LABEL))
   {
     if(COFF_DWORD(cse.e.e.e_zeroes) == 0L &&
	COFF_DWORD(cse.e.e.e_offset) >= 4)
     {
       pn.nameoff = COFF_DWORD(cse.e.e.e_offset);
       pn.addinfo = 0;
     }
     else
     {
       pn.nameoff = where;
       pn.addinfo = E_SYMNMLEN;
     }
     pn.pa = COFF_DWORD(coff386so[sec_num-1].s_scnptr)+COFF_DWORD(cse.e_value);
     pn.attr = cse.e_sclass[0] == C_EXT ? SC_GLOBAL : SC_LOCAL;
     PubNames.insert(pn);
   }
   if(handle.eof()) break;
 }
}

Symbol_Info Coff_Parser::get_public_symbol(__filesize_t pa,bool as_prev)
{
    Symbol_Info rc;
    if(PubNames.empty()) coff_ReadPubNameList(*coff_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    rc.pa=find_symbolic_information(PubNames,rc._class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	rc.name=coff_ReadPubName(*coff_cache,*idx);
    }
    return rc;
}

Object_Info Coff_Parser::_get_object_attribute(__filesize_t pa) const
{
    Object_Info rc;
    uint_fast16_t i;
    rc.start = 0;
    rc.end = main_handle.flength();
    rc._class = OC_NOOBJECT;
    rc.bitness = DAB_USE32;
    rc.number = 0;
    for(i = 0;i < nsections;i++) {
	if(pa >= rc.start && pa < COFF_DWORD(coff386so[i].s_scnptr)) {
	    /** means between two objects */
	    rc.end = COFF_DWORD(coff386so[i].s_scnptr);
	    rc.number = 0;
	    break;
	}
	if(pa >= COFF_DWORD(coff386so[i].s_scnptr) && pa < COFF_DWORD(coff386so[i].s_scnptr) + COFF_DWORD(coff386so[i].s_size)) {
	    rc.number = i+1;
	    rc.start = COFF_DWORD(coff386so[i].s_scnptr);
	    rc.end = rc.start + COFF_DWORD(coff386so[i].s_size);
	    rc._class = (COFF_DWORD(coff386so[i].s_flags) & STYP_TEXT) == STYP_TEXT ? OC_CODE : OC_DATA;
	    rc.name.assign((const char *)coff386so[i].s_name,sizeof(coff386so[i].s_name));
	    break;
	}
	rc.start = COFF_DWORD(coff386so[i].s_scnptr) + COFF_DWORD(coff386so[i].s_size);
    }
    return rc;
}

Object_Info Coff_Parser::get_object_attribute(__filesize_t pa) {
    return _get_object_attribute(pa);
}

int Coff_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe(binary_stream& main_handle) {
    uint_fast16_t id;
    main_handle.seek(0,binary_stream::Seek_Set);
    id = main_handle.read(type_word);
    return !(I386BADMAG(id));
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) Coff_Parser(h,_parent,u); }
extern const Binary_Parser_Info coff_info = {
    "coff-i386 (Common Object File Format)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
