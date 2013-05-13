#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/le.c
 * @brief       This file contains implementation of LE (Linear Executable) file
 *              format decoder.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "beye.h"
#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "bin_util.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace	usr {
static const char* txt[]={ "LEHelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "", "LEHead", "MapTbl", "Object" };
const char* LE_Parser::prompt(unsigned idx) const { return txt[idx]; }

bool LE_Parser::__ReadMapTblLE(binary_stream& handle,memArray * obj,unsigned n)
{
 size_t i;
  handle.seek(lxe.le.leObjectPageMapTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  for(i = 0;i < n;i++)
  {
    LE_PAGE lep;
    char stmp[80];
    if(IsKbdTerminate() || handle.eof()) break;
    handle.read(&lep,sizeof(LE_PAGE));
    sprintf(stmp,"#=%08lXH Flags: %04hX = %s",(long)lep.number,lep.flags,lxeGetMapAttr(lep.flags));
    if(!ma_AddString(obj,stmp,true)) break;
  }
  return true;
}

__filesize_t  LE_Parser::__calcPageEntryLE(LE_PAGE *mt,unsigned long idx) const
{
  __filesize_t ret;
  __filesize_t dataoff;
  dataoff = idx*lxe.le.lePageSize;
  if(mt->flags == 1 || mt->flags == 5) ret = lxe.le.leObjectIterDataMapOffset + dataoff;
  else
    if(mt->flags == 0) ret = lxe.le.leDataPagesOffset + dataoff;
    else               ret = -1;
  return ret;
}

__filesize_t LE_Parser::CalcPageEntry(unsigned long pageidx) const
{
  binary_stream* handle;
  bool found;
  unsigned i;
  LE_PAGE mt;
  if(!pageidx) return -1;
  handle = lx_cache;
  handle->seek(lxe.le.leObjectPageMapTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  found = false;
  for(i = 0;i < lxe.le.lePageCount;i++)
  {
    handle->read((any_t*)&mt,sizeof(LE_PAGE));
    if(handle->eof()) break;
    if(mt.number == pageidx)
    {
      found = true;
      break;
    }
  }
  if(found) return __calcPageEntryLE((LE_PAGE*)&mt,pageidx - 1);
  else      return beye_context().bm_file().tell();
}

__filesize_t LE_Parser::CalcEntryPoint(unsigned long objnum,__filesize_t _offset) const
{
  binary_stream* handle;
  unsigned long i,start,pidx,j;
  __filesize_t ret,pageoff;
  LX_OBJECT lo;
  LE_PAGE mt;
  if(!objnum) return -1;
  handle = lx_cache;
  handle->seek(lxe.le.leObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  handle->seek(sizeof(LX_OBJECT)*(objnum - 1),binary_stream::Seek_Cur);
  handle->read((any_t*)&lo,sizeof(LX_OBJECT));
/*  if((lo.o32_flags & 0x00002000L) == 0x00002000L) USE16 = 0;
  else                                            USE16 = 0xFF; */
  pageoff = lxe.le.leObjectPageMapTableOffset + beye_context().headshift;
  start = 0;
  ret = -1;
  for(i = 0;i < lo.o32_mapsize;i++)
  {
    bool is_eof;
    is_eof = false;
    if(_offset >= start && _offset < start + lxe.le.lePageSize)
    {
      bool found;
      handle->seek(pageoff,binary_stream::Seek_Set);
      pidx = i + lo.o32_pagemap;
      found = false;
      for(j = 0;j < lxe.le.lePageCount;j++)
      {
	handle->read((any_t*)&mt,sizeof(LE_PAGE));
	if((is_eof = handle->eof()) != 0) break;
	if(mt.number == pidx) { found = true; break; }
      }
      if(found) ret = __calcPageEntryLE((LE_PAGE*)&mt,pidx - 1) + _offset - start;
      else      ret = beye_context().bm_file().tell();
      break;
    }
    if(is_eof) break;
    start += lxe.le.lePageSize;
  }
  return ret;
}

__filesize_t LE_Parser::CalcEntryLE(const LX_ENTRY *lxent)
{
  __filesize_t ret;
  ret = beye_context().bm_file().tell();
      switch(lxent->b32_type)
      {
	case 1: ret = CalcEntryPoint(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset16);
		      break;
	case 2: ret = CalcEntryPoint(lxent->b32_obj,lxent->entry.e32_variant.e32_callgate.offset);
		      break;
	case 3: ret = CalcEntryPoint(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset32);
		      break;
	case 4: ShowFwdModOrdLX(lxent);
	case 5:
	default: break;
      }
  return ret;
}

__filesize_t LE_Parser::CalcEntryBungleLE(unsigned ordinal,bool dispmsg)
{
  binary_stream* handle;
  bool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  uint_fast16_t numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = beye_context().bm_file().tell();
  handle = lx_cache;
  handle->seek(lxe.le.leEntryTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  i = 0;
  found = false;
  while(1)
  {
   cnt = handle->read(type_byte);
   type = handle->read(type_byte);
   if(!cnt) break;
   if(type) numobj = handle->read(type_word);
   for(j = 0;j < cnt;j++,i++)
   {
     char size;
     switch(type)
     {
       case 0: size = 0; break;
       case 1: size = 2; break;
       case 2:
       case 0x80:
       case 3: size = 4; break;
       default:
       case 4: size = 6; break;
     }
     if(i == ordinal - 1)
     {
       lxent.b32_type = type;
       found = true;
       if(size)
       {
	 lxent.b32_obj = numobj;
	 lxent.entry.e32_flags = handle->read(type_byte);
	 handle->read((any_t*)&lxent.entry.e32_variant,size);
       }
       break;
     }
     else
       if(size) handle->seek(size + sizeof(char),binary_stream::Seek_Cur);
     if(handle->eof()) break;
   }
   if(found) break;
 }
 if(found) ret = CalcEntryLE((LX_ENTRY *)&lxent);
 else      if(dispmsg) beye_context().ErrMessageBox(NOT_ENTRY,"");
 return ret;
}

__filesize_t LE_Parser::action_F10()
{
    __filesize_t fpos = beye_context().bm_file().tell();
    int ret;
    std::string title = " Map of pages ";
    ssize_t nnames = (unsigned)lxe.le.lePageCount;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadMapTblLE(beye_context().sc_bm_file(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret != -1) fpos = CalcPageEntry(ret + 1);
    return fpos;
}

__filesize_t LE_Parser::action_F3()
{
    __filesize_t fpos = beye_context().bm_file().tell();
    int ret;
    unsigned ordinal;
    std::string title = RES_NAMES;
    ssize_t nnames = LXRNamesNumItems(beye_context().sc_bm_file());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXRNamesReadItems(beye_context().sc_bm_file(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
	if(ret != -1) {
	    const char* cptr;
	    char buff[40];
	    cptr = strrchr((char*)obj->data[ret],LB_ORD_DELIMITER);
	    cptr++;
	    strcpy(buff,cptr);
	    ordinal = atoi(buff);
	}
    }
    ma_Destroy(obj);
    exit:
    if(ret != -1) fpos = CalcEntryBungleLE(ordinal,true);
    return fpos;
}

__filesize_t LE_Parser::action_F4()
{
    __filesize_t fpos = beye_context().bm_file().tell();
    int ret;
    unsigned ordinal;
    std::string title = NORES_NAMES;
    ssize_t nnames = LXNRNamesNumItems(beye_context().sc_bm_file());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXNRNamesReadItems(beye_context().sc_bm_file(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
	if(ret != -1) {
	    const char* cptr;
	    char buff[40];
	    cptr = strrchr((char*)obj->data[ret],LB_ORD_DELIMITER);
	    cptr++;
	    strcpy(buff,cptr);
	    ordinal = atoi(buff);
	}
    }
    ma_Destroy(obj);
    exit:
    if(ret != -1) fpos = CalcEntryBungleLE(ordinal,true);
    return fpos;
}

LE_Parser::LE_Parser(CodeGuider& __code_guider)
	:LX_Parser(__code_guider)
{
    beye_context().sc_bm_file().seek(beye_context().headshift,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(&lxe.le,sizeof(LEHEADER));
}

LE_Parser::~LE_Parser()
{
}

__filesize_t LE_Parser::action_F1()
{
  hlpDisplay(10004);
  return beye_context().bm_file().tell();
}

bool LE_Parser::address_resolving(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  if(cfpos >= beye_context().headshift && cfpos < beye_context().headshift + sizeof(LEHEADER))
  {
     strcpy(addr,"LEH :");
     strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift));
  }
  else
  if(cfpos >= beye_context().headshift + lxe.le.leObjectTableOffset &&
     cfpos <  beye_context().headshift + lxe.le.leObjectTableOffset + sizeof(LX_OBJECT)*lxe.le.leObjectCount)
  {
     strcpy(addr,"LEOD:");
     strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - lxe.le.leObjectTableOffset));
  }
  else
  if(cfpos >= beye_context().headshift + lxe.le.leObjectPageMapTableOffset &&
     cfpos <  beye_context().headshift + lxe.le.leObjectPageMapTableOffset + sizeof(LE_PAGE)*lxe.le.lePageCount)
  {
    strcpy(addr,"LEPD:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - lxe.le.leObjectPageMapTableOffset));
  }
  else bret = false;
  return bret;
}

int LE_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe() {
   char id[2];
   beye_context().headshift = IsNewExe();
   if(beye_context().headshift)
   {
     beye_context().sc_bm_file().seek(beye_context().headshift,binary_stream::Seek_Set);
     beye_context().sc_bm_file().read(id,sizeof(id));
     if(id[0] == 'L' && id[1] == 'E') return true;
   }
   return false;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) LE_Parser(_parent); }
extern const Binary_Parser_Info le_info = {
    "LE (Linear Executable)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
