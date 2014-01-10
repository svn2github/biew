
#include "libbeye/libbeye.h"
using namespace	usr;
#include <sstream>

/**
 * @namespace   libbeye
 * @file        libbeye/osdep/linux/vio.c
 * @brief       general implementation of video i/o functions for linux.
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
 * @author      Alexander Krisak and Andrew Golovnia
 * @date        24.07.2003
 * @note        Russian locales support: KOI-8, CP866, CP1251, ISO8859-5.
 *              Tested at ASPLinux 7.3 and ASPLinux 9
 **/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: vio.c,v 1.18 2009/09/03 16:57:40 nickols_k Exp $
*/
#include <iostream>

#ifndef lint
static const char rcs_id[] = "$Id: vio.c,v 1.18 2009/09/03 16:57:40 nickols_k Exp $";
#endif

#define _XOPEN_SOURCE 500

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/vt.h>

#include "system.h"
#include "unix/console.h"
#include "vio_interface.h"
#include "tconsole.h"
#include "libbeye/tvideo_buffer.h"

namespace	usr {
    struct termdesc {
	const char*	name;
	uint8_t		type;
    };

    struct transparent_color {
	uint8_t		last;
	uint8_t		color[0x10];
    };

    class vio_vcsa : public vio_interface {
	public:
	    vio_vcsa(System&,const std::string& user_cp,unsigned long flags);
	    virtual ~vio_vcsa();

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
#if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1)
	    static unsigned		PWRITE(int handle,const any_t* buf,size_t len,__filesize_t offset) { return ::pwrite(handle, buf, len, offset); }
#else
	    static unsigned		PWRITE(int handle,const any_t*  buf,size_t len,__filesize_t offset) { ::lseek(handle, offset, SEEK_SET); ::write(handle, buf, len); }
#endif
	    static unsigned		_bg(unsigned x) { return x>>4;}
	    static unsigned		_fg(unsigned x) { return x&0x0f; }
	    static uint8_t		_2color(unsigned x) { return __ansi_color[x&7]; }
	    static unsigned		LEN(unsigned x) { return x<<4; }

	    unsigned			twrite(const char* x) const { return ::write(out_fd, x, strlen(x)); }
	    uint8_t*			_addr(tAbsCoord x,tAbsCoord y) const { return viomem + (x) + (y) * tvioWidth; }

	    char*			_2ansi(uint8_t attr) const;
	    int				printable(uint8_t c) const;
	    void			gotoxy(tAbsCoord x, tAbsCoord y) const;

	    tAbsCoord			tvioWidth, tvioHeight;
	    unsigned			tvioNumColors;

	    int				console_flags, output_7, transparent;

	    tAbsCoord			saveX, saveY, firstX, firstY;
	    int				cursor_type;
	    int				viohandle, out_fd, output_G1, no_frames;

	    unsigned			violen;
	    uint8_t*			viomem;
	    char*			vtmp;

	    std::string			screen_cp;
	    const termdesc*		terminal;

	    transparent_color		tp;

	    System&			sys;

	    static const unsigned	VMAX_X=__TVIO_MAXSCREENWIDTH;
	    static const unsigned	VMAX_Y=0x400;
	    static const unsigned	VTMP_LEN=0x80;
	    static const char*		VIO_FILE;

