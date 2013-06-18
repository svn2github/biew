#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include "libbeye/osdep/__os_dep.h"
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/unix/vio.c
 * @brief       slang/curses/vt100 implementation of screen functions
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: vio.c,v 1.16 2009/09/20 14:39:37 nickols_k Exp $
*/
#include <iostream>

#ifndef lint
static const char rcs_id[] = "$Id: vio.c,v 1.16 2009/09/20 14:39:37 nickols_k Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "console.h"

#define VMAX_X __TVIO_MAXSCREENWIDTH
#define VMAX_Y	0x400

#define	_addr(x, y)	(viomem + ((x) + (y) * tvioWidth))

int console_flags = 0, on_console = 0, output_7 = 0, transparent = 0;
tAbsCoord tvioWidth = 80, tvioHeight = 25;
unsigned tvioNumColors = 16;

static int initialized = 0, cursor_status = __TVIO_CUR_NORM;
static tAbsCoord saveX, saveY, firstX = 0, firstY = 0;

static unsigned violen;
static unsigned char *viomem;

static struct {
    unsigned char last;
    unsigned char color[0x10];
} tp = { 0, {0} };

#ifdef HAVE_ICONV
static char *screen_cp;
static unsigned is_unicode=0;
static any_t*nls_handle;
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define _bg(x) ((x) >> 4)
#define _fg(x) ((x) & 0x0f)

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#define VTMP_LEN 100

static char *vtmp;
static int out_fd;
static int _color[8] = {0,4,2,6,1,5,3,7};

static unsigned char frames_vt100[0x30] =
"aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjlaaaaa";

#define twrite(x)	write(out_fd,(x),strlen(x))

/**
    convert vga attrubute to ansi escape sequence
*/
static char *__FASTCALL__ _2ansi(unsigned char attr)
{
    int bg = _bg(attr);
    int bc = _color[bg & 7];

    if (transparent) {
	int i;
	for (i = 0; i < tp.last; i++)
	    if (bg == tp.color[i]) bc = 9;
    }

    sprintf(vtmp,
	"\033[%d;3%d;4%d%sm",
	_fg(attr) > 7,
	_color[_fg(attr) & 7],
	bc,
	bg > 7 ? ";5" : ""
    );
    return vtmp;
}

static unsigned char frames_dumb[0x30] =
": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

/*

*/

inline static int __FASTCALL__ printable(unsigned char c)
{
    int result;
    if(is_unicode) result = !(c < 0x20 || c == 0x7f);
    else    result = !(c < 0x20 || c == 0x7f || c == 0x9b); /* 0x80< c < 0xA0 */

    if (result && terminal->type == TERM_XTERM && !is_unicode)
	result = !(
		c == 0x84 || c == 0x85 || c == 0x88 ||
		(c >= 0x8D && c <= 0x90) ||
		(c >= 0x96 && c <= 0x98) ||
		(c >= 0x9A && c <= 0x9F) );

    return result;
}

static void gotoxy(int x, int y)
{
    sprintf(vtmp,"\033[%d;%dH", y + 1, x + 1);
    twrite(vtmp);
}

int __FASTCALL__ __vioGetCursorType()
{
    return cursor_status;
}

void __FASTCALL__ __vioSetCursorType(int type)
{
    cursor_status = type;
    if	(terminal->type == TERM_LINUX &&
	(cursor_status == __TVIO_CUR_SOLID || type == __TVIO_CUR_SOLID)) {
	sprintf(vtmp,"\033[?%dc", type == __TVIO_CUR_SOLID ? 7 : 0);
	twrite(vtmp);
    }
    sprintf(vtmp, "\033[?25%c", type == __TVIO_CUR_OFF ? 'l' : 'h');
    twrite(vtmp);
}

void __FASTCALL__ __vioGetCursorPos(tAbsCoord *x,tAbsCoord *y)
{
    *x = saveX;
    *y = saveY;
}

void __FASTCALL__ __vioSetCursorPos(tAbsCoord x,tAbsCoord y)
{
    saveX = x;
    saveY = y;
    gotoxy(x, y);
}

void __FASTCALL__ __vioReadBuff(tAbsCoord x, tAbsCoord y, tvioBuff *buff, unsigned len)
{
    unsigned char *addr = _addr(x, y);

    memcpy(buff->attrs, addr, len);
    memcpy(buff->chars, addr + violen, len);
    memcpy(buff->oem_pg, addr + (violen << 1), len);
}

