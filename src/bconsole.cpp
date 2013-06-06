#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        bconsole.c
 * @brief       This file contains low level BEYE console functions.
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
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added "ungotstring" function to enable inline assemblers
**/
#include <stack>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "libbeye/kbd_code.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/osdep/system.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include "beye.h"
#include "editor.h"
#include "colorset.h"
#include "beyeutil.h"
#include "tstrings.h"
#include "search.h"
#include "bconsole.h"

namespace	usr {
enum {
    MAXINPUT=79,
    FORMFEED=12
};

static std::stack<int> KB_Buff;

void __FASTCALL__ initBConsole( unsigned long vio_flg,unsigned long twin_flg )
{
  BeyeContext& bctx = beye_context();
  twInit(bctx.system(),bctx.codepage,vio_flg,twin_flg);
  if(bctx.tconsole().vio_width() < 80 || beye_context().tconsole().vio_height() < 3)
  {
    if(bctx.tconsole().vio_width()>16&&beye_context().tconsole().vio_height()>2) {
	unsigned evt,x,y;
	TWindow *win;
	x = (bctx.tconsole().vio_width()-17)/2;
	y = (bctx.tconsole().vio_height()-3)/2;
	win = new(zeromem) TWindow(x,y,x+16,y+2,TWindow::Flag_None | TWindow::Flag_NLS);
	if(!win) goto done;
	win->set_title(" Error ",TWindow::TMode_Center,error_cset.border);
	win->into_center();
	win->set_color(error_cset.main);
	win->set_frame(TWindow::DOUBLE_FRAME,error_cset.border);
	win->goto_xy(1,1);
	win->puts("Screensize<80x3");
	win->show();
	do {
	    evt = GetEvent(NULL,NULL,win);
	}while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_ENTER));
	delete win;
    }
    done:
    twDestroy();
    std::cerr<<"Current size of video buffer is: w="<<beye_context().tconsole().vio_width()<<" h="<<beye_context().tconsole().vio_height()<<std::endl;
    throw std::runtime_error("Size of video buffer must be larger than 79x2");
  }
}

void __FASTCALL__ termBConsole()
{
  twDestroy();
}

/**
   read the next keyboard character
*/
static int  __FASTCALL__ getkey(int hard, void (*func)())
{
    int rc;
    if(!KB_Buff.empty()) { rc=KB_Buff.top(); KB_Buff.pop(); }
    else rc=GetEvent( func ? func : hard ? hard > 1 ?
			    drawAsmEdPrompt : drawEditPrompt : drawEmptyPrompt,
			    func ? NULL : hard ? hard > 1 ?
			    EditAsmActionFromMenu: NULL: NULL,
			    NULL);
    return rc;
}

static bool  __FASTCALL__ ungotkey(int keycode)
{
    KB_Buff.push(keycode);
    return true;
}

bool __FASTCALL__ ungotstring(char *string)
{
  int pos;
  for (pos = strlen(string)-1; pos>=0; pos--) {
    if (ungotkey(string[pos]) == false)
      return false;
  }
  return true;
}

int __FASTCALL__ xeditstring(TWindow* w,char *s,const char *legal,unsigned maxlength, void (*func)())
{
  return eeditstring(w,s,legal,&maxlength,1,NULL,__ESS_ENABLEINSERT,NULL,func);
}

inline bool __CONST_FUNC__ isSpace(int val) { return ((val+1)%3 ? 0 : 1); }
inline bool __CONST_FUNC__ isFirstD(int pos) { return isSpace(pos-1); }
inline bool __CONST_FUNC__ isSecondD(int pos) { return isSpace(pos+1); }

static bool insert = true;

