#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include "libbeye/osdep/__os_dep.h"
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/dos/keyboard.c
 * @brief       This file contains implementation of keyboard handles for DOS.
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
**/
#include <bios.h>
#include <limits.h>

#include "libbeye/kbd_code.h"

static int __ms_nbtns;

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
   if((__ms_nbtns = __init_mouse()) != INT_MAX)
   {
     __MsSetState(true);
   }
}

void __FASTCALL__ __term_keyboard()
{
  if(__ms_nbtns != INT_MAX)
  {
    __MsSetState(false);
    __term_mouse();
  }
}

static int  __FASTCALL__ __normalize_code(int code)
{
  switch(code)
  {
    case 0x50E0: return 0x5000; /* down arrow right(gray) */
    case 0x48E0: return 0x4800; /* up  arrow right(gray) */
    case 0x4B30: return 0x4B00; /* letf arrow right(gray) */
    case 0x4D30: return 0x4D00; /* right arrow right(gray) */
    default: return code;
  }
}

static int _mou_btns = 0;
static tAbsCoord _mou_x = 0,_mou_y = 0;

static int  __FASTCALL__ isMouseEventPresent( unsigned long flg, int flush_queue )
{
  int _btns;
  tAbsCoord x,y;
  if(__ms_nbtns == INT_MAX) return 0;
  _btns = __MsGetBtns();
  __MsGetPos(&x,&y);
  if(_btns != _mou_btns ||
     x != _mou_x ||
     y != _mou_y ||
     (((flg & KBD_NONSTOP_ON_MOUSE_PRESS) == KBD_NONSTOP_ON_MOUSE_PRESS) && _btns))
  {
    if(flush_queue)
    {
     _mou_btns = _btns;
     _mou_x = x;
     _mou_y = y;
    }
    return 1;
  }
  return 0;
}

static int __old_ks = 0;

static int  __FASTCALL__ isShiftKeysChange( int flush_queue )
{
  int __ks;
  __ks = __kbdGetShiftsKey();
  if(__ks != __old_ks)
  {
    if(flush_queue) __old_ks = __ks;
    return 1;
  }
  return 0;
}

static int  __FASTCALL__ __test_key( unsigned long flg, int flush_queue )
{
  int ret;
  ret = _bios_keybrd(_KEYBRD_READY);
  if(!ret)
  {
    if(isMouseEventPresent(flg,flush_queue)) return KE_MOUSE;
    if(isShiftKeysChange(flush_queue))   return KE_SHIFTKEYS;
  }
  return __normalize_code(ret);
}


int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
  return __test_key(flg,0);
}

int __FASTCALL__ __kbdGetShiftsKey()
{
  int ret = _bios_keybrd(_KEYBRD_SHIFTSTATUS);
  if(ret & 0x01) ret |= 3; /* return right shift as letf */
  return ret;
}

int __FASTCALL__ __kbdGetKey ( unsigned long flg )
{
  int key;
  while(1)
  {
    key = __test_key(flg,1);
    if(key) break;
    __OsYield();
  }
  if(!(key == KE_MOUSE || key == KE_SHIFTKEYS))
	   key = __normalize_code(_bios_keybrd(_KEYBRD_READ));
  return key;
}

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
  int avail_kbd;
  int rval, rval2;
  const char *type;
  avail_kbd = _bios_keybrd(9);
  if(avail_kbd&0x40)
  {
    rval = _bios_keybrd(0x20);
    rval2 = _bios_keybrd(0x22);
    type = "122-kbd";
  }
  else
  if(avail_kbd&0x20)
  {
    rval = _bios_keybrd(0x10);
    rval2 = _bios_keybrd(0x12);
    type = "enh-kbd";
  }
  else
  {
    rval = _bios_keybrd(0x0);
    rval2 = _bios_keybrd(0x2);
    type = "std-kbd";
  }
  strcpy(head,"Type    Keycode Shifts");
  sprintf(text,"%s %04X      %04X", type, rval, rval2);
  return rval==0x011B?0:1;
}

static bool ms_visible = false;
static bool ms_inited = false;

int __FASTCALL__ __init_mouse()
{
  int ret;
  union REGS inreg,outreg;
  memset(&inreg,0,sizeof(inreg));
  inreg.x.ax = 0;
  int86(0x33,&inreg,&outreg);
  if(!outreg.x.ax) return INT_MAX;
  if(!outreg.x.bx || outreg.x.bx == -1) ret = 2;
  else                                  ret = outreg.x.bx;
  inreg.x.ax = 4;
  inreg.x.cx = tvioWidth / 2;
  inreg.x.dx = tvioHeight / 2;
  int86(0x33,&inreg,&outreg);
  ms_inited = true;
  return ret;
}

void __FASTCALL__ __term_mouse()
{
  if(ms_visible) __MsSetState(false);
}

bool __FASTCALL__ __MsGetState()
{
  return ms_visible;
}

void __FASTCALL__ __MsSetState( bool is_visible )
{
  union REGS inreg;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = is_visible ? 1 : 2;
    int86(0x33,&inreg,&inreg);
    ms_visible = is_visible;
  }
}

#define PIXEL_PER_CHAR 8

void __FASTCALL__ __MsGetPos( tAbsCoord *mx, tAbsCoord *my )
{
  union REGS inreg,outreg;
  *mx = *my = 0;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = 3;
    int86(0x33,&inreg,&outreg);
    *mx = outreg.x.cx / PIXEL_PER_CHAR;
    *my = outreg.x.dx / PIXEL_PER_CHAR;
  }
}

int __FASTCALL__ __MsGetBtns()
{
  union REGS inreg,outreg;
  if(ms_inited)
  {
    memset(&inreg,0,sizeof(inreg));
    inreg.x.ax = 3;
    int86(0x33,&inreg,&outreg);
    return outreg.x.bx;
  }
  else return 0;
}
