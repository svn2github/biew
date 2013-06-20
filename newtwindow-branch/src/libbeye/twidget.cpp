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
#include "libbeye/twidget.h"

namespace	usr {
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

const unsigned char TWidget::brightness[16] =
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

uint8_t TWidget::color_map[16] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

void TWidget::adjustColor(Color& fore,Color& back)
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

DefColor TWidget::__set_color(Color fore,Color back)
{
    DefColor rc;
    rc.user = LOGFB_TO_PHYS(fore,back);
    adjustColor(fore,back);
    rc.system = LOGFB_TO_PHYS(fore,back);
    return rc;
}

void TWidget::remap_color(Color color,unsigned char value)
{
    color_map[color & 0x0F] = value & 0x0F;
}

unsigned char TWidget::get_mapped_color(Color color)
{
    return color_map[color & 0x0F];
}

const unsigned char TWidget::SINGLE_FRAME[8] = { TWC_Sd_Sr, TWC_SH, TWC_Sl_Sd, TWC_SV, TWC_SV, TWC_Su_Sr, TWC_SH, TWC_Sl_Su };
const unsigned char TWidget::DOUBLE_FRAME[8] = { TWC_Dd_Dr, TWC_DH, TWC_Dl_Dd, TWC_DV, TWC_DV, TWC_Du_Dr, TWC_DH, TWC_Dl_Du };
const unsigned char TWidget::MEDIUM_FRAME[8] = { TWC_FL_BLK, TWC_UP_HBLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_LF_HBLK, TWC_FL_BLK };
const unsigned char TWidget::THICK_FRAME[8] = { TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK, TWC_FL_BLK };
const unsigned char TWidget::UP3D_FRAME[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
const unsigned char TWidget::DN3D_FRAME[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#ifndef NDEBUG
#ifdef __TSC__
#pragma save , call(inline=>on) /** GPF in protmode */
static void winInternalError() = { 0xFF, 0xFF };
#elif defined _MSC_VER
static inline void winInternalError() {};
#else
static inline void winInternalError() { (void)0xFFFFFFFF; };
#endif
bool TWidget::test_win() const { return true; }
void TWidget::check_win() const { if(!test_win()) winInternalError(); }
#else
void TWidget::check_win() const {}
#endif

void TWidget::create(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
{
    TObject::create(x1,y1,_width,_height,_flags);

    ::memcpy(Frame,SINGLE_FRAME,8);
    surface.fill(tvideo_symbol(TWC_DEF_FILLER,0,text.system));
    check_win();
    text=__set_color(LightGray,Black);
    frame=__set_color(LightGray,Black);
    title=__set_color(LightGray,Black);
    footer=__set_color(LightGray,Black);

    paint_internal();
}

TWidget::TWidget(tAbsCoord x1, tAbsCoord y1, tAbsCoord _width, tAbsCoord _height, twc_flag _flags)
	:TObject(x1,y1,_width,_height,_flags)
	,surface(_width*_height)
{
    TWidget::create(x1,y1,_width,_height,_flags);
}

TWidget::TWidget(tAbsCoord x1_, tAbsCoord y1_,
		 tAbsCoord _width, tAbsCoord _height,
		 twc_flag _flags, const std::string& classname)
	:TObject(x1_,y1_,_width,_height,_flags,classname)
	,surface(_width*_height)
{
    TWidget::create(x1_, y1_, _width, _height, _flags);
}

TWidget::~TWidget()
{
    if(Title) delete Title;
    if(Footer) delete Footer;
}

ColorAttr TWidget::set_color(Color fore,Color back)
{
    ColorAttr ret;
    ret = text.user;
    text=__set_color(fore,back);
    return ret;
}

ColorAttr TWidget::set_color(ColorAttr ca) {  return set_color(FORE_COLOR(ca),BACK_COLOR(ca)); }

ColorAttr TWidget::get_color() const { return text.user; }

void TWidget::get_color(Color& fore,Color& back) const
{
    ColorAttr ca = get_color();
    PHYS_TO_LOGFB(ca,fore,back);
}

Color TWidget::text_color(Color col)
{
    Color back,ret;
    ColorAttr attr;
    attr = get_color();
    back = BACK_COLOR(attr);
    ret = FORE_COLOR(attr);
    set_color(col,back);
    return ret;
}

Color TWidget::text_bkgnd(Color col)
{
    Color fore,ret;
    ColorAttr attr;
    attr = get_color();
    fore = FORE_COLOR(attr);
    ret = BACK_COLOR(attr);
    set_color(fore, col);
    return ret;
}

void TWidget::set_frame(const unsigned char *_frame,Color fore,Color back)
{
    flags |= Flag_Has_Frame;
    ::memcpy(Frame,_frame,8);
    frame=__set_color(fore,back);
    paint_internal();
}

void TWidget::set_frame(const unsigned char *_frame,ColorAttr attr)
{
    set_frame(_frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

void TWidget::get_frame(unsigned char *_frame,ColorAttr& attr) const
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) {
	::memcpy(_frame,Frame,8);
	attr = frame.user;
    }
}

void TWidget::get_frame(unsigned char *_frame,Color& fore,Color& back) const
{
    ColorAttr attr = 0;
    get_frame(_frame,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
}

void TWidget::set_title(const std::string& _title,title_mode mode,Color fore,Color back)
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

void TWidget::set_title(const std::string& _title,title_mode mode,ColorAttr attr)
{
    set_title(_title,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

TWidget::title_mode TWidget::get_title(char *_title,unsigned cb_title,ColorAttr& attr) const
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

TWidget::title_mode TWidget::get_title(char *_title,unsigned cb_title,Color& fore,Color& back) const
{
    ColorAttr attr;
    title_mode ret;
    ret = get_title(_title,cb_title,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
    return ret;
}

void TWidget::set_footer(const std::string& _footer,title_mode mode,Color fore,Color back)
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

void TWidget::set_footer(const std::string& _footer,title_mode mode,ColorAttr attr)
{
    set_footer(_footer,mode,FORE_COLOR(attr),BACK_COLOR(attr));
}

TWidget::title_mode TWidget::get_footer(char *_footer,unsigned cb_footer,ColorAttr& attr) const
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


TWidget::title_mode TWidget::get_footer(char *_footer,unsigned cb_footer,Color& fore,Color& back) const
{
    ColorAttr attr;
    title_mode ret;
    ret = get_footer(_footer,cb_footer,attr);
    fore = FORE_COLOR(attr);
    back = BACK_COLOR(attr);
    return ret;
}

/**
 *  Three basic functions for copying from buffer to screen:
 *  ========================================================
 *  updatescreencharfrombuff: low level implementation updatescreen family
 *  updatescreenchar:  correctly copyed user record from win to screen
 *  restorescreenchar: correctly copyed window memory from win to screen
 */
void TWidget::updatescreencharfrombuff(tRelCoord x,
					tRelCoord y,
					const tvideo_buffer& buff,
					tvideo_buffer *accel) const
{
    unsigned idx,aidx;
    if((iflags & TObject::Visible) == TObject::Visible) {
	idx = y*wwidth+x;
	aidx = x;
	TWidget* top=static_cast<TWidget*>(__find_over(x,y));
	if(top) {
	    unsigned tidx;
	    tAbsCoord tx,ty;
	    tx = X1 - top->X1 + x;
	    ty = Y1 - top->Y1 + y;
	    tidx = tx + ty*top->wwidth;
	    if(accel) {
		TWidget* vis=NULL;
		tAbsCoord xx,yy;
		xx = x+X1;
		yy = y+Y1;
		vis=static_cast<TWidget*>(__at_point(vis,xx,yy));
		tx = xx - vis->X1;
		ty = yy - vis->Y1;
		tidx = tx + ty*vis->wwidth;
		(*accel)[aidx]=vis->surface[tidx];
	    }
	    top->check_win();
	} else {
	    bool ms_vis;
	    bool is_hidden = false;
	    if(accel) {
		(*accel)[aidx]=buff[idx];
	    } else if((iflags & TObject::Enabled) == TObject::Enabled) {
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
		    tvideo_buffer it(buff[idx],1);
		    tconsole->vio_write_buff(outx,outy,it);
		    if(is_hidden) tconsole->mouse_set_state(true);
		}
		check_win();
	    }
	}
    }
}

void TWidget::snapshot() /**< for snapshot */
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
		surface.assign_at(idx,tmp);
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
void TWidget::updatescreen(bool full_area)
{
    unsigned i,j, tidx;
    tAbsCoord xs,xe,ys,ye,cx,rw;
    unsigned aoff;
    tvideo_buffer accel(__TVIO_MAXSCREENWIDTH);
    bool ms_vis, is_hidden = false, is_top;
    if((iflags & TObject::Visible) == TObject::Visible) {
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
	if(is_top && full_area && wwidth == tconsole->vio_width() && !X1 && (iflags & TObject::Enabled) == TObject::Enabled) {
	    /* Special case of redrawing window interior at one call */
	    tvideo_buffer cp=surface;
	    tconsole->vio_write_buff(0, Y1, cp);
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
		if((iflags & TObject::Enabled) == TObject::Enabled) {
		    outy = Y1+i;
		    if(cx + rw > tconsole->vio_width()) rw = tconsole->vio_width() > cx ? tconsole->vio_width() - cx : 0;
		    if(outy <= tconsole->vio_height() && rw) {
			tidx = i*wwidth+aoff;
			tvideo_buffer it(is_top?surface.sub_buffer(tidx,rw):accel.sub_buffer(aoff,rw));
			tconsole->vio_write_buff(cx,outy,it);
		    }
		}
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
	check_win();
    }
}

void TWidget::updatescreenpiece(tRelCoord stx,tRelCoord endx,tRelCoord y)
{
    unsigned i,line, tidx;
    tAbsCoord _stx,_endx;
    tvideo_buffer accel(__TVIO_MAXSCREENWIDTH);
    bool ms_vis, is_hidden = false, is_top;
    if((iflags & TObject::Visible) == TObject::Visible) {
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
	if((iflags & TObject::Enabled) == TObject::Enabled) {
	    tAbsCoord outx;
	    unsigned rw;
	    outx = X1+_stx;
	    rw = _endx - _stx;
	    if(outx + rw > tconsole->vio_width()) rw = tconsole->vio_width() > outx ? tconsole->vio_width() - outx : 0;
	    if(line <= tconsole->vio_height() && rw) {
		tidx = (y-1)*wwidth+_stx;
		tvideo_buffer it(is_top?surface.sub_buffer(tidx,rw):accel.sub_buffer(_stx,rw));
		tconsole->vio_write_buff(outx,line,it);
	    }
	}
	if(is_hidden) tconsole->mouse_set_state(true);
	check_win();
    }
}

tRelCoord TWidget::calc_title_off(title_mode mode,unsigned w,unsigned slen)
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
		stx = (w - slen)-2;
		break;
    }
    return stx;
}

void TWidget::__draw_frame(tRelCoord xs, tRelCoord ys, tRelCoord xe, tRelCoord ye, const any_t*_frame, DefColor color)
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
    igoto_xy(xe-1,ys);
    oem_ch = ((char*)_frame)[2];
    wputc_oem(frm[2],do_oem_pg(oem_ch),up ? gr : cfr,false);
    for(i = ys+1;i < ye;i++) {
	igoto_xy(xs,i);
	oem_ch = ((char*)_frame)[3];
	wputc_oem(frm[3],do_oem_pg(oem_ch),csel,true);
    }
    csel = up ? up == 1 ? bl : lt : cfr;
    for(i = ys+1;i < ye;i++) {
	igoto_xy(xe-1,i);
	oem_ch = ((char*)_frame)[4];
	wputc_oem(frm[4],do_oem_pg(oem_ch),csel,true);
    }
    igoto_xy(xs,ye-1);
    oem_ch = ((char*)_frame)[5];
    wputc_oem(frm[5],do_oem_pg(oem_ch),up ? gr : cfr,false);
    for(i = xs+1;i < xe; i++) {
	igoto_xy(i,ye-1);
	oem_ch = ((char*)_frame)[6];
	wputc_oem(frm[6],do_oem_pg(oem_ch),csel,false);
    }
    igoto_xy(xe-1,ye-1);
    oem_ch = ((char*)_frame)[7];
    wputc_oem(frm[7],do_oem_pg(oem_ch),csel,false);
    cur_x = sx;
    cur_y = sy;
}

void TWidget::make_frame()
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
	    igoto_xy(stx+i,h-1);
	    wputc(Footer[i],footer.system,false);
	}
    }
    updatescreenpiece(0,wwidth,1);
    updatescreenpiece(0,wwidth,h);
    cur_x = sx;
    cur_y = sy;
}

void TWidget::paint_internal()
{
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) make_frame();
}

void TWidget::draw_frame(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const unsigned char *_frame,Color fore, Color back)
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

void TWidget::draw_frame(tRelCoord x1, tRelCoord y1, tRelCoord x2, tRelCoord y2,const unsigned char *_frame,ColorAttr attr)
{
    draw_frame(x1,y1,x2,y2,_frame,FORE_COLOR(attr),BACK_COLOR(attr));
}

void TWidget::show()
{
    if(!(iflags & TObject::Visible) == TObject::Visible) {
	TObject::show();
	updatescreen(true);
    }
}

void TWidget::show_on_top()
{
    TObject::show_on_top();
    updatescreen(true);
}

void TWidget::show_beneath(TWidget& prev)
{
    TObject::show_beneath(prev);
    updatescreen(true);
}

void TWidget::hide()
{
    TObject::hide();
}

void TWidget::resize(tAbsCoord _width,tAbsCoord _height)
{
    size_t ncopy,delta,fillsize;
    size_t from,to,size,i,loop,start,idx;
    tAbsCoord oldw,oldh;
    tRelCoord x,y;
    bool vis;
    x = where_x();
    y = where_y();
    vis = (iflags & TObject::Visible) == TObject::Visible;
    TWidget* prev = static_cast<TWidget*>(__prevwin());
    if(vis) hide();
    size = _width*_height;
    tvideo_buffer newbody(size);
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
	newbody.assign_at(to,surface.sub_buffer(from,ncopy));
	if(oldw < _width) {
	    idx = to+ncopy-delta;
	    newbody.fill_at(idx,tvideo_symbol(TWC_DEF_FILLER,0,text.system),fillsize);
	}
	check_win();
	to += _width;
	from += oldw;
    }
    if(oldh < _height) {
	for(;i < _height;i++) {
	    newbody.fill_at(to,tvideo_symbol(TWC_DEF_FILLER,0,text.system),_width);
	    to += _width;
	}
    }
    wwidth = _width;
    wheight = _height;
    surface = newbody;
    X2 = X1 + _width;
    Y2 = Y1 + _height;
    check_win();
    paint_internal();
    if(vis) show_beneath(*prev);
    goto_xy(x,y);
}

