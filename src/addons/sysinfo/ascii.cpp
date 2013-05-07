#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_addons
 * @file        addons/sys/ascii.c
 * @brief       This file contains simple implementation ASCII table viewer.
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
 * @author      Alexander Krisak and Andrew Golovnia
 * @date        23.07.2003
 * @note        Russian locales support: KOI-8, CP866, CP1251, ISO8859-5.
 *              Tested at ASPLinux 7.3 and ASPLinux 9
**/
#include <string.h>

#include "addons/addon.h"

#include "bconsole.h"
#include "reg_form.h"
#include "beyeutil.h"
#include "colorset.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
    class ASCII_Addon : public Addon {
	public:
	    ASCII_Addon();
	    virtual ~ASCII_Addon();
	
	    virtual void	run();
	private:
	    TWindow*		CreatePanelNF(tAbsCoord x1,tAbsCoord y1,tAbsCoord x2,tAbsCoord y2);
    };

ASCII_Addon::ASCII_Addon() {}
ASCII_Addon::~ASCII_Addon() {}

TWindow*  ASCII_Addon::CreatePanelNF(tAbsCoord x1,tAbsCoord y1,tAbsCoord x2,tAbsCoord y2)
{
 TWindow *win;
 win = WindowOpen(x1,y1,x2,y2);
 win->set_color(dialog_cset.main);
 win->clear();
 win->show();
 return win;
}

void ASCII_Addon::run()
{
  TWindow * hwnd = CrtDlgWndnls(" ASCII table ",34,18);
  tAbsCoord x1,x2,y1,y2;
  hwnd->get_pos(&x1,&x2,&y1,&y2);
  TWindow * hpnl = CreatePanelNF(x1+4,y1+4,x2-1,y2-1);
  unsigned evt;
  int i,j;
  unsigned char str[35];
  hwnd->freeze();
  strcpy((char *)str,"°³0 1 2 3 4 5 6 7 8 9 A B C D E F");
  hwnd->direct_write(1,1,str,34);
  str[0] = TWC_SH;
  str[1] = 0;
  for(i = 0;i < 34;i++)  hwnd->direct_write(i+1,2,str,1);
  str[1] = TWC_SV;
  str[2] = 0;
  for(i = 0;i < 16;i++) { str[0] = i < 0x0A ? i + '0' : i - 0x0A + 'A'; hwnd->direct_write(1,i+3,str,2); }
  str[0] = TWC_SV_SH;
  str[1] = 0;
  hwnd->direct_write(2,2,str,1);
  hpnl->freeze();
  for(i = 0;i < 16;i++)
  {
    for(j = 0;j < 16;j++) { str[j*2] = i*16 + j; str[j*2 + 1] = ' '; }
    hpnl->direct_write(1,i+1,str,31);
  }
  hpnl->refresh();
  hwnd->refresh();
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,hwnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(hpnl);
  CloseWnd(hwnd);
}

static Addon* query_interface() { return new(zeromem) ASCII_Addon(); }
extern const Addon_Info AsciiTable = {
    "~ASCII table",
    query_interface
};
} // namespace beye

