#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_addons
 * @file        addons/sys/kbdview.c
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
 * @since       2003
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stddef.h>

#include "addons/addon.h"

#include "colorset.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
    class InputView_Addon : public Addon {
	public:
	    InputView_Addon();
	    virtual ~InputView_Addon();
	
	    virtual void	run();
    };

InputView_Addon::InputView_Addon() {}
InputView_Addon::~InputView_Addon() {}

void InputView_Addon::run()
{
  TWindow * hwnd = CrtDlgWndnls(" Input viewer ",78,2);
  int rval, do_exit;
  char head[80], text[80];
  drawEmptyPrompt();
  hwnd->freeze();
  hwnd->set_footer(" [Escape] - quit ",TW_TMODE_RIGHT,dialog_cset.selfooter);
  hwnd->refresh();
  do_exit=0;
  do
  {
    rval = __inputRawInfo(head,text);
    if(rval==-1)
    {
	ErrMessageBox("Not implemented yet!","");
	break;
    }
    hwnd->goto_xy(1,1);
    hwnd->puts(head);
    hwnd->clreol();
    hwnd->goto_xy(1,2);
    hwnd->puts(text);
    hwnd->clreol();
    if(!rval) do_exit++;
  }
  while(do_exit<2);
  CloseWnd(hwnd);
}

static Addon* query_interface() { return new(zeromem) InputView_Addon(); }
extern const Addon_Info InputViewer = {
    "~Input viewer",
    query_interface
};
} // namespace beye
