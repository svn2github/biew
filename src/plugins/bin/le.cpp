#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
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
#include "bmfile.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
extern BFile* lx_cache;

static __filesize_t __FASTCALL__ ShowNewHeaderLE( void )
{
  return ShowNewHeaderLX();
}

static bool __FASTCALL__ __ReadMapTblLE(BFile& handle,memArray * obj,unsigned n)
{
 size_t i;
  handle.seek(lxe.le.leObjectPageMapTableOffset + beye_context().headshift,SEEKF_START);
  for(i = 0;i < n;i++)
  {
    LE_PAGE lep;
    char stmp[80];
    if(IsKbdTerminate() || handle.eof()) break;
    handle.read_buffer(&lep,sizeof(LE_PAGE));
    sprintf(stmp,"#=%08lXH Flags: %04hX = %s",(long)lep.number,lep.flags,lxeGetMapAttr(lep.flags));
    if(!ma_AddString(obj,stmp,true)) break;
  }
  return true;
}

static __filesize_t  __FASTCALL__ __calcPageEntryLE(LE_PAGE *mt,unsigned long idx)
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

__filesize_t __FASTCALL__ CalcPageEntryLE(unsigned long pageidx)
{
  BFile* handle;
  bool found;
  unsigned i;
  LE_PAGE mt;
  if(!pageidx) return -1;
  handle = lx_cache;
  handle->seek(lxe.le.leObjectPageMapTableOffset + beye_context().headshift,SEEK_SET);
  found = false;
  for(i = 0;i < lxe.le.lePageCount;i++)
  {
    handle->read_buffer((any_t*)&mt,sizeof(LE_PAGE));
    if(handle->eof()) break;
    if(mt.number == pageidx)
    {
      found = true;
      break;
    }
  }
  if(found) return __calcPageEntryLE((LE_PAGE*)&mt,pageidx - 1);
  else      return BMGetCurrFilePos();
}

__filesize_t __FASTCALL__ CalcEntryPointLE(unsigned long objnum,__filesize_t _offset)
{
  BFile* handle;
  unsigned long i,start,pidx,j;
  __filesize_t ret,pageoff;
  LX_OBJECT lo;
  LE_PAGE mt;
  if(!objnum) return -1;
  handle = lx_cache;
  handle->seek(lxe.le.leObjectTableOffset + beye_context().headshift,SEEK_SET);
  handle->seek(sizeof(LX_OBJECT)*(objnum - 1),SEEKF_CUR);
  handle->read_buffer((any_t*)&lo,sizeof(LX_OBJECT));
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
      handle->seek(pageoff,SEEKF_START);
      pidx = i + lo.o32_pagemap;
      found = false;
      for(j = 0;j < lxe.le.lePageCount;j++)
      {
	handle->read_buffer((any_t*)&mt,sizeof(LE_PAGE));
	if((is_eof = handle->eof()) != 0) break;
	if(mt.number == pidx) { found = true; break; }
      }
      if(found) ret = __calcPageEntryLE((LE_PAGE*)&mt,pidx - 1) + _offset - start;
      else      ret = BMGetCurrFilePos();
      break;
    }
    if(is_eof) break;
    start += lxe.le.lePageSize;
  }
  return ret;
}

__filesize_t __FASTCALL__ CalcEntryLE(const LX_ENTRY *lxent)
{
  __filesize_t ret;
  ret = BMGetCurrFilePos();
      switch(lxent->b32_type)
      {
	case 1: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset16);
		      break;
	case 2: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_callgate.offset);
		      break;
	case 3: ret = CalcEntryPointLE(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset32);
		      break;
	case 4: ShowFwdModOrdLX(lxent);
	case 5:
	default: break;
      }
  return ret;
}

