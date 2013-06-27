#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include "libbeye/osdep/__os_dep.h"
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/qnxnto/keyboard.c
 * @brief       This file contains implementation of keyboard handles for QNX6.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2003
 * @note        Development, fixes and improvements
 * @note        Big thanks to Mike Gorchak for tvision-1.0.10-1/src.
**/

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <Ph.h>

#include "libbeye/kbd_code.h"

#define DEV_SCRLOCK     _LINESTATUS_CON_SCROLL
#define DEV_NUMLOCK     _LINESTATUS_CON_NUM
#define DEV_CAPSLOCK    _LINESTATUS_CON_CAPS
#define DEV_SHIFT       _LINESTATUS_CON_SHIFT
#define DEV_CTRL        _LINESTATUS_CON_CTRL
#define DEV_ALT         _LINESTATUS_CON_ALT

#define PHK_SHIFT       Pk_KM_Shift
#define PHK_CTRL        Pk_KM_Ctrl
#define PHK_ALT         Pk_KM_Alt
#define PHK_CAPSLOCK    Pk_KM_Caps_Lock
#define PHK_NUMLOCK     Pk_KM_Num_Lock
#define PHK_SCRLOCK     Pk_KM_Scroll_Lock

int _shift_state=0;
int _old_dev_state;
extern int _mouse_buttons;
extern int photon,console;

extern int ph_ig;
extern struct _Ph_ctrl *(*p_PhAttach)(char const*,PhChannelParms_t const*);
#define so_PhAttach(a,b) (*p_PhAttach)(a,b)
extern int (*p_PhInputGroup)(PhEvent_t const*);
#define so_PhInputGroup(a) (*p_PhInputGroup)(a)
extern int (*p_PhQueryCursor)(unsigned short,PhCursorInfo_t*);
#define so_PhQueryCursor(a,b) (*p_PhQueryCursor)(a,b)

int __FASTCALL__ getms();

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
	__init_mouse();
	_shift_state=0;
	raw();
	intrflush(stdscr,FALSE);
	meta(stdscr,TRUE);
	keypad(stdscr,TRUE);
	nodelay(stdscr,TRUE);
	return;
}

void __FASTCALL__ __term_keyboard()
{
	__term_mouse();
	keypad(stdscr,FALSE);
	nodelay(stdscr,FALSE);
}

int __FASTCALL__ __kbdGetShiftsKey()
{
	long dbuf;
	long rbuf;
	int ss=0;
	if(console)
	{
		if(devctl(fileno(stdin),DCMD_CHR_LINESTATUS,&rbuf,sizeof(long),&dbuf))
			return _shift_state;
		if(rbuf&DEV_ALT) ss|=KS_ALT;
		if(rbuf&DEV_CTRL) ss|=KS_CTRL;
		if(rbuf&DEV_SHIFT) ss|=KS_SHIFT;
		if(rbuf&DEV_SCRLOCK) ss|=KS_SCRLOCK;
		if(rbuf&DEV_CAPSLOCK) ss|=KS_CAPSLOCK;
		if(rbuf&DEV_NUMLOCK) ss|=KS_NUMLOCK;
		return ss;
	}
	else if(photon/*&&ph_ig!=0*/)
		{
			static PhCursorInfo_t buf;
			so_PhQueryCursor(ph_ig,&buf);
			if(buf.key_mods&PHK_ALT) ss|=KS_ALT;
			if(buf.key_mods&PHK_CTRL) ss|=KS_CTRL;
			if(buf.key_mods&PHK_SHIFT) ss|=KS_SHIFT;
			if(buf.key_mods&PHK_SCRLOCK) ss|=KS_SCRLOCK;
			if(buf.key_mods&PHK_CAPSLOCK) ss|=KS_CAPSLOCK;
			if(buf.key_mods&PHK_NUMLOCK) ss|=KS_NUMLOCK;
			return ss;
		}
	return _shift_state;
}

static int  __FASTCALL__ isShiftKeysChange( int flush_queue )
{
	int ss;

	ss=__kbdGetShiftsKey();
	if(ss!=_shift_state)
	{
		_shift_state=ss;
		return 1;
	}
	return 0;
}

int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
	int c;

	c=getch();
