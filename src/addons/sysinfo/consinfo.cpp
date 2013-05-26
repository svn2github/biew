#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_addons
 * @file        addons/sys/consinfo.c
 * @brief       This file contains simple implementation console information.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <algorithm>

#include <string.h>
#include <stddef.h>

#include "addons/addon.h"

#include "beye.h"
#include "colorset.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/kbd_code.h"

namespace	usr {
    class ConsoleInfo_Addon : public Addon {
	public:
	    ConsoleInfo_Addon();
	    virtual ~ConsoleInfo_Addon();
	
	    virtual void	run();
    };

ConsoleInfo_Addon::ConsoleInfo_Addon() {}
ConsoleInfo_Addon::~ConsoleInfo_Addon() {}

void ConsoleInfo_Addon::run()
{
  BeyeContext& bctx = beye_context();
  TWindow * hwnd = CrtDlgWndnls(" Console information ",63,std::min(tAbsCoord(21),bctx.tconsole().vio_height()-2));
  unsigned evt;
  int i,j,len;
  unsigned char str[80];
  hwnd->freeze();
  strcpy((char *)str,"°³ 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F ³Name");
  len = strlen((char *)str);
  hwnd->goto_xy(1,1);
  hwnd->printf(
	   "Console: %ux%ux%u\n"
	   "Skin:    %s"
	   ,bctx.tconsole().vio_width()
	   ,bctx.tconsole().vio_height()
	   ,bctx.tconsole().vio_num_colors()
	   ,bctx.scheme_name.c_str());
  hwnd->write(1,3,str,len);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++)  hwnd->write(i+1,4,str,1);
  str[1] = TWC_SV;
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; hwnd->write(1,i+5,str,2); }
  str[0] = TWC_SH_SV;
  hwnd->write(2,4,str,1);
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++)
    {
       hwnd->set_color(Color(i),Color(j));
       str[0] = ' '; str[1] = '*'; str[2] = ' ';
       hwnd->write(j*3+3,i+5,str,3);
    }
  }
  hwnd->set_color(dialog_cset.main);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++) hwnd->write(i+1,21,str,1);
  str[0] = TWC_SH_Su;
  hwnd->write(2,21,str,1);
  str[0] = TWC_SV;
  for(i = 0;i < 16;i++) hwnd->write(51,i+5,str,1);
  str[0] = TWC_SH_SV;
  hwnd->write(51,4,str,1);
  str[0] = TWC_SH_Su;
  hwnd->write(51,21,str,1);
  for(i = 0;i < 16;i++) { hwnd->goto_xy(52,i+5); hwnd->puts(named_color_def[i].name); }
  hwnd->refresh();
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  delete hwnd;
}

static Addon* query_interface() { return new(zeromem) ConsoleInfo_Addon(); }
extern const Addon_Info ConsoleInfo = {
    "~Console information",
    query_interface
};
} // namespace	usr
