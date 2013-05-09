#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_II
 * @file        plugins/disasm/null_da.c
 * @brief       This file contains implementation of Data disassembler.
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

#include "beye.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/file_ini.h"

namespace	usr {
    class Data_Disassembler : public Disassembler {
	public:
	    Data_Disassembler(DisMode& parent);
	    virtual ~Data_Disassembler();
	
	    virtual const char*	prompt(unsigned idx) const;
	    virtual bool	action_F3();

	    virtual DisasmRet	disassembler(__filesize_t shift,MBuffer insn_buff,unsigned flags);

	    virtual void	show_short_help() const;
	    virtual int		max_insn_len();
	    virtual ColorAttr	get_insn_color(unsigned long clone);

	    virtual int		get_bitness();
	    virtual char	clone_short_name(unsigned long clone);
	    virtual void	read_ini(Ini_Profile&);
	    virtual void	save_ini(Ini_Profile&);
	private:
	    DisMode&		parent;
	    int			nulWidth;
	    char*		outstr;

	    static const char*	width_names[];
    };

const char* Data_Disassembler::width_names[] =
{
   "~Byte",
   "~Word",
   "~Double word",
   "~Quad word"
};

bool Data_Disassembler::action_F3()
{
  unsigned nModes;
  int i;
  nModes = sizeof(width_names)/sizeof(char *);
  i = SelBoxA(width_names,nModes," Select bitness mode: ",nulWidth);
  if(i != -1)
  {
    nulWidth = i;
    return true;
  }
  return false;
}

DisasmRet Data_Disassembler::disassembler(__filesize_t ulShift,
					MBuffer buffer,
					unsigned flags)
{
  DisasmRet ret;
  int cl;
  DisMode::e_disarg type;
  const char *preface;
  if(!((flags & __DISF_SIZEONLY) == __DISF_SIZEONLY))
  {
    memset(&ret,0,sizeof(ret));
    ret.str = outstr;
    switch(nulWidth)
    {
      case 0: preface = "db ";
	      type = DisMode::Arg_Byte;
	      cl = 1;
	      break;
      default:
      case 1: preface = "dw ";
	      type = DisMode::Arg_Word;
	      cl = 2;
	      break;
      case 2: preface = "dd ";
	      type = DisMode::Arg_DWord;
	      cl = 4;
	      break;
      case 3: preface = "dq ";
	      type = DisMode::Arg_QWord;
	      cl = 8;
	      break;
    }
    ret.codelen = cl;
    strcpy(outstr,preface);
    parent.append_digits(outstr,ulShift,APREF_USE_TYPE,cl,buffer,type);
  }
  else
    if(flags & __DISF_GETTYPE) ret.pro_clone = __INSNT_ORDINAL;
    else
    switch(nulWidth)
    {
      case 0: ret.codelen = 1; break;
      default:
      case 1: ret.codelen = 2; break;
      case 2: ret.codelen = 4; break;
      case 3: ret.codelen = 8; break;
    }
  return ret;
}

void Data_Disassembler::show_short_help() const
{
  hlpDisplay(20010);
}

int Data_Disassembler::max_insn_len() { return 8; }
ColorAttr Data_Disassembler::get_insn_color( unsigned long clone )
{
  UNUSED(clone);
  return disasm_cset.cpu_cset[0].clone[0];
}
int Data_Disassembler::get_bitness() { return DAB_USE16; }
char Data_Disassembler::clone_short_name( unsigned long clone )
{
  UNUSED(clone);
  return ' ';
}
Data_Disassembler::Data_Disassembler( DisMode& _parent )
		:Disassembler(_parent)
		,parent(_parent)
		,nulWidth(1)
{
  outstr = new char [1000];
  if(!outstr)
  {
    MemOutBox("Data disassembler initialization");
    exit(EXIT_FAILURE);
  }
}

Data_Disassembler::~Data_Disassembler()
{
   delete outstr;
}

void Data_Disassembler::read_ini( Ini_Profile& ini )
{
  std::string tmps;
  if(beye_context().is_valid_ini_args())
  {
    tmps=beye_context().read_profile_string(ini,"Beye","Browser","SubSubMode3","1");
    nulWidth = (int)strtoul(tmps.c_str(),NULL,10);
    if(nulWidth > 3) nulWidth = 0;
  }
}

void Data_Disassembler::save_ini( Ini_Profile& ini )
{
  char tmps[10];
  sprintf(tmps,"%i",nulWidth);
  beye_context().write_profile_string(ini,"Beye","Browser","SubSubMode3",tmps);
}

const char* Data_Disassembler::prompt(unsigned idx) const {
    switch(idx) {
	case 1: return "Width"; break;
	default: break;
    }
    return "";
}

static Disassembler* query_interface(DisMode& _parent) { return new(zeromem) Data_Disassembler(_parent); }
extern const Disassembler_Info data_disassembler_info = {
    DISASM_DATA,
    "~Data",	/**< plugin name */
    query_interface
};
} // namespace	usr