/*	if(c!=-1)
	{
		FILE *f=fopen("___kb.out","a+");
		fprintf(f,"0x%08x - 0x%08x\n",c,_2B(c));
		fclose(f);
	}*/
	if(c==ERR)
	{
		c=getms();
		if(c==KE_MOUSE) return c;
		c=0;
	}

	if(isShiftKeysChange(0))
	{
		if(c!=0) ungetch(c);
		return KE_SHIFTKEYS;
	}
	if((!console)&&(!photon)||(photon&&ph_ig==0))
		switch(c)
		{
		case KE_STATUS_ALT:
			_shift_state=KS_ALT;
			return KE_SHIFTKEYS;
		case KE_STATUS_CONTROL:
			_shift_state=KS_CTRL;
			return KE_SHIFTKEYS;
		case KE_STATUS_SHIFT:
			_shift_state=KS_SHIFT;
			return KE_SHIFTKEYS;
		case KE_STATUS_RESET:
			_shift_state=0;
			return KE_SHIFTKEYS;
		}
	switch(c)
	{
	case KE_ENTER2:
		c=KE_ENTER;
		break;
	case KE_BKSPACE1:
	case KE_BKSPACE2:
		c=KE_BKSPACE;
		break;
	default:
		if(c>0xff) c=_2B(c);
		break;
	}
	return c;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
	int c;
	do
	{
		c=__kbdTestKey(flg);
		if(c==KE_SHIFTKEYS) return c;
		if(c==0)
		{
			__OsYield();
			if(flg==KBD_NONSTOP_ON_MOUSE_PRESS&&_mouse_buttons)
				return KE_MOUSE;
		}
	}
	while(c==0);
	if(_shift_state&KS_CTRL) c|=ADD_CONTROL;
	if(_shift_state&KS_ALT) c|=ADD_ALT;
	if(_shift_state&KS_SHIFT) c|=ADD_SHIFT;
	return c;
}

#define DEV_MOUSE	"/dev/devi/mouse0"
/**
 * @note        This mouse access method is based at QSSL mouse drivers.
 *              Mouse access is posible in console mode via character device
 *              named /dev/devi/mouse0. In QNX 6.1.0 - 6.2.1 this name is
 *              provided by devi-hirun photon input manager. Manager may be
 *              runed as a resource manager like this:
 *              # /usr/photon/bin/devi-hirun -Pr ps2 mousedev
 *              Situation may changes after QNX 6.3.0 release...
**/

int _mouse_fd=-1;
int _mouse_state=0;
int _mouse_buttons=0;
static int _mouse_x=0,_mouse_y=0;
static int _mouse_x_raw=0,_mouse_y_raw=0;
#define RAW_DX	8
#define RAW_DY	16

extern tAbsCoord tvioWidth,tvioHeight;
extern mono;
extern unsigned violen;
extern unsigned char *viomem;
extern tAbsCoord saveX,saveY;
extern int PCTable;

#define	_addr(x,y) (viomem+((x)+(y)*tvioWidth))

void __FASTCALL__ _putp(char *str);
void __FASTCALL__ __putp(char *str);
void __FASTCALL__ _putbuf(char **p,char *str);
void __FASTCALL__ _mapcolor(char **p,int col);
void __FASTCALL__ _insertchar(char **p,unsigned char c);

void __FASTCALL__ _mouse_hide();
void __FASTCALL__ _mouse_show();

int __FASTCALL__ __init_mouse()
{
	_mouse_fd=open(DEV_MOUSE,O_RDONLY);
	if(_mouse_fd==-1) return 0;
	_mouse_state=true;

	return 0;
}

void __FASTCALL__ __term_mouse()
{
	if(_mouse_fd!=-1) close(_mouse_fd);
}

bool __FASTCALL__ __MsGetState()
{
	return _mouse_state;
}

void __FASTCALL__ __MsSetState(bool is_visible)
{
	if(_mouse_fd!=-1)
	{
		if(is_visible==false&&_mouse_state==true)
			_mouse_hide();
		if(_mouse_state==false&&is_visible==true)
			_mouse_show();
	}
	_mouse_state=is_visible;
}