int __FASTCALL__ eeditstring(TWindow* w,char *s,const char *legal, unsigned *maxlength,unsigned _y,unsigned *stx,unsigned attr,char *undo, void (*func)())
{
 int c;
 unsigned len = attr & __ESS_HARDEDIT ? *maxlength : attr & __ESS_NON_C_STR ? _y : strlen(s);
 unsigned pos = len;
 unsigned y = attr & __ESS_HARDEDIT ? _y : 1;
 int lastkey,func_getkeys;
 char ashex = attr & __ESS_ASHEX;
 bool freq = (attr & __ESS_HARDEDIT) == __ESS_HARDEDIT;
 if(stx) pos = *stx;
 if(!(attr & __ESS_HARDEDIT))
   TWindow::set_cursor_type(attr & __ESS_ENABLEINSERT ? insert ? TWindow::Cursor_Normal : TWindow::Cursor_Solid : TWindow::Cursor_Normal);
 do
 {
  unsigned i;
  Loop:
  w->freeze();
  if(!(attr & __ESS_NOREDRAW))
  {
    if(!undo) w->write(1,y,(const uint8_t*)s,len);
    else
    {
      for(i = 0;i < len;i++)
      {
	w->set_color(s[i] == undo[i] ? browser_cset.edit.main : browser_cset.edit.change);
	w->write(i+1,y,(const uint8_t*)&s[i],1);
      }
    }
    if(!(attr & __ESS_HARDEDIT))
    {
      w->goto_xy(len + 1,y);
      if(ashex)
	if(isSecondD(pos) && pos >= len)
	{
	  w->putch('.');
	  w->goto_xy(len + 2,y);
	}
      for(i = len; i < *maxlength;i++)
	 w->putch((attr & __ESS_FILLER_7BIT) == __ESS_FILLER_7BIT ? TWC_DEF_FILLER : TWC_MED_SHADE);
    }
  }
  w->refresh(y);
  w->goto_xy(pos + 1, y);
  func_getkeys = attr & __ESS_HARDEDIT ? attr & 0x0020 ? 2 : 1 : 0;
  new_keycode:
  c = getkey(func_getkeys, func);
  lastkey = c;
  attr |= __ESS_NOREDRAW;
  switch(c)
  {
   case KE_MOUSE:
   case KE_SHIFTKEYS:
		      goto new_keycode;
   case KE_HOME : if(!(ashex && isSecondD(pos))) pos = 0; break;
   case KE_END  : if(!(ashex && isSecondD(pos))) pos = len; break;
   case KE_LEFTARROW : if(pos)
		       {
			 if(!ashex) pos--;
			 else
			  if(!isSecondD(pos))
			  {
			    if(pos > 3) pos -=3;
			    else        pos = 0;
			  }
		       }
		       break;
   case KE_RIGHTARROW: if(pos < len)
		       {
			 if(!ashex) pos++;
			 else
			  if(!isSecondD(pos))
			  {
			     pos += 3;
			     if(pos >= len) pos = len;
			  }
		       }
		       break;
   case KE_BKSPACE   : if (pos > 0 && !(attr & __ESS_HARDEDIT))
		       {
			 attr &= ~__ESS_NOREDRAW;
			 memmove(&s[pos-1], &s[pos], len - pos + 1);
			 pos--; len--;
			 if(ashex)
			   if(isSpace(pos) && pos > 2)
			   {
			     memmove(&s[pos-2], &s[pos], len - pos + 2);
			     pos-=2; len-=2;
			   }
		       }
		       break;
   case KE_DEL : if (pos < len && !(attr & __ESS_HARDEDIT))
		    {
		      attr &= ~__ESS_NOREDRAW;
		      memmove(&s[pos], &s[pos+1], len - pos);
		      len--;
			 if(ashex)
			   if(!isSpace(pos) && pos > 2)
			   {
			     memmove(&s[pos], &s[pos+2], len - pos);
			     len-=2;
			   }
		    }
		    break;
   case KE_CTL_BKSPACE:
		   if(undo)
		   {
		     attr &= ~__ESS_NOREDRAW;
		     if(pos) pos--; if(ashex) if(isSpace(pos) && pos) pos--;
		     s[pos] = undo[pos];
		   }
		   break;
   case KE_INS :   if(attr & __ESS_ENABLEINSERT &&
		      !(attr & __ESS_HARDEDIT) &&
		      !ashex)
			    insert = insert ? false : true;
		    TWindow::set_cursor_type(attr & __ESS_ENABLEINSERT ? insert ? TWindow::Cursor_Normal : TWindow::Cursor_Solid : TWindow::Cursor_Normal);
		    break;
   case KE_ENTER  : break;
   case KE_ESCAPE : len = 0; break;
   default     :
		  /** FunKey trough int 16 returned XX00 exclude Ctrl0-CtrlZ */
		  if(!(c & 0x00FF) || c == KE_TAB || c == KE_SHIFT_TAB || c == KE_BKSPACE) { c = KE_ENTER; break; }
		  if ( legal == 0 || strchr(legal, c) != NULL )
		  {
		    attr &= ~__ESS_NOREDRAW;
		    if(!freq && !(attr & __ESS_WANTRETURN)) { freq = true; s[0] = 0; len = pos = 0; ungotkey(c); break; }
		    if(pos < *maxlength)
		    {
		     if(insert)
		     {
		      if(len < *maxlength)
		      {
			memmove(&s[pos + 1], &s[pos], len - pos + 1);
			len++;
		      }
		     }
		     else
		       if (pos >= len) len++;
		     if(len > *maxlength) len = *maxlength;
		     s[pos++] = c;
		       if(ashex)
		       {
			 if(isSpace(pos))
			 {
			   if (pos >= len) len++;
			   if(len > *maxlength) len = *maxlength;
			   s[pos] = attr & __ESS_HARDEDIT && !((pos + 1) % 12) ? '-' : ' ';
			   pos++;
			 }
			 if(isSecondD(pos) && pos < len) s[pos] = '.';
		       }
		    }
		    else s[pos - 1] = c;
		  }
		  break;
  } /* switch */
  if(!freq) freq = true;
  if(!(attr & __ESS_HARDEDIT)) s[len] = 0;
  if(ashex)
    if(!isFirstD(pos))
      if(c != KE_ESCAPE) goto Loop;
  if(attr & __ESS_WANTRETURN) if(!(ashex && isSecondD(pos))) break;
 }
 while ( (c != KE_ENTER) && (c != KE_ESCAPE));
 if(!(attr & __ESS_HARDEDIT)) TWindow::set_cursor_type(TWindow::Cursor_Off);
 if(!(attr & __ESS_WANTRETURN && attr & __ESS_NOTUPDATELEN)) *maxlength = len;
 if(stx) *stx = pos;
 return lastkey;
} /* editstring */

