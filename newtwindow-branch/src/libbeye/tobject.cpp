#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/twin.c
 * @brief       This file contains implementation of Text Window manager.
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
 * @warning     Program is destroyed, from printf misapplication
 * @bug         Limitation of printf using
 * @todo        Accelerate windows interaction algorithm
**/
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "libbeye/osdep/system.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/tobject.h"

namespace	usr {
TConsole* TObject::tconsole = NULL;
System*  TObject::msystem = NULL;
TObject* TObject::head = NULL;
TObject* TObject::cursorwin = NULL;
unsigned long TObject::twin_flags = 0L;

TObject::e_cursor TObject::c_type = Cursor_Unknown;
void TObject::set_cursor_type(TObject::e_cursor type)
{
  if(type != c_type)
  {
    tconsole->vio_set_cursor_type(type);
    c_type = type;
  }
}

TObject::e_cursor TObject::get_cursor_type()
{
  if(c_type == Cursor_Unknown) c_type = e_cursor(tconsole->vio_get_cursor_type());
  return c_type;
}

TConsole& __FASTCALL__ twInit(System& sys,const std::string& user_cp, unsigned long vio_flags, unsigned long twin_flgs )
{
  const char *nls_cp;
  TObject::twin_flags = twin_flgs;
  TObject::msystem=&sys;
  nls_cp=!user_cp.empty()?user_cp.c_str():"IBM866";
  TObject::tconsole = new(zeromem) TConsole(nls_cp,vio_flags);
  if(TObject::tconsole->vio_width() > __TVIO_MAXSCREENWIDTH) {
    std::ostringstream os;
    twDestroy();
    os<<"Size of video buffer is too large: "<<TObject::tconsole->vio_width()<<" (max = "<<__TVIO_MAXSCREENWIDTH<<")";
    throw std::runtime_error(std::string("Internal twin library error: ")+os.str());
  }
  TObject::set_cursor_type(TObject::Cursor_Off);
  return *TObject::tconsole;
}

void __FASTCALL__ twDestroy()
{
  TObject::set_cursor_type(TObject::Cursor_Normal);
  twcDestroyClassSet();
  delete TObject::tconsole;
}

enum {
    IFLG_VISIBLE      =0x00000001UL,
    IFLG_ENABLED      =0x00000002UL,
    IFLG_CURSORBEENOFF=0x80000000UL
};

void TObject::__athead() { if(head) next = head; head = this; }

TObject* TObject::__find_over(tAbsCoord x,tAbsCoord y) const {
    TObject *iter,*ret;
    tAbsCoord xx,yy;
    iter = head;
    ret = NULL;
    xx = X1+x;
    yy = Y1+y;
    while(iter && iter != this) {
	if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	    if((yy >= iter->Y1) && (yy < iter->Y2) &&
		(xx >= iter->X1) && (xx < iter->X2))
		    ret = iter;
	}
	iter = iter->next;
    }
    return ret;
}

bool TObject::is_overlapped() const
{
    TObject *iter;
    bool ret = false;
    iter = head;
    while( iter && iter != this ) {
	if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	    if(!((iter->Y2 <= Y1) || (iter->Y1 >= Y2) ||
		(iter->X2 <= X1) || (iter->X1 >= X2))) {
		    ret = true;
		    break;
	    }
	}
	iter = iter->next;
    }
    return ret;
}

/*
   In most cases all output is being performed into non overlapped windows.
   We should turn on special optimization for such windows, though overlapped
   windows will seem relatively slower. There are additional possibilities to
   accelerate all output by computing non overlapped lines of window or parts
   of lines, but I don't want to do such complex task.
*/
TObject* TObject::__at_point(TObject* iter,tAbsCoord x,tAbsCoord y) {
  iter = head;
  while( iter )
  {
    if((iter->iflags & IFLG_VISIBLE) == IFLG_VISIBLE)
    {
      if((y >= iter->Y1) && (y < iter->Y2) &&
	 (x >= iter->X1) && (x < iter->X2))
		       break;
    }
    iter = iter->next;
  }
  return iter;
}

void TObject::create(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) { _width ++; _height ++; }

    unsigned size = _width*_height;

    wsize = size;
    wwidth = _width;
    wheight = _height;

    X1 = x1;
    Y1 = y1;
    X2 = x1+_width;
    Y2 = y1+_height;
    __athead();

    flags = _flags;
    iflags = IFLG_ENABLED;

    cur_x = cur_y = 0;
    set_focus();
}

