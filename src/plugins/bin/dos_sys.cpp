#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/dos_sys.c
 * @brief       This file contains implementation of DOS driver file format.
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
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/dos_sys.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class DosSys_Parser : public Binary_Parser {
	public:
	    DosSys_Parser(binary_stream&,CodeGuider&);
	    virtual ~DosSys_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	private:
	    DOSDRIVER		drv;
	    binary_stream&	main_handle;
    };
static const char* txt[]={ "SysHlp", "", "", "", "", "", "", "", "", "" };
const char* DosSys_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t DosSys_Parser::show_header()
{
 int keycode;
 TWindow *hwnd;
 bool charun;
 __fileoff_t fpos;
 fpos = beye_context().tell();
 hwnd = CrtDlgWndnls(" DOS Device Driver Header ",57,13);
 charun = (drv.ddAttribute & 0x8000) == 0x8000;
 if(charun) hwnd->printf("Device Name               = %8s\n",drv.ddName);
 else       hwnd->printf("Device Supports             %u Block Units\n",(unsigned)drv.ddName[0]);
 hwnd->printf(
	  "Attributes :\n"
	  "  [%c]  STDIN\n"
	  "  [%c]  STDOUT\n"
	  "  [%c]  STDNUL\n"
	  "  [%c]  CLOCK\n"
	  "  [%c]  Support Logical Device\n"
	  "  [%c]  Support OPEN/CLOSE/RM\n"
	  "  [%c]  Non IBM Block-Device\n"
	  "  [%c]  Support IOCTL\n"
	  "  Device is                %s\n"
	 ,Gebool(drv.ddAttribute & 0x0001)
	 ,Gebool(drv.ddAttribute & 0x0002)
	 ,Gebool(drv.ddAttribute & 0x0004)
	 ,Gebool(drv.ddAttribute & 0x0008)
	 ,Gebool(drv.ddAttribute & 0x0040)
	 ,Gebool(drv.ddAttribute & 0x0800)
	 ,Gebool(drv.ddAttribute & 0x2000)
	 ,Gebool(drv.ddAttribute & 0x4000)
	 ,(drv.ddAttribute & 0x8000 ? "CHARACTERS" : "BLOCKS"));
 hwnd->set_color(dialog_cset.entry);
 hwnd->printf(">STRATEGY routine offset  = %04hXH bytes [Enter]",drv.ddStrategyOff);
 hwnd->printf("\n"); hwnd->clreol();
 hwnd->set_color(dialog_cset.altentry);
 hwnd->printf(">INTERRUPT routine offset = %04hXH bytes [Ctrl+Enter | F5]",drv.ddInterruptOff);
 hwnd->clreol();
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_ENTER) { fpos = drv.ddStrategyOff; break; }
   else
     if(keycode == KE_CTL_ENTER || keycode == KE_F(5)) { fpos = drv.ddInterruptOff; break; }
     else
       if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 delete hwnd;
 return fpos;
}

DosSys_Parser::DosSys_Parser(binary_stream& h,CodeGuider& code_guider)
	    :Binary_Parser(h,code_guider)
	    ,main_handle(h)
{
    unsigned char id[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(id,sizeof(id));
    if(id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF && id[3] == 0xFF) {
	main_handle.seek(4,binary_stream::Seek_Set);
	main_handle.read((any_t*)&drv,sizeof(DOSDRIVER));
    }
}
DosSys_Parser::~DosSys_Parser() {}
int DosSys_Parser::query_platform() const { return DISASM_CPU_IX86; }

bool DosSys_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
  bool bret = true;
  if(cfpos < sizeof(DOSDRIVER)+4) addr=std::string("SYSH:")+Get4Digit(cfpos);
  else bret = false;
  return bret;
}

__filesize_t DosSys_Parser::action_F1()
{
  hlpDisplay(10014);
  return beye_context().tell();
}

__filesize_t DosSys_Parser::va2pa(__filesize_t va)
{
  return va;
}

__filesize_t DosSys_Parser::pa2va(__filesize_t pa)
{
  return pa;
}

static bool probe(binary_stream& main_handle) {
  unsigned char id[4];
  bool ret = false;
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(id,sizeof(id));
  if(id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF && id[3] == 0xFF) ret = true;
  return ret;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent) { return new(zeromem) DosSys_Parser(h,_parent); }
extern const Binary_Parser_Info dossys_info = {
    "DOS-driver",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