void TWidget::clear(unsigned char filler)
{
    size_t to,size,i,loop,start,delta,idx,fillsize;
    tRelCoord cx,cy;
    char oempg = 0;
    cx = cur_x;
    cy = cur_y;
    to = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    size = wwidth-1;
    delta = 0;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) delta = 2;
    start = (flags & Flag_Has_Frame) == Flag_Has_Frame ? 1 : 0;
    loop = wheight-1;
    if((flags & Flag_Has_Frame) == Flag_Has_Frame) loop--;
    fillsize = size-delta;
    if((flags & Flag_NLS) == Flag_NLS) {
	oempg = filler;
	msystem->nls_oem2osdep(&filler,1);
    }
    for(i = start;i < loop;i++) {
	idx = to+i*wwidth;
	oempg = ((flags & Flag_NLS) == Flag_NLS ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0);
	surface.fill_at(idx,tvideo_symbol(filler,oempg,text.system),fillsize);
	check_win();
	updatescreenpiece(0,wwidth,i+1);
    }
    cur_x = cx;
    cur_y = cy;
}

void TWidget::clreol(unsigned char filler)
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
    oempg = ((flags & Flag_NLS) == Flag_NLS ? NLS_IS_OEMPG(oempg) ? oempg : 0 : 0);
    surface.fill_at(idx,tvideo_symbol(filler,oempg,text.system),size);
    check_win();
    updatescreenpiece(cur_x,wwidth,cur_y+1);
}

