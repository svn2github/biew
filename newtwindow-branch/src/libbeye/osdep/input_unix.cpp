#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include "libbeye/osdep/__os_dep.h"
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/unix/keyboard.c
 * @brief       slang/curses/vt100 keyboard library
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

    $Id: keyboard.c,v 1.9 2009/09/20 13:43:37 nickols_k Exp $
*/
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef lint
static const char rcs_id[] = "$Id: keyboard.c,v 1.9 2009/09/20 13:43:37 nickols_k Exp $";
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>

#include "system.h"
#include "input_interface.h"
#include "libbeye/kbd_code.h"
#include "unix/console.h"

#ifdef HAVE_MOUSE
#include <gpm.h>
#endif

namespace	usr {
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
    struct seqtbl {
	char c;
	int key;
    };

    typedef seqtbl pseq[];
    typedef seqtbl p1seq[1]; /**< dummy type to make compiler happy */

    struct eseq {
	char pre1;
	char pre2;
	char suf;
	p1seq *s;
    };

    /*
	keyboard FIFO
    */
    static const unsigned	KBUFSIZE=64;
    struct kbd_fifo {
	int		pool[KBUFSIZE];
	unsigned	current;
    };

    class input_unix : public input_interface {
	public:
	    input_unix(System&,const std::string& user_cp);
	    virtual ~input_unix();

	    virtual int			get_key( unsigned long flg);
	    virtual int			test_key( unsigned long flg );
	    virtual int			get_shifts();
	    virtual int			raw_info(char *head, char *text);

	    virtual bool		ms_get_state() const;
	    virtual void		ms_set_state(bool is_visible);
	    virtual void		ms_get_pos(tAbsCoord& x, tAbsCoord& y) const;
	    virtual int			ms_get_btns();

	    static void			__ReadNextEvent(int);
	private:
	    int				get(unsigned char* x) const { return ::read(in_fd,x,1); }
	    void			pushEvent(unsigned _event);
	    void			ReadNextEvent();

	    char			rawkb_buf[100];
	    unsigned			rawkb_len; /* length of rawkb_buf*/
	    unsigned			rawkb_mode;
	    int				rawkb_escape;

#ifdef HAVE_ICONV
	    any_t*			nls_handle;
	    int				is_unicode;
#endif

	    unsigned			rawkb_size; /* size of rawkb_buf elements 1,2 or 4*/
	    int				rawkb_method;

	    int				in_fd;
	    struct termios		sattr;
	    kbd_fifo			keybuf;

#ifdef HAVE_MOUSE
	    int				gpmhandle;

	    mevent			mouse;

	    int				shift_status;		/**< status of shift keys */
#endif
	    bool			mouse_status;	/**< mouse state */

	    System&			sys;

	    static const unsigned	SEQ_LEN=10;	/**< max sequense length */
	    static const unsigned	SEQ_NUM=9;	/**< number of sequence categories */

