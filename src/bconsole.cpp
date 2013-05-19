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
#include <algorithm>
#include <iostream>
#include <fstream>

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

static int KB_Buff[64];
static unsigned char KB_freq = 0;

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
    std::cerr<<"Size of video buffer must be larger than 79x2"<<std::endl;
    std::cerr<<"Current size of video buffer is: w="<<beye_context().tconsole().vio_width()<<" h="<<beye_context().tconsole().vio_height()<<std::endl;
    exit(EXIT_FAILURE);
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
 return KB_freq ? KB_Buff[--KB_freq] :
		  GetEvent( func ? func : hard ? hard > 1 ?
			    drawAsmEdPrompt : drawEditPrompt : drawEmptyPrompt,
			    func ? NULL : hard ? hard > 1 ?
			    EditAsmActionFromMenu: NULL: NULL,
			    NULL);
}

static bool  __FASTCALL__ ungotkey(int keycode)
{
  bool ret = false;
  if(KB_freq < sizeof(KB_Buff)/sizeof(int))
  {
    KB_Buff[KB_freq++] = keycode;
    ret = true;
  }
  return ret;
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

inline bool isSpace(int val) { return ((val+1)%3 ? 0 : 1); }
inline bool isFirstD(int pos) { return isSpace(pos-1); }
inline bool isSecondD(int pos) { return isSpace(pos+1); }

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
    if(!undo) w->direct_write(1,y,s,len);
    else
    {
      for(i = 0;i < len;i++)
      {
	w->set_color(s[i] == undo[i] ? browser_cset.edit.main : browser_cset.edit.change);
	w->direct_write(i+1,y,&s[i],1);
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

void __FASTCALL__ MemOutBox(const std::string& user_msg)
{
  beye_context().ErrMessageBox(user_msg," Not enough memory! ");
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
    pw->direct_write(2,3,outb,sizeof(outb));
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

static TWindow *  __FASTCALL__ _CrtMnuWindowDD(const std::string& title,tAbsCoord x1, tAbsCoord y1, tAbsCoord x2,tAbsCoord y2)
{
 TWindow *win;
 TWindow::twc_flag flags = TWindow::Flag_Has_Frame|TWindow::Flag_NLS;
 win = new(zeromem) TWindow(x1,y1,x2-x1+2,y2-y1+2,flags);
 if(!x1 && !y1) win->into_center();
 win->set_color(menu_cset.main);
 win->clear();
 win->set_frame(TWindow::DOUBLE_FRAME,menu_cset.border);
 if(!title.empty()) win->set_title(title,TWindow::TMode_Center,menu_cset.title);
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

static void  __FASTCALL__ PaintLine(TWindow& w,unsigned i,const std::string& name,
					unsigned width,unsigned mord_width,
					bool isOrdinal,
					bool useAcc,bool is_hl)
{
  size_t namelen;
  char buffer[__TVIO_MAXSCREENWIDTH + 1];
  memset(buffer,TWC_DEF_FILLER,sizeof(buffer));
  buffer[__TVIO_MAXSCREENWIDTH] = 0; /* [dBorca] play it safe for strchr below */
  namelen = name.length();
  if(isOrdinal)
  {
    size_t endptr;
    endptr = name.rfind(LB_ORD_DELIMITER);
    if(endptr!=std::string::npos)
    {
      unsigned len, rlen;
      // write name
      len = endptr;
      rlen = len;
      if(len > width - mord_width-1)
	  rlen = width - mord_width-3;
      memcpy(buffer,name.c_str(),rlen);
      if(len > rlen) memcpy(buffer+rlen,"..", 2);           // using 2 dots now -XF
      // write ordinal. it's left aligned now -XF
      buffer[width - mord_width - 1] = '@';
      len = rlen = namelen - (len+1);
      if(rlen > mord_width) rlen = mord_width - 2;
      memcpy(&buffer[width - mord_width], &name.c_str()[endptr+1], rlen);
      if(len > rlen) memcpy(buffer+width-mord_width+rlen,"..", 2);
    }
  }
  else if(!name.empty()) memcpy(buffer,name.c_str(),std::min(namelen,size_t(width)));
  if(useAcc)
  {
    const char *st,*ends,*ptr;
    char ch;
    w.goto_xy(3,i+1);
    st = buffer;
    ends = buffer+width;
    while(1)
    {
      ptr = strchr(st,'~');
      if(ptr)
      {
	unsigned outlen;
	outlen = ptr-st;
	w.direct_write(w.where_x(),w.where_y(),st,outlen);
	w.goto_xy(w.where_x()+outlen,w.where_y());
	st = ptr;
	ch = *(++st);
	if(ch != '~')
	{
	  ColorAttr ca;
	  ca = w.get_color();
	  w.set_color(is_hl ? menu_cset.hotkey.focused : menu_cset.hotkey.active);
	  w.putch(ch);
	  w.set_color(ca);
	}
	st++;
      }
      else
      {
	w.direct_write(w.where_x(),w.where_y(),st,(unsigned)(ends-st));
	break;
      }
    }
  }
  else  w.direct_write(3,i+1,buffer,width);
}

static void  __FASTCALL__ Paint(TWindow& win,const std::vector<std::string>& names,
					unsigned start,
					unsigned height,unsigned width,
					unsigned mord_width,
					bool isOrdinal,bool useAcc,
					unsigned cursor)
{
 unsigned i, pos = 0;
 size_t nlist=names.size();
 win.freeze();
 width -= 3;
 if (height>2 && height<nlist)
     pos = 1 + (start+cursor)*(height-2)/nlist;
 for(i = 0;i < height;i++)
 {
   win.set_color(menu_cset.main);
   win.goto_xy(1,i + 1);
   if (i == 0)
       win.putch(start ? TWC_UP_ARROW : TWC_DEF_FILLER);
   else if(i == height-1)
       win.putch(start + height < nlist ? TWC_DN_ARROW : TWC_DEF_FILLER);
   else if (i == pos)
       win.putch(TWC_THUMB);
   else win.putch(TWC_DEF_FILLER);
   win.goto_xy(2,i + 1);
   win.putch(TWC_SV);
   win.set_color(menu_cset.item.active);
   PaintLine(win,i,names[i + start],width,mord_width,isOrdinal,useAcc,cursor == i);
 }
 win.refresh();
}

static char byNam;

bool __FASTCALL__ _lb_searchtext(const char *str,const char *tmpl,unsigned searchlen,const int *cache, unsigned flg)
{
  return strFind(str, strlen(str), tmpl, searchlen, cache, flg) ? true : false;
}

static bool list_compare(const std::string& s1,const std::string& s2)
{
  bool ret;
  if(byNam)  ret = s1 < s2;
  else {
    size_t o1,o2;

    o1 = s1.rfind(LB_ORD_DELIMITER);
    o2 = s2.rfind(LB_ORD_DELIMITER);
    if(o1!=std::string::npos && o2!=std::string::npos) {
      unsigned long ord1,ord2;
      std::string buff1, buff2;
       buff1=s1.substr(o1+1,6);
       buff2=s2.substr(o2+1,6);
       ord1 = atol(buff1.c_str());
       ord2 = atol(buff2.c_str());
       ret = ord1<ord2;
    }
    else  ret = s1<s2;
  }
  return ret;
}

int __FASTCALL__ ListBox(std::vector<std::string>& names,const std::string& title,int assel,unsigned defsel)
{
    TWindow * wlist;
    char *acctable = 0;
    unsigned i,j,nlist,width,height,mwidth = title.length();
    unsigned mordstr_width, mord_width;
    int ret,start,ostart,cursor,ocursor,scursor;
    bool isOrdinal,sf;
    nlist=names.size();
    if(!nlist) return -1;
    isOrdinal = true;
    scursor = -1;
    i = 0;
    if((assel & LB_USEACC) == LB_USEACC) {
	acctable = new char [names.size()];
	if(!acctable) {
	    MemOutBox("Displaying list");
	     return -1;
	}
	memset(acctable,0,nlist*sizeof(char));

	for(i = 0;i < nlist;i++) {
	    unsigned len;
	    len = names[i].length();
	    for(j = 0;j < len;j++) {
		if(names[i][j] == '~' && names[i][j+1] != '~') {
		    acctable[i] = toupper(names[i][j+1]);
		    break;
		}
	    }
	}
    }
    if(!names[0].empty()) if(names[0].rfind(LB_ORD_DELIMITER)==std::string::npos) isOrdinal = false;
    mordstr_width = mord_width = 0;
    if(!isOrdinal) for(i = 0;i < nlist;i++) {
	j = !names[i].empty()?names[i].length():0;
	if(j > mwidth) mwidth = j;
    } else {
	const char *ord_delimiter;
	for(i = 0;i < nlist;i++) {
	    ord_delimiter = !names[i].empty()?strrchr(names[i].c_str(), LB_ORD_DELIMITER):NULL;
	    if(ord_delimiter) {
		j = ord_delimiter - names[i].c_str();
		if(j > mordstr_width) mordstr_width = j;
		j = &names[i][names[i].length()] - ord_delimiter;
		if(j > mord_width) mord_width = j;
	    }
	}
	// name now has higher priority than ordinal -XF
	if(mordstr_width > (unsigned)(beye_context().tconsole().vio_width()-10))
	    mordstr_width = (unsigned)(beye_context().tconsole().vio_width()-10);
	if(mord_width > (unsigned)(beye_context().tconsole().vio_width()-4)-mordstr_width-1)
	    mord_width = (unsigned)(beye_context().tconsole().vio_width()-4)-mordstr_width-1;
	    mwidth = mordstr_width+mord_width+1;
    }
    mwidth += 4;
    if(mwidth > (unsigned)(beye_context().tconsole().vio_width()-1)) mwidth = beye_context().tconsole().vio_width()-1;         // maximal width increased to beye_context().tconsole().vio_width()-1 -XF
    height = nlist < (unsigned)(beye_context().tconsole().vio_height() - 4) ? nlist : beye_context().tconsole().vio_height() - 4;
    wlist = _CrtMnuWindowDD(title,0,0,mwidth-1,height);
    if((assel & LB_SELECTIVE) == LB_SELECTIVE) wlist->set_footer(" [ENTER] - Go ",TWindow::TMode_Right,dialog_cset.selfooter);
restart:
    ostart = start = cursor = ocursor = 0;
    if(defsel != std::numeric_limits<size_t>::max() && defsel < nlist) {
	cursor = defsel;
	while((unsigned)cursor > height) { start += height; cursor -= height; }
	ostart = start;
	ocursor = cursor;
    }
    Paint(*wlist,names,(unsigned)start,height,mwidth,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,(unsigned)cursor);
    width = mwidth - 3;
    if((assel & LB_SELECTIVE) == LB_SELECTIVE) {
	wlist->set_color(menu_cset.item.focused);
	PaintLine(*wlist,(unsigned)cursor,names[cursor + start],width,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,true);
    }
    sf = false;
    for(;;) {
	unsigned ch;
	ch = GetEvent(isOrdinal ? drawOrdListPrompt : (assel & LB_SORTABLE) ? drawListPrompt : drawSearchListPrompt,NULL,wlist);
	if(ch == KE_ESCAPE || ch == KE_F(10)) { ret = -1; break; }
	if(ch == KE_ENTER)                    { ret = start + cursor; break; }
	if(ch!=KE_F(7) && ch!=KE_SHIFT_F(7))  scursor = -1;
	switch(ch) {
	    case KE_F(2):
	    case KE_F(3):
		if(isOrdinal || (assel & LB_SORTABLE)) {
		    byNam = ch == KE_F(2);
		    if(!isOrdinal && !byNam) break;
		    std::sort(names.begin(),names.end(),list_compare);
		    goto restart;
	        }
	        break;
	    case KE_F(4): { /** save content to disk */
		char ofname[256];
		ofname[0] = 0;
		if(GetStringDlg(ofname," Save info to file : "," [ENTER] - Proceed ",NAME_MSG)) {
		    std::ofstream out;
		    out.open(ofname,std::ios_base::out);
		    if(out.is_open()) {
			strncpy(ofname,title.c_str(),sizeof(ofname));
			ofname[sizeof(ofname)-1] = '\0';
			if(GetStringDlg(ofname," User comments : "," [ENTER] - Proceed "," Description : ")) out<<ofname<<std::endl<<std::endl;
			for(i = 0;i < nlist;i++) {
			    char *p;
			    char stmp[4096];
			    stmp[0]='\0';
			    if(!names[i].empty()) strcpy(stmp,names[i].c_str());
			    p = !names[i].empty()?strchr(stmp,LB_ORD_DELIMITER):NULL;
			    if(p) {
				*p = 0;
				out<<stmp;
				for(j = p - stmp;j < 50;j++) out<<" ";
				out<<" @"<<(p+1);
				if(p) *p = LB_ORD_DELIMITER;
			    }
			    else out<<names[i];
			    out<<std::endl;
			}
			out.close();
		    }
		    else beye_context().errnoMessageBox(WRITE_FAIL,"",errno);
		}
	    }
	    break;
	    case KE_F(7): /** perform binary search in list */
	    case KE_SHIFT_F(7): {
		static char searchtxt[21] = "";
		static unsigned char searchlen = 0;
		static unsigned sflg = SF_NONE;

		if (!(ch==KE_SHIFT_F(7) && searchlen) &&
		    !SearchDialog(SD_SIMPLE,searchtxt,&searchlen,&sflg)) break;

		int direct, cache[UCHAR_MAX+1];
		bool found;
		int ii,endsearch,startsearch;
		searchtxt[searchlen] = 0;
		endsearch = sflg & SF_REVERSE ? -1 : (int)nlist;
		direct = sflg & SF_REVERSE ? -1 : 1;
		startsearch = (assel & LB_SELECTIVE) == LB_SELECTIVE ?
				cursor + start :
				scursor != -1 ?
				scursor :
				start;
		if(startsearch > (int)(nlist-1)) startsearch = nlist-1;
		if(startsearch < 0) startsearch = 0;
		if((assel & LB_SELECTIVE) == LB_SELECTIVE || scursor != -1) sflg & SF_REVERSE ? startsearch-- : startsearch++;
		found = false;
		fillBoyerMooreCache(cache, searchtxt, searchlen, sflg & SF_CASESENS);
		for(ii = startsearch;ii != endsearch;ii+=direct) {
		    if(!names[ii].empty()) {
			if(_lb_searchtext(names[ii].c_str(),searchtxt,searchlen,cache,sflg)) {
			    start = scursor = ii;
			    if((unsigned)start > nlist - height) start = nlist - height;
			    ostart = start - 1;
			    if((assel & LB_SELECTIVE) == LB_SELECTIVE) cursor = scursor - start;
			    found = true;
			    break;
			}
		    }
		}
		if(!found) scursor = -1;
		if(scursor == -1) beye_context().ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
	    }
	    break;
	    case KE_DOWNARROW : if((assel & LB_SELECTIVE) == LB_SELECTIVE) cursor ++; else start ++; break;
	    case KE_UPARROW   : if((assel & LB_SELECTIVE) == LB_SELECTIVE) cursor --; else start --; break;
	    case KE_PGDN   : start += height; break;
	    case KE_PGUP   : start -= height; break;
	    case KE_CTL_PGDN  : start = nlist - height; cursor = height; break;
	    case KE_CTL_PGUP  : start = cursor = 0; break;
	    default :
		/** Try accelerate choose */
		if((assel & LB_USEACC) == LB_USEACC) {
		    if((unsigned char)(ch & 0x00FF) > 31) {
			ch = toupper(ch & 0x00FF);
			for(i = 0;i < nlist;i++)
			    if(ch == (unsigned)acctable[i]) { ret = i; goto Done; }
		    }
		}
	}
	if((assel & LB_SELECTIVE) == LB_SELECTIVE) {
	    if(cursor < 0) { cursor = 0; start--; }
	    if((unsigned)cursor > height - 1) { cursor = height - 1; start++; }
	}
	if(start < 0) start = 0;
	if((unsigned)start > nlist - height) start = nlist - height;
	if(start != ostart) {
	    ostart = start;
	    Paint(*wlist,names,(unsigned)start,height,mwidth,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,(unsigned)cursor);
	    sf = true;
	}
	if((cursor != ocursor || sf) && (assel & LB_SELECTIVE) == LB_SELECTIVE) {
	    wlist->set_color(menu_cset.item.active);
	    PaintLine(*wlist,(unsigned)ocursor,names[ocursor + start],width,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,false);
	    wlist->set_color(menu_cset.item.focused);
	    PaintLine(*wlist,(unsigned)cursor,names[cursor + start],width,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,true);
	    ocursor = cursor;
	    sf = false;
	}
	if(scursor != -1) {
	    wlist->set_color(menu_cset.highlight);
	    if(scursor >= start && (unsigned)scursor < start + height)
		PaintLine(*wlist,(unsigned)(scursor - start),names[scursor],width,mord_width,isOrdinal,(assel & LB_USEACC) == LB_USEACC,true);
	}
    }
Done:
    delete wlist;
    if(acctable) delete acctable;
    return ret;
}

int __FASTCALL__ ListBox(const char** names,unsigned nlist,const std::string& title,int assel,unsigned defsel)
{
    std::vector<std::string> v;
    for(size_t i=0;i<nlist;i++) v.push_back(names[i]);
    return ListBox(v,title,assel,defsel);
}
} // namespace	usr