void TWidget::wputc_oem(char ch,char oempg,char color,bool update)
{
    unsigned idx;
    idx = cur_x + cur_y*wwidth;
    surface[idx]=tvideo_symbol(ch,oempg,color);
    check_win();
    if(update) updatescreenchar(cur_x+1,cur_y+1,NULL);
}

void TWidget::putch(char ch)
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

char TWidget::getch() const
{
    unsigned idx;
    tRelCoord cx,cy;
    cx = X1 + cur_x;
    cy = Y1 + cur_y;
    idx = cx + cy*wwidth;
    return surface[idx].symbol();
}

int TWidget::puts(const std::string& str)
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
	    t_vchar coem;
	    if((flags & Flag_NLS) == Flag_NLS && NLS_IS_OEMPG(as_oem))
		coem = as_oem;
	    else coem = 0;
	    surface[vidx++]=tvideo_symbol(ch,coem,text.system);
	    cur_x++;
	    freq++;
	}
    }
    updatescreenpiece(usx,cur_x,cur_y+1);
    goto_xy(cur_x,cur_y);
    if(__nls_ptr) delete __nls_ptr;
    return freq;
}

int TWidget::printf(const std::string& fmt,...)
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

int TWidget::direct_write(tRelCoord x, tRelCoord y,const any_t*str,unsigned len)
{
    int rc;
    ColorAttr* buff=new ColorAttr[len];
    ::memset(buff,text.system,len);
    rc=direct_write(x,y,str,buff,len);
    delete buff;
    return rc;
}