void __FASTCALL__ __MsGetPos(tAbsCoord *mx, tAbsCoord *my)
{
	*mx=_mouse_x;
	*my=_mouse_y;
}

int __FASTCALL__ __MsGetBtns()
{
	register int m=_mouse_buttons,c;
	c=__kbdTestKey(0);
	if(m==_mouse_buttons) __OsYield();
	return _mouse_buttons;
}

void __FASTCALL__ _write_char(int c,int ca,int x,int y)
{
	char line[64];
	char *p;

	p=line;
	_putbuf(&p,tparm(cursor_address,y,x));
	p-=4;
	_putbuf(&p,"\x1B""[?7l");
	if(!mono)
	{
		_mapcolor(&p,ca);
	}
	else
	{
		_putbuf(&p,exit_attribute_mode);
		if(ca==0x0f) _putbuf(&p,enter_bold_mode);
		else if(ca==0x70) _putbuf(&p,enter_reverse_mode);
	}

	_insertchar(&p,c);
	*p=0;

	_putbuf(&p,"\x1B""[10m");
	PCTable=-1;
	_putbuf(&p,"\x1B""[?7h");
	_putbuf(&p,tparm(cursor_address,saveY,saveX));
	p-=4;
	if(mono) _putbuf(&p,exit_attribute_mode);
	*p=0;
	__putp(line);
}

void __FASTCALL__ _mouse_hide()
{
	int c,ca;
	char *addr;

	addr=_addr(_mouse_x,_mouse_y);
	c=addr[violen];
	ca=addr[0];
	_write_char(c,ca,_mouse_x,_mouse_y);
}

void __FASTCALL__ _mouse_show()
{
	int c,ca;
	char *addr;

	addr=_addr(_mouse_x,_mouse_y);
	c=addr[violen];
	ca=addr[0];
	_write_char(c,0xff^ca,_mouse_x,_mouse_y);
}

int __FASTCALL__ getms()
{
	fd_set fds;
	int ret;
	register int b;
	int n_mouse_x,n_mouse_y;
	struct timeval tv;
	struct _mouse_packet pkt;
#define PKT_SIZE	sizeof(struct _mouse_packet)

	if(_mouse_fd==-1) return 0;

	FD_ZERO(&fds);
	FD_SET(_mouse_fd,&fds);
	tv.tv_sec=0;
	tv.tv_usec=1;

	ret=select(_mouse_fd+1,&fds,NULL,NULL,&tv);
	if(ret==0||ret==-1) return 0;
	if(!FD_ISSET(_mouse_fd,&fds)) return 0;

	ret=read(_mouse_fd,&pkt,PKT_SIZE);
	if(ret!=PKT_SIZE) return 0;

	_mouse_x_raw+=pkt.dx;
	_mouse_y_raw-=pkt.dy;

	ret=0;
	if(_mouse_x_raw<0) _mouse_x_raw=0;
	if(_mouse_y_raw<0) _mouse_y_raw=0;
	n_mouse_x=_mouse_x_raw/RAW_DX;
	n_mouse_y=_mouse_y_raw/RAW_DY;
	if(n_mouse_x>=tvioWidth)
	{
		n_mouse_x=tvioWidth-1;
		_mouse_x_raw=n_mouse_x*RAW_DX;
	}
	if(n_mouse_y>=tvioHeight)
	{
		n_mouse_y=tvioHeight-1;
		_mouse_y_raw=n_mouse_y*RAW_DY;
	}

	b=0;
	if(pkt.hdr.buttons&_POINTER_BUTTON_LEFT)
		b|=MS_LEFTPRESS;
	if(pkt.hdr.buttons&_POINTER_BUTTON_RIGHT)
		b|=MS_RIGHTPRESS;
	if(pkt.hdr.buttons&_POINTER_BUTTON_MIDDLE)
		b|=MS_MIDDLEPRESS;

	if(_mouse_buttons!=b||_mouse_x!=n_mouse_x||_mouse_y!=n_mouse_y)
		ret=KE_MOUSE;

	_mouse_buttons=b;
	if(_mouse_state)
		_mouse_hide();
	_mouse_x=n_mouse_x;
	_mouse_y=n_mouse_y;
	if(_mouse_state)
		_mouse_show();

	return ret;
}

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
    return -1;
}
