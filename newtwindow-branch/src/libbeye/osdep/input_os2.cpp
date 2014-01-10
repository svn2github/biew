#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/os2/keyboard.c
 * @brief       This file contains implementation of keyboard handles for OS/2.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <limits.h>
#define INCL_SUB
#define INCL_DOSSIGNALS
#include <os2.h>

#include "libbeye/kbd_code.h"

static HKBD kbdHandle;
static int __ms_nbtns;

void __FASTCALL__ __init_keyboard( const char *user_cp )
{
   KbdOpen(&kbdHandle);
   if(kbdHandle) KbdGetFocus(0,kbdHandle);
   if((__ms_nbtns = __init_mouse()) != INT_MAX)
   {
     __MsSetState(true);
   }
}

void __FASTCALL__ __term_keyboard()
{
  KbdFreeFocus(kbdHandle);
  KbdClose(kbdHandle);
  if(__ms_nbtns != INT_MAX)
  {
    __MsSetState(false);
    __term_mouse();
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
	USHORT retval = 0;
	USHORT save;
	KBDINFO info;
	KBDKEYINFO key;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	KbdPeek(&key,kbdHandle);
	if (key.fbStatus & 0x40) {
		retval = key.chChar + (key.chScan << 8);
	} else
		retval = 0;

	if (key.chChar == 224)
		retval &= 0xff00;
	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
	if(retval == 0)
	{
	   if(isMouseEventPresent(flg,flush_queue)) retval = KE_MOUSE;
	   else if(isShiftKeysChange(flush_queue)) retval = KE_SHIFTKEYS;
	}
	return retval;
}


int __FASTCALL__ __kbdTestKey( unsigned long flg )
{
  return __test_key(flg,0);
}

int __FASTCALL__ __kbdGetShiftsKey()
{
	USHORT retval = 0;
	USHORT save;
	KBDINFO info;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	retval = info.fsState;

	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
	if(retval & 0x01) retval |= 3; /* return right shift as letf */
	return retval;
}

int __FASTCALL__ __kbdGetKey( unsigned long flg )
{
  USHORT retval = 0;
  while(1)
  {
    retval = __test_key(flg,1);
    if(retval) break;
    __OsYield();
  }
  if(!(retval == KE_MOUSE || retval == KE_SHIFTKEYS))
  {
	USHORT save;
	KBDINFO info;
	KBDKEYINFO key;

	info.cb = sizeof(KBDINFO);

	KbdGetStatus(&info,kbdHandle);
	save = info.fsMask;
	info.fsMask &= 0xfff7;
	KbdSetStatus(&info,kbdHandle);

	KbdCharIn(&key,0,kbdHandle);
	retval = key.chChar + (key.chScan << 8);

	if (key.chChar == 224)
		retval &= 0xff00;
	info.fsMask = save;
	KbdSetStatus(&info,kbdHandle);
   }
   return retval;
}

int __FASTCALL__ __inputRawInfo(char *head, char *text)
{
    return -1;
}

static HMOU mouHandle;
static int mou_btns;
static bool ms_visible = false;
static USHORT mouStatus;

int __FASTCALL__ __init_mouse()
{
   USHORT rc,nbtn,status;
   rc = MouOpen(NULL,&mouHandle);
   if(rc) { mouHandle = 0; return INT_MAX; }
   MouGetNumButtons(&nbtn,mouHandle);
   MouGetDevStatus(&mouStatus,mouHandle);
   status = 0x200;
   MouSetDevStatus(&status,mouHandle);
   mou_btns = nbtn;
   MouDrawPtr(mouHandle);
   return nbtn;
}

void __FASTCALL__ __term_mouse()
{
  __MsSetState(false);
  MouSetDevStatus(&mouStatus,mouHandle);
  if(mouHandle) MouClose(mouHandle);
  mouHandle = 0;
}

bool __FASTCALL__ __MsGetState()
{
  return ms_visible;
}

void __FASTCALL__ __MsSetState( bool state )
{
  switch(state)
  {
    case true:
     if(!ms_visible)
     {
       MouDrawPtr(mouHandle);
       ms_visible=true;
     }
     break;
    default:
    case false:
    {
      NOPTRRECT mrect;
      if(ms_visible)
      {
	mrect.row = mrect.col = 0;
	mrect.cCol = tvioWidth - 1;
	mrect.cRow = tvioHeight - 1;
	MouRemovePtr(&mrect,mouHandle);
	ms_visible=false;
      }
    }
  }
}

#define PIXEL_PER_CHAR 8

void __FASTCALL__ __MsGetPos(tAbsCoord *mx,tAbsCoord *my )
{
  USHORT devstatus;
  PTRLOC ploc;
  *mx = *my = 0;
  if(mouHandle)
  {
    MouGetPtrPos(&ploc,mouHandle);
    MouGetDevStatus(&devstatus,mouHandle);
    *mx = ploc.col;
    *my = ploc.row;
    if(!(devstatus & 0x0200))
    {
      *mx = ploc.col/PIXEL_PER_CHAR;
      *my = ploc.row/PIXEL_PER_CHAR;
    }
  }
}

int __FASTCALL__ __MsGetBtns()
{
  static int ret;
  USHORT fwait;
  MOUQUEINFO mqinfo;
  static MOUEVENTINFO minfo;
  ret = 0;
  if(mouHandle)
  {
    fwait = 0;
    while(1)
    {
      MouGetNumQueEl(&mqinfo,mouHandle);
      if(mqinfo.cEvents < 1) break;
      MouReadEventQue(&minfo,&fwait,mouHandle);
    }
    if((minfo.fs & 0x04) == 0x04 || (minfo.fs & 0x02) == 0x02) ret |= MS_LEFTPRESS;
    if((minfo.fs & 0x10) == 0x10 || (minfo.fs & 0x08) == 0x08)
    {
       if(mou_btns == 2) ret |= MS_RIGHTPRESS;
       else              ret |= MS_MIDDLEPRESS;
    }
    if((minfo.fs & 0x40) == 0x40 || (minfo.fs & 0x20) == 0x20) ret |= MS_RIGHTPRESS;
  }
  return ret;
}