static __filesize_t  __FASTCALL__ CalcEntryBungleLE(unsigned ordinal,bool dispmsg)
{
  BFile* handle;
  bool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  uint_fast16_t numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = BMGetCurrFilePos();
  handle = lx_cache;
  handle->seek(lxe.le.leEntryTableOffset + beye_context().headshift,SEEK_SET);
  i = 0;
  found = false;
  while(1)
  {
   cnt = handle->read_byte();
   type = handle->read_byte();
   if(!cnt) break;
   if(type) numobj = handle->read_word();
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
	 lxent.entry.e32_flags = handle->read_byte();
	 handle->read_buffer((any_t*)&lxent.entry.e32_variant,size);
       }
       break;
     }
     else
       if(size) handle->seek(size + sizeof(char),SEEKF_CUR);
     if(handle->eof()) break;
   }
   if(found) break;
 }
 if(found) ret = CalcEntryLE((LX_ENTRY *)&lxent);
 else      if(dispmsg) ErrMessageBox(NOT_ENTRY,NULL);
 return ret;
}

static unsigned __FASTCALL__ leMapTblNumEntries(BFile& handle)
{
  UNUSED(handle);
  return (unsigned)lxe.le.lePageCount;
}

static __filesize_t __FASTCALL__ ShowMapTableLE( void )
{
 __filesize_t fpos;
 int ret;
 fpos = BMGetCurrFilePos();
 ret = fmtShowList(leMapTblNumEntries,__ReadMapTblLE,
		   " Map of pages ", LB_SELECTIVE, NULL);
 if(ret != -1)
 {
   fpos = CalcPageEntryLE(ret + 1);
 }
 return fpos;
}

static __filesize_t __FASTCALL__ ShowResNamLE( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  ret = fmtShowList(LXRNamesNumItems,LXRNamesReadItems,
		    RES_NAMES,
		    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryBungleLE(ordinal,true);
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowNResNmLE( void )
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  {
    int ret;
    unsigned ordinal;
    ret = fmtShowList(LXNRNamesNumItems,LXNRNamesReadItems,
		      NORES_NAMES,
		      LB_SELECTIVE | LB_SORTABLE,&ordinal);
    if(ret != -1)
    {
      fpos = CalcEntryBungleLE(ordinal,true);
    }
  }
  return fpos;
}

static bool __FASTCALL__ isLE( void )
{
   char id[2];
   beye_context().headshift = IsNewExe();
   if(beye_context().headshift)
   {
     bmReadBufferEx(id,sizeof(id),beye_context().headshift,SEEKF_START);
     if(id[0] == 'L' && id[1] == 'E') return true;
   }
   return false;
}

static void __FASTCALL__ LEinit(CodeGuider& code_guider)
{
    UNUSED(code_guider);
   BFile& main_handle = bmbioHandle();
   LXType = FILE_LE;
   bmReadBufferEx(&lxe.le,sizeof(LEHEADER),beye_context().headshift,SEEKF_START);
   if((lx_cache = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) lx_cache = &main_handle;
}

static void __FASTCALL__ LEdestroy( void )
{
   BFile& main_handle = bmbioHandle();
   if(lx_cache != &bNull && lx_cache != &main_handle) delete lx_cache;
}

static __filesize_t __FASTCALL__ LEHelp( void )
{
  hlpDisplay(10004);
  return BMGetCurrFilePos();
}

static bool __FASTCALL__ leAddressResolv(char *addr,__filesize_t cfpos)
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

static int __FASTCALL__ lePlatform( void ) { return DISASM_CPU_IX86; }

extern const REGISTRY_BIN leTable =
{
  "LE (Linear Executable)",
  { "LEHelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", NULL, "LEHead", "MapTbl", "Object" },
  { LEHelp, ShowModRefLX, ShowResNamLE, ShowNResNmLE, ShowImpProcLXLE, ShowEntriesLX, NULL, ShowNewHeaderLE, ShowMapTableLE, ShowObjectsLX },
  isLE, LEinit, LEdestroy,
  NULL,
  NULL,
  lePlatform,
  NULL,
  NULL,
  leAddressResolv,
  NULL,
  NULL,
  NULL,
  NULL
};
} // namespace beye