TWindow *__FASTCALL__ PleaseWaitWnd()
{
   TWindow *w;
   w = CrtDlgWndnls(SYSTEM_BUSY,14,1);
   w->goto_xy(1,1); w->puts(PLEASE_WAIT);
   return w;
}

struct percent_data
{
  time_t   _time;
  time_t   prev_time;
  unsigned _percents;
  bool    is_first;
};

static long PercentWndCallBack(TWindow *it,unsigned event, unsigned long param,const any_t*data)
{
  struct percent_data *my_data;
  UNUSED(param);
  UNUSED(data);
  switch(event)
  {
    case WM_CREATE:
		     if((my_data=new percent_data) != NULL)
		     {
			it->set_user_data(my_data);
		     }
		     break;
    case WM_DESTROY:
		     my_data = (percent_data*)it->get_user_data();
		     if(my_data) delete my_data;
		     break;
     default: break;
  }
  return 0L;
}

TWindow *__FASTCALL__ PercentWnd(const std::string& text,const std::string& title)
{
  TWindow *ret;
  static time_t sttime;
  struct percent_data* my_data;
  twcRegisterClass("PERCENT_WND", __CS_ORDINAL, PercentWndCallBack);
  ret = new(zeromem) TWindow(1,1,53,6,TWindow::Flag_Has_Frame | TWindow::Flag_NLS,"PERCENT_WND");
  ret->into_center();
  ret->set_color(dialog_cset.main);
  ret->set_frame(TWindow::UP3D_FRAME,dialog_cset.main);
  ret->set_title(title,TWindow::TMode_Center,dialog_cset.title);
  ret->set_footer(" [ Ctrl-Break ] - Abort ",TWindow::TMode_Right,dialog_cset.footer);
  ret->clear();
  ret->goto_xy(1,1); ret->puts(text);
  ret->draw_frame(1,2,52,4,TWindow::DN3D_FRAME,dialog_cset.main);
  ret->show();
  time(&sttime);
  my_data = (percent_data*)ret->get_user_data();
  if(my_data)
  {
     my_data->_time = my_data->prev_time = sttime;
     my_data->_percents = 0;
     my_data->is_first = true;
  }
  return ret;
}

bool __FASTCALL__ ShowPercentInWnd(TWindow *pw,unsigned percents)
{
  unsigned cells,remaind, prev_prcnt = 0;
  time_t sttime =0,curtime,deltat, prev_time = 0;
  struct tm *tm;
  struct percent_data* my_data;
  bool is_first = true;
  char outb[50];
  bool ret;
  my_data = (percent_data*)pw->get_user_data();
  if(my_data)
  {
    prev_prcnt = my_data->_percents;
    sttime = my_data->_time;
    prev_time = my_data->prev_time;
    is_first = my_data->is_first;
  }
  if(percents != prev_prcnt || is_first)
  {
    if(percents > 100) percents = 100;
    cells = percents/2;
    remaind = percents%2;
    memset(outb,TWC_FL_BLK,cells);
    if(remaind) outb[cells++] = TWC_LF_HBLK;
    if(cells < sizeof(outb)) memset(&outb[cells],TWC_DEF_FILLER,sizeof(outb)-cells);
    pw->write(2,3,(const uint8_t*)outb,sizeof(outb));
  }
  time(&curtime);
  if(prev_time != curtime || is_first)
  {
    deltat = curtime - sttime;
    tm = gmtime(&deltat);
    strftime(outb,sizeof(outb),"%X",tm);
    pw->goto_xy(1,5);
    pw->puts("Elapsed time: ");
    pw->puts(outb);
  }
  if(my_data)
  {
    my_data->_percents = percents;
    my_data->is_first = false;
    my_data->prev_time = curtime;
  }
  ret = !IsKbdTerminate();
  CleanKbdTermSig();
  return ret;
}

