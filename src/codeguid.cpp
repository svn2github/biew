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
CodeGuider::CodeGuider() {
    strcpy(codeguid_image,"=>[X]");
}
CodeGuider::~CodeGuider() {}

  /*
      Added by "Kostya Nosov" <k-nosov@yandex.ru>:
      for removing difference keys for same locations of jump
   */
char CodeGuider::gidGetAddressKey( unsigned _index ) const
{
  size_t i,j,sz=GoAddr.size();
  char key;
  bool found;
  __filesize_t addr1,addr2;
  key = 0;
  addr1 = GoAddr[_index].first;
  for (i = 0;i < sz;i++)
  {
    addr2 = GoAddr[i].first;
    if (addr2 == addr1) break;
    /* check for presence addr2 above */
    found = false;
    for (j = 0;j < i;j++)
    {
      if (GoAddr[j].first == addr2)
      {
	found = true;
	break;
      }
    }
    if (!found) key++;
  }
  return key < 10 ? key + '0' : key - 10 + 'A';
}

int CodeGuider::gidGetKeyIndex( char key ) const
{
  int res,_index;
  bool found;
  __filesize_t addr;
  if (key > 'Z') key = key - 'z' + 'Z';
  key = key > '9' ? key - 'A' + 10 : key - '0';
  res = GoAddr.size();
  _index = 0;
  size_t j,i,sz=GoAddr.size();
  for (i = 0;i < sz;i++) {
    addr = GoAddr[i].first;
    /* check for presence addr above */
    found = false;
    for (j = 0;j < i;j++) {
      if (GoAddr[j].first == addr) {
	found = true;
	break;
      }
    }
    if (i > 0 && !found) _index++;
    if (_index == key) {
      res = i;
      break;
    }
  }
  return res;
}

char* CodeGuider::gidBuildKeyStr()
{
    codeguid_image[3] = gidGetAddressKey(GoAddr.size()-1);
    return codeguid_image;
}

void CodeGuider::reset_go_address( int keycode )
{
    Alarm = 0;
    if(keycode == KE_DOWNARROW) {
	size_t i,sz=GoAddr.size();
	if(sz) {
	    if(GoAddr[0].second == 0) GoAddr.erase(GoAddr.begin());
	    for(i = 0;i < sz;i++) {
		char dig;
		GoAddr[i].second=GoAddr[i].second-1;
		dig = gidGetAddressKey(i);
		twDirectWrite(twGetClientWidth(MainWnd)-1,GoAddr[i].second+2,&dig,1);
	    }
	}
    } else if(keycode == KE_UPARROW) {
	size_t i,sz=GoAddr.size();
	Alarm = UCHAR_MAX;
	if(sz) {
	    for(i = 0;i < sz;i++) GoAddr[i].second=GoAddr[i].second+1;
	    if(GoAddr.back().second >= twGetClientHeight(MainWnd)) GoAddr.pop_back();
	}
    } else GoAddr.clear();
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
	size_t i,sz;
	GoAddr.insert(GoAddr.begin(),std::make_pair(addr,parent.get_curr_line_num()));
	if(len < where) {
	    memset(&str[len],TWC_DEF_FILLER,where-len);
	    str[where] = 0;
	}
	strcat(str,codeguid_image);
	str[where + 3] = '0';
	sz=GoAddr.size();
	for(i = 1;i < sz;i++) {
	    char dig;
	    dig = gidGetAddressKey(i);
	    twDirectWrite(width-1,GoAddr[i].second+1,&dig,1);
	}
    } else {
	GoAddr.push_back(std::make_pair(addr,parent.get_curr_line_num()));
	if(len < where) {
	    memset(&str[len],TWC_DEF_FILLER,where-len);
	    str[where] = 0;
	}
	strcpy((char *)&str[where],(char *)gidBuildKeyStr());
    }
}

void CodeGuider::add_back_address()
{
    BackAddr.push_back(BMGetCurrFilePos());
}

__filesize_t CodeGuider::get_go_address(unsigned keycode)
{
    __filesize_t ret;
    if(keycode == KE_BKSPACE) {
	if(BackAddr.size()) {
	    ret = BackAddr.back();
	    BackAddr.pop_back();
	}
	else ret=BMGetCurrFilePos();
    } else {
	unsigned ptr;
	keycode &= 0x00FF;
	ptr = gidGetKeyIndex(keycode);
	if(ptr < GoAddr.size()) {
	    add_back_address();
	    ret = GoAddr[ptr].first;
	} else ret = BMGetCurrFilePos();
    }
    return ret;
}

char* CodeGuider::encode_address(__filesize_t cfpos,bool AddressDetail) const
{
    static char addr[11];
    strcpy(addr,is_BMUse64()?Get16Digit(cfpos):Get8Digit(cfpos));
    if(AddressDetail && beye_context().active_format()->AddressResolving)
	beye_context().active_format()->AddressResolving(addr,cfpos);
    strcat(addr,": ");
    return addr;
}
} // namespace beye

