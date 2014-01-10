#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/linux/keyboard.c
 * @brief       Linux direct console / vt100 keyboard library
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
    Copyright (C) 1999-2001 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: keyboard.c,v 1.14 2009/09/24 09:12:13 nickols_k Exp $
*/
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef lint
static const char rcs_id[] = "$Id: keyboard.c,v 1.14 2009/09/24 09:12:13 nickols_k Exp $";
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/vt.h>

#include "input_interface.h"
#include "kbd_code.h"
#include "unix/console.h"
#include "system.h"

#ifdef HAVE_MOUSE
#include <gpm.h>
#endif

namespace	usr {
#define KBUFSIZE	64	/**< size of keyboard buffer */

#define KSCAN_SHIFTL	0x2a
#define KSCAN_SHIFTR	0x36
#define KSCAN_CTRL	0x1d
#define KSCAN_ALT	0x38
#define KSCAN_CAPS	0x3a
#define KSCAN_NUM	0x45
#define KSCAN_SCROLL	0x46
#define KE_NONE		0xffffffff
    struct kbd_fifo {
	int		pool[KBUFSIZE];
	unsigned	current;
    };
    class input_linux : public input_interface {
	public:
	    input_linux(System& s,const std::string& user_cp);
	    virtual ~input_linux();

	    virtual bool		get_cbreak() const;
	    virtual void		set_cbreak( bool state );

	    virtual int			get_key( unsigned long flg);
	    virtual int			test_key( unsigned long flg );
	    virtual int			get_shifts();
	    virtual int			raw_info(char *head, char *text);

	    virtual bool		ms_get_state() const;
	    virtual void		ms_set_state(bool is_visible);
	    virtual void		ms_get_pos(tAbsCoord& x, tAbsCoord& y) const;
	    virtual int			ms_get_btns();

	    static void			__console_leave(int signum);
	    static void			__console_enter(int signum);
	    static void			__ReadNextEvent(int signum);
	    static void			dummy_handler (int signum);
	private:
	    int				get(unsigned char* x) const { return ::read(in_fd,x,1); }
	    void			pushEvent(unsigned _event);
	    void			ReadNextEvent();
	    void			console_leave() const;
	    void			console_enter();

	    static const unsigned	KEYNUM=128;
	    static const unsigned	KSCANSIZE=84; /**< size of scancode table */

	    volatile int	shift_status;	/**< status of shift keys */
	    volatile int	keypressed[KEYNUM];	/**< indicates whether key is down */
	    volatile int	console_restart;
	    bool		mouse_status;
	    struct termios	sattr;
	    struct termios	tattr;	/**< terminal attributes */
	    int			in_fd;

	    kbd_fifo		keybuf;

#ifdef HAVE_MOUSE
	    int			gpmhandle;
#endif

	    char		rawkb_buf[KBUFSIZE];
	    unsigned		rawkb_len; /* size of rawkb_buf*/
	    unsigned		rawkb_mode;
	    int			rawkb_escape;

	    mevent		mouse;

	    System&		sys;

	    bool			break_status;	/**< CTRL+BREAK flag */

