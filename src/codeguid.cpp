#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        codeguid.c
 * @brief       This file contains code navigation routines.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        12.09.2000
 * @note        removing difference keys for same locations of jump
**/
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "beye.h"
#include "bmfile.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "codeguid.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/twin.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"

namespace beye {
enum {
    BACK_ADDR_SIZE=256,
    GO_ADDR_SIZE  =37
};

CodeGuider::CodeGuider()
	    :BackAddrPtr(-1)
	    ,GoAddrPtr(-1)
{
    strcpy(codeguid_image,"=>[X]");
    BackAddr = new __filesize_t[BACK_ADDR_SIZE];
    GoAddr = new __filesize_t[GO_ADDR_SIZE];
    GoLineNums = new unsigned[GO_ADDR_SIZE];
}

CodeGuider::~CodeGuider()
{
    delete GoLineNums;
    delete GoAddr;
    delete BackAddr;
}

  /*
      Added by "Kostya Nosov" <k-nosov@yandex.ru>:
      for removing difference keys for same locations of jump
   */
char CodeGuider::gidGetAddressKey( unsigned _index )
{
  int i,j;
  char key;
  bool found;
  __filesize_t addr1,addr2;
  key = 0;
  addr1 = GoAddr[_index];
  for (i = 0;i <= GoAddrPtr;i++)
  {
    addr2 = GoAddr[i];
    if (addr2 == addr1) break;
    /* check for presence addr2 above */
    found = false;
    for (j = 0;j < i;j++)
    {
      if (GoAddr[j] == addr2)
      {
	found = true;
	break;
      }
    }
    if (!found) key++;
  }
  return key < 10 ? key + '0' : key - 10 + 'A';
}

int CodeGuider::gidGetKeyIndex( char key )
{
  int res,i,j,_index;
  bool found;
  __filesize_t addr;
  if (key > 'Z') key = key - 'z' + 'Z';
  key = key > '9' ? key - 'A' + 10 : key - '0';
  res = GoAddrPtr + 1;
  _index = 0;
  for (i = 0;i <= GoAddrPtr;i++)
  {
    addr = GoAddr[i];
    /* check for presence addr above */
    found = false;
    for (j = 0;j < i;j++)
    {
      if (GoAddr[j] == addr)
      {
	found = true;
	break;
      }
    }
    if (i > 0 && !found) _index++;
    if (_index == key)
    {
      res = i;
      break;
    }
  }
  return res;
}

char* CodeGuider::gidBuildKeyStr()
{
    codeguid_image[3] = gidGetAddressKey(GoAddrPtr);
    return codeguid_image;
}

void CodeGuider::reset_go_address( int keycode )
{
    Alarm = 0;
    if(keycode == KE_DOWNARROW) {
	int i;
	if(GoAddrPtr >= 0) {
	    if(GoLineNums[0] == 0) {
		memmove(GoAddr,&GoAddr[1],GoAddrPtr*sizeof(__filesize_t));
		memmove(GoLineNums,&GoLineNums[1],GoAddrPtr*sizeof(unsigned int));
		GoAddrPtr--;
	    }
	    for(i = 0;i <= GoAddrPtr;i++) {
		char dig;
		GoLineNums[i]--;
		dig = gidGetAddressKey(i);
		twDirectWrite(twGetClientWidth(MainWnd)-1,GoLineNums[i]+2,&dig,1);
	    }
	}
    } else if(keycode == KE_UPARROW) {
	int i;
	Alarm = UCHAR_MAX;
	if(GoAddrPtr >= 0) {
	    for(i = 0;i <= GoAddrPtr;i++) GoLineNums[i]++;
	    if(GoLineNums[GoAddrPtr] >= twGetClientHeight(MainWnd)) GoAddrPtr--;
	}
    } else GoAddrPtr = -1;
}

void CodeGuider::add_go_address(const DisMode& parent,char *str,__filesize_t addr)
{
    tAbsCoord width = twGetClientWidth(MainWnd);
    unsigned bytecodes=beye_context().active_mode()->get_max_symbol_size()*2;
    int len,where;
    if(parent.prepare_mode()) return;
    len = strlen((char *)str);
    where = (parent.panel_mode() == DisMode::Panel_Full ? width :
	    parent.panel_mode() == DisMode::Panel_Medium ? width-HA_LEN() : width-(HA_LEN()+1)-bytecodes) - 5;
    if(Alarm) {
	int i;
	if(GoAddrPtr < GO_ADDR_SIZE - 2) GoAddrPtr++;
	memmove(&GoAddr[1],&GoAddr[0],GoAddrPtr*sizeof(long));
	memmove(&GoLineNums[1],&GoLineNums[0],GoAddrPtr*sizeof(int));
	GoAddr[0] = addr;
	GoLineNums[0] = parent.get_curr_line_num();
	if(len < where) {
	    memset(&str[len],TWC_DEF_FILLER,where-len);
	    str[where] = 0;
	}
	strcat(str,codeguid_image);
	str[where + 3] = '0';
	for(i = 1;i <= GoAddrPtr;i++) {
	    char dig;
	    dig = gidGetAddressKey(i);
	    twDirectWrite(width-1,GoLineNums[i]+1,&dig,1);
	}
    } else if(GoAddrPtr < GO_ADDR_SIZE - 2) {
	GoAddrPtr++;
	GoAddr[GoAddrPtr] = addr;
	GoLineNums[GoAddrPtr] = parent.get_curr_line_num();
	if(len < where) {
	    memset(&str[len],TWC_DEF_FILLER,where-len);
	    str[where] = 0;
	}
	strcpy((char *)&str[where],(char *)gidBuildKeyStr());
    }
}

void CodeGuider::add_back_address()
{
    if(BackAddrPtr >= BACK_ADDR_SIZE - 2) {
	memmove(BackAddr,&BackAddr[1],BackAddrPtr);
	BackAddrPtr--;
    }
    BackAddrPtr++;
    BackAddr[BackAddrPtr] = BMGetCurrFilePos();
}

__filesize_t CodeGuider::get_go_address(unsigned keycode)
{
    __filesize_t ret;
    if(keycode == KE_BKSPACE) ret = BackAddrPtr >= 0 ? BackAddr[BackAddrPtr--] : BMGetCurrFilePos();
    else {
	int ptr;
	keycode &= 0x00FF;
	ptr = gidGetKeyIndex(keycode);
	if(ptr <= GoAddrPtr) {
	    add_back_address();
	    ret = GoAddr[ptr];
	} else ret = BMGetCurrFilePos();
    }
    return ret;
}

char* CodeGuider::encode_address(__filesize_t cfpos,bool AddressDetail)
{
    static char addr[11];
    strcpy(addr,((BMFileFlags&BMFF_USE64)?Get16Digit(cfpos):Get8Digit(cfpos)));
    if(AddressDetail && beye_context().active_format()->AddressResolving)
	beye_context().active_format()->AddressResolving(addr,cfpos);
    strcat(addr,": ");
    return addr;
}
} // namespace beye

