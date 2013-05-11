#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_II
 * @file        plugins/disasm/arm.c
 * @brief       This file contains implementation of ARM disassembler.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libbeye/bswap.h"

#include "beye.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "plugins/disasm/arm/arm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/file_ini.h"

namespace	usr {
DisasmRet ARM_Disassembler::disassembler(__filesize_t ulShift,
					MBuffer buffer,
					unsigned flags)
{
  DisasmRet ret;
  armBigEndian = beye_context().bin_format().query_endian(ulShift)==DAE_BIG?1:0;
  if(flags == __DISF_NORMAL)
  {
    memset(&ret,0,sizeof(ret));
    ret.str = outstr;
    ret.codelen = armBitness==DAB_USE32?4:2;
    if(armBitness==DAB_USE32)
    {
	uint32_t opcode32;
	opcode32=armBigEndian?be2me_32(*((uint32_t *)buffer)):le2me_32(*((uint32_t *)buffer));
	arm32Disassembler(&ret,ulShift,opcode32,flags);
    }
    else
    {
	uint16_t opcode16;
	opcode16=armBigEndian?be2me_16(*((uint16_t *)buffer)):le2me_16(*((uint16_t *)buffer));
	arm16Disassembler(&ret,ulShift,opcode16,flags);
    }
  }
  else
  {
    if(flags & __DISF_GETTYPE) ret.pro_clone = __INSNT_ORDINAL;
    else ret.codelen = armBitness==DAB_USE32?4:2;
  }
  return ret;
}

bool ARM_Disassembler::action_F1()
{
  hlpDisplay(20040);
  return false;
}

void ARM_Disassembler::show_short_help() const
{
 char *msgAsmText,*title;
 char **strs;
 unsigned size,i,evt;
 unsigned long nstrs;
 TWindow * hwnd;
 Beye_Help bhelp;
 if(!bhelp.open(true)) return;
 size = (unsigned)bhelp.get_item_size(20041);
 if(!size) goto armhlp_bye;
 msgAsmText = new char [size+1];
 if(!msgAsmText)
 {
   mem_off:
   MemOutBox(" Help Display ");
   goto armhlp_bye;
 }
 if(!bhelp.load_item(20041,msgAsmText))
 {
   delete msgAsmText;
   goto armhlp_bye;
 }
 msgAsmText[size] = 0;
 if(!(strs = bhelp.point_strings(msgAsmText,size,&nstrs))) goto mem_off;
 title = msgAsmText;
 hwnd = CrtHlpWndnls(title,73,14);
 for(i = 0;i < nstrs;i++)
 {
   unsigned rlen;
   tvioBuff it;
   t_vchar chars[__TVIO_MAXSCREENWIDTH];
   t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
   ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
   it.chars = chars;
   it.oem_pg = oem_pg;
   it.attrs = attrs;
   rlen = strlen(strs[i]);
   rlen = bhelp.fill_buffer(&it,__TVIO_MAXSCREENWIDTH,strs[i],rlen,0,NULL,0);
   hwnd->write(2,i+2,&it,rlen);
 }
 delete msgAsmText;
 hwnd->goto_xy(2,3);
 {
   hwnd->goto_xy(2,3);
   i=0;
   {
     hwnd->set_color(disasm_cset.engine[0].engine);
     hwnd->puts("ARM CPU");
     hwnd->clreol();
   }
   hwnd->goto_xy(2,4);
   {
     hwnd->set_color(disasm_cset.engine[1].engine);
     hwnd->puts("VFP extension");
     hwnd->clreol();
   }
   hwnd->goto_xy(2,5);
   {
     hwnd->set_color(disasm_cset.engine[2].engine);
     hwnd->puts("XScale extensions");
     hwnd->clreol();
   }
 }
 do
 {
   evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
 }
 while(!(evt == KE_ESCAPE || evt == KE_F(10)));
 delete hwnd;
 armhlp_bye:
 bhelp.close();
}

int ARM_Disassembler::max_insn_len() { return 8; }
ColorAttr ARM_Disassembler::get_insn_color( unsigned long clone )
{
  if((clone & ARM_XSCALE)==ARM_XSCALE) return disasm_cset.engine[2].engine;
  else
  if((clone & ARM_FPU)==ARM_FPU) return disasm_cset.engine[1].engine;
  else
	return disasm_cset.engine[0].engine;
}
ColorAttr ARM_Disassembler::get_opcode_color( unsigned long clone ) { return get_insn_color(clone); }

int ARM_Disassembler::get_bitness() { return armBitness; }
char ARM_Disassembler::clone_short_name( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}

ARM_Disassembler::ARM_Disassembler( DisMode& _parent )
	    :Disassembler(_parent)
	    ,parent(_parent)
	    ,armBitness(DAB_USE32)
	    ,armBigEndian(1)
{
  outstr = new char[1000];
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
  arm16Init();
  arm32Init();
}

ARM_Disassembler::~ARM_Disassembler()
{
   arm32Term();
   arm16Term();
   delete outstr;
}

void ARM_Disassembler::read_ini( Ini_Profile& ini )
{
  std::string tmps;
  if(beye_context().is_valid_ini_args())
  {
    tmps=beye_context().read_profile_string(ini,"Beye","Browser","SubSubMode3","1");
    armBitness = (int)strtoul(tmps.c_str(),NULL,10);
    if(armBitness > 1 && armBitness != DAB_AUTO) armBitness = 0;
    tmps=beye_context().read_profile_string(ini,"Beye","Browser","SubSubMode4","1");
    armBigEndian = (int)strtoul(tmps.c_str(),NULL,10);
    if(armBigEndian > 1) armBigEndian = 0;
  }
}

void ARM_Disassembler::save_ini(Ini_Profile& ini)
{
  char tmps[10];
  sprintf(tmps,"%i",armBitness);
  beye_context().write_profile_string(ini,"Beye","Browser","SubSubMode3",tmps);
  sprintf(tmps,"%i",armBigEndian);
  beye_context().write_profile_string(ini,"Beye","Browser","SubSubMode4",tmps);
}

static const char *arm_bitness_names[] =
{
   "~Thumb-16",
   "~Full-32"
};

bool ARM_Disassembler::action_F3()
{
  unsigned nModes;
  int i;
  nModes = sizeof(arm_bitness_names)/sizeof(char *);
  i = SelBoxA(arm_bitness_names,nModes," Select bitness mode: ",armBitness);
  if(i != -1)
  {
    armBitness = ((i==0)?DAB_USE16:DAB_USE32);
    return true;
  }
  return false;
}

static const char *arm_endian_names[] =
{
   "~Little endian",
   "~Big endian"
};

bool ARM_Disassembler::action_F4()
{
  unsigned nModes;
  int i;
  nModes = sizeof(arm_endian_names)/sizeof(char *);
  i = SelBoxA(arm_endian_names,nModes," Select endian mode: ",armBigEndian);
  if(i != -1)
  {
    armBigEndian = i;
    return true;
  }
  return false;
}

const char* ARM_Disassembler::prompt(unsigned idx) const {
    switch(idx) {
	case 0: return "x86Hlp"; break;
	case 2: return "Bitnes"; break;
	case 3: return "Endian"; break;
	default: break;
    }
    return "";
}

static Disassembler* query_interface(DisMode& _parent) { return new(zeromem) ARM_Disassembler(_parent); }

extern const Disassembler_Info arm_disassembler_info = {
    DISASM_CPU_ARM,
    "A~RMv5TE/XScale",	/**< plugin name */
    query_interface
};
} // namespace	usr