TWindow * __FASTCALL__ CrtDlgWndnls(const std::string& title,tAbsCoord width,tAbsCoord height )
{
 TWindow *win;
 TWindow::twc_flag flags;
 flags = TWindow::Flag_Has_Frame|TWindow::Flag_NLS;
 win = new(zeromem) TWindow(0,0,width+2,height+2,flags);
 win->into_center();
 win->set_color(dialog_cset.main);
 win->clear();
 win->set_frame(TWindow::DOUBLE_FRAME,dialog_cset.border);
 if(!title.empty()) win->set_title(title,TWindow::TMode_Center,dialog_cset.title);
 win->show();
 return win;
}

TWindow * __FASTCALL__ CrtHlpWndnls(const std::string& title,tAbsCoord x2,tAbsCoord y2)
{
 TWindow *win;
 TWindow::twc_flag flags = TWindow::Flag_Has_Frame|TWindow::Flag_NLS;
 win = new(zeromem) TWindow(0,0,x2+2,y2+2,flags);
 win->into_center();
 win->set_color(help_cset.main);
 win->clear();
 win->set_frame(TWindow::DOUBLE_FRAME,help_cset.border);
 if(!title.empty()) win->set_title(title,TWindow::TMode_Center,help_cset.title);
 win->show();
 return win;
}

TWindow * __FASTCALL__ CreateEditor(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,TWindow::twc_flag flags)
{
 TWindow *ret;
 ret = new(zeromem) TWindow(X1,Y1,X2-X1+1,Y2-Y1+1,flags);
 ret->set_color(dialog_cset.editor.active);
 ret->clear();
 return ret;
}

void BeyeContext::message_box(const std::string& text,const std::string& title,
					       ColorAttr base,ColorAttr frame) const
{
    unsigned evt;
    unsigned slen,tlen;
    slen = text.length() + 3;
    tlen = title.length() + 2;
    slen = std::min(std::max(slen,tlen)+1,unsigned(78));
    error_wnd().resize(slen,3);
    error_wnd().into_center();
    error_wnd().set_frame(TWindow::DOUBLE_FRAME,frame);
    error_wnd().set_title(title,TWindow::TMode_Center,frame);
    error_wnd().set_color(base);
    error_wnd().clear();
    error_wnd().show_on_top();
    error_wnd().goto_xy(2,1);
    error_wnd().puts(text);
    do {
	evt = GetEvent(drawEmptyPrompt,NULL,&(error_wnd()));
    }while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_SPACE || evt == KE_ENTER));
    error_wnd().hide();
    error_wnd().resize(beye_context().tconsole().vio_width(),beye_context().tconsole().vio_height()); /* It for reserving memory */
}

void BeyeContext::TMessageBox(const std::string& text,const std::string& title) const
{
    message_box(text,title,dialog_cset.main,dialog_cset.title);
}

void BeyeContext::NotifyBox(const std::string& text,const std::string& title) const
{
    message_box(text,!title.empty() ? title : NOTE_MSG,notify_cset.main,notify_cset.border);
}

void BeyeContext::ErrMessageBox(const std::string& text,const std::string& title) const
{
    message_box(text,!title.empty() ? title : ERROR_MSG,error_cset.main,error_cset.border);
}

void BeyeContext::WarnMessageBox(const std::string& text,const std::string& title) const
{
    message_box(text,!title.empty() ? title : WARN_MSG,warn_cset.main,warn_cset.border);
}

void BeyeContext::errnoMessageBox(const std::string& text,const std::string& title,int __errno__) const
{
    char stmp[256];
    sprintf(stmp,"%s: %i (%s)",text.c_str(),__errno__,strerror(__errno__));
    beye_context().ErrMessageBox(stmp,title);
}

} // namespace	usr