void __FASTCALL__ __vioWriteBuff(tAbsCoord x, tAbsCoord y, const tvioBuff *buff, unsigned len)
{
    unsigned i;
    unsigned char c;
    unsigned char *addr;
    tAbsCoord xx, yy;
#define	LEN(x) (x << 4)
    unsigned char mode = 0, old_mode = -1;
    unsigned char cache_pb[LEN(VMAX_X)];
    unsigned char *dpb,*pb = len > VMAX_X ? new char [LEN(len)] : cache_pb;
    unsigned slen;

    dpb=pb;

    memset(pb, 0, LEN(len));

/*    if (!len) return; */

    addr = _addr(x, y);
    memcpy(addr, buff->attrs, len);
    memcpy(addr + violen, buff->chars, len);
    memcpy(addr + (violen << 1), buff->oem_pg, len);

    __vioGetCursorPos(&xx, &yy);
    gotoxy(x, y);

    for (i = 0; i < len; i++) {
	c = buff->chars[i];

#define cp buff->oem_pg[i]
#define ca buff->attrs[i]

	if (cp && cp >= _PSMIN && cp <= _PSMAX && !is_unicode) {
		mode = 1;
		c = output_7 ?
		    frames_dumb[cp - _PSMIN] :
		    frames_vt100[cp - _PSMIN];
	} else {
	    mode = 0;
	}

	if (output_7) c &= 0x7f;
	else {
	    char *map = mode ? "\016" : "\017";
	    if (old_mode != mode)
	    {
		strcpy(dpb,map);
		dpb += strlen(map);
	    }
	    old_mode = mode;
	}
	if (!c) c = ' '; else if (!printable(c)) c = '.';

	/* TODO: make sure that compiler produces right order of conditions! */
	if ((i && ca != buff->attrs[i - 1]) || i == len || !i)
	{
	    unsigned char *d;
	    d = _2ansi(ca);
	    strcpy(dpb, d);
	    dpb += strlen(d);
	}
	if(!is_unicode) {
	    *dpb=c; dpb++;
	}
	else {
	    unsigned len=1;
	    char *destb=nls_recode2screen_cp(nls_handle,&c,&len);
	    memcpy(dpb,destb,len);
	    delete destb;
	    dpb+=len;
	}
    }
    *dpb=0;
    dpb=pb;
    slen=strlen(dpb);
    while(slen)
    {
	unsigned stored=twrite(dpb);
	dpb+=stored;
	slen-= stored;
    }
    gotoxy(xx, yy);
    if (pb != cache_pb) delete pb;
#undef	LEN

#undef	ca
#undef	cp
}

void __FASTCALL__ __init_vio(const char *user_cp,unsigned long flags)
{
    struct winsize w;

#ifdef HAVE_ICONV
    screen_cp=nls_get_screen_cp();
    if(strncasecmp(screen_cp,"UTF",3)==0) {
	is_unicode=1;
    }
    nls_handle=nls_init(screen_cp,user_cp);
    if(nls_handle==NULL) is_unicode=0;
#endif
    console_flags = flags;

    if (!output_7) output_7 = TESTFLAG(console_flags, __TVIO_FLG_USE_7BIT);
    do_nls = 1;

    vtmp = new char [VTMP_LEN];
    memset(vtmp, 0, VTMP_LEN);

    out_fd = open(ttyname(STDOUT_FILENO), O_WRONLY);
    if (out_fd < 0) out_fd = STDOUT_FILENO;

    ioctl(out_fd, TIOCGWINSZ, &w);
    tvioWidth = w.ws_col;
    tvioHeight = w.ws_row;

    if (tvioWidth <= 0) tvioWidth = 80;
    if (tvioHeight <= 0) tvioHeight = 25;
    if (tvioWidth > VMAX_X) tvioWidth = VMAX_X;
    if (tvioHeight > VMAX_Y) tvioHeight = VMAX_Y;
    saveX = firstX;
    saveY = firstY;
    violen = tvioWidth * tvioHeight;

    viomem = new unsigned char[(violen << 1) + violen];
    memset(viomem, 0, (violen << 1) + violen);

    if (terminal->type != TERM_XTERM || transparent)
	twrite("\033(K\033)0");
    if (terminal->type == TERM_XTERM)
	twrite("\033[?1001s\033[?1000h\033]0;BEYE: Binary EYEer\007");
    twrite("\033[0m\033[3h");
    initialized = 1;
}

void __FASTCALL__ __term_vio()
{
    if (!initialized) return;

    __vioSetCursorPos(firstX, firstY);
    __vioSetCursorType(__TVIO_CUR_NORM);

    if (terminal->type == TERM_XTERM) {
	twrite("\033[?1001r\033[?1000l");
	if (transparent) twrite("\033(K");
    }
    twrite("\033[3l\033[0m\033[2J");
    close(out_fd);
    delete vtmp;

    delete viomem;
    nls_term(nls_handle);
    initialized = 0;
}

void __FASTCALL__ __vioSetTransparentColor(unsigned char value)
{
    if (value == 0xff) {
	int i = 0;
	for (i = 0; i < 0x10; i++) tp.color[i] = 0xff;
	tp.last = 0;
    } else if (tp.last < 0x10) {
	tp.color[tp.last] = value;
	tp.last++;
    }
}