TObject::TObject(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
{
    create(x1,y1,_width,_height,_flags);
}

TObject::TObject(tAbsCoord x1_, tAbsCoord y1_,
		 tAbsCoord _width, tAbsCoord _height,
		 twc_flag _flags, const std::string& classname)
{
    create(x1_, y1_, _width, _height, _flags);
    const TwClass* cls;
    cls = twcFindClass(classname);
    if(cls) {
	method = reinterpret_cast<any_t*>(cls->method);
	class_flags = cls->flags;
    }
    send_message(WM_CREATE,0L,NULL);
}

TObject::~TObject()
{
    send_message(WM_DESTROY,0L,NULL);
    hide();

    __unlistwin();
    if(cursorwin == this) cursorwin = __findcursorablewin();
    if(cursorwin) cursorwin->paint_cursor();
}

void TObject::__unlistwin()
{
    if(head == this) head = next;
    else {
	TObject *iter;
	iter = head;
	while(iter) {
	    if(iter->next) if(iter->next == this) iter->next = next;
	    iter = iter->next;
	}
    }
}

TObject* TObject::__prevwin()
{
    TObject *ret = NULL;
    if(head != this) {
	TObject* iter;
	iter = head;
	while(iter) {
	     if(iter->next) if(iter->next == this) { ret = iter; break; }
	    iter = iter->next;
	}
    }
    return ret;
}

TObject* TObject::__findcursorablewin()
{
    TObject *iter,*ret;
    iter = head;
    ret = NULL;
    while(iter) {
	if((iter->flags & Flag_Has_Cursor) == Flag_Has_Cursor) { ret = iter; break; }
	iter = iter->next;
    }
    return ret;
}

TObject* TObject::at_pos(tAbsCoord x,tAbsCoord y)
{
    TObject *ret=NULL;
    tAbsCoord xx,yy;
    xx = x-1;
    yy = y-1;
    ret = __at_point(ret,xx,yy);
    return ret;
}

void TObject::cvt_win_coords(tRelCoord x, tRelCoord y,tAbsCoord& xa,tAbsCoord& ya) const
{
    if(flags & Flag_Has_Frame) { x++; y++; }
    xa = X1+x;
    ya = Y1+y;
}

bool TObject::cvt_screen_coords(tAbsCoord x, tAbsCoord y,tRelCoord& xr,tRelCoord& yr) const
{
    xr = x - X1;
    yr = y - Y1;
    if(flags & Flag_Has_Frame) { xr--; yr--; }
    return is_valid_xy(xr,yr) ? true : false;
}

bool TObject::is_piece_visible(tRelCoord x, tRelCoord y) const
{
    TObject *over;
    if(flags & Flag_Has_Frame) { x++; y++; }
    over=__find_over(X1+x,Y1+y);
    return over ? true : false;
}

void TObject::igoto_xy(tRelCoord x,tRelCoord y)
{
    if(x && y) {
	cur_x = x-1;
	cur_y = y-1;
	if(cur_x >= wwidth) cur_x = wwidth-1;
	if(cur_y >= wheight) cur_y = wheight-1;
    }
}

void TObject::set_xy(tRelCoord x,tRelCoord y)
{
    tRelCoord _width,_height;
    if(x && y) {
	x--; y--;
	cur_x = x;
	cur_y = y;
	_width = wwidth;
	_height = wheight;
	if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	    _width--;
	    _height--;
	    cur_x++;
	    cur_y++;
	}
	if(cur_x > _width) cur_x = _width-1;
	if(cur_y > _height) cur_y = _height-1;
    }
}

void TObject::paint_cursor() const
{
    TObject * top=NULL;
    static tAbsCoord c_x = UCHAR_MAX, c_y = UCHAR_MAX;
    e_cursor _c_type = Cursor_Unknown;
    unsigned x,y;
    e_cursor type;
    if(cursorwin && (cursorwin->iflags & IFLG_ENABLED) == IFLG_ENABLED) {
	top=cursorwin->__find_over(cursorwin->cur_x,cursorwin->cur_y-1);
	if(!top && (cursorwin->iflags & IFLG_VISIBLE) == IFLG_VISIBLE && cursorwin == this) {
	    type = get_cursor_type();
	    if(type == Cursor_Off) {
		set_cursor_type(_c_type == Cursor_Unknown ? Cursor_Normal : _c_type);
		_c_type = _c_type == Cursor_Unknown ? Cursor_Normal : _c_type;
	    }
	    x = X1 + cur_x;
	    y = Y1 + cur_y;
	    if(!(x == c_x && y == c_y)) {
		tconsole->vio_set_cursor_pos(x,y);
		c_x = x;
		c_y = y;
	    }
	}
	else goto hide_cursor;
    } else {
	c_x = c_y = UCHAR_MAX;
hide_cursor:
	type = get_cursor_type();
	if(type != Cursor_Off) {
	    _c_type = type;
	    set_cursor_type(Cursor_Off);
	}
    }
}

void TObject::goto_xy(tRelCoord x,tRelCoord y)
{
    set_xy(x,y);
    paint_cursor();
}