	    static const unsigned	scancode_table[KSCANSIZE];
	    static const unsigned	scancode_caps_table[KSCANSIZE];
    };

/**
    scancode tables
*/

const unsigned input_linux::scancode_table[KSCANSIZE] =
{
    KE_ESCAPE,	'1',	'2',	'3',	'4',	'5',	'6',	'7',
    '8',	'9',	'0',	'-',	'=',	KE_BKSPACE,KE_TAB,'q',
    'w',	'e',	'r',	't',	'y',	'u',	'i',	'o',
    'p',	'[',	']',	KE_ENTER,KE_NONE,'a',	's',	'd',
    'f',	'g',	'h',	'j',	'k',	'l',	';',	0x27,
    '`',	KE_NONE,0x5c,	'z',	'x',	'c',	'v',	'b',
    'n',	'm',	',',	'.',	'/',	KE_NONE,'*',	KE_NONE,
    ' ',	KE_NONE,KE_F(1),KE_F(2),KE_F(3),KE_F(4),KE_F(5),KE_F(6),
    KE_F(7),	KE_F(8),KE_F(9),KE_F(10),KE_NONE,KE_NONE,KE_HOME,KE_UPARROW,
    KE_PGUP,	'-',	KE_LEFTARROW,0,KE_RIGHTARROW,'+',KE_END,KE_DOWNARROW,
    KE_PGDN,	KE_INS,	KE_DEL,KE_NONE
};

const unsigned input_linux::scancode_caps_table[KSCANSIZE] =
{
    KE_ESCAPE,	'!',	'@',	'#',	'$',	'%',	'^',	'&',
    '*',	'(',	')',	'_',	'+',	KE_BKSPACE,KE_TAB,'Q',
    'W',	'E',	'R',	'T',	'Y',	'U',	'I',	'O',
    'P',	'{',	'}',	KE_ENTER,0,	'A',	'S',	'D',
    'F',	'G',	'H',	'J',	'K',	'L',	':',	0x22,
    '~',	KE_NONE,'|',	'Z',	'X',	'C',	'V',	'B',
    'N',	'M',	'<',	'>',	'?',	KE_NONE,'*',	KE_NONE,
    ' ',	0,	KE_F(1),KE_F(2),KE_F(3),KE_F(4),KE_F(5),KE_F(6),
    KE_F(7),	KE_F(8),	KE_F(9),	KE_F(10),	KE_NONE,	KE_NONE,	KE_HOME,KE_UPARROW,
    KE_PGUP,	'-',	KE_LEFTARROW,0,KE_RIGHTARROW,'+',KE_END,KE_DOWNARROW,
    KE_PGDN,	KE_INS,	KE_DEL,	KE_NONE
};

bool input_linux::get_cbreak() const
{
#ifndef	__ENABLE_SIGIO
    ReadNextEvent();
#endif
    return break_status;
}

void input_linux::set_cbreak(bool state)
{
    break_status = state;
}

void input_linux::pushEvent(unsigned _event)
{
    unsigned event=_event;
    if (event && keybuf.current < KBUFSIZE) {
	if (keybuf.current)
	    memmove(keybuf.pool, &keybuf.pool[1], keybuf.current);
	keybuf.pool[0] = event;
	keybuf.current++;
    }
}

void input_linux::console_leave() const
{
    ioctl(in_fd, TCSETSW, &sattr);
    ioctl(in_fd, KDSKBMODE, K_XLATE);
    ioctl(in_fd, VT_RELDISP, VT_ACKACQ);
    signal(SIGUSR1, &input_linux::__console_leave);
}

void input_linux::console_enter()
{
    ioctl(in_fd, KDSKBMODE, K_RAW);
    ioctl(in_fd, TCSETSW, &tattr);
    memset((any_t*)&keypressed, 0, KEYNUM * sizeof(int));
    console_restart = 1;
    signal(SIGUSR2, &input_linux::__console_enter);
}

static input_linux* handle;
void input_linux::__console_leave(int) { handle->console_leave(); }
void input_linux::__console_enter(int) { handle->console_enter(); }
void input_linux::__ReadNextEvent(int)  { handle->ReadNextEvent(); }
void input_linux::dummy_handler (int) {}

/**
    ReadNextEvent is non-blocking
*/

void input_linux::ReadNextEvent()
{
#define ret(x)	pushEvent((x)); return;
#define set_s(x) shift_status &= (x); shift_status ^= (x); ret(KE_SHIFTKEYS);

    unsigned key = 0;
    size_t i;

#ifdef HAVE_MOUSE
    if (gpmhandle) {
	fd_set gpmfds;
	struct timeval t = { 0, 0 };

	FD_ZERO(&gpmfds);
	FD_SET(gpmhandle, &gpmfds);

	if (select(gpmhandle + 1, &gpmfds, NULL, NULL, &t)) {
	    Gpm_Event ge;
	    Gpm_GetEvent(&ge);
	    if (ge.type & GPM_DOWN) {
		if (ge.buttons & GPM_B_LEFT)
		    mouse.buttons |= MS_LEFTPRESS;
		if (ge.buttons & GPM_B_MIDDLE)
		    mouse.buttons |= MS_MIDDLEPRESS;
		if (ge.buttons & GPM_B_RIGHT)
		    mouse.buttons |= MS_RIGHTPRESS;
		mouse.x = ge.x - 1;
		mouse.y = ge.y - 1;
		mouse.pressed = 1;
		if (mouse.buttons) ret(KE_MOUSE);
	    } else if (ge.type & GPM_UP) mouse.pressed = 0;
	}
    }
#endif

    /* If key is ready, place it into keyboard buffer */

    unsigned char c = 0;
    int ss = shift_status;

    do { get(&c); } while (c == 0xe0);
    if (c == 0xe1) get(&c);
    if (c == 0xC6) { break_status = true; if(rawkb_mode) { rawkb_buf[0]=c; rawkb_len=1; rawkb_mode=0; } return; } /* CTRL+BREAK check */
    if (c) {
	if(rawkb_mode) { rawkb_buf[0]=c; rawkb_len=1; }
	keypressed[c & (KEYNUM - 1)] = c & KEYNUM ? 0 : 1;
	for (i = 1; i < KSCANSIZE; i++)
	    if (keypressed[i] &&
	    (i != KSCAN_SHIFTL) &&
	    (i != KSCAN_SHIFTR) &&
	    (i != KSCAN_CTRL) &&
	    (i != KSCAN_ALT) &&
	    (i != KSCAN_CAPS) &&
	    (i != KSCAN_NUM) &&
	    (i != KSCAN_SCROLL))
	    key = scancode_table[i - 1];
    }
    if (get_shifts() != ss) key = KE_SHIFTKEYS;
    if ((shift_status & KS_SHIFT) == KS_SHIFT) {
	for (i = 1; i < KSCANSIZE; i++)
	    if (keypressed[i] &&
	    (i != KSCAN_SHIFTL) &&
	    (i != KSCAN_SHIFTR) &&
	    (i != KSCAN_CTRL) &&
	    (i != KSCAN_ALT) &&
	    (i != KSCAN_CAPS) &&
	    (i != KSCAN_NUM) &&
	    (i != KSCAN_SCROLL))
	    key = scancode_caps_table[i - 1];
    }
    if (((shift_status & KS_ALT) == KS_ALT) &&
	((shift_status & KS_CTRL) == KS_CTRL) &&
	(key >= KE_F(1) && key <= KE_F(10))) {

	struct vt_stat vt;
	int newvt;

	if (ioctl(in_fd, VT_GETSTATE, &vt) != 0) goto c_end;
	switch (key) {
	    case KE_F(1): newvt = 1; break;
	    case KE_F(2): newvt = 2; break;
	    case KE_F(3): newvt = 3; break;
	    case KE_F(4): newvt = 4; break;
	    case KE_F(5): newvt = 5; break;
	    case KE_F(6): newvt = 6; break;
	    case KE_F(7): newvt = 7; break;
	    default: goto c_end;
	}

	if (vt.v_active != newvt) {
	    if (ioctl(in_fd, VT_ACTIVATE, newvt) != 0) goto c_end;
	    console_restart = 0;
	    while (!console_restart) sys.yield_timeslice();
	}
    }
c_end:
    if(!rawkb_mode) { ret(key); }
    else { rawkb_escape=(key==KE_ESCAPE&&rawkb_len==1); rawkb_mode=0; }
#undef set_s
#undef ret
}

int input_linux::get_shifts()
{
    /* shift_status = 6;ioctl(in_fd,TIOCLINUX,&shift_status); */
     shift_status = 0;
     if (keypressed[KSCAN_SHIFTL] || keypressed[KSCAN_SHIFTR]) shift_status |= KS_SHIFT;
     if (keypressed[KSCAN_CTRL])	shift_status |= KS_CTRL;
     if (keypressed[KSCAN_ALT])	shift_status |= KS_ALT;
     if (keypressed[KSCAN_CAPS])	shift_status |= KS_CAPSLOCK;
     if (keypressed[KSCAN_NUM])	shift_status |= KS_NUMLOCK;
     if (keypressed[KSCAN_SCROLL])	shift_status |= KS_SCRLOCK;
    return shift_status;
}

int input_linux::test_key(unsigned long flg)
{
    if(ms_get_btns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;
    return keybuf.current;
}

int input_linux::get_key(unsigned long flg)
{
    int key = 0, s = 0;

    if (ms_get_btns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;

    while (!keybuf.current) {
	sys.yield_timeslice();
#ifdef	HAVE_MOUSE
	if (gpmhandle) ReadNextEvent();
#endif
    }
    key = keybuf.pool[--keybuf.current];

    if (!(key == KE_MOUSE || key == KE_SHIFTKEYS)) {
	if ((shift_status & KS_ALT) == KS_ALT)		s |= ADD_ALT;
	if ((shift_status & KS_SHIFT) == KS_SHIFT)	s |= ADD_SHIFT;
	if ((shift_status & KS_CTRL) == KS_CTRL) {
	    s |= ADD_CONTROL;
	    if (key == 'o' || key == 'O') key = KE_CTL_(O);    /* CTRL+O */
	}
    }

    return key | s;
}

bool input_linux::ms_get_state() const
{
    return mouse_status;
}

void input_linux::ms_set_state(bool ms_visible)
{
    mouse_status = ms_visible;
}

void input_linux::ms_get_pos(tAbsCoord& x, tAbsCoord& y) const
{
    x = mouse.x;
    y = mouse.y;
}

int input_linux::ms_get_btns()
{
#ifdef	HAVE_MOUSE
    if (gpmhandle) ReadNextEvent();
#endif
    return mouse.pressed ? mouse.buttons : 0;
}

int input_linux::raw_info(char *head, char *text)
{
    unsigned i;
    char appends[10];
    strcpy(head,"Name  Value");
    strcpy(text,"Raw   ");
    rawkb_escape=0;
    rawkb_len=0;
    rawkb_mode=1;
    while(rawkb_mode) usleep(0);
    for(i=0;i<rawkb_len;i++)
    {
	if(isprint(rawkb_buf[i])) { appends[0]=rawkb_buf[i]; appends[1]=0; }
	else sprintf(appends,"\\0%o ",rawkb_buf[i]);
	strcat(text,appends);
    }
    return rawkb_escape?0:1;
}

input_linux::input_linux(System& s,const std::string& user_cp)
	    :input_interface(s,user_cp)
	    ,mouse_status(true)
	    ,sys(s)
{
    signal(SIGIO, &input_linux::dummy_handler);
    in_fd = ::open(ttyname(STDIN_FILENO), O_RDONLY);
    if (in_fd < 0) in_fd = STDIN_FILENO;

#define _MODE_ O_NONBLOCK | O_ASYNC

    if (::fcntl(in_fd, F_SETFL, fcntl(in_fd, F_GETFL) | _MODE_) < 0) throw missing_device_exception();

    tcgetattr(in_fd, &tattr);
    sattr = tattr;
    tattr.c_lflag &= ~(ICANON | ECHO | ISIG);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tattr.c_iflag &= ~(INPCK | ISTRIP | IXON | ICRNL);
    tattr.c_oflag |= OPOST | ONLCR;
    tcsetattr(in_fd, TCSANOW, &tattr);

    if (::ioctl(in_fd, KDSKBMODE, K_RAW) < 0) throw missing_device_exception();

    struct vt_mode vt;

    handle=this;
    ::signal(SIGUSR1, &input_linux::__console_leave);
    ::signal(SIGUSR2, &input_linux::__console_enter);
    ::ioctl(in_fd, VT_GETMODE, &vt);
    vt.mode = VT_PROCESS;
    vt.relsig = SIGUSR1;
    vt.acqsig = SIGUSR2;
    ::ioctl(in_fd, VT_SETMODE, &vt);

#ifdef	HAVE_MOUSE
    {
	Gpm_Connect gc;

	gc.eventMask = ~0;
	gc.defaultMask = GPM_MOVE|GPM_HARD;
	gc.minMod = gc.maxMod = 0;
	gpmhandle = Gpm_Open(&gc, 0);
	if (gpmhandle < 0) gpmhandle = 0;
    }
#endif

    keybuf.current = 0;

    ::signal(SIGIO, &input_linux::__ReadNextEvent);
}

input_linux::~input_linux()
{
    ::ioctl(in_fd, KDSKBMODE, K_XLATE);
    tcsetattr(in_fd, TCSANOW, &sattr);
    ::close(in_fd);
#ifdef	HAVE_MOUSE
    if (gpmhandle) Gpm_Close();
#endif
}

static input_interface* query_interface(System& s,const std::string& user_cp) { return new(zeromem) input_linux(s,user_cp); }

extern const input_interface_info input_linux_info = {
    "linux input",
    query_interface
};
} // namespace	usr