int TWidget::direct_write(tRelCoord x, tRelCoord y,const any_t*str,const ColorAttr* attrs,unsigned len)
{
    unsigned i,rlen,ioff;
    const t_vchar *__nls = NULL,*__oem = NULL;
    t_vchar nlsBuff[__TVIO_MAXSCREENWIDTH];
    t_vchar oemBuff[__TVIO_MAXSCREENWIDTH];
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
    std::vector<tvideo_symbol> v(rlen);
    for(i=0;i<rlen;i++) v[i]=tvideo_symbol(__nls[i],__oem[i],__attr[i]);
    surface.assign_at(ioff,v);
//    text.system=attrs[len-1];
    check_win();
    updatescreenpiece(x,x+rlen,y+1);
    return rlen;
}

int TWidget::write(tRelCoord x, tRelCoord y,const uint8_t* str,unsigned len)
{
    int rc;
    ColorAttr* buff=new ColorAttr[len];
    ::memset(buff,text.system,len);
    rc=write(x,y,str,buff,len);
    delete buff;
    return rc;
}

int TWidget::write(tRelCoord x, tRelCoord y,const uint8_t* str,const ColorAttr* attrs,unsigned len)
{
    unsigned i,rlen,ioff;
    const t_vchar *__nls = NULL,*__oem = NULL;
    t_vchar nlsBuff[__TVIO_MAXSCREENWIDTH];
    t_vchar oemBuff[__TVIO_MAXSCREENWIDTH];
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
    std::vector<tvideo_symbol> v(rlen);
    for(i=0;i<rlen;i++) v[i]=tvideo_symbol(__nls[i],__oem[i],__attr[i]);
    surface.assign_at(ioff,v);
//    text.system=attrs[len-1];
    check_win();
    updatescreenpiece(x,x+rlen,y+1);
    return rlen;
}

