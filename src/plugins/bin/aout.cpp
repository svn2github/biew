#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/aout.c
 * @brief       This file contains implementation of a.out file format decoder.
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
#include <stdio.h>
#include <string.h>

#include "libbeye/bswap.h"
#include "colorset.h"
#include "beyeutil.h"
#include "bin_util.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#define ARCH_SIZE 32
#include "plugins/bin/aout64.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class AOut_Parser : public Binary_Parser {
	public:
	    AOut_Parser(binary_stream&,CodeGuider&);
	    virtual ~AOut_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual int			query_endian(__filesize_t) const;
	    virtual bool		address_resolving(char *,__filesize_t);
	    static bool			check_fmt(uint32_t id);
	private:
	    bool			probe_fmt(uint32_t id);
	    const char*			aout_encode_machine(uint32_t info,unsigned *id) const;
	    const char*			aout_encode_hdr(uint32_t info) const;
	    inline uint16_t		AOUT_HALF(const uint16_t* cval) const { return FMT_WORD(*cval,is_msbf); }
	    inline uint32_t		AOUT_WORD(const uint32_t* cval) const { return FMT_DWORD(*cval,is_msbf); }
	    inline uint64_t		AOUT_QWORD(const uint64_t* cval) const { return FMT_QWORD(*cval,is_msbf); }

	    bool is_msbf; /* is most significand byte first */
	    bool is_64bit;
	    binary_stream&		main_handle;
    };
static const char* txt[]={ "AOutHl", "", "", "", "", "", "", "", "", "" };
const char* AOut_Parser::prompt(unsigned idx) const { return txt[idx]; }

const char* AOut_Parser::aout_encode_hdr(uint32_t info) const
{
   switch(N_MAGIC(AOUT_WORD(&info)))
   {
     case OMAGIC: return " a.out-32: Object file ";
     case NMAGIC: return " a.out-32: Pure executable ";
     case ZMAGIC: return " a.out-32: Demand-paged executable ";
     case BMAGIC: return " b.out-32: Object file ";
     case QMAGIC: return " a.out-32: 386BSD demand-paged executable ";
     case CMAGIC: return " a.out-core";
     case OMAGIC64: return " a.out-64: Object file ";
     case NMAGIC64: return " a.out-64: Pure executable ";
     case ZMAGIC64: return " a.out-64: Demand-paged executable ";
     default:     return " Unknown a.out or b.out format ";
   }
}

const char* AOut_Parser::aout_encode_machine(uint32_t info,unsigned* id) const
{
   *id=DISASM_DATA;
   switch(N_MACHTYPE(AOUT_WORD(&info)))
   {
     case 0: *id=DISASM_CPU_SPARC; return "Old Sun-2";
     case 1: *id=DISASM_CPU_PPC; return "M-68010";
     case 2: *id=DISASM_CPU_PPC; return "M-68020";
     case 3: *id=DISASM_CPU_SPARC; return "Sparc";
     case 100: *id=DISASM_CPU_IX86; return "i386";
     case 151: *id=DISASM_CPU_MIPS; return "MIPS1(R3000)";
     case 152: *id=DISASM_CPU_MIPS; return "MIPS2(R6000)";
     default:  return "Unknown CPU";
   }
}

__filesize_t AOut_Parser::show_header()
{
  struct external_exec aout;
  __filesize_t fpos;
  unsigned keycode,dummy;
  TWindow *w;
  fpos = beye_context().tell();
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&aout,sizeof(struct external_exec));
  uint32_t* p_info = (uint32_t*)&aout.e_info;
  w = CrtDlgWndnls(aout_encode_hdr(*p_info),54,7);
  w->goto_xy(1,1);
  w->printf("Flags & CPU                 = %02XH %s(%s)\n"
	   "Length of text section      = %08lXH\n"
	   "Length of data section      = %08lXH\n"
	   "Length of bss area          = %08lXH\n"
	   "Length of symbol table      = %08lXH\n"
	   ,N_FLAGS(*p_info),aout_encode_machine(*p_info,&dummy),is_msbf?"big-endian":"little-endian"
	   ,AOUT_WORD((uint32_t *)&aout.e_text)
	   ,AOUT_WORD((uint32_t *)&aout.e_data)
	   ,AOUT_WORD((uint32_t *)&aout.e_bss)
	   ,AOUT_WORD((uint32_t *)&aout.e_syms));
  w->set_color(dialog_cset.entry);
  w->printf("Start address               = %08lXH"
	   ,AOUT_WORD((uint32_t *)&aout.e_entry));
  w->printf("\n"); w->clreol();
  w->set_color(dialog_cset.main);
  w->printf("Length of text relocation   = %08lXH\n"
	   "Length of data relocation   = %08lXH"
	   ,AOUT_WORD((uint32_t *)&aout.e_trsize)
	   ,AOUT_WORD((uint32_t *)&aout.e_drsize));
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = AOUT_WORD((uint32_t *)aout.e_entry); break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  delete w;
  return fpos;
}

bool AOut_Parser::check_fmt( uint32_t id )
{
  int a32,a64;
  a32=!(N_BADMAG(id));
  a64=!(N_BADMAG64(id));
  return a32 || a64 || N_MAGIC(id)==CMAGIC;
}

bool AOut_Parser::probe_fmt( uint32_t id )
{
  int a32,a64;
  a32=!(N_BADMAG(id));
  a64=!(N_BADMAG64(id));
  if(a64) is_64bit=1;
  return a32 || a64 || N_MAGIC(id)==CMAGIC;
}

AOut_Parser::AOut_Parser(binary_stream& h,CodeGuider&c)
	    :Binary_Parser(h,c)
	    ,main_handle(h)
{
    uint32_t id;
    main_handle.seek(0,binary_stream::Seek_Set);
    id = main_handle.read(type_dword);
    if(probe_fmt(id)) return;
    id=be2me_32(id);
    if(probe_fmt(id)) is_msbf=1;
}
AOut_Parser::~AOut_Parser() {}

int AOut_Parser::query_bitness(__filesize_t off) const
{
   UNUSED(off);
   return is_64bit?DAB_USE64:DAB_USE32;
}

int AOut_Parser::query_endian(__filesize_t off) const
{
   UNUSED(off);
   return is_msbf?DAE_BIG:DAE_LITTLE;
}

bool AOut_Parser::address_resolving(char *addr,__filesize_t fpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  if(fpos < sizeof(struct external_exec))
  {
    strcpy(addr,"a.outh:");
    strcpy(&addr[7],Get2Digit(fpos));
  }
  else bret = false;
  return bret;
}

__filesize_t AOut_Parser::action_F1()
{
  hlpDisplay(10000);
  return beye_context().tell();
}

int AOut_Parser::query_platform() const {
 unsigned id;
 struct external_exec aout;
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&aout,sizeof(struct external_exec));
 aout_encode_machine(*((uint32_t *)aout.e_info),&id);
 return id;
}

static bool probe(binary_stream& main_handle) {
  uint32_t id;
  main_handle.seek(0,binary_stream::Seek_Set);
  id = main_handle.read(type_dword);
  if(AOut_Parser::check_fmt(id)) return 1;
  id=be2me_32(id);
  if(AOut_Parser::check_fmt(id)) return 1;
  return 0;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent) { return new(zeromem) AOut_Parser(h,_parent); }
extern const Binary_Parser_Info aout_info = {
    "a.out (Assembler and link Output)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