	    static const uint8_t	__ansi_color[8];
	    static const termdesc	termtab[];
    };

const char* vio_vcsa::VIO_FILE="/dev/vcsa0";
const uint8_t vio_vcsa::__ansi_color[8] = {0,4,2,6,1,5,3,7};
const termdesc vio_vcsa::termtab[] = {
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

char* vio_vcsa::_2ansi(uint8_t attr) const
{
    int bg = _bg(attr);
    int bc = _2color(bg);

    if (transparent) {
	int i;
	for (i = 0; i < tp.last; i++)
	    if (bg == tp.color[i]) bc = 9;
    }

    sprintf(vtmp,
	"\033[%d;3%d;4%d%sm",
	_fg(attr) > 7,
	_2color(_fg(attr) & 7),
	bc,
	bg > 7 ? ";5" : ""
    );
    return vtmp;
}

int vio_vcsa::printable(uint8_t c) const
{
    int result;
    result = !(c < 0x20 || c == 0x7f || c == 0x9b); /* 0x80< c < 0xA0 */

    if (result && terminal->type == TERM_XTERM)
	result = !(
		c == 0x84 || c == 0x85 || c == 0x88 ||
		(c >= 0x8D && c <= 0x90) ||
		(c >= 0x96 && c <= 0x98) ||
		(c >= 0x9A && c <= 0x9F) );

    return result;
}

void vio_vcsa::gotoxy(tAbsCoord x, tAbsCoord y) const
{
    sprintf(vtmp,"\033[%d;%dH", (unsigned)(y + 1), (unsigned)(x + 1));
    twrite(vtmp);
}

int vio_vcsa::get_cursor_type() const
{
    return cursor_type;
}

int vio_vcsa::set_cursor_type(int type)
{
    cursor_type = type;
    if	(terminal->type == TERM_LINUX && (cursor_type == __TVIO_CUR_SOLID || type == __TVIO_CUR_SOLID)) {
	sprintf(vtmp,"\033[?%dc", type == __TVIO_CUR_SOLID ? 7 : 0);
	twrite(vtmp);
    }
    sprintf(vtmp,"\033[?25%c", type ? 'h' : 'l');
    twrite(vtmp);
    return cursor_type;
}

void vio_vcsa::get_cursor_pos(tAbsCoord& x, tAbsCoord& y) const
{
    x = saveX;
    y = saveY;
}

void vio_vcsa::set_cursor_pos(tAbsCoord x, tAbsCoord y)
{
    saveX = x;
    saveY = y;
    gotoxy(x, y);
}

tvideo_buffer vio_vcsa::read_buffer(tAbsCoord x, tAbsCoord y, size_t len)
{
    uint8_t *addr = _addr(x, y);
    tvideo_buffer rc(len);
    for(size_t i=0;i<len;i++) rc[i]=tvideo_symbol(((t_vchar*)(addr+violen))[i],((t_vchar*)(addr+(violen<<1)))[i],((ColorAttr*)addr)[i]);

    return rc;
}

void vio_vcsa::write_buffer(tAbsCoord x, tAbsCoord y, const tvideo_buffer& buff)
{
    uint8_t cache_pb[LEN(VMAX_X)];
    uint8_t *pb, *addr;
    size_t len = buff.size();

/*    if (!len) return; */

    pb = len > VMAX_X ? new uint8_t [LEN(len)] : cache_pb;

    addr = _addr(x, y);

    for(size_t i=0;i<len;i++) {
	tvideo_symbol s=buff[i];
	addr[i]=s.attr();
	*(addr+violen+i)=s.symbol();
	*(addr+(violen<<1)+i)=s.oempg();
    }

    __INTERLEAVE_BUFFERS(len, pb, addr+violen, addr);

    PWRITE(viohandle, pb, len << 1, 4 + ((x + y * tvioWidth) << 1));

    if (pb != cache_pb) delete pb;
}

vio_vcsa::vio_vcsa(System& s,const std::string& user_cp,unsigned long flags)
	:vio_interface(s,user_cp,flags)
	,tvioNumColors(16)
	,cursor_type(__TVIO_CUR_NORM)
	,sys(s)
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

    struct vt_mode vtmode;
    char *tty = ttyname(STDOUT_FILENO);

    console_flags = flags;

    out_fd = open(tty, O_WRONLY);
    if (out_fd < 0) out_fd = STDOUT_FILENO;

    if (TESTFLAG(console_flags, __TVIO_FLG_DIRECT_CONSOLE_ACCESS))
	if(::ioctl(out_fd, VT_GETMODE, &vtmode) < 0) throw missing_device_exception();

    size_t len = strlen(tty);
    for (i = 0; i < len; i ++) if (isdigit((int)*(tty + i))) break;

    std::ostringstream os;
    if (i < len) os<<"/dev/vcsa"<<(tty + i);

    if ((viohandle = ::open(os.str().c_str(), O_RDWR)) < 0) {
	os.str("/dev/vcsa");
	if ((viohandle = ::open(os.str().c_str(), O_RDWR)) < 0) {
	    std::cerr<<"Can't open "<<os.str()<<": "<<strerror(errno)<<std::endl;
	    std::cerr<<"Direct console access disabled..."<<std::endl;
	    throw missing_device_exception();
	}
    }
    uint8_t b[4];
    read(viohandle, &b, 4);
    tvioHeight = b[0]; tvioWidth = b[1];
    firstX = b[2]; firstY = b[3];

    if (!output_7) output_7 = TESTFLAG(console_flags, __TVIO_FLG_USE_7BIT);
    if (tvioWidth <= 0) tvioWidth = 80;
    if (tvioHeight <= 0) tvioHeight = 25;
    if (tvioWidth > VMAX_X) tvioWidth = VMAX_X;
    if (tvioHeight > VMAX_Y) tvioHeight = VMAX_Y;
    saveX = firstX;
    saveY = firstY;
    violen = tvioWidth * tvioHeight;

    vtmp = new char [VTMP_LEN];
    viomem = new uint8_t[(violen << 1) + violen];

    memset(viomem, 0, (violen << 1) + violen);
    memset(vtmp, 0, VTMP_LEN);

    uint8_t *buf = new uint8_t [violen << 1];

    if (buf != NULL) {
	read(viohandle, buf, violen << 1);
	for (i = 0; i < (violen << 1); i += 2) {
	    viomem[violen + (i >> 1)] = buf[i];
	    viomem[i >> 1] = buf[i + 1];
	}
	delete buf;
    }
}

vio_vcsa::~vio_vcsa()
{
    set_cursor_type(__TVIO_CUR_NORM);
    set_cursor_pos(firstX, firstY);

    ::close(viohandle);

    delete vtmp;
    delete viomem;

    ::close(out_fd);
}

void vio_vcsa::set_transparent_color(uint8_t value)
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

tAbsCoord vio_vcsa::get_width() const { return tvioWidth; }
tAbsCoord vio_vcsa::get_height() const { return tvioHeight; }
unsigned vio_vcsa::get_num_colors() const { return tvioNumColors; }

static vio_interface* query_interface(System& s,const std::string& user_cp,unsigned long flags) { return new(zeromem) vio_vcsa(s,user_cp,flags); }

extern const vio_interface_info vio_vcsa_info = {
    "/dev/vcsa video interface",
    query_interface
};
} // namespace	usr
