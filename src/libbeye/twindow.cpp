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
#include "libbeye/twindow.h"

namespace	usr {
TConsole* TWindow::tconsole = NULL;
System*  TWindow::msystem = NULL;
TWindow* TWindow::head = NULL;
TWindow* TWindow::cursorwin = NULL;
static unsigned long twin_flags = 0L;

TWindow::e_cursor TWindow::c_type = Cursor_Unknown;
void TWindow::set_cursor_type(TWindow::e_cursor type)
{
  if(type != c_type)
  {
    tconsole->vio_set_cursor_type(type);
    c_type = type;
  }
}

TWindow::e_cursor TWindow::get_cursor_type()
{
  if(c_type == Cursor_Unknown) c_type = e_cursor(tconsole->vio_get_cursor_type());
  return c_type;
}

TConsole& __FASTCALL__ twInit(System& sys,const std::string& user_cp, unsigned long vio_flags, unsigned long twin_flgs )
{
  const char *nls_cp;
  twin_flags = twin_flgs;
  TWindow::msystem=&sys;
  nls_cp=!user_cp.empty()?user_cp.c_str():"IBM866";
  TWindow::tconsole = new(zeromem) TConsole(nls_cp,vio_flags);
  if(TWindow::tconsole->vio_width() > __TVIO_MAXSCREENWIDTH)
  {
    char outs[256];
    twDestroy();
    sprintf(outs,"Size of video buffer is too large: %u (max = %u)",TWindow::tconsole->vio_width(),__TVIO_MAXSCREENWIDTH);
    throw std::runtime_error(std::string("Internal twin library error: ")+outs);
  }
  TWindow::set_cursor_type(TWindow::Cursor_Off);
  return *TWindow::tconsole;
}

void __FASTCALL__ twDestroy()
{
  TWindow::set_cursor_type(TWindow::Cursor_Normal);
  twcDestroyClassSet();
  delete TWindow::tconsole;
}
/*
  Hypothesis:
    When 16-bits color is coded then weight of component is:
    Blue  - 4 bits
    Green - 6 bits
    Red   - 6 bits
  cause of human eye more sensitivity for blue color,
  therefore weight of colors must will:

  Blue      - 6 balls
  Green     - 4 balls
  Red       - 4 balls

  Brightness is 128 192 255 i.e.:
		2   3   4
  Cause of programs can used multiple color conversation better use same
  values for each color component
*/
enum {
    BR_BLUE  =1,
    BR_GREEN =1,
    BR_RED   =1
};

const unsigned char TWindow::brightness[16] =
{
	/* RGB: Name */
    BR_RED*0+BR_GREEN*0+BR_BLUE*0, /**< 000: Black */
    BR_RED*0+BR_GREEN*0+BR_BLUE*2, /**< 002: Blue */
    BR_RED*0+BR_GREEN*2+BR_BLUE*0, /**< 020: Green */
    BR_RED*0+BR_GREEN*2+BR_BLUE*2, /**< 022: Cyan */
    BR_RED*2+BR_GREEN*0+BR_BLUE*0, /**< 200: Red */
    BR_RED*2+BR_GREEN*0+BR_BLUE*2, /**< 202: Magenta */
    BR_RED*2+BR_GREEN*2+BR_BLUE*0, /**< 220: Brown */
    BR_RED*3+BR_GREEN*3+BR_BLUE*3, /**< 333: LightGray */
    BR_RED*2+BR_GREEN*2+BR_BLUE*2, /**< 222: Gray */
    BR_RED*0+BR_GREEN*0+BR_BLUE*4, /**< 004: LightBlue */
    BR_RED*0+BR_GREEN*4+BR_BLUE*0, /**< 040: LightGreen */
    BR_RED*0+BR_GREEN*4+BR_BLUE*4, /**< 044: LightCyan */
    BR_RED*4+BR_GREEN*0+BR_BLUE*0, /**< 400: LightRed */
    BR_RED*4+BR_GREEN*0+BR_BLUE*4, /**< 404: LightMagenta */
    BR_RED*4+BR_GREEN*4+BR_BLUE*0, /**< 440: Yellow */
    BR_RED*4+BR_GREEN*4+BR_BLUE*4  /**< 444: White */
};

uint8_t TWindow::color_map[16] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

void TWindow::adjustColor(Color& fore,Color& back)
{
    if(tconsole->vio_num_colors() < 16 || (twin_flags & TWIF_FORCEMONO)) {
	unsigned char br_fore, br_back;
	br_fore = brightness[fore & 0x0F];
	br_back = brightness[back & 0x0F];
	if(br_fore >= br_back) {
	    fore = LightGray;
	    back = Black;
	} else {
	    fore = Black;
	    back = LightGray;
	}
    }
    fore = Color(color_map[fore & 0x0F]);
    back = Color(color_map[back & 0x0F]);
}

DefColor TWindow::__set_color(Color fore,Color back)
{
    DefColor rc;
    rc.user = LOGFB_TO_PHYS(fore,back);
    adjustColor(fore,back);
    rc.system = LOGFB_TO_PHYS(fore,back);
    return rc;
}

void __FASTCALL__ TWindow::remap_color(Color color,unsigned char value)
{
    color_map[color & 0x0F] = value & 0x0F;
}

unsigned char TWindow::get_mapped_color(Color color)
{
    return color_map[color & 0x0F];
}

const unsigned char TWindow::SINGLE_FRAME[8] = { TWC_Sd_Sr, TWC_SH, TWC_Sl_Sd, TWC_SV, TWC_SV, TWC_Su_Sr, TWC_SH, TWC_Sl_Su };
const unsigned char TWindow::DOUBLE_FRAME[8] = { TWC_Dd_Dr, TWC_DH, TWC_Dl_Dd, TWC_DV, TWC_DV, TWC_Du_Dr, TWC_DH, TWC_Dl_Du };
const unsigned char TWindow::MEDIUM_FRAME[8] = { TWC_FL_BLK, TWC_UP_HBLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_LF_HBLK, TWC_FL_BLK };
const unsigned char TWindow::THICK_FRAME[8] = { TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK };
const unsigned char TWindow::UP3D_FRAME[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
const unsigned char TWindow::DN3D_FRAME[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#ifndef NDEBUG
#ifdef __TSC__
#pragma save , call(inline=>on) /** GPF in protmode */
static void winInternalError() = { 0xFF, 0xFF };
#elif defined _MSC_VER
static inline void winInternalError() {};
#else
static inline void winInternalError() { (void)0xFFFFFFFF; };
#endif
bool TWindow::test_win() const
{
    bool ret;
    ret = *((any_t**)(body.chars + wsize)) == body.chars &&
	*((any_t**)(body.oem_pg + wsize)) == body.oem_pg &&
	*((any_t**)(body.attrs + wsize)) == body.attrs &&
	*((any_t**)(saved.chars + wsize)) == saved.chars &&
	*((any_t**)(saved.oem_pg + wsize)) == saved.oem_pg &&
	*((any_t**)(saved.attrs + wsize)) == saved.attrs ? true : false;
    return ret;
}
void TWindow::check_win() const { if(!test_win()) winInternalError(); }
#else
void TWindow::check_win() const {}
#endif

enum {
    IFLG_VISIBLE      =0x00000001UL,
    IFLG_ENABLED      =0x00000002UL,
    IFLG_CURSORBEENOFF=0x80000000UL
};


void TWindow::__athead() { if(head) next = head; head = this; }

TWindow* TWindow::__find_over(tAbsCoord x,tAbsCoord y) const {
    TWindow *iter,*ret;
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

bool TWindow::is_overlapped() const
{
    TWindow *iter;
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
TWindow* TWindow::__at_point(TWindow* iter,tAbsCoord x,tAbsCoord y) {
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

void TWindow::create(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, unsigned _flags)
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) { _width ++; _height ++; }
    makewin(x1-1,y1-1,_width,_height);
    flags = _flags;
    iflags = IFLG_ENABLED;
    ::memcpy(Frame,SINGLE_FRAME,8);
    ::memset(body.chars,TWC_DEF_FILLER,wsize);
    ::memset(body.oem_pg,0,wsize);
    ::memset(body.attrs,text.system,wsize);
    check_win();
    text=__set_color(LightGray,Black);
    frame=__set_color(LightGray,Black);
    title=__set_color(LightGray,Black);
    footer=__set_color(LightGray,Black);
    cur_x = cur_y = 0;
    set_focus();
    paint_internal();
}

TWindow::TWindow(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
{
    create(x1,y1,_width,_height,_flags);
}

TWindow::TWindow(tAbsCoord x1_, tAbsCoord y1_,
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

TWindow::~TWindow()
{
    send_message(WM_DESTROY,0L,NULL);
    hide();
    if(Title) delete Title;
    if(Footer) delete Footer;
    delete body.chars;
    delete body.oem_pg;
    delete body.attrs;
    delete saved.chars;
    delete saved.oem_pg;
    delete saved.attrs;
    __unlistwin();
    if(cursorwin == this) cursorwin = __findcursorablewin();
    if(cursorwin) cursorwin->paint_cursor();
}

ColorAttr TWindow::set_color(Color fore,Color back)
{
    ColorAttr ret;
    ret = text.user;
    text=__set_color(fore,back);
    return ret;
}

ColorAttr TWindow::set_color(ColorAttr ca) {  return set_color(FORE_COLOR(ca),BACK_COLOR(ca)); }

ColorAttr TWindow::get_color() const { return text.user; }

void TWindow::get_color(Color& fore,Color& back) const
{
    ColorAttr ca = get_color();
    PHYS_TO_LOGFB(ca,fore,back);
}

Color TWindow::text_color(Color col)
{
    Color back,ret;
    ColorAttr attr;
    attr = get_color();
    back = BACK_COLOR(attr);
    ret = FORE_COLOR(attr);
    set_color(col,back);
    return ret;
}

Color TWindow::text_bkgnd(Color col)
{
    Color fore,ret;
    ColorAttr attr;
    attr = get_color();
    fore = FORE_COLOR(attr);
    ret = BACK_COLOR(attr);
    set_color(fore, col);
    return ret;
}

void TWindow::set_frame(const unsigned char *_frame,Color fore,Color back)
{
    flags |= Flag_Has_Frame;
    ::memcpy(Frame,_frame,8);
    frame=__set_color(fore,back);
    paint_internal();
}

void TWindow::set_frame(const unsigned char *_frame,ColorAttr attr)
{
    set_frame(_frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

void TWindow::get_frame(unsigned char *_frame,ColorAttr& attr) const
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	::memcpy(_frame,Frame,8);
	attr = frame.user;
    }
}

void TWindow::get_frame(unsigned char *_frame,Color& fore,Color& back) const
{
    ColorAttr attr = 0;
    get_frame(_frame,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
}

void TWindow::set_title(const std::string& _title,title_mode mode,Color fore,Color back)
{
    unsigned slen;
    slen = _title.length();
    if(Title) delete Title;
    Title = new char [slen+1];
    ::strcpy(Title,_title.c_str());
    if((flags & Flag_NLS) == Flag_NLS)
	 msystem->nls_oem2osdep((unsigned char *)Title,slen);
    TitleMode = mode;
    title=__set_color(fore,back);
    paint_internal();
}

void TWindow::set_title(const std::string& _title,title_mode mode,ColorAttr attr)
{
    set_title(_title,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

TWindow::title_mode TWindow::get_title(char *_title,unsigned cb_title,ColorAttr& attr) const
{
    title_mode ret = TMode_Left;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame && Title) {
	::strncpy(_title,Title,cb_title);
	_title[cb_title-1] = '\0';
	attr = title.user;
	ret = TitleMode;
    }
    return ret;
}

TWindow::title_mode TWindow::get_title(char *_title,unsigned cb_title,Color& fore,Color& back) const
{
    ColorAttr attr;
    title_mode ret;
    ret = get_title(_title,cb_title,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
    return ret;
}

void TWindow::set_footer(const std::string& _footer,title_mode mode,Color fore,Color back)
{
    unsigned slen;
    slen = _footer.length();
    if(Footer) delete Footer;
    Footer = new char [slen+1];
    ::strcpy(Footer,_footer.c_str());
    if((flags & Flag_NLS) == Flag_NLS)
	msystem->nls_oem2osdep((unsigned char *)Footer,slen);
    FooterMode = mode;
    footer=__set_color(fore,back);
    paint_internal();
}

void TWindow::set_footer(const std::string& _footer,title_mode mode,ColorAttr attr)
{
    set_footer(_footer,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

TWindow::title_mode TWindow::get_footer(char *_footer,unsigned cb_footer,ColorAttr& attr) const
{
    title_mode ret = title_mode(0);
    if((flags & Flag_Has_Frame) == Flag_Has_Frame && Footer) {
	::strncpy(_footer,Footer,cb_footer);
	_footer[cb_footer-1] = '\0';
	attr = footer.user;
	ret = FooterMode;
    }
    return ret;
}


TWindow::title_mode TWindow::get_footer(char *_footer,unsigned cb_footer,Color& fore,Color& back) const
{
    ColorAttr attr;
    title_mode ret;
    ret = get_footer(_footer,cb_footer,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
    return ret;
}

void TWindow::__unlistwin()
{
    if(head == this) head = next;
    else {
	TWindow *iter;
	iter = head;
	while(iter) {
	    if(iter->next) if(iter->next == this) iter->next = next;
	    iter = iter->next;
	}
    }
}

TWindow* TWindow::__prevwin()
{
    TWindow *ret = NULL;
    if(head != this) {
	TWindow* iter;
	iter = head;
	while(iter) {
	     if(iter->next) if(iter->next == this) { ret = iter; break; }
	    iter = iter->next;
	}
    }
    return ret;
}

TWindow* TWindow::__findcursorablewin()
{
    TWindow *iter,*ret;
    iter = head;
    ret = NULL;
    while(iter) {
	if((iter->flags & Flag_Has_Cursor) == Flag_Has_Cursor) { ret = iter; break; }
	iter = iter->next;
    }
    return ret;
}

TWindow* TWindow::at_pos(tAbsCoord x,tAbsCoord y)
{
    TWindow *ret=NULL;
    tAbsCoord xx,yy;
    xx = x-1;
    yy = y-1;
    ret = __at_point(ret,xx,yy);
    return ret;
}

void TWindow::cvt_win_coords(tRelCoord x, tRelCoord y,tAbsCoord& xa,tAbsCoord& ya) const
{
    if(flags & Flag_Has_Frame) { x++; y++; }
    xa = X1+x;
    ya = Y1+y;
}

bool TWindow::cvt_screen_coords(tAbsCoord x, tAbsCoord y,tRelCoord& xr,tRelCoord& yr) const
{
    xr = x - X1;
    yr = y - Y1;
    if(flags & Flag_Has_Frame) { xr--; yr--; }
    return is_valid_xy(xr,yr) ? true : false;
}

bool TWindow::is_piece_visible(tRelCoord x, tRelCoord y) const
{
    TWindow *over;
    if(flags & Flag_Has_Frame) { x++; y++; }
    over=__find_over(X1+x,Y1+y);
    return over ? true : false;
}

void TWindow::igoto_xy(tRelCoord x,tRelCoord y)
{
    if(x && y) {
	cur_x = x-1;
	cur_y = y-1;
	if(cur_x >= wwidth) cur_x = wwidth-1;
	if(cur_y >= wheight) cur_y = wheight-1;
    }
}

void TWindow::set_xy(tRelCoord x,tRelCoord y)
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

void TWindow::paint_cursor() const
{
    TWindow * top=NULL;
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

void TWindow::goto_xy(tRelCoord x,tRelCoord y)
{
    set_xy(x,y);
    paint_cursor();
}

void TWindow::makewin(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height)
{
    unsigned size;

    size = _width*_height;
    wsize = size;
    wwidth = _width;
    wheight = _height;
    body.chars = new t_vchar[size];
    body.oem_pg = new t_vchar[size];
    body.attrs = new ColorAttr[size];
    saved.chars = new t_vchar[size];
    saved.oem_pg = new t_vchar[size];
    saved.attrs = new ColorAttr[size];
    X1 = x1;
    Y1 = y1;
    X2 = x1+_width;
    Y2 = y1+_height;
    __athead();
}

/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatescreencharfrombuff: low level implementation updatescreen family
 *  updatescreenchar:  correctly copyed user record from win to screen
 *  restorescreenchar: correctly copyed window memory from win to screen
 */

void TWindow::updatescreencharfrombuff(tRelCoord x,
					tRelCoord y,
					const tvioBuff& buff,
					tvioBuff *accel) const
{
    tvioBuff it;
    unsigned idx,aidx;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	TWindow * top;
	idx = y*wwidth+x;
	aidx = x;
	top=__find_over(x,y);
	if(top) {
	    unsigned tidx;
	    tAbsCoord tx,ty;
	    tx = X1 - top->X1 + x;
	    ty = Y1 - top->Y1 + y;
	    tidx = tx + ty*top->wwidth;
	    top->saved.chars[tidx] = buff.chars[idx];
	    top->saved.oem_pg[tidx] = buff.oem_pg[idx];
	    top->saved.attrs[tidx] = buff.attrs[idx];
	    if(accel) {
		TWindow *vis=NULL;
		tAbsCoord xx,yy;
		xx = x+X1;
		yy = y+Y1;
		vis=__at_point(vis,xx,yy);
		tx = xx - vis->X1;
		ty = yy - vis->Y1;
		tidx = tx + ty*vis->wwidth;
		accel->chars[aidx] = vis->body.chars[tidx];
		accel->oem_pg[aidx] = vis->body.oem_pg[tidx];
		accel->attrs[aidx] = vis->body.attrs[tidx];
	    }
	    top->check_win();
	} else {
	    bool ms_vis;
	    bool is_hidden = false;
	    if(accel) {
		accel->chars[aidx] = buff.chars[idx];
		accel->oem_pg[aidx] = buff.oem_pg[idx];
		accel->attrs[aidx] = buff.attrs[idx];
	    } else if((iflags & IFLG_ENABLED) == IFLG_ENABLED) {
		tAbsCoord outx,outy;
		ms_vis = tconsole->mouse_get_state();
		outx = (unsigned)X1+x;
		outy = (unsigned)Y1+y;
		if(outx+1 <= tconsole->vio_width() && outy <= tconsole->vio_height()) {
		    if(ms_vis) {
			tAbsCoord mx,my;
			tconsole->mouse_get_pos(mx,my);
			if(mx == outx && my == outy) {
			    is_hidden = true;
			    tconsole->mouse_set_state(false);
			}
		    }
		    it.chars = &buff.chars[idx];
		    it.oem_pg = &buff.oem_pg[idx];
		    it.attrs = &buff.attrs[idx];
		    tconsole->vio_write_buff(outx,outy,tvideo_buffer(it.chars,it.oem_pg,it.attrs,1));
		    if(is_hidden) tconsole->mouse_set_state(true);
		}
		check_win();
	    }
	}
    }
}

/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatewinmemcharfromscreen: correctly copied screen to window memory
 *  screen2win: quick implementation of copying screen to window memory
 *  snapshot:   snap shot of screen to win body
 */
void TWindow::updatewinmemcharfromscreen(tRelCoord x,tRelCoord y,const tvioBuff& accel)
{
    unsigned idx,aidx;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	TWindow * top;
	idx = y*wwidth+x;
	aidx = x;
	top=__find_over(x,y);
	if(top) {
	    unsigned tidx;
	    tAbsCoord tx,ty;
	    tx = X1 - top->X1 + x;
	    ty = Y1 - top->Y1 + y;
	    tidx = tx + ty*top->wwidth;
	    saved.chars[idx] = top->saved.chars[tidx];
	    saved.oem_pg[idx] = top->saved.oem_pg[tidx];
	    saved.attrs[idx] = top->saved.attrs[tidx];
	    top->saved.chars[tidx] = body.chars[idx];
	    top->saved.oem_pg[tidx] = body.oem_pg[idx];
	    top->saved.attrs[tidx] = body.attrs[idx];
	    top->check_win();
	} else {
	    saved.chars[idx] = accel.chars[aidx];
	    saved.oem_pg[idx] = accel.oem_pg[aidx];
	    saved.attrs[idx] = accel.attrs[aidx];
	}
    }
}

void TWindow::screen2win()
{
    unsigned i,lwidth,idx;
    tAbsCoord inx;
    bool ms_vis;
    bool is_hidden = false;
    inx = X1;
    lwidth = wwidth;
    if(inx + lwidth > tconsole->vio_width()) lwidth = tconsole->vio_width() > inx ? tconsole->vio_width() - inx : 0;
    if(lwidth) {
	ms_vis = tconsole->mouse_get_state();
	if(ms_vis) {
	    tAbsCoord mx,my;
	    tconsole->mouse_get_pos(mx,my);
	    if(mx >= inx && mx <= X2 && my >= Y1 && my <= Y2) {
		is_hidden = true;
		tconsole->mouse_set_state(false);
	    }
	}
	if(wwidth == tconsole->vio_width() && !X1) {
	    tvideo_buffer tmp=tconsole->vio_read_buff(0, Y1, wwidth*wheight);
	    ::memcpy(saved.chars,tmp.get_chars(),tmp.length());
	    ::memcpy(saved.oem_pg,tmp.get_oempg(),tmp.length());
	    ::memcpy(saved.attrs,tmp.get_attrs(),tmp.length());
	} else {
	    for(i = 0;i < wheight;i++) {
		tAbsCoord iny;
		iny = Y1+i;
		if(iny <= tconsole->vio_height()) {
		    idx = i*wwidth;
		    tvideo_buffer tmp=tconsole->vio_read_buff(inx,iny,lwidth);
		    ::memcpy(&saved.chars[idx],tmp.get_chars(),tmp.length());
		    ::memcpy(&saved.oem_pg[idx],tmp.get_oempg(),tmp.length());
		    ::memcpy(&saved.attrs[idx],tmp.get_attrs(),tmp.length());
		} else break;
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
    }
    check_win();
}

void TWindow::snapshot() /**< for snapshot */
{
    unsigned i,lwidth,idx;
    tAbsCoord inx;
    bool ms_vis;
    bool is_hidden = false;
    inx = X1;
    lwidth = wwidth;
    if(inx + lwidth > tconsole->vio_width()) lwidth = tconsole->vio_width() > inx ? tconsole->vio_width() - inx : 0;
    if(lwidth) {
	ms_vis = tconsole->mouse_get_state();
	if(ms_vis) {
	    tAbsCoord mx,my;
	    tconsole->mouse_get_pos(mx,my);
	    if(mx >= inx && mx <= X2 && my >= Y1 && my <= Y2) {
		is_hidden = true;
		tconsole->mouse_set_state(false);
	    }
	}
	for(i = 0;i < wheight;i++) {
	    tAbsCoord iny;
	    iny = Y1+i;
	    if(iny <= tconsole->vio_height()) {
		idx = i*wwidth;
		tvideo_buffer tmp=tconsole->vio_read_buff(inx,iny,lwidth);
		::memcpy(&body.chars[idx],tmp.get_chars(),tmp.length());
		::memcpy(&body.oem_pg[idx],tmp.get_oempg(),tmp.length());
		::memcpy(&body.attrs[idx],tmp.get_attrs(),tmp.length());
	    }
	    else break;
	}
	if(is_hidden) tconsole->mouse_set_state(true);
    }
    check_win();
}

/**
 *  Helpful functions:
 *  ==================
 *  savedwin2screen:  restore entire window memory to screen
 *  updatescreen:     restore entire user record of window to screen
 *  updatescreenpiece:restore one piece of line from user record of window to screen
 *  updatewinmem:     update entire window memory from screen
 */

void TWindow::savedwin2screen()
{
    unsigned i,j, tidx;
    tvioBuff accel;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    bool ms_vis, is_hidden = false, is_top;
    accel.chars = chars;
    accel.oem_pg = oem_pg;
    accel.attrs = attrs;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	tAbsCoord mx,my;
	ms_vis = tconsole->mouse_get_state();
	if(ms_vis) {
	    tconsole->mouse_get_pos(mx,my);
	    if(mx >= X1 && mx <= X2 && my >= Y1 && my <= Y2) {
		is_hidden = true;
		tconsole->mouse_set_state(false);
	    }
	}
	is_top = __topmost();
	if(is_top && wwidth == tconsole->vio_width() && !X1) {
	    /* Special case of redrawing window interior at one call */
	    tconsole->vio_write_buff(0, Y1, tvideo_buffer(saved.chars,saved.oem_pg,saved.attrs,wwidth*wheight));
	} else {
	    for(i = 0;i < wheight;i++) {
		tAbsCoord outx,outy;
		unsigned nwidth;
		if(!is_top) for(j = 0;j < wwidth;j++) restorescreenchar(j+1,i+1,&accel);
		else {
		    tidx = i*wwidth;
		    accel.chars = &saved.chars[tidx];
		    accel.attrs = &saved.attrs[tidx];
		    accel.oem_pg = &saved.oem_pg[tidx];
		}
		outx = X1;
		outy = Y1+i;
		nwidth = wwidth;
		if(outx + nwidth > tconsole->vio_width()) nwidth = tconsole->vio_width() > outx ? tconsole->vio_width() - outx : 0;
		if(outy <= tconsole->vio_height() && nwidth) tconsole->vio_write_buff(outx,outy,tvideo_buffer(accel.chars,accel.oem_pg,accel.attrs,nwidth));
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
	check_win();
    }
}

void TWindow::updatescreen(bool full_area)
{
    unsigned i,j, tidx;
    tAbsCoord xs,xe,ys,ye,cx,rw;
    unsigned aoff;
    tvioBuff accel,it;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    bool ms_vis, is_hidden = false, is_top;
    accel.chars = chars;
    accel.oem_pg = oem_pg;
    accel.attrs = attrs;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	tAbsCoord mx,my;
	ms_vis = tconsole->mouse_get_state();
	if(ms_vis) {
	    xs = X1;
	    xe = X2;
	    ys = Y1;
	    ye = Y2;
	    if((flags & Flag_Has_Frame) == Flag_Has_Frame && (!full_area)) {
		xs++; xe--; ys++; ye--;
	    }
	    tconsole->mouse_get_pos(mx,my);
	    if(mx >= xs && mx <= xe && my >= ys && my <= ye) {
		is_hidden = true;
		tconsole->mouse_set_state(false);
	    }
	}
	is_top = __topmost();
	if(is_top && full_area && wwidth == tconsole->vio_width() && !X1 && (iflags & IFLG_ENABLED) == IFLG_ENABLED) {
	    /* Special case of redrawing window interior at one call */
	    tconsole->vio_write_buff(0, Y1, tvideo_buffer(body.chars,body.oem_pg,body.attrs,wwidth*wheight));
	} else {
	    xs = ys = 0;
	    xe = wwidth;
	    ye = wheight;
	    cx = X1;
	    rw = wwidth;
	    aoff = 0;
	    if((flags & Flag_Has_Frame) == Flag_Has_Frame && (!full_area)) {
		xs++; xe--; ys++; ye--;
		cx++; rw-=2;
		aoff = 1;
	    }
	    for(i = ys;i < ye;i++) {
		tAbsCoord outy;
		if(!is_top) for(j = xs;j < xe;j++) updatescreenchar(j+1,i+1,&accel);
		if((iflags & IFLG_ENABLED) == IFLG_ENABLED) {
		    outy = Y1+i;
		    if(cx + rw > tconsole->vio_width()) rw = tconsole->vio_width() > cx ? tconsole->vio_width() - cx : 0;
		    if(outy <= tconsole->vio_height() && rw) {
			tidx = i*wwidth+aoff;
			it.chars = is_top ? &body.chars[tidx] : &accel.chars[aoff];
			it.oem_pg = is_top ? &body.oem_pg[tidx] : &accel.oem_pg[aoff];
			it.attrs = is_top ? &body.attrs[tidx] : &accel.attrs[aoff];
			tconsole->vio_write_buff(cx,outy,tvideo_buffer(it.chars,it.oem_pg,it.attrs,rw));
		    }
		}
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
	check_win();
    }
}

void TWindow::updatescreenpiece(tRelCoord stx,tRelCoord endx,tRelCoord y)
{
    unsigned i,line, tidx;
    tAbsCoord _stx,_endx;
    tvioBuff accel,it;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    bool ms_vis, is_hidden = false, is_top;
    accel.chars = chars;
    accel.oem_pg = oem_pg;
    accel.attrs = attrs;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	tAbsCoord mx,my;
	ms_vis = tconsole->mouse_get_state();
	tconsole->mouse_get_pos(mx,my);
	line = Y1+y-1;
	if(ms_vis && my == line) {
	    is_hidden = true;
	    tconsole->mouse_set_state(false);
	}
	_stx = std::min(stx,wwidth);
	_endx = std::min(endx,wwidth);
	is_top = __topmost();
	if(!is_top) {
	    if(_stx < _endx)
		for(i = stx;i < endx;i++)
		    updatescreenchar(i+1,y,&accel);
	}
	if((iflags & IFLG_ENABLED) == IFLG_ENABLED) {
	    tAbsCoord outx;
	    unsigned rw;
	    outx = X1+_stx;
	    rw = _endx - _stx;
	    if(outx + rw > tconsole->vio_width()) rw = tconsole->vio_width() > outx ? tconsole->vio_width() - outx : 0;
	    if(line <= tconsole->vio_height() && rw) {
		tidx = (y-1)*wwidth+_stx;
		it.chars = is_top ? &body.chars[tidx] : &accel.chars[_stx];
		it.oem_pg = is_top ? &body.oem_pg[tidx] : &accel.oem_pg[_stx];
		it.attrs = is_top ? &body.attrs[tidx] : &accel.attrs[_stx];
		tconsole->vio_write_buff(outx,line,tvideo_buffer(it.chars,it.oem_pg,it.attrs,rw));
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
	check_win();
    }
}

void TWindow::updatewinmem()
{
    unsigned i,j,tidx;
    tvioBuff accel;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    bool ms_vis, is_hidden = false, is_top;
    accel.chars = chars;
    accel.oem_pg = oem_pg;
    accel.attrs = attrs;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	tAbsCoord mx,my;
	ms_vis = tconsole->mouse_get_state();
	if(ms_vis) {
	    tconsole->mouse_get_pos(mx,my);
	    if(mx >= X1 && mx <= X2 && my >= Y1 && my <= Y2) {
		is_hidden = true;
		tconsole->mouse_set_state(false);
	    }
	}
	is_top = __topmost();
	if(is_top && wwidth == tconsole->vio_width() && !X1) {
	    /* Special case of redrawing window interior at one call */
	    tvideo_buffer tmp=tconsole->vio_read_buff(0, Y1, wwidth*wheight);
	    ::memcpy(saved.chars,tmp.get_chars(),tmp.length());
	    ::memcpy(saved.oem_pg,tmp.get_oempg(),tmp.length());
	    ::memcpy(saved.attrs,tmp.get_attrs(),tmp.length());
	} else {
	    for(i = 0;i < wheight;i++) {
		tAbsCoord inx,iny;
		unsigned lwidth;
		lwidth = wwidth;
		inx = X1;
		iny = Y1+i;
		if(inx + lwidth > tconsole->vio_width()) lwidth = tconsole->vio_width() > inx ? tconsole->vio_width() - inx : 0;
		if(iny <= tconsole->vio_height() && lwidth) {
		    if(is_top) {
			tidx = i*wwidth;
			accel.chars = &saved.chars[tidx];
			accel.attrs = &saved.attrs[tidx];
			accel.oem_pg = &saved.oem_pg[tidx];
		    }
		    tvideo_buffer tmp=tconsole->vio_read_buff(inx,iny,lwidth);
		    ::memcpy(accel.chars,tmp.get_chars(),tmp.length());
		    ::memcpy(accel.oem_pg,tmp.get_oempg(),tmp.length());
		    ::memcpy(accel.attrs,tmp.get_attrs(),tmp.length());
		}
		if(!is_top)
		    for(j = 0;j < wwidth;j++)
			updatewinmemcharfromscreen(j,i,accel);
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
    }
}

tRelCoord TWindow::calc_title_off(title_mode mode,unsigned w,unsigned slen)
{
    tRelCoord stx;
    switch(mode) {
	case TMode_Left:
		stx = 2;
		break;
	case TMode_Center:
		stx = ((w - slen)>>1) + 1;
		break;
	default:
	case TMode_Right:
		stx = w - slen;
		break;
    }
    return stx;
}

void TWindow::__draw_frame(tRelCoord xs, tRelCoord ys, tRelCoord xe, tRelCoord ye, const any_t*_frame, DefColor color)
{
    unsigned i;
    ColorAttr cfr,csel;
    tRelCoord sx,sy;
    char frm[8];
    char up,oem_ch;
    ColorAttr lt = 0,gr = 0,bl = 0;
    sx = cur_x;
    sy = cur_y;
    cfr = color.system;
    up = 0;
    ::memcpy(frm,_frame,8);
    if(::memcmp(_frame,UP3D_FRAME,8) == 0 ||
	::memcmp(_frame,DN3D_FRAME,8) == 0) {
	::memcpy(frm,SINGLE_FRAME,8);
	if(tconsole->vio_num_colors() >= 16 && BACK_COLOR(cfr) != Black) {
	    Color cfbk;
	    if(::memcmp(_frame,UP3D_FRAME,8) == 0) up = 1;
	    else up = 2;
	    cfbk = BACK_COLOR(cfr);
	    lt = LOGFB_TO_PHYS((Color)(cfbk < 8 ? cfbk + 8 : White),cfbk);
	    gr = LOGFB_TO_PHYS(cfbk == LightGray ? Gray : LightGray,cfbk);
	    bl = LOGFB_TO_PHYS(cfbk == LightGray ? Black : Gray,cfbk);
	}
    }
    if((flags & Flag_NLS) == Flag_NLS) msystem->nls_oem2osdep((unsigned char *)frm,sizeof(frm));
    igoto_xy(xs,ys);
    csel = up ? up == 1 ? lt : bl : cfr;
    oem_ch = ((char*)_frame)[0];
    wputc_oem(frm[0],do_oem_pg(oem_ch),csel,false);
    for(i = xs+1;i < xe; i++) {
	igoto_xy(i,ys);
	oem_ch = ((char*)_frame)[1];
	wputc_oem(frm[1],do_oem_pg(oem_ch),csel,false);
    }
    igoto_xy(xe,ys);
    oem_ch = ((char*)_frame)[2];
    wputc_oem(frm[2],do_oem_pg(oem_ch),up ? gr : cfr,false);
    for(i = ys+1;i < ye;i++) {
	igoto_xy(xs,i);
	oem_ch = ((char*)_frame)[3];
	wputc_oem(frm[3],do_oem_pg(oem_ch),csel,true);
    }
    csel = up ? up == 1 ? bl : lt : cfr;
    for(i = ys+1;i < ye;i++) {
	igoto_xy(xe,i);
	oem_ch = ((char*)_frame)[4];
	wputc_oem(frm[4],do_oem_pg(oem_ch),csel,true);
    }
    igoto_xy(xs,ye);
    oem_ch = ((char*)_frame)[5];
    wputc_oem(frm[5],do_oem_pg(oem_ch),up ? gr : cfr,false);
    for(i = xs+1;i < xe; i++) {
	igoto_xy(i,ye);
	oem_ch = ((char*)_frame)[6];
	wputc_oem(frm[6],do_oem_pg(oem_ch),csel,false);
    }
    igoto_xy(xe,ye);
    oem_ch = ((char*)_frame)[7];
    wputc_oem(frm[7],do_oem_pg(oem_ch),csel,false);
    cur_x = sx;
    cur_y = sy;
}

void TWindow::make_frame()
{
    unsigned i,w,h;
    tRelCoord sx,sy;
    w = wwidth;
    h = wheight;
    sx = cur_x;
    sy = cur_y;
    __draw_frame(1,1,w,h,Frame,frame);
    if(Title) {
	unsigned slen;
	tRelCoord stx;
	slen = ::strlen(Title);
	if(slen > w) slen = w;
	stx = calc_title_off(TitleMode,w,slen);
	for(i = 0;i < slen;i++) {
	    igoto_xy(stx+i,1);
	    wputc(Title[i],title.system,false);
	}
    }
    if(Footer) {
	unsigned slen;
	tRelCoord stx;
	slen = ::strlen(Footer);
	if(slen > w) slen = w;
	stx = calc_title_off(FooterMode,w,slen);
	for(i = 0;i < slen;i++) {
	    igoto_xy(stx+i,h);
	    wputc(Footer[i],footer.system,false);
	}
    }
    updatescreenpiece(0,wwidth,1);
    updatescreenpiece(0,wwidth,h);
    cur_x = sx;
    cur_y = sy;
}

void TWindow::paint_internal()
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) make_frame();
}

void TWindow::draw_frame(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const unsigned char *_frame,Color fore, Color back)
{
    DefColor dcol;
    tRelCoord sx,sy;
    sx = cur_x;
    sy = cur_y;
    if(::memcmp(_frame,UP3D_FRAME,8) == 0 ||
	::memcmp(_frame,DN3D_FRAME,8) == 0) dcol=__set_color(fore,back);
    else dcol = text;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	x1++; y1++; x2++; y2++;
    }
    __draw_frame(x1,y1,x2,y2,_frame,dcol);
    updatescreenpiece(x1-1,x2,y1);
    updatescreenpiece(x1-1,x2,y2);
    cur_x = sx;
    cur_y = sy;
}

void TWindow::draw_frame(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const unsigned char *_frame,ColorAttr attr)
{
    draw_frame(x1,y1,x2,y2,_frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

void TWindow::show()
{
    send_message(WM_SHOW,0L,NULL);
    if(!(iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	iflags |= IFLG_VISIBLE;
	__unlistwin();
	__athead();
	updatewinmem();
	updatescreen(true);
	if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	    cursorwin = this;
	}
	paint_cursor();
    }
}

void TWindow::show_on_top()
{
    send_message(WM_TOPSHOW,0L,NULL);
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) hide();
    iflags |= IFLG_VISIBLE;
    __unlistwin();
    __athead();
    screen2win();
    updatescreen(true);
    if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	cursorwin = this;
    }
    paint_cursor();
}

void TWindow::show_beneath(TWindow& prev)
{
    send_message(WM_SHOWBENEATH,0L,&prev);
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) hide();
    iflags |= IFLG_VISIBLE;
    __unlistwin();
    __atwin(&prev);
    updatewinmem();
    updatescreen(true);
}

void TWindow::hide()
{
    send_message(WM_HIDE,0L,NULL);
    if(cursorwin == this) set_cursor_type(Cursor_Off);
    savedwin2screen();
    iflags &= ~IFLG_VISIBLE;
}

void TWindow::get_pos(tAbsCoord& x1,tAbsCoord& y1,tAbsCoord& x2,tAbsCoord& y2)
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

unsigned TWindow::width() const { return wwidth; }
unsigned TWindow::height() const { return wheight; }
unsigned TWindow::client_width() const { return flags & Flag_Has_Frame ? wwidth-2 : wwidth; }
unsigned TWindow::client_height() const {  return flags & Flag_Has_Frame ? wheight-2 : wheight; }

void TWindow::move(tAbsCoord dx,tAbsCoord dy)
{
    TWindow* prev;
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

void TWindow::resize(tAbsCoord _width,tAbsCoord _height)
{
    TWindow *prev;
    tvioBuff newbody;
    size_t ncopy,delta,fillsize;
    size_t from,to,size,i,loop,start,idx;
    tAbsCoord oldw,oldh;
    tRelCoord x,y;
    bool vis;
    x = where_x();
    y = where_y();
    vis = (iflags & IFLG_VISIBLE) == IFLG_VISIBLE;
    prev = __prevwin();
    if(vis) hide();
    size = _width*_height;
    newbody.chars = new t_vchar[size];
    newbody.oem_pg = new t_vchar[size];
    newbody.attrs = new ColorAttr[size];
    oldw = wwidth;
    oldh = wheight;
    /* --- Compute copy parameters --- */
    to = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    from = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    delta = 0;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) delta = 2;
    start = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    loop = std::min(oldh,_height);
    ncopy = std::min(_width,oldw);
    fillsize = _width-oldw;
    for(i = start;i < loop;i++) {
	::memcpy(&newbody.chars[to],&body.chars[from],ncopy);
	::memcpy(&newbody.oem_pg[to],&body.oem_pg[from],ncopy);
	::memcpy(&newbody.attrs[to],&body.attrs[from],ncopy);
	if(oldw < _width) {
	    idx = to+ncopy-delta;
	    ::memset(&newbody.chars[idx],TWC_DEF_FILLER,fillsize);
	    ::memset(&newbody.oem_pg[idx],0,fillsize);
	    ::memset(&newbody.attrs[idx],text.system,fillsize);
	}
	check_win();
	to += _width;
	from += oldw;
    }
    if(oldh < _height) {
	for(;i < _height;i++) {
	    ::memset(&newbody.chars[to],TWC_DEF_FILLER,_width);
	    ::memset(&newbody.oem_pg[to],0,_width);
	    ::memset(&newbody.attrs[to],text.system,_width);
	    to += _width;
	}
    }
    delete saved.chars;
    delete saved.oem_pg;
    delete saved.attrs;
    wsize = size;
    wwidth = _width;
    wheight = _height;
    saved.chars = new t_vchar[wsize];
    saved.oem_pg = new t_vchar[wsize];
    saved.attrs = new ColorAttr[wsize];
    delete body.chars;
    delete body.oem_pg;
    delete body.attrs;
    body = newbody;
    X2 = X1 + _width;
    Y2 = Y1 + _height;
    check_win();
    paint_internal();
    if(vis) show_beneath(*prev);
    goto_xy(x,y);
}

void TWindow::into_center(tAbsCoord w,tAbsCoord h)
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

void TWindow::into_center(const TWindow& parent) { into_center(parent.wwidth,parent.wheight); }
void TWindow::into_center() { into_center(tconsole->vio_width(),tconsole->vio_height()); }

void TWindow::clear(unsigned char filler)
{
    size_t to,size,i,loop,start,delta,idx,fillsize;
    tRelCoord cx,cy;
    char oempg = 0;
    cx = cur_x;
    cy = cur_y;
    to = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    size = wwidth;
    delta = 0;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) delta = 2;
    start = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    loop = wheight;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) loop--;
    fillsize = size-delta;
    if((flags & Flag_NLS) == Flag_NLS) {
	oempg = filler;
	msystem->nls_oem2osdep(&filler,1);
    }
    for(i = start;i < loop;i++) {
	idx = to+i*size;
	::memset(&body.chars[idx],filler,fillsize);
	::memset(&body.oem_pg[idx],((flags & Flag_NLS) == Flag_NLS ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0),fillsize);
	::memset(&body.attrs[idx],text.system,fillsize);
	check_win();
	updatescreenpiece(0,wwidth,i+1);
    }
    cur_x = cx;
    cur_y = cy;
}

void TWindow::clreol(unsigned char filler)
{
    size_t size,idx;
    char oempg = 0;
    size = wwidth - cur_x;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) size--;
    idx = cur_x + cur_y*wwidth;
    if((flags & Flag_NLS) == Flag_NLS) {
	oempg = filler;
	msystem->nls_oem2osdep(&filler,1);
    }
    ::memset(&body.chars[idx],filler,size);
    ::memset(&body.oem_pg[idx],((flags & Flag_NLS) == Flag_NLS ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0),size);
    ::memset(&body.attrs[idx],text.system,size);
    check_win();
    updatescreenpiece(cur_x,wwidth,cur_y+1);
}

TWindow* TWindow::set_focus()
{
    TWindow *ret;
    ret = cursorwin;
    if((flags & Flag_Has_Cursor) == Flag_Has_Cursor) {
	cursorwin = this;
    }
    paint_cursor();
    return ret;
}

TWindow* TWindow::get_focus() { return cursorwin; }

tRelCoord TWindow::where_x() const { return (flags & Flag_Has_Frame) == Flag_Has_Frame ? cur_x : cur_x+1; }
tRelCoord TWindow::where_y() const { return (flags & Flag_Has_Frame) == Flag_Has_Frame ? cur_y : cur_y+1; }

void TWindow::wputc_oem(char ch,char oempg,char color,bool update)
{
    unsigned idx;
    idx = cur_x + cur_y*wwidth;
    body.chars[idx] = ch;
    body.oem_pg[idx] = oempg;
    body.attrs[idx] = color;
    check_win();
    if(update) updatescreenchar(cur_x+1,cur_y+1,NULL);
}

void TWindow::putch(char ch)
{
    tRelCoord cx,cy;
    char as_oem = 0;
    cx = cur_x;
    cy = cur_y;
    if(is_valid_xy(cx,cy)) {
	if((flags & Flag_NLS) == Flag_NLS) {
	    as_oem = ch;
	    msystem->nls_oem2osdep((unsigned char *)&ch,1);
	    if(!NLS_IS_OEMPG(as_oem)) as_oem = 0;
	    wputc_oem(ch,as_oem,text.system,true);
	}
	else wputc(ch,text.system,true);
	cur_x++;
    }
}

char TWindow::getch() const
{
    unsigned idx;
    tRelCoord cx,cy;
    cx = X1 + cur_x;
    cy = Y1 + cur_y;
    idx = cx + cy*wwidth;
    return body.chars[idx];
}

int TWindow::puts(const std::string& str)
{
    char* __nls = NULL,* __nls_ptr=NULL;
    const char *__oem_ptr;
    int freq = 0;
    unsigned vidx;
    tAbsCoord usx;
    tRelCoord cx,cy;
    char ch,as_oem;
    if((flags & Flag_NLS) == Flag_NLS) {
	unsigned len;
	len = str.length();
	__nls = new char [len+1];
	__nls_ptr = __nls;
	if(__nls) {
	    ::strcpy(__nls,str.c_str());
	     msystem->nls_oem2osdep((unsigned char *)__nls,len);
        } else __nls = const_cast<char*>(str.c_str());
    }
    else __nls = const_cast<char*>(str.c_str());
    __oem_ptr = str.c_str();
    vidx = cur_x + cur_y*wwidth;
    usx = cur_x;
    while((ch=*__nls++)!=0) {
	as_oem=*__oem_ptr++;
	if(ch == '\n') {
	    updatescreenpiece(usx,cur_x,cur_y+1);
	    set_xy(1,cur_y+1);
	    usx = cur_x;
	    vidx = cur_x + cur_y*wwidth;
	    continue;
	}
	if(ch == '\r') {
	    set_xy(1,cur_y);
	    usx = cur_x;
	    vidx = cur_x + cur_y*wwidth;
	    continue;
	}
	cx = cur_x;
	cy = cur_y;
	if(is_valid_xy(cx,cy)) {
	    body.chars[vidx] = ch;
	    if((flags & Flag_NLS) == Flag_NLS && NLS_IS_OEMPG(as_oem))
		body.oem_pg[vidx] = as_oem;
	    else body.oem_pg[vidx] = 0;
	    body.attrs[vidx++] = text.system;
	    cur_x++;
	    freq++;
	}
    }
    updatescreenpiece(usx,cur_x,cur_y+1);
    paint_cursor();
    if(__nls_ptr) delete __nls_ptr;
    return freq;
}

int TWindow::printf(const std::string& fmt,...)
{
    char *buff;
    int ret;
    va_list args;
    buff = new char [tconsole->vio_width()*tconsole->vio_height()]; /* Danger - format can be great that
					 allocated buffer, but it meaningless */
    ret = 0;
    if(buff) {
	va_start(args,fmt);
	vsprintf(buff,fmt.c_str(),args);
	va_end(args);
	ret = puts(buff);
	delete buff;
    }
    return ret;
}

int TWindow::direct_write(tRelCoord x, tRelCoord y,const any_t*str,unsigned len)
{
    int rc;
    ColorAttr* buff=new ColorAttr[len];
    ::memset(buff,text.system,len);
    rc=direct_write(x,y,str,buff,len);
    delete buff;
    return rc;
}

int TWindow::direct_write(tRelCoord x, tRelCoord y,const any_t*str,const ColorAttr* attrs,unsigned len)
{
    unsigned i,rlen,ioff;
    const char *__nls = NULL,*__oem = NULL;
    char nlsBuff[__TVIO_MAXSCREENWIDTH];
    char oemBuff[__TVIO_MAXSCREENWIDTH];
    ColorAttr __attr[__TVIO_MAXSCREENWIDTH];
    if(!((flags & Flag_Has_Frame) == Flag_Has_Frame)) {
	x--;  y--;
    }
    if(!is_valid_xy(x,y)) return 0;
    rlen = wwidth;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) rlen-=1;
    rlen -= x;
    rlen = std::min(rlen,len);
    if((flags & Flag_NLS) == Flag_NLS) {
	::memcpy(nlsBuff,str,rlen);
	::memcpy(oemBuff,str,rlen);
	for(i = 0;i < rlen;i++) if(!NLS_IS_OEMPG(oemBuff[i])) oemBuff[i] = 0;
	msystem->nls_oem2osdep((unsigned char *)nlsBuff,rlen);
    }
    else {
	::memcpy(nlsBuff,str,rlen);
	::memset(oemBuff,0,rlen);
    }
    __nls = nlsBuff;
    __oem = oemBuff;
    ioff = x+y*wwidth;
    for(i=0;i<rlen;i++) {
	Color fore,back;
	fore = FORE_COLOR(attrs[i]);
	back = BACK_COLOR(attrs[i]);
	adjustColor(fore,back);
	__attr[i] = LOGFB_TO_PHYS(fore,back);
    }
    ::memcpy(&body.chars[ioff],__nls,rlen);
    ::memcpy(&body.oem_pg[ioff],__oem,rlen);
    ::memcpy(&body.attrs[ioff],__attr,rlen);
//    text.system=attrs[len-1];
    check_win();
    updatescreenpiece(x,x+rlen,y+1);
    return rlen;
}

int TWindow::write(tRelCoord x, tRelCoord y,const uint8_t* str,unsigned len)
{
    int rc;
    ColorAttr* buff=new ColorAttr[len];
    ::memset(buff,text.system,len);
    rc=write(x,y,str,buff,len);
    delete buff;
    return rc;
}

int TWindow::write(tRelCoord x, tRelCoord y,const uint8_t* str,const ColorAttr* attrs,unsigned len)
{
    unsigned i,rlen,ioff;
    const char *__nls = NULL,*__oem = NULL;
    char nlsBuff[__TVIO_MAXSCREENWIDTH];
    char oemBuff[__TVIO_MAXSCREENWIDTH];
    ColorAttr __attr[__TVIO_MAXSCREENWIDTH];
    if(!((flags & Flag_Has_Frame) == Flag_Has_Frame)) {
	x--;  y--;
    }
    if(!is_valid_xy(x,y)) return 0;
    rlen = wwidth;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) rlen-=1;
    rlen -= x;
    rlen = std::min(rlen,len);
    if((flags & Flag_NLS) == Flag_NLS) {
	::memcpy(nlsBuff,str,rlen);
	::memcpy(oemBuff,str,rlen);
	for(i = 0;i < rlen;i++) if(!NLS_IS_OEMPG(oemBuff[i])) oemBuff[i] = 0;
	msystem->nls_oem2osdep((unsigned char *)nlsBuff,rlen);
    }
    else {
	::memcpy(nlsBuff,str,rlen);
	::memset(oemBuff,0,rlen);
    }
    __nls = nlsBuff;
    __oem = oemBuff;
    ioff = x+y*wwidth;
    for(i=0;i<rlen;i++) {
	Color fore,back;
	fore = FORE_COLOR(attrs[i]);
	back = BACK_COLOR(attrs[i]);
	adjustColor(fore,back);
	__attr[i] = LOGFB_TO_PHYS(fore,back);
    }
    ::memcpy(&body.chars[ioff],__nls,rlen);
    ::memcpy(&body.oem_pg[ioff],__oem,rlen);
    ::memcpy(&body.attrs[ioff],__attr,rlen);
//    text.system=attrs[len-1];
    check_win();
    updatescreenpiece(x,x+rlen,y+1);
    return rlen;
}

void TWindow::write(tRelCoord x,tRelCoord y,const tvideo_buffer& buff)
{
    const any_t*pbuff;
    unsigned rlen,i,loop_len;
    char nbuff[__TVIO_MAXSCREENWIDTH];
    rlen = (unsigned)x+buff.length() > wwidth ? wwidth-(unsigned)x+1 : buff.length();
    if((unsigned)y <= wheight) {
	pbuff = buff.get_attrs();
	if((twin_flags & TWIF_FORCEMONO) && tconsole->vio_num_colors() >= 16) {
	    ::memcpy(nbuff,buff.get_attrs(),rlen);
	    loop_len = rlen;
	    for(i = 1;i < loop_len;i++) {
		Color fore,back;
		fore = FORE_COLOR(nbuff[i]);
		back = BACK_COLOR(nbuff[i]);
		adjustColor(fore,back);
		nbuff[i] = LOGFB_TO_PHYS(fore,back);
	    }
	    pbuff = nbuff;
	}
	i = (x-1)+(y-1)*wwidth;
	::memcpy(&body.chars[i],buff.get_chars(),rlen);
	::memcpy(&body.oem_pg[i],buff.get_oempg(),rlen);
	::memcpy(&body.attrs[i],pbuff,rlen);
	check_win();
	updatescreenpiece(x-1,x-1+rlen,y);
	paint_cursor();
    }
}

tvideo_buffer TWindow::read(tRelCoord x,tRelCoord y,size_t len) const
{
    unsigned rlen,idx;
    rlen = (unsigned)x+len > wwidth ? wwidth-(unsigned)x+1 : len;
    tvideo_buffer rc(rlen);
    if((unsigned)y <= wheight) {
	idx = (x-1)+(y-1)*wwidth;
	rc.assign(&body.chars[idx],&body.oem_pg[idx],&body.attrs[idx],rlen);
	check_win();
	paint_cursor();
    }
    return rc;
}

void TWindow::freeze() { iflags &= ~IFLG_ENABLED; }

void TWindow::refresh(tRelCoord y)
{
    iflags |= IFLG_ENABLED;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) y++;
    updatescreenpiece(0,wwidth,y);
    paint_cursor();
}

void TWindow::refresh_piece(tRelCoord stx,tRelCoord endx,tRelCoord y)
{
    iflags |= IFLG_ENABLED;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) { stx++; endx++; y++; }
    updatescreenpiece(stx-1,endx-1,y);
    paint_cursor();
}

void TWindow::refresh()
{
    iflags |= IFLG_ENABLED;
    updatescreen((flags & Flag_Has_Frame) == Flag_Has_Frame ? false : true);
    paint_cursor();
}

void TWindow::refresh_full()
{
    iflags |= IFLG_ENABLED;
    updatescreen(true);
    paint_cursor();
}

any_t* TWindow::get_user_data() const { return usrData; }

any_t* TWindow::set_user_data(any_t*data) {
    any_t*ret;
    ret = usrData;
    usrData = data;
    return ret;
}

void TWindow::scroll_up(tRelCoord ypos, unsigned npos)
{
    size_t i;
    tRelCoord _wwidth;
    if(!npos || !is_valid_y(ypos)) return;
    _wwidth = wwidth;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) _wwidth-=2;
    for(i = npos-1;i < ypos;i++) {
	tvideo_buffer accel=read(1,i+2,_wwidth);
	write(1,i+1,accel);
    }
}

void TWindow::scroll_down(tRelCoord ypos, unsigned npos)
{
    size_t i,lim;
    tRelCoord _wwidth,_wheight;
    if(!npos || !is_valid_y(ypos)) return;
    lim = ypos > npos ? ypos - npos + 1 : 1;
    _wwidth = wwidth;
    _wheight = wheight;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	_wwidth-=2;
	_wheight-=2;
    }
    for(i = _wheight;i > lim;i--) {
	tvideo_buffer accel=read(1,i-npos,_wwidth);
	write(1,i,accel);
    }
}

void TWindow::scroll_left(tRelCoord xpos, unsigned npos)
{
    /** @todo Not tested!!! */
    size_t i,j,lim;
    tRelCoord _wheight;
    if(!npos || !is_valid_x(xpos)) return;
    lim = xpos > npos ? xpos - npos : 0;
    _wheight = wheight;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	_wheight-=2;
    }
    for(i = lim;i < xpos;i++) {
	/** @todo Optimize this block */
	for(j = 0;j < _wheight;j++) {
	    tvideo_buffer accel=read(i+2,j,1);
	    write(i+1,j,accel);
	}
    }
}

void TWindow::scroll_right(tRelCoord xpos, unsigned npos)
{
     /** @todo Not tested!!! */
    size_t i,j,lim;
    tRelCoord _wwidth,_wheight;
    if(!npos || !is_valid_x(xpos)) return;
    lim = xpos > npos ? xpos - npos + 1 : 1;
    _wwidth = wwidth;
    _wheight = wheight;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	_wwidth-=2;
	_wheight-=2;
    }
    for(i = _wwidth;i > lim;i--) {
	/** @todo Optimize this block */
	for(j = 0;j < _wheight;j++) {
	    tvideo_buffer accel=read(i-npos,j,1);
	    write(i,j,accel);
	}
    }
}

long TWindow::send_message(unsigned event,unsigned long event_param,const any_t*event_data)
{
    if(method) return ((twClassFunc)(method))(this,event,event_param,event_data);
    return 0L;
}
} // namespace	usr
