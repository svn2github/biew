#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_addons
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
#include "libbeye/kbd_code.h"

namespace beye {
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
  TWindow * hwnd = CrtDlgWndnls(" Console information ",63,std::min(tAbsCoord(21),tvioHeight-2));
  unsigned evt;
  int i,j,len;
  unsigned char str[80];
  twUseWin(hwnd);
  twFreezeWin(hwnd);
  strcpy((char *)str,"°³ 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F ³Name");
  len = strlen((char *)str);
  twGotoXY(1,1);
  twPrintF("Console: %ux%ux%u\n"
	   "Skin:    %s"
	   ,tvioWidth
	   ,tvioHeight
	   ,tvioNumColors
	   ,beye_context().scheme_name.c_str());
  twDirectWrite(1,3,str,len);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++)  twDirectWrite(i+1,4,str,1);
  str[1] = TWC_SV;
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; twDirectWrite(1,i+5,str,2); }
  str[0] = TWC_SH_SV;
  twDirectWrite(2,4,str,1);
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++)
    {
       twSetColor(Color(i),Color(j));
       str[0] = ' '; str[1] = '*'; str[2] = ' ';
       twDirectWrite(j*3+3,i+5,str,3);
    }
  }
  twSetColorAttr(dialog_cset.main);
  str[0] = TWC_SH;
  for(i = 0;i < 63;i++) twDirectWrite(i+1,21,str,1);
  str[0] = TWC_SH_Su;
  twDirectWrite(2,21,str,1);
  str[0] = TWC_SV;
  for(i = 0;i < 16;i++) twDirectWrite(51,i+5,str,1);
  str[0] = TWC_SH_SV;
  twDirectWrite(51,4,str,1);
  str[0] = TWC_SH_Su;
  twDirectWrite(51,21,str,1);
  for(i = 0;i < 16;i++) { twGotoXY(52,i+5); twPutS(named_color_def[i].name); }
  twRefreshWin(hwnd);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(hwnd);
}

static Addon* query_interface() { return new(zeromem) ConsoleInfo_Addon(); }
extern const Addon_Info ConsoleInfo = {
    "~Console information",
    query_interface
};
} // namespace beye
