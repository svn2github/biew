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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "vio_interface.h"
#include "tconsole.h"
#include "unix/console.h"

namespace	usr {

    struct termdesc {
	const char*	name;
	uint8_t		type;
    };

    struct transparent_color {
	uint8_t		last;
	uint8_t		color[0x10];
    };

    class vio_vt100 : public vio_interface {
	public:
	    vio_vt100(const std::string& user_cp,unsigned long flags);
	    virtual ~vio_vt100();

	    virtual void		set_transparent_color(uint8_t);
	    virtual void		write_buffer(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff);
	    virtual tvideo_buffer	read_buffer(tAbsCoord x,tAbsCoord y,size_t len);
	    virtual void		get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const;
	    virtual void		set_cursor_pos(tAbsCoord x,tAbsCoord y);
	    virtual int			get_cursor_type() const;
	    virtual int			set_cursor_type(int);

	    virtual tAbsCoord		get_width() const;
	    virtual tAbsCoord		get_height() const;
	    virtual unsigned		get_num_colors() const;
	private:
	    static unsigned		_bg(unsigned x) { return x>>4; }
	    static unsigned		_fg(unsigned x) { return x&0x0f; }
	    static size_t		LEN(size_t x) { return x<<4; }

	    uint8_t*			_addr(tAbsCoord x, tAbsCoord y) const { return viomem + ((x) + (y) * tvioWidth); }
	    unsigned			twrite(const char* x) const { return ::write(out_fd,x,strlen(x)); }

	    char*			_2ansi(uint8_t attr) const;
	    int				printable(uint8_t c) const;
	    void			gotoxy(int x, int y) const;

	    int				console_flags;
	    int				output_7;
	    int				transparent;
	    tAbsCoord			saveX, saveY, firstX, firstY;
	    unsigned			violen;
	    uint8_t*			viomem;
	    transparent_color		tp;
	    char*			vtmp;
	    int				out_fd;
#ifdef HAVE_ICONV
	    unsigned			is_unicode;
	    any_t*			nls_handle;
#endif
	    const termdesc*		terminal;

	    int				cursor_status;
	    tAbsCoord			tvioWidth, tvioHeight;
	    unsigned			tvioNumColors;

	    std::string			screen_cp;

	    static const unsigned	VMAX_X=__TVIO_MAXSCREENWIDTH;
	    static const unsigned	VMAX_Y=0x400;
	    static const unsigned	VTMP_LEN=100;

