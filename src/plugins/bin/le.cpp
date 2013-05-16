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

#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "udn.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
static const char* txt[]={ "LEHelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "", "LEHead", "MapTbl", "Object" };
const char* LE_Parser::prompt(unsigned idx) const { return txt[idx]; }

bool LE_Parser::__ReadMapTblLE(binary_stream& handle,memArray * obj,unsigned n)
{
 size_t i;
  handle.seek(lxe.le.leObjectPageMapTableOffset + headshift(),binary_stream::Seek_Set);
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
  handle->seek(lxe.le.leObjectPageMapTableOffset + headshift(),binary_stream::Seek_Set);
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
  else      return beye_context().tell();
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
  handle->seek(lxe.le.leObjectTableOffset + headshift(),binary_stream::Seek_Set);
  handle->seek(sizeof(LX_OBJECT)*(objnum - 1),binary_stream::Seek_Cur);
  handle->read((any_t*)&lo,sizeof(LX_OBJECT));
/*  if((lo.o32_flags & 0x00002000L) == 0x00002000L) USE16 = 0;
  else                                            USE16 = 0xFF; */
  pageoff = lxe.le.leObjectPageMapTableOffset + headshift();
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
      else      ret = beye_context().tell();
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
  ret = beye_context().tell();
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
  ret = beye_context().tell();
  handle = lx_cache;
  handle->seek(lxe.le.leEntryTableOffset + headshift(),binary_stream::Seek_Set);
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
    __filesize_t fpos = beye_context().tell();
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
    bval = __ReadMapTblLE(main_handle(),obj,nnames);
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
    __filesize_t fpos = beye_context().tell();
    int ret;
    unsigned ordinal;
    std::string title = RES_NAMES;
    ssize_t nnames = LXRNamesNumItems(main_handle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXRNamesReadItems(main_handle(),obj,nnames);
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
    __filesize_t fpos = beye_context().tell();
    int ret;
    unsigned ordinal;
    std::string title = NORES_NAMES;
    ssize_t nnames = LXNRNamesNumItems(main_handle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXNRNamesReadItems(main_handle(),obj,nnames);
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

LE_Parser::LE_Parser(binary_stream& h,CodeGuider& __code_guider,udn& u)
	:LX_Parser(h,__code_guider,u)
{
    main_handle().seek(headshift(),binary_stream::Seek_Set);
    main_handle().read(&lxe.le,sizeof(LEHEADER));
}

LE_Parser::~LE_Parser()
{
}

__filesize_t LE_Parser::action_F1()
{
  hlpDisplay(10004);
  return beye_context().tell();
}

bool LE_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  if(cfpos >= headshift() && cfpos < headshift() + sizeof(LEHEADER))
  {
     addr="LEH :";
     addr+=Get4Digit(cfpos - headshift());
  }
  else
  if(cfpos >= headshift() + lxe.le.leObjectTableOffset &&
     cfpos <  headshift() + lxe.le.leObjectTableOffset + sizeof(LX_OBJECT)*lxe.le.leObjectCount)
  {
     addr="LEOD:";
     addr+=Get4Digit(cfpos - headshift() - lxe.le.leObjectTableOffset);
  }
  else
  if(cfpos >= headshift() + lxe.le.leObjectPageMapTableOffset &&
     cfpos <  headshift() + lxe.le.leObjectPageMapTableOffset + sizeof(LE_PAGE)*lxe.le.lePageCount)
  {
    addr="LEPD:";
    addr+=Get4Digit(cfpos - headshift() - lxe.le.leObjectPageMapTableOffset);
  }
  else bret = false;
  return bret;
}

int LE_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe(binary_stream& main_handle) {
   char id[2];
   __filesize_t headshift = MZ_Parser::is_new_exe(main_handle);
   if(headshift)
   {
     main_handle.seek(headshift,binary_stream::Seek_Set);
     main_handle.read(id,sizeof(id));
     if(id[0] == 'L' && id[1] == 'E') return true;
   }
   return false;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) LE_Parser(h,_parent,u); }
extern const Binary_Parser_Info le_info = {
    "LE (Linear Executable)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