	    static const pseq		seq0,seq1,seq2,seq3,seq4,seq5,seq6,seq7,seq8;
	    static const eseq		S[SEQ_NUM];
    };

/**
    translatable sequences
*/

const pseq input_unix::seq0 = {
    {'A',KE_UPARROW},
    {'B',KE_DOWNARROW},
    {'C',KE_RIGHTARROW},
    {'D',KE_LEFTARROW},
    {'P',KE_F(1)},
    {'Q',KE_F(2)},
    {'R',KE_F(3)},
    {'w',KE_F(3)},
    {'y',KE_F(3)},
    {'S',KE_F(4)},
    {'x',KE_F(4)},
    {'t',KE_F(5)},
    {'v',KE_F(5)},
    {'u',KE_F(6)},
    {'l',KE_F(6)},
    {'q',KE_F(7)},
    {'s',KE_F(7)},
    {'r',KE_F(8)},
    {'p',KE_F(9)},
    {'n',KE_F(9)},
    {0,0}},
input_unix::seq1 = {
    {'A',KE_UPARROW},
    {'B',KE_DOWNARROW},
    {'C',KE_RIGHTARROW},
    {'D',KE_LEFTARROW},
    {'H',KE_HOME},
    {'K',KE_END},
    {0,0}},
input_unix::seq2 = {
    {'A',KE_F(1)},
    {'B',KE_F(2)},
    {'C',KE_F(3)},
    {'D',KE_F(4)},
    {'E',KE_F(5)},
    {0,0}},
input_unix::seq3 = {
    {'1',KE_HOME},
    {'2',KE_INS},
    {'3',KE_DEL},
    {'4',KE_END},
    {'5',KE_PGUP},
    {'6',KE_PGDN},
    {'7',KE_HOME},
    {'8',KE_END},
    {'J',KE_CTL_PGDN},
    {0,0}},
input_unix::seq4 = {
    {'1',KE_F(1)},
    {'2',KE_F(2)},
    {'3',KE_F(3)},
    {'4',KE_F(4)},
    {'5',KE_F(5)},
    {'7',KE_F(6)},
    {'8',KE_F(7)},
    {'9',KE_F(8)},
    {0,0}},
input_unix::seq5 = {
    {'0',KE_F(9)},
    {'1',KE_F(10)},
    {'3',KE_SHIFT_F(1)},
    {'4',KE_SHIFT_F(2)},
    {'5',KE_SHIFT_F(3)},
    {'6',KE_SHIFT_F(4)},
    {'8',KE_SHIFT_F(5)},
    {'9',KE_SHIFT_F(6)},
    {0,0}},
input_unix::seq6 = {
    {'1',KE_SHIFT_F(7)},
    {'2',KE_SHIFT_F(8)},
    {'3',KE_SHIFT_F(9)},
    {'4',KE_SHIFT_F(10)},
    {0,0}},
input_unix::seq7 = {
    {'1',KE_CTL_F(1)},
    {'2',KE_CTL_F(2)},
    {'3',KE_CTL_F(3)},
    {'4',KE_CTL_F(4)},
    {'5',KE_CTL_F(5)},
    {'7',KE_CTL_F(6)},
    {'8',KE_CTL_F(7)},
    {'9',KE_CTL_F(8)},
    {0,0}},
input_unix::seq8 = {
    {'0',KE_CTL_F(10)},
    {'1',KE_CTL_F(11)},
    {0,0}};

const eseq input_unix::S[SEQ_NUM] = {
{'O', 0, 0, (p1seq *)seq0 },
{'[', 0, 0, (p1seq *)seq1 },
{'[', '[', 0, (p1seq *)seq2 },
{'[', 0, '~', (p1seq *)seq3 },
{'[', '1', '~', (p1seq *)seq4 },
{'[', '2', '~', (p1seq *)seq5 },
{'[', '3', '~',	(p1seq *)seq6 },
{'[', '1', '^', (p1seq *)seq7 },
{'[', '2', '^', (p1seq *)seq8 }
};

/*
    mouse event
*/

void input_unix::pushEvent(unsigned _event)
{
    unsigned event=_event;
#if defined (HAVE_ICONV)
    if(is_unicode) {
	static unsigned char utf_buff[8];
	static unsigned utf_ptr=0;
	char *destb;
	int err;
	unsigned char *eptr;
	unsigned i,len;
	if((event&(~0xFF))==0) {
	    utf_buff[utf_ptr]=event;
	    len=utf_ptr+1;
	    if((err=nls_test(nls_handle,(const char*)utf_buff,&len))!=0) {
		utf_ptr++;
		return;
	    }
	    len=utf_ptr+1;
	    destb=nls_recode2screen_cp(nls_handle,(const char*)utf_buff,&len);
	    event=0;
	    eptr=(unsigned char *)&event;
	    for(i=0;i<std::min(sizeof(unsigned),size_t(len));i++) {
		event<<=8;
		eptr[0]=destb[i];
	    }
	    utf_ptr=0;
	    delete destb;
	}
    }
#endif
    if (event) {
	if (keybuf.current < KBUFSIZE) {
	    if (keybuf.current)
		memmove(keybuf.pool, &keybuf.pool[1], keybuf.current);
	    keybuf.pool[0] = event;
	    keybuf.current++;
	}
    } else {
	break_status = true;
    }
}

#define ret(x)	pushEvent(x); return;
#define set_s(x) shift_status &= (x); shift_status ^= (x); ret(KE_SHIFTKEYS);

/**
    ReadNextEvent is non-blocking call
*/
void input_unix::ReadNextEvent()
{
    unsigned key = 0;
    size_t i;
    unsigned char c[SEQ_LEN];

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

    if (get(&c[0]) < 0) return;

    switch(c[0]) {
	case KE_ESCAPE		: break;
	case KE_STATUS_RESET	: if(!rawkb_mode) { set_s(0); } else { rawkb_mode=0; return; }
	case KE_STATUS_ALT	: if(!rawkb_mode) { set_s(KS_ALT); } else { rawkb_mode=0; return; }
	case KE_STATUS_SHIFT	: if(!rawkb_mode) { set_s(KS_SHIFT); } else { rawkb_mode=0; return; }
	case KE_STATUS_CONTROL	: if(!rawkb_mode) { set_s(KS_CTRL); } else { rawkb_mode=0; return; }
	case KE_ENTER2		: key = KE_ENTER; break;
	case KE_BKSPACE2	: key = KE_BKSPACE; break;
	case KE_C_O		: key = KE_CTL_(O); break;
	case 0			: if(!rawkb_mode) { ret(0); } else { rawkb_mode=0; return; }
	default			: key = c[0];
    }
    if (key)
    {
	if(rawkb_mode)
	{
	    rawkb_buf[0]=c[0];
	    rawkb_len=1;
	}
	goto place_key;
    }
    for (i = 1; i < SEQ_LEN - 1; i++) if(get(&c[i]) < 0) break;
    if(rawkb_mode)
    {
	memcpy(rawkb_buf,c,i);
	rawkb_len=i;
    }

    if (i < 3) {
	key = c[0];
	goto place_key;
    }


/*
    track mouse
*/

    if (c[1] == '[' && c[2] == 'M' && i == 6) {
	switch (c[3] & 0x03) {
	    case 0: mouse.buttons |= MS_LEFTPRESS; break;
	    case 1: mouse.buttons |= MS_MIDDLEPRESS; break;
	    case 2: mouse.buttons |= MS_RIGHTPRESS; break;
	    default: mouse.pressed = 0; return;
	}
	mouse.x = c[4] - '!';
	mouse.y = c[5] - '!';
	mouse.pressed = 1;
	key = KE_MOUSE;
	goto place_key;
    }

/*
    translate escape sequence
*/
    for (i = 0; i < SEQ_NUM && !key; i++) {
	int j, n;

	if (c[1] != S[i].pre1) continue;
	n = 2;
	if (S[i].pre2) {
	    if (c[n] != S[i].pre2) continue;
	    n++;
	}
	for (j = 0; S[i].s[j]->c; j++) if (S[i].s[j]->c == c[n]) {
	    if (!(S[i].suf && S[i].suf != c[n + 1])) {
		key = S[i].s[j]->key; break;
	    }
	}
    }

place_key:
    if (key)
    {
	if(!rawkb_mode) { ret(key); }
	else { rawkb_escape=(key==KE_ESCAPE&&rawkb_len==1); rawkb_mode=0; }
    }
}
#undef set_s
#undef ret

static input_unix* handle;
void input_unix::__ReadNextEvent(int) { handle->ReadNextEvent(); }

int input_unix::get_shifts()
{
    return shift_status;
}

int input_unix::test_key(unsigned long flg)
{
    if(ms_get_btns() && flg == KBD_NONSTOP_ON_MOUSE_PRESS) return KE_MOUSE;
    return keybuf.current;
}

int input_unix::get_key(unsigned long flg)
{
    int key = 0, s = 0;

    if (test_key(flg) == KE_MOUSE) return KE_MOUSE;

    while (!keybuf.current) { sys.yield_timeslice(); ReadNextEvent(); }
    key = keybuf.pool[--keybuf.current];
    if (!(key == KE_MOUSE || key == KE_SHIFTKEYS)) {
	if ((shift_status & KS_ALT) == KS_ALT)		s |= ADD_ALT;
	if ((shift_status & KS_SHIFT) == KS_SHIFT)	s |= ADD_SHIFT;
	if ((shift_status & KS_CTRL) == KS_CTRL) {
	    s |= ADD_CONTROL;
	    if (key == 'o' || key == 'O') key = KE_CTL_(O);    /* CTRL+O */
	}
	shift_status = 0;
    }
    return key | s;
}

int input_unix::raw_info(char *head, char *text)
{
    unsigned i;
    char appends[20];
    strcpy(head,"ModeName Value");
    rawkb_escape=0;
    rawkb_len=0;
    rawkb_mode=1;
    while(rawkb_mode) { usleep(0); if(!rawkb_method) ReadNextEvent(); }
    strcpy(text,"VT100");
    for(i=strlen(text);i<9;i++) strcat(text," ");
    for(i=0;i<rawkb_len;i++)
    {
	if(isprint(rawkb_buf[i])&&rawkb_size==1) { appends[0]=rawkb_buf[i]; appends[1]=0; }
	else sprintf(appends,"\\0%o ",(int)(rawkb_size==4?(int)rawkb_buf[i]:rawkb_size==2?(short)rawkb_buf[i]:(char)rawkb_buf[i]));
	strcat(text,appends);
    }
    return rawkb_escape?0:1;
}

/*

*/

bool input_unix::ms_get_state() const
{
    return mouse_status;
}

void input_unix::ms_set_state(bool ms_visible)
{
    mouse_status = ms_visible;
}

void input_unix::ms_get_pos(tAbsCoord& x, tAbsCoord& y) const
{
    x = mouse.x;
    y = mouse.y;
}

int input_unix::ms_get_btns()
{
#ifdef HAVE_MOUSE
    ReadNextEvent();
#endif
    return mouse.pressed ? mouse.buttons : 0;
}


input_unix::input_unix(System& s,const std::string& user_cp)
		:input_interface(s,user_cp)
		,rawkb_size(sizeof(char))
		,rawkb_method(1)
#ifdef HAVE_MOUSE
		,mouse_status(true)
#else
		,mouse_status(false)
#endif
		,sys(s)
{
    struct termios tattr;

#ifdef	__ENABLE_SIGIO
#define _MODE_ O_NONBLOCK | O_ASYNC
#else
#define _MODE_ O_NONBLOCK
#endif

#ifdef HAVE_ICONV
    {
    const char *screen_cp;
	screen_cp=nls_get_screen_cp();
	if(strncasecmp(screen_cp,"UTF",3)==0) {
	    is_unicode=1;
	}
	nls_handle=nls_init(user_cp.c_str(),screen_cp);
	if(nls_handle==NULL) is_unicode=0;
    }
#endif

    in_fd = ::open(ttyname(STDIN_FILENO), O_RDONLY);
    if (in_fd < 0) in_fd = STDIN_FILENO;

    if (::fcntl(in_fd, F_SETFL, ::fcntl(in_fd, F_GETFL) | _MODE_) < 0) throw missing_device_exception();

    tcgetattr(in_fd, &tattr);
    sattr = tattr;
    tattr.c_lflag &= ~(ICANON | ECHO | ISIG);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tattr.c_iflag &= ~(INPCK | ISTRIP | IXON);
    tattr.c_oflag |= OPOST | ONLCR;
    tcsetattr(in_fd, TCSANOW, &tattr);

#ifdef	HAVE_MOUSE
    {
	Gpm_Connect gc = { ~0, GPM_MOVE|GPM_HARD, 0, 0};
	gpmhandle = Gpm_Open(&gc, 0);
	if (gpmhandle < 0) gpmhandle = 0;
    }
#endif

    keybuf.current = 0;

#ifdef	__ENABLE_SIGIO
    /* everything is ready, start to receive SIGIO */
    handle=this;
    ::signal(SIGIO, &input_unix::__ReadNextEvent);
#endif

}

input_unix::~input_unix()
{
#ifdef HAVE_ICONV
    nls_term(nls_handle);
#endif
    tcsetattr(in_fd, TCSANOW, &sattr);
    ::close(in_fd);
#ifdef	HAVE_MOUSE
    if (gpmhandle) Gpm_Close();
#endif
}

static input_interface* query_interface(System& s,const std::string& user_cp) { return new(zeromem) input_unix(s,user_cp); }

extern const input_interface_info input_unix_info = {
    "unix input",
    query_interface
};
} // namespace	usr