	    static const int		_color[8];
	    static const uint8_t	frames_vt100[0x31];
	    static const uint8_t	frames_dumb[0x31];
	    static const termdesc	termtab[];
    };


#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

const int vio_vt100::_color[8] = {0,4,2,6,1,5,3,7};
const uint8_t vio_vt100::frames_vt100[0x31] = "aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjlaaaaa";
const uint8_t vio_vt100::frames_dumb[0x31] = ": %|{+++++|+++++`++}-++++++++-+++++++++++++#%[]~";

const termdesc vio_vt100::termtab[] = {
    { "linux",		TERM_LINUX },
    { "console",	TERM_LINUX },
    { "xterm",		TERM_XTERM },
    { "xterm-color",	TERM_XTERM},
    { "color-xterm",	TERM_XTERM},
    { "beterm",		TERM_XTERM },
    { "vt100",		TERM_VT100 },
    { "ansi",		TERM_ANSI  },
    { NULL,		TERM_UNKNOWN}
};

/**
    convert vga attrubute to ansi escape sequence
*/
char* vio_vt100::_2ansi(uint8_t attr) const
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

int vio_vt100::printable(uint8_t c) const
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

void vio_vt100::gotoxy(int x, int y) const
{
    sprintf(vtmp,"\033[%d;%dH", y + 1, x + 1);
    twrite(vtmp);
}

int vio_vt100::get_cursor_type() const
{
    return cursor_status;
}

int vio_vt100::set_cursor_type(int type)
{
    cursor_status = type;
    if	(terminal->type == TERM_LINUX &&
	(cursor_status == __TVIO_CUR_SOLID || type == __TVIO_CUR_SOLID)) {
	sprintf(vtmp,"\033[?%dc", type == __TVIO_CUR_SOLID ? 7 : 0);
	twrite(vtmp);
    }
    sprintf(vtmp, "\033[?25%c", type == __TVIO_CUR_OFF ? 'l' : 'h');
    twrite(vtmp);
    return cursor_status;
}

void vio_vt100::get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const
{
    x = saveX;
    y = saveY;
}

void vio_vt100::set_cursor_pos(tAbsCoord x,tAbsCoord y)
{
    saveX = x;
    saveY = y;
    gotoxy(x, y);
}

tvideo_buffer vio_vt100::read_buffer(tAbsCoord x, tAbsCoord y, size_t len)
{
    uint8_t *addr = _addr(x, y);
    tvideo_buffer rc(len);
    for(size_t i=0;i<len;i++) rc[i]=tvideo_symbol(((t_vchar*)(addr+violen))[i],((t_vchar*)(addr+(violen<<1)))[i],((ColorAttr*)addr)[i]);

    return rc;
}

void vio_vt100::write_buffer(tAbsCoord x, tAbsCoord y, const tvideo_buffer& buff)
{
    size_t i;
    uint8_t c;
    uint8_t *addr;
    tAbsCoord xx, yy;
    uint8_t mode = 0, old_mode = -1;
    uint8_t cache_pb[LEN(VMAX_X)];
    size_t len = buff.size();
    uint8_t *dpb,*pb = len > VMAX_X ? new uint8_t [LEN(len)] : cache_pb;
    unsigned slen;

    dpb=pb;

    memset(pb, 0, LEN(len));

/*    if (!len) return; */

    addr = _addr(x, y);
    for(i=0;i<len;i++) {
	tvideo_symbol s=buff[i];
	addr[i]=s.attr;
	*(addr+violen+i)=s.symbol;
	*(addr+(violen<<1)+i)=s.oempg;
    }

    get_cursor_pos(xx, yy);
    gotoxy(x, y);

    for (i = 0; i < len; i++) {
	c = buff[i].symbol;

	if (buff[i].oempg && buff[i].oempg >= _PSMIN && buff[i].oempg <= _PSMAX && !is_unicode) {
		mode = 1;
		c = output_7 ?
		    frames_dumb[buff[i].oempg - _PSMIN] :
		    frames_vt100[buff[i].oempg - _PSMIN];
	} else {
	    mode = 0;
	}

	if (output_7) c &= 0x7f;
	else {
	    const char *map = mode ? "\016" : "\017";
	    if (old_mode != mode)
	    {
		strcpy((char*)dpb,map);
		dpb += strlen(map);
	    }
	    old_mode = mode;
	}
	if (!c) c = ' '; else if (!printable(c)) c = '.';

	/* TODO: make sure that compiler produces right order of conditions! */
	if ((i && buff[i].attr != buff[i-1].attr) || i == len || !i)
	{
	    char *d = _2ansi(buff[i].attr);
	    strcpy((char*)dpb, d);
	    dpb += strlen(d);
	}
	if(!is_unicode) {
	    *dpb=c; dpb++;
	}
	else {
	    unsigned _len=1;
	    char *destb=nls_recode2screen_cp(nls_handle,(const char*)&c,&_len);
	    memcpy(dpb,destb,_len);
	    delete destb;
	    dpb+=_len;
	}
    }
    *dpb=0;
    dpb=pb;
    slen=strlen((const char*)dpb);
    while(slen)
    {
	unsigned stored=twrite((const char*)dpb);
	dpb+=stored;
	slen-= stored;
    }
    gotoxy(xx, yy);
    if (pb != cache_pb) delete pb;
}

vio_vt100::vio_vt100(const std::string& user_cp,unsigned long flags)
	    :vio_interface(user_cp,flags)
	    ,cursor_status(__TVIO_CUR_NORM)
	    ,tvioWidth(80)
	    ,tvioHeight(25)
	    ,tvioNumColors(16)
{
    size_t i;
    const char *t = getenv("TERM");

    for (i = 0; termtab[i].name && strcasecmp(t, termtab[i].name); i++);
    terminal = &termtab[i];

    if (terminal->type == TERM_UNKNOWN) throw std::runtime_error(std::string("Sorry, I can't handle terminal type '")+t);

    if (i == 5) output_7 = 1;	/* beterm is (B)roken (E)vil (TERM)inal */

    if (terminal->type == TERM_XTERM) {
	t = getenv("COLORTERM");
	if (t != NULL && !strcasecmp(t, "Eterm")) transparent = 1;
    }

    struct winsize w;
#ifdef HAVE_ICONV
    screen_cp=nls_get_screen_cp();
    if(::strncasecmp(screen_cp.c_str(),"UTF",3)==0) {
	is_unicode=1;
    }
    nls_handle=nls_init(screen_cp.c_str(),user_cp.c_str());
    if(nls_handle==NULL) is_unicode=0;
#endif
    console_flags = flags;

    if (!output_7) output_7 = TESTFLAG(console_flags, __TVIO_FLG_USE_7BIT);
    do_nls = 1;

    vtmp = new char [VTMP_LEN];
    memset(vtmp, 0, VTMP_LEN);

    out_fd = ::open(ttyname(STDOUT_FILENO), O_WRONLY);
    if (out_fd < 0) out_fd = STDOUT_FILENO;

    ::ioctl(out_fd, TIOCGWINSZ, &w);
    tvioWidth = w.ws_col;
    tvioHeight = w.ws_row;

    if (tvioWidth <= 0) tvioWidth = 80;
    if (tvioHeight <= 0) tvioHeight = 25;
    if (tvioWidth > VMAX_X) tvioWidth = VMAX_X;
    if (tvioHeight > VMAX_Y) tvioHeight = VMAX_Y;
    saveX = firstX;
    saveY = firstY;
    violen = tvioWidth * tvioHeight;

    viomem = new uint8_t[(violen << 1) + violen];
    memset(viomem, 0, (violen << 1) + violen);

    if (terminal->type != TERM_XTERM || transparent)
	twrite("\033(K\033)0");
    if (terminal->type == TERM_XTERM)
	twrite("\033[?1001s\033[?1000h\033]0;BEYE: Binary EYEer\007");
    twrite("\033[0m\033[3h");
}

vio_vt100::~vio_vt100()
{
    set_cursor_pos(firstX, firstY);
    set_cursor_type(__TVIO_CUR_NORM);

    if (terminal->type == TERM_XTERM) {
	twrite("\033[?1001r\033[?1000l");
	if (transparent) twrite("\033(K");
    }
    twrite("\033[3l\033[0m\033[2J");
    ::close(out_fd);
    delete vtmp;

    delete viomem;
    nls_term(nls_handle);
}

void vio_vt100::set_transparent_color(uint8_t value)
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

tAbsCoord vio_vt100::get_width() const { return tvioWidth; }
tAbsCoord vio_vt100::get_height() const { return tvioHeight; }
unsigned vio_vt100::get_num_colors() const { return tvioNumColors; }

static vio_interface* query_interface(const std::string& user_cp,unsigned long flags) { return new(zeromem) vio_vt100(user_cp,flags); }

extern const vio_interface_info vio_vt100_info = {
    "vt100 video interface",
    query_interface
};
} // namespace	usr