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
#include "libbeye/twindow.h"

namespace	usr {
bool TWindow::test_win() const
{
    bool ret;
    ret = TWidget::test_win() &&
	*((any_t**)(saved.chars + wsize)) == saved.chars &&
	*((any_t**)(saved.oem_pg + wsize)) == saved.oem_pg &&
	*((any_t**)(saved.attrs + wsize)) == saved.attrs ? true : false;
    return ret;
}

enum {
    IFLG_VISIBLE      =0x00000001UL,
    IFLG_ENABLED      =0x00000002UL,
    IFLG_CURSORBEENOFF=0x80000000UL
};

void TWindow::create(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
{
    unsigned size = _width*_height;
    saved.chars = new t_vchar[size];
    saved.oem_pg = new t_vchar[size];
    saved.attrs = new ColorAttr[size];

    TWidget::create(x1,y1,_width,_height,_flags);
}

TWindow::TWindow(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
	:TWidget(x1,y1,_width,_height,_flags)
{
    TWindow::create(x1,y1,_width,_height,_flags);
}

TWindow::TWindow(tAbsCoord x1_, tAbsCoord y1_,
		 tAbsCoord _width, tAbsCoord _height,
		 twc_flag _flags, const std::string& classname)
	:TWidget(x1_,y1_,_width,_height,_flags,classname)
{
    TWindow::create(x1_, y1_, _width, _height, _flags);
}

TWindow::~TWindow()
{
    delete saved.chars;
    delete saved.oem_pg;
    delete saved.attrs;
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
    unsigned idx;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	idx = y*wwidth+x;
	TWindow* top=static_cast<TWindow*>(__find_over(x,y));
	if(top) {
	    unsigned tidx;
	    tAbsCoord tx,ty;
	    tx = X1 - top->X1 + x;
	    ty = Y1 - top->Y1 + y;
	    tidx = tx + ty*top->wwidth;
	    top->saved.chars[tidx] = buff.chars[idx];
	    top->saved.oem_pg[tidx] = buff.oem_pg[idx];
	    top->saved.attrs[tidx] = buff.attrs[idx];
	}
    }
    TWidget::updatescreencharfrombuff(x,y,buff,accel);
}

/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatewinmemcharfromscreen: correctly copied screen to window memory
 *  screen2win: quick implementation of copying screen to window memory
 *  snapshot:   snap shot of screen to win surface
 */
void TWindow::updatewinmemcharfromscreen(tRelCoord x,tRelCoord y,const tvioBuff& accel)
{
    unsigned idx,aidx;
    if((iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	idx = y*wwidth+x;
	aidx = x;
	TWindow* top=static_cast<TWindow*>(__find_over(x,y));
	if(top) {
	    unsigned tidx;
	    tAbsCoord tx,ty;
	    tx = X1 - top->X1 + x;
	    ty = Y1 - top->Y1 + y;
	    tidx = tx + ty*top->wwidth;
	    saved.chars[idx] = top->saved.chars[tidx];
	    saved.oem_pg[idx] = top->saved.oem_pg[tidx];
	    saved.attrs[idx] = top->saved.attrs[tidx];
	    top->saved.chars[tidx] = get_surface().chars[idx];
	    top->saved.oem_pg[tidx] = get_surface().oem_pg[idx];
	    top->saved.attrs[tidx] = get_surface().attrs[idx];
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

void TWindow::show()
{
    if(!(iflags & IFLG_VISIBLE) == IFLG_VISIBLE) {
	updatewinmem();
	TWidget::show();
    }
}

void TWindow::show_on_top()
{
    screen2win();
    TWidget::show_on_top();
}

void TWindow::show_beneath(TWindow& prev)
{
    updatewinmem();
    TWidget::show_beneath(prev);
}

void TWindow::hide()
{
    TWidget::hide();
    savedwin2screen();
}

void TWindow::resize(tAbsCoord _width,tAbsCoord _height)
{
    size_t size=_width*_height;
    delete saved.chars;
    delete saved.oem_pg;
    delete saved.attrs;
    wsize = size;
    wwidth = _width;
    wheight = _height;
    saved.chars = new t_vchar[wsize];
    saved.oem_pg = new t_vchar[wsize];
    saved.attrs = new ColorAttr[wsize];
    TWidget::resize(_width,_height);
}
} // namespace	usr
