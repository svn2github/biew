#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        events.c
 * @brief       This file contains console event handler of BEYE project.
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
#include <queue>

#include <limits.h>
#include <string.h>

#include "bconsole.h"
#include "beye.h"
#include "libbeye/kbd_code.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/osdep/system.h"

namespace	usr {

static std::queue<int> kb_queue;

bool  __FASTCALL__ IsKbdTerminate()
{
  return beye_context().system().get_cbreak( );
}

void __FASTCALL__ CleanKbdTermSig()
{
  beye_context().system().set_cbreak(false);
}

static tAbsCoord mx,my;

static int  __FASTCALL__ getPromptKey(int flag)
{
 int ret;
 if(mx <= 7) ret = KE_F(1);
 else
   if(mx <= 15) ret = KE_F(2);
   else
     if(mx <= 23) ret = KE_F(3);
     else
       if(mx <= 31) ret = KE_F(4);
       else
	 if(mx <= 39) ret = KE_F(5);
	 else
	   if(mx <= 47) ret = KE_F(6);
	   else
	     if(mx <= 55) ret = KE_F(7);
	     else
	       if(mx <= 63) ret = KE_F(8);
	       else
		 if(mx <= 71) ret = KE_F(9);
		 else         ret = KE_F(10);
 if(flag & KS_SHIFT) ret += KE_SHIFT_F(1) - KE_F(1);
 else
   if(flag & KS_ALT) ret += KE_ALT_F(1) - KE_F(1);
   else
     if(flag & KS_CTRL) ret += KE_CTL_F(1) - KE_F(1);
 return ret;
}

static int  __FASTCALL__ __GetEvent( void (*prompt)() ,TWindow *win)
{
 static unsigned char oFlag = UCHAR_MAX;
 static void (* oprompt)() = 0;
 unsigned char Flag;
 int key;
 /** Unconditional repaint prompt */
 TConsole& tconsole = beye_context().tconsole();
 Flag = tconsole.kbd_get_shifts();
 if(Flag != oFlag || oprompt !=prompt)
 {
    oFlag = Flag; oprompt = prompt;
    if(prompt) (*prompt)();
 }
 while(1)
 {
     key = tconsole.kbd_get_key(beye_context().kbdFlags);
     switch( key )
     {
       case KE_SHIFTKEYS:
	     Flag = tconsole.kbd_get_shifts();
	     if(Flag != oFlag || oprompt !=prompt)
	     {
	       oFlag = Flag; oprompt = prompt;
	       if(prompt) (*prompt)();
	     }
	     break;
       case KE_MOUSE:
	     tconsole.mouse_get_pos(mx,my);
	     if((tconsole.mouse_get_buttons() & MS_LEFTPRESS) == MS_LEFTPRESS)
	     {
		  if(my == (unsigned)(tconsole.vio_height() - 1))
		  {
		    while(1)
		    {
		      if((tconsole.mouse_get_buttons() & MS_LEFTPRESS) != MS_LEFTPRESS) break;
		      tconsole.mouse_get_pos(mx,my);
		    }
		    if(my != (unsigned)(tconsole.vio_height() - 1)) continue;
		    return getPromptKey(tconsole.kbd_get_shifts());
		  }
		  else
		  {
		    tAbsCoord X1,Y1,X2,Y2;
		    unsigned wdh,hght;
		    if(win)
		    {
		      win->get_pos(X1,Y1,X2,Y2);
		      X1--; Y1--; X2--; Y2--;
		    }
		    else
		    {
		      X1 = 0; X2 = beye_context().main_wnd().client_width(); Y1 = 1; Y2 = beye_context().main_wnd().client_height() - 1;
		      TWindow* wnd=TWindow::at_pos(mx,my);
		        if(!wnd) return KE_MOUSE;
		        X1 = 0; X2 = wnd->client_width(); Y1 = 1; Y2 = wnd->client_height() - 1;
		    }
		    wdh = X2 - X1;
		    hght = Y2 - Y1;
		    if(win && (mx < X1 || mx > X2 || my < Y1 || my > Y2))
		    {
		       while(1)
		       {
			  if((tconsole.mouse_get_buttons() & MS_LEFTPRESS) != MS_LEFTPRESS) break;
		       }
		       return KE_ESCAPE;
		    }
		    if(my <= Y2 && my >= Y1 + 3*hght/4) return KE_DOWNARROW;
		    else
		      if(my >= Y1 && my <= Y1 + hght/4) return KE_UPARROW;
		      else
			if(mx <= X2 && mx >= X1 + 3*wdh/4) return KE_RIGHTARROW;
			else
			  if(mx >= X1 && mx <= X1 + wdh/4) return KE_LEFTARROW;
		  }
	      }
	default: return key;
     }
 }
 return key;
}

static void  __FASTCALL__ __GetEventQue(void (*prompt)(), TWindow *win)
{
  int key;
  key = __GetEvent(prompt,win);
  kb_queue.push(key);
}

int __FASTCALL__ GetEvent( void (*prompt)(),int (*alt_action)(),TWindow * win)
{
  int key;
  while(kb_queue.empty()) __GetEventQue(prompt,win);
  key = kb_queue.front();
  kb_queue.pop();
  if(key==KE_TAB && alt_action) key=(*alt_action)();
  return key;
}

void __FASTCALL__ PostEvent(int code)
{
   kb_queue.push(code);
}
} // namespace	usr