void TObject::show()
{
    send_message(WM_SHOW,0L,NULL);
    if(!(iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	iflags |= IFLG_VISIBLE;
	__unlistwin();
	__athead();
	if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	    cursorwin = this;
	}
	paint_cursor();
    }
}

void TObject::show_on_top()
{
    send_message(WM_TOPSHOW,0L,NULL);
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) hide();
    iflags |= IFLG_VISIBLE;
    __unlistwin();
    __athead();
    if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	cursorwin = this;
    }
    paint_cursor();
}

void TObject::show_beneath(TObject& prev)
{
    send_message(WM_SHOWBENEATH,0L,&prev);
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) hide();
    iflags |= IFLG_VISIBLE;
    __unlistwin();
    __atwin(&prev);
}

void TObject::hide()
{
    send_message(WM_HIDE,0L,NULL);
    if(cursorwin == this) set_cursor_type(Cursor_Off);
    iflags &= ~IFLG_VISIBLE;
}

void TObject::get_pos(tAbsCoord& x1,tAbsCoord& y1,tAbsCoord& x2,tAbsCoord& y2)
{
    x1 = X1+1;
    y1 = Y1+1;
    x2 = X2;
    y2 = Y2;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	x2--;
	y2--;
    }
}

unsigned TObject::width() const { return wwidth; }
unsigned TObject::height() const { return wheight; }
unsigned TObject::client_width() const { return flags & Flag_Has_Frame ? wwidth-2 : wwidth; }
unsigned TObject::client_height() const {  return flags & Flag_Has_Frame ? wheight-2 : wheight; }

void TObject::move(tAbsCoord dx,tAbsCoord dy)
{
    TObject* prev;
    tRelCoord x,y;
    int vis;
    vis = (iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
    x = where_x();
    y = where_y();
    prev = __prevwin();
    if(vis) hide();
    X1 += dx;
    Y1 += dy;
    X2 += dx;
    Y2 += dy;
    if(vis) show_beneath(*prev);
    goto_xy(x,y);
}

void TObject::resize(tAbsCoord _width,tAbsCoord _height)
{
    TObject *prev;
    size_t size;
    tRelCoord x,y;
    bool vis;
    x = where_x();
    y = where_y();
    vis = (iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
    prev = __prevwin();
    if(vis) hide();
    size = _width*_height;

    wsize = size;
    wwidth = _width;
    wheight = _height;

    X2 = X1 + _width;
    Y2 = Y1 + _height;
    if(vis) show_beneath(*prev);
    goto_xy(x,y);
}

void TObject::into_center(tAbsCoord w,tAbsCoord h)
{
    tAbsCoord ww,wh,pww,pwh;
    int vis = (iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
    if(vis) hide();
    ww = wwidth;
    wh = wheight;
    pww = w;
    pwh = h;
    X1 = ( pww - ww )>>1;
    X2 = ( pww + ww )>>1;
    Y1 = ( pwh - wh )>>1;
    Y2 = ( pwh + wh )>>1;
    if(vis) show();
}

void TObject::into_center(const TObject& parent) { into_center(parent.wwidth,parent.wheight); }
void TObject::into_center() { into_center(tconsole->vio_width(),tconsole->vio_height()); }

TObject* TObject::set_focus()
{
    TObject *ret;
    ret = cursorwin;
    if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	cursorwin = this;
    }
    paint_cursor();
    return ret;
}

TObject* TObject::get_focus() { return cursorwin; }

tRelCoord TObject::where_x() const { return (flags & Flag_Has_Frame) == Flag_Has_Frame ? cur_x : cur_x+1; }
tRelCoord TObject::where_y() const { return (flags & Flag_Has_Frame) == Flag_Has_Frame ? cur_y : cur_y+1; }

void TObject::freeze() { iflags &= ~IFLG_ENABLED; }

void TObject::refresh(tRelCoord y)
{
    iflags |= IFLG_ENABLED;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) y++;
    paint_cursor();
}

void TObject::refresh_piece(tRelCoord stx,tRelCoord endx,tRelCoord y)
{
    iflags |= IFLG_ENABLED;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) { stx++; endx++; y++; }
    paint_cursor();
}

void TObject::refresh()
{
    iflags |= IFLG_ENABLED;
    paint_cursor();
}

void TObject::refresh_full()
{
    iflags |= IFLG_ENABLED;
    paint_cursor();
}

any_t* TObject::get_user_data() const { return usrData; }

any_t* TObject::set_user_data(any_t*data) {
    any_t*ret;
    ret = usrData;
    usrData = data;
    return ret;
}

long TObject::send_message(unsigned event,unsigned long event_param,const any_t*event_data)
{
    if(method) return ((twClassFunc)(method))(this,event,event_param,event_data);
    return 0L;
}
} // namespace	usr
