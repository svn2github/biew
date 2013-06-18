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
#include <sstream>
#include <iomanip>

#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/dos_sys.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class DosSys_Parser : public Binary_Parser {
	public:
	    DosSys_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~DosSys_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	    virtual std::string		address_resolving(__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	private:
	    DOSDRIVER		drv;
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "SysHlp", "", "", "", "", "", "", "", "", "" };
const char* DosSys_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t DosSys_Parser::show_header() const
{
 int keycode;
 TWindow *hwnd;
 bool charun;
 __fileoff_t fpos;
 fpos = bctx.tell();
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

DosSys_Parser::DosSys_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    unsigned char id[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    binary_packet bp=main_handle.read(sizeof(id)); memcpy(id,bp.data(),bp.size());
    if(id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF && id[3] == 0xFF) {
	main_handle.seek(4,binary_stream::Seek_Set);
	bp=main_handle.read(sizeof(DOSDRIVER)); memcpy(&drv,bp.data(),bp.size());
    } else throw bad_format_exception();
}
DosSys_Parser::~DosSys_Parser() {}
int DosSys_Parser::query_platform() const { return DISASM_CPU_IX86; }

std::string DosSys_Parser::address_resolving(__filesize_t cfpos)
{
    std::ostringstream oss;
    if(cfpos < sizeof(DOSDRIVER)+4) oss<<"SYSH:"<<std::hex<<std::setfill('0')<<std::setw(4)<<cfpos;
    return oss.str();
}

__filesize_t DosSys_Parser::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(10014);
	bhelp.close();
    }
    return bctx.tell();
}

__filesize_t DosSys_Parser::va2pa(__filesize_t va) const { return va; }
__filesize_t DosSys_Parser::pa2va(__filesize_t pa) const { return pa; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) DosSys_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info dossys_info = {
    "DOS-driver",	/**< plugin name */
    query_interface
};
} // namespace	usr