void TWidget::write(tRelCoord x,tRelCoord y,const tvideo_buffer& buff)
{
    unsigned rlen,i;
    rlen = (unsigned)x+buff.size() > wwidth ? wwidth-(unsigned)x+1 : buff.size();
    if((unsigned)y <= wheight) {
	i = (x-1)+(y-1)*wwidth;
	surface.assign_at(i,buff,rlen);
	check_win();
	updatescreenpiece(x-1,x-1+rlen,y);
    }
}

tvideo_buffer TWidget::read(tRelCoord x,tRelCoord y,size_t len) const
{
    unsigned rlen,idx;
    rlen = (unsigned)x+len > wwidth ? wwidth-(unsigned)x+1 : len;
    tvideo_buffer rc(rlen);
    if((unsigned)y <= wheight) {
	idx = (x-1)+(y-1)*wwidth;
	rc=surface.sub_buffer(idx,rlen);
	check_win();
    }
    return rc;
}

void TWidget::refresh(tRelCoord y)
{
    TObject::refresh(y);
    updatescreenpiece(0,wwidth,y);
}

void TWidget::refresh_piece(tRelCoord stx,tRelCoord endx,tRelCoord y)
{
    TObject::refresh_piece(stx,endx,y);
    updatescreenpiece(stx-1,endx-1,y);
}

void TWidget::refresh()
{
    TObject::refresh();
    updatescreen((flags & Flag_Has_Frame) == Flag_Has_Frame ? false : true);
}

void TWidget::refresh_full()
{
    TObject::refresh_full();
    updatescreen(true);
}

void TWidget::scroll_up(tRelCoord ypos, unsigned npos)
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

void TWidget::scroll_down(tRelCoord ypos, unsigned npos)
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

void TWidget::scroll_left(tRelCoord xpos, unsigned npos)
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

void TWidget::scroll_right(tRelCoord xpos, unsigned npos)
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
} // namespace	usr
