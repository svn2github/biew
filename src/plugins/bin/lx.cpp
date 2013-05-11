#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/lx.c
 * @brief       This file contains implementation of LX (Linear eXecutable) file
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "beye.h"
#include "colorset.h"
#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "plugins/bin/ne.h"
#include "bin_util.h"
#include "bmfile.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"

namespace	usr {
union LX_LE lxe;
int LXType;

binary_stream* lx_cache = &bNull;

static __filesize_t  __FASTCALL__ CalcEntryPointLX(unsigned long objnum,__filesize_t _offset);

static const char * LXordering[] =
{
 "little endian",
 "big endian"
};

const char * LXcputype[] =
{
 "Unknown",
 "80286",
 "80386",
 "80486",
 "80586",
 "80686",
 "80787",
 "80887"
};

const char * LXostype[] =
{
  "Unknown",
  "OS/2",
  "Windows",
  "DOS 4.x",
  "Windows/386",
  "Unknown",
  "Unknown",
  "Unknown"
};

const char * __osModType[] =
{
  "PROGRAM",
  "LIBRARY",
  "PROT. MODE PROGRAM",
  "PROT. MODE LIBRARY",
  "PHYSICAL DEVICE DRIVER",
  "VIRTUAL DEVICE DRIVER",
  "PROT. MODE PHYSICAL DEVICE DRIVER",
  "PROT. MODE VIRTUAL DEVICE DRIVER"
};

static __filesize_t LXEntryPoint = 0;

static const char *  __FASTCALL__ GetOrderingLX(unsigned char type)
{
 if(type < 2) return LXordering[type];
 else         return "";
}

static const char *  __FASTCALL__ GetCPUTypeLX(int num)
{
 if(num > 5) num = 0;
 return LXcputype[num];
}

static const char *  __FASTCALL__ GetOSTypeLX(int num)
{
  if(num > 4) num = 0;
  return LXostype[num];
}

static const char *  __FASTCALL__ __getOSModType(char type)
{
  return __osModType[type & 0x07];
}

static void  PaintNewHeaderLX_1(TWindow* w)
{
  w->printf(
	   "Signature                        = '%c%c'\n"
	   "Byte order                       = %02XH (%s)\n"
	   "Word order                       = %02XH (%s)\n"
	   "Format level                     = %08lXH\n"
	   "OS Type                          = %s\n"
	   "CPU Type                         = %s\n"
	   "Module version                   = %hu.%04X\n"
	   "Linear flags :                     [%08lXH]\n"
	   "  Contest DATA in EXE: %s\n"
	   "  [%c] Per-process library initializtion\n"
	   "  [%c] Internal fixups have been applied\n"
	   "  [%c] External fixups have been applied\n"
	   "  %s\n"
	   "  [%c] Module is not loadable (contains errors)\n"
	   "  Module type is : %s\n"
	   "  [%c] Symmetric Multi Processor mode disabled\n"
	   "  [%c] Per-process library termination\n"
	   "Number of pages                  = %08lXH\n"
	   "EIP objects number               = %08lXH\n"
	   "EIP                              = %08lXH\n"
	   "ESP objects number               = %08lXH\n"
	   "ESP                              = %08lXH"
	   ,lxe.lx.lxSignature[0],lxe.lx.lxSignature[1]
	   ,(int)lxe.lx.lxByteOrdering,GetOrderingLX(lxe.lx.lxByteOrdering)
	   ,(int)lxe.lx.lxWordOrdering,GetOrderingLX(lxe.lx.lxWordOrdering)
	   ,lxe.lx.lxFormatLevel
	   ,GetOSTypeLX(lxe.lx.lxOSType)
	   ,GetCPUTypeLX(lxe.lx.lxCPUType)
	   ,(unsigned short)(lxe.lx.lxModuleVersion >> 16),(unsigned)(unsigned short)(lxe.lx.lxModuleVersion)
	   ,lxe.lx.lxModuleFlags
	   ,__nedata[lxe.lx.lxModuleFlags & 0x0000003]
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000004L) == 0x00000004L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000010L) == 0x00000010L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000020L) == 0x00000020L)
	   ,GetPMWinAPI((unsigned)(lxe.lx.lxModuleFlags))
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00002000L) == 0x00002000L)
	   ,__getOSModType(((lxe.lx.lxModuleFlags & 0x00038000L) >> 15) & 0x07)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00080000L) == 0x00080000L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x40000000L) == 0x40000000L)
	   ,lxe.lx.lxPageCount
	   ,lxe.lx.lxEIPObjectNumbers
	   ,lxe.lx.lxEIP
	   ,lxe.lx.lxESPObjectNumbers
	   ,lxe.lx.lxESP);
}

static void  PaintNewHeaderLX_2(TWindow* w)
{
  w->printf(
	   "Page size                        = %08lXH\n"
	   "Page offset shift                = %08lXH\n"
	   "Fixup section size               = %08lXH\n"
	   ,lxe.lx.lxPageSize
	   ,lxe.lx.lxPageOffsetShift
	   ,lxe.lx.lxFixupSectionSize);
  if(LXType == FILE_LX) w->printf("Fixup section checksum           = %08lXH\n",lxe.lx.lxFixupSectionChecksum);
  else                  w->printf("Page checksum                    = %08lXH\n",lxe.lx.lxFixupSectionChecksum);
  w->printf(
	   "Loader section size              = %08lXH\n"
	   "Loader section checksum          = %08lXH\n"
	   "Object table offset              = %08lXH\n"
	   "Number of objects in module      = %08lXH\n"
	   "Object page table offset         = %08lXH\n"
	   "Object iter page  offset         = %08lXH\n"
	   "Resource table offset            = %08lXH\n"
	   "Number of resource table entries = %08lXH\n"
	   "Resident name table offset       = %08lXH\n"
	   "Entry table table offset         = %08lXH\n"
	   "Module directives offset         = %08lXH\n"
	   "Number module directives         = %08lXH\n"
	   "Fixup page table offset          = %08lXH\n"
	   "Fixup record table offset        = %08lXH\n"
	   "Import module table offset       = %08lXH\n"
	   "Import module table entries      = %08lXH\n"
	   "Import procedure table offset    = %08lXH"
	   ,lxe.lx.lxLoaderSectionSize
	   ,lxe.lx.lxLoaderSectionChecksum
	   ,lxe.lx.lxObjectTableOffset
	   ,lxe.lx.lxObjectCount
	   ,lxe.lx.lxObjectPageTableOffset
	   ,lxe.lx.lxObjectIterPageOffset
	   ,lxe.lx.lxResourceTableOffset
	   ,lxe.lx.lxNumberResourceTableEntries
	   ,lxe.lx.lxResidentNameTableOffset
	   ,lxe.lx.lxEntryTableOffset
	   ,lxe.lx.lxModuleDirectivesOffset
	   ,lxe.lx.lxNumberModuleDirectives
	   ,lxe.lx.lxFixupPageTableOffset
	   ,lxe.lx.lxFixupRecordTableOffset
	   ,lxe.lx.lxImportModuleTableOffset
	   ,lxe.lx.lxImportModuleTableEntries
	   ,lxe.lx.lxImportProcedureTableOffset);
}

static void  PaintNewHeaderLX_3(TWindow* w)
{
  w->printf(
	   "Per - page checksum  offset      = %08lXH\n"
	   "Data pages offset                = %08lXH\n"
	   "Number of preload pages          = %08lXH\n"
	   "Non resident name table offset   = %08lXH\n"
	   "Non resident name table length   = %08lXH\n"
	   "Non resident name table checksum = %08lXH\n"
	   ,lxe.lx.lxPerPageChecksumOffset
	   ,lxe.lx.lxDataPagesOffset
	   ,lxe.lx.lxNumberPreloadPages
	   ,lxe.lx.lxNonResidentNameTableOffset
	   ,lxe.lx.lxNonResidentNameTableLength
	   ,lxe.lx.lxNonResidentNameTableChecksum);
  if(LXType == FILE_LX)
  {
    w->printf(
	     "Auto DS objects number           = %08lXH\n"
	     "Debug info offset                = %08lXH\n"
	     "Debug info length                = %08lXH\n"
	     "Number instance preload          = %08lXH\n"
	     "Number instance demand           = %08lXH\n"
	     "Heap size                        = %08lXH\n"
	     "Stack size                       = %08lXH\n"
	     ,lxe.lx.lxAutoDSObjectNumber
	     ,lxe.lx.lxDebugInfoOffset
	     ,lxe.lx.lxDebugInfoLength
	     ,lxe.lx.lxNumberInstancePreload
	     ,lxe.lx.lxNumberInstanceDemand
	     ,lxe.lx.lxHeapSize
	     ,lxe.lx.lxStackSize);
  }
  else
  {
    w->printf(
	     "Debug info offset                = %08lXH\n"
	     "Debug info length                = %08lXH\n"
	     ,lxe.lx.lxAutoDSObjectNumber
	     ,lxe.lx.lxDebugInfoOffset);
  }
  w->set_color(dialog_cset.entry);
  w->printf(">Entry Point                     = %08lXH",LXEntryPoint);
  w->clreol();
  w->set_color(dialog_cset.main);
}

static void ( * lxphead[])(TWindow*) =
{
  PaintNewHeaderLX_1,
  PaintNewHeaderLX_2,
  PaintNewHeaderLX_3
};

static void __FASTCALL__ PaintNewHeaderLX(TWindow * win,const any_t**ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  win->freeze();
  win->clear();
  sprintf(text," Linear eXecutable Header [%d/%d] ",npage + 1,tpage);
  win->set_title(text,TWindow::TMode_Center,dialog_cset.title);
  win->set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
  if(npage < 3)
  {
    win->goto_xy(1,1);
    (*(lxphead[npage]))(win);
  }
  win->refresh_full();
}

__filesize_t __FASTCALL__ ShowNewHeaderLX()
{
  __filesize_t fpos;
  LXEntryPoint = LXType == FILE_LX ? CalcEntryPointLX(lxe.lx.lxEIPObjectNumbers,lxe.lx.lxEIP) : CalcEntryPointLE(lxe.lx.lxEIPObjectNumbers,lxe.lx.lxEIP);
  if(LXEntryPoint == FILESIZE_MAX) LXEntryPoint = 0;
  fpos = BMGetCurrFilePos();
  if(PageBox(70,21,NULL,3,PaintNewHeaderLX) != -1)
  {
    if(LXEntryPoint) fpos = LXEntryPoint;
  }
  return fpos;
}

unsigned __FASTCALL__ LXRNamesNumItems(binary_stream& handle)
{
  return GetNamCountNE(handle,beye_context().headshift + lxe.lx.lxResidentNameTableOffset);
}

unsigned __FASTCALL__ LXNRNamesNumItems(binary_stream& handle)
{
  return GetNamCountNE(handle,lxe.lx.lxNonResidentNameTableOffset);
}

bool __FASTCALL__ LXRNamesReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
{
   return RNamesReadItems(handle,obj,nnames,lxe.lx.lxResidentNameTableOffset + beye_context().headshift);
}

static unsigned __FASTCALL__ LXImpNamesNumItems(binary_stream& handle)
{
  __filesize_t fpos;
  unsigned char len;
  unsigned count;
  handle.seek(lxe.lx.lxImportProcedureTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  fpos = handle.tell();
  count = 0;
  while(fpos < lxe.lx.lxFixupSectionSize + lxe.lx.lxFixupPageTableOffset + beye_context().headshift)
  {
    len = handle.read(type_byte);
    handle.seek(len,binary_stream::Seek_Cur);
    fpos = handle.tell();
    if(handle.eof()) break;
    count++;
  }
  return count;
}

static bool __FASTCALL__ LXImpNamesReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 unsigned char byte;
 handle.seek(lxe.lx.lxImportProcedureTableOffset + beye_context().headshift,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   char nam[256];
   byte = handle.read(type_byte);
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read(nam,byte);
   nam[byte] = 0;
   if(!ma_AddString(obj,nam,true)) break;
 }
 return true;
}


bool __FASTCALL__ LXNRNamesReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
{
   return RNamesReadItems(handle,obj,nnames,lxe.lx.lxNonResidentNameTableOffset);
}

#if 0
extern unsigned __FASTCALL__ RNameReadFull(binary_stream& handle,char * names,unsigned nindex,__filesize_t _offset);
static unsigned __FASTCALL__ LXRNamesReadFullName(binary_stream& handle,char * names,unsigned index)
{
   return RNameReadFull(handle,names,index,lxe.lx.lxResidentNameTableOffset + beye_context().headshift);
}

static unsigned __FASTCALL__ LXNRNamesReadFullName(binary_stream& handle,char * names,unsigned index)
{
   return RNameReadFull(handle,names,index,lxe.lx.lxNonResidentNameTableOffset);
}
#endif
static bool __FASTCALL__  __ReadModRefNamesLX(binary_stream& handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 unsigned char byte;
 handle.seek(lxe.lx.lxImportModuleTableOffset + beye_context().headshift,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   char nam[256];
   byte = handle.read(type_byte);
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read(nam,byte);
   nam[byte] = 0;
   if(!ma_AddString(obj,nam,true)) break;
 }
 return true;
}

static void  __FASTCALL__ objpaintLX(TWindow* w,const LX_OBJECT *nam)
{
 w->goto_xy(1,1);
 w->printf(
	  "Virtual Size                         = %lX bytes\n"
	  "BVA (base virtual address)           = %08lX\n"
	  "FLAGS: %lX\n"
	  "   [%c] Readable object\n"
	  "   [%c] Writable object\n"
	  "   [%c] Executable object\n"
	  "   [%c] Resource object\n"
	  "   [%c] Object is discardable\n"
	  "   [%c] Object is shared\n"
	  "   [%c] Object has preload pages\n"
	  "   [%c] Object has invalid pages\n"
	  "   [%c] Object is permanent and swappable\n"
	  "   [%c] Object is permanent and resident\n"
	  "   [%c] Object is permanent and long lockable\n"
	  "   [%c] 16:16 alias required (80x86 specific)\n"
	  "   [%c] Big/Default bit setting (80x86 specific)\n"
	  "   [%c] Object is conforming for code (80x86 specific)\n"
	  "   [%c] Object I/O privilege level (80x86 specific)\n"
	  "   [%c] Object is loadable to High Memory (>512MiB)\n"
	  "Object page map index                = %lu\n"
	  "Number of entries in object page map = %lu"
	  ,nam->o32_size
	  ,nam->o32_base
	  ,nam->o32_flags
	  ,Gebool((nam->o32_flags & 0x00000001L) == 0x00000001L)
	  ,Gebool((nam->o32_flags & 0x00000002L) == 0x00000002L)
	  ,Gebool((nam->o32_flags & 0x00000004L) == 0x00000004L)
	  ,Gebool((nam->o32_flags & 0x00000008L) == 0x00000008L)
	  ,Gebool((nam->o32_flags & 0x00000010L) == 0x00000010L)
	  ,Gebool((nam->o32_flags & 0x00000020L) == 0x00000020L)
	  ,Gebool((nam->o32_flags & 0x00000040L) == 0x00000040L)
	  ,Gebool((nam->o32_flags & 0x00000080L) == 0x00000080L)
	  ,Gebool((nam->o32_flags & 0x00000100L) == 0x00000100L)
	  ,Gebool((nam->o32_flags & 0x00000200L) == 0x00000200L)
	  ,Gebool((nam->o32_flags & 0x00000400L) == 0x00000400L)
	  ,Gebool((nam->o32_flags & 0x00001000L) == 0x00001000L)
	  ,Gebool((nam->o32_flags & 0x00002000L) == 0x00002000L)
	  ,Gebool((nam->o32_flags & 0x00004000L) == 0x00004000L)
	  ,Gebool((nam->o32_flags & 0x00008000L) == 0x00008000L)
	  ,Gebool((nam->o32_flags & 0x00010000L) == 0x00010000L)
	  ,nam->o32_pagemap
	  ,nam->o32_mapsize);
}

static void __FASTCALL__ ObjPaintLX(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const LX_OBJECT ** nam = (const LX_OBJECT **)names;
 win->freeze();
 win->clear();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 win->set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
 win->set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
 objpaintLX(win,nam[start]);
 win->refresh_full();
}

static bool  __FASTCALL__ __ReadObjectsLX(binary_stream& handle,memArray * obj,unsigned n)
{
 size_t i;
  for(i = 0;i < n;i++)
  {
    LX_OBJECT lxo;
    if(IsKbdTerminate() || handle.eof()) break;
    handle.read(&lxo,sizeof(LX_OBJECT));
    if(!ma_AddData(obj,&lxo,sizeof(LX_OBJECT),true)) break;
  }
  return true;
}

static bool  __FASTCALL__ __ReadEntriesLX(binary_stream& handle,memArray *obj)
{
 unsigned i;
 unsigned char j;
 unsigned char cnt,type;
 uint_fast16_t numobj = 0;
 LX_ENTRY _lxe;
 i = 0;
 while(1)
 {
   bool is_eof;
   is_eof = false;
   cnt = handle.read(type_byte);
   type = handle.read(type_byte);
   if(!cnt) break;
   if(type) numobj = handle.read(type_word);
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
     is_eof = handle.eof();
     if(IsKbdTerminate() || is_eof) goto exit;
     _lxe.b32_type = type;
     if(size)
     {
       _lxe.b32_obj = numobj;
       _lxe.entry.e32_flags = handle.read(type_byte);
       handle.read(&_lxe.entry.e32_variant,size);
     }
     if(!ma_AddData(obj,&_lxe,sizeof(LX_ENTRY),true)) goto exit;
   }
   if(is_eof) break;
 }
 exit:
 return true;
}

static void __FASTCALL__ lxReadPageDesc(binary_stream& handle,LX_MAP_TABLE *mt,unsigned long pageidx)
{
  handle.seek(beye_context().headshift+lxe.lx.lxObjectPageTableOffset+
	  sizeof(LX_MAP_TABLE)*(pageidx - 1),binary_stream::Seek_Set);
  handle.read((any_t*)mt,sizeof(LX_MAP_TABLE));
}

static __filesize_t  __FASTCALL__ __calcPageEntry(LX_MAP_TABLE *mt)
{
  __filesize_t dataoff;
  __filesize_t ret;
  dataoff = mt->o32_pagedataoffset << lxe.lx.lxPageOffsetShift;
  switch(mt->o32_pageflags)
  {
    default:
    case PAGE_VALID: ret = lxe.lx.lxDataPagesOffset;
		     break;
    case PAGE_ITERDATA2: /* This is very undocumented type.
			    I do not know - how handle it !!!*/
    case PAGE_ITERDATA: ret = lxe.lx.lxObjectIterPageOffset;
			break;
    case PAGE_RANGE:
    case PAGE_INVALID:
    case PAGE_ZEROED:  ret = 0; break;
  }
  return ret + dataoff;
}

static __filesize_t  __FASTCALL__ CalcPageEntry(unsigned long pageidx)
{
  binary_stream& handle = *lx_cache;
  LX_MAP_TABLE mt;
  if(!pageidx) return -1;
  lxReadPageDesc(handle,&mt,pageidx);
  return __calcPageEntry(&mt);
}

static __filesize_t  __FASTCALL__ CalcEntryPointLX(unsigned long objnum,__filesize_t _offset)
{
  binary_stream& handle = *lx_cache;
  unsigned long i,diff;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  if(!objnum) return BMGetCurrFilePos();
  handle.seek(lxe.lx.lxObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  handle.seek(sizeof(LX_OBJECT)*(objnum - 1),binary_stream::Seek_Cur);
  handle.read((any_t*)&lo,sizeof(LX_OBJECT));
  i = _offset / lxe.lx.lxPageSize;
  diff = _offset - i*lxe.lx.lxPageSize;
  lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
  return __calcPageEntry(&mt) + diff;
}

static void __FASTCALL__ ReadLXLEImpMod(__filesize_t offtable,unsigned num,char *str)
{
  binary_stream* handle;
  unsigned i;
  unsigned char len;
  char buff[256];
  handle = lx_cache;
  handle->seek(offtable,binary_stream::Seek_Set);
  for(i = 1;i < num;i++)
  {
    len = handle->read(type_byte);
    handle->seek(len,binary_stream::Seek_Cur);
  }
  len = handle->read(type_byte);
  handle->read((any_t*)buff,len);
  buff[len] = 0;
  strcat(str,buff);
}

static void __FASTCALL__ ReadLXLEImpName(__filesize_t offtable,unsigned num,char *str)
{
  binary_stream* handle;
  unsigned char len;
  char buff[256];
  handle = lx_cache;
  handle->seek(offtable+num,binary_stream::Seek_Set);
  len = handle->read(type_byte);
  handle->read((any_t*)buff,len);
  buff[len] = 0;
  strcat(str,buff);
}

void __FASTCALL__ ShowFwdModOrdLX(const LX_ENTRY *lxent)
{
  char buff[513];
  buff[0] = 0;
  ReadLXLEImpMod(lxe.lx.lxImportModuleTableOffset + beye_context().headshift,lxent->entry.e32_variant.e32_fwd.modord,buff);
  strcat(buff,".");
  if((lxent->entry.e32_flags & 0x01) == 0x01)
  {
    sprintf(&buff[strlen(buff)],"@%u",(unsigned)lxent->entry.e32_variant.e32_fwd.value);
  }
  else ReadLXLEImpName(lxe.lx.lxImportProcedureTableOffset + beye_context().headshift,(unsigned)lxent->entry.e32_variant.e32_fwd.value,buff);
  beye_context().TMessageBox(buff," Forwarder entry point ");
}

static __filesize_t  __FASTCALL__ CalcEntryLX(const LX_ENTRY *lxent)
{
  __filesize_t ret;
  ret = BMGetCurrFilePos();
      switch(lxent->b32_type)
      {
	case 1: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset16);
		      break;
	case 2: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_callgate.offset);
		      break;
	case 3: ret = CalcEntryPointLX(lxent->b32_obj,lxent->entry.e32_variant.e32_offset.offset32);
		      break;
	case 4: ShowFwdModOrdLX(lxent);
	case 5:
	default: break;
      }
  return ret;
}

static __filesize_t  __FASTCALL__ CalcEntryBungleLX(unsigned ordinal,bool dispmsg)
{
  binary_stream* handle;
  bool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  uint_fast16_t numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = BMGetCurrFilePos();
  handle = lx_cache;
  handle->seek(lxe.lx.lxEntryTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  i = 0;
  found = false;
  while(1)
  {
   bool is_eof;
   is_eof = false;
   cnt = handle->read(type_byte);
   type = handle->read(type_byte);
   if(!cnt) break;
   if(type) numobj = handle->read(type_word);
   if(handle->eof()) break;
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
	 is_eof = handle->eof();
       }
       break;
     }
     else
       if(size) handle->seek(size + sizeof(char),binary_stream::Seek_Cur);
     if(is_eof) break;
   }
   if(found || is_eof) break;
 }
 if(found) ret = CalcEntryLX((LX_ENTRY *)&lxent);
 else      if(dispmsg) beye_context().ErrMessageBox(NOT_ENTRY,"");
 return ret;
}

__filesize_t __FASTCALL__ ShowObjectsLX()
{
 binary_stream& handle = *lx_cache;
 __filesize_t fpos;
 unsigned nnames;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 nnames = (unsigned)lxe.lx.lxObjectCount;
 if(!nnames) { beye_context().NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,true))) return fpos;
 handle.seek(lxe.lx.lxObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
 if(__ReadObjectsLX(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,20,(const any_t**)obj->data,obj->nItems,ObjPaintLX);
    if(ret != -1)  fpos = LXType == FILE_LX ? CalcPageEntry(((LX_OBJECT *)obj->data[ret])->o32_pagemap) : CalcPageEntryLE(((const LX_OBJECT *)obj->data[ret])->o32_pagemap);
 }
 ma_Destroy(obj);
 return fpos;
}

const char * mapattr[] =
{
"Valid Physical Page In .EXE",
"Iterated Data Page",
"Invalid Page",
"Zero Filled Page",
"Range Of Pages",
"Iterated Data Page Type II"
};

const char * __FASTCALL__ lxeGetMapAttr(unsigned long attr)
{
  if (attr > 5) return "Unknown";
  else  return mapattr[attr];
}

const char *__e32type[] =
{
  "Empty",
  "Entry16",
  "Gate16",
  "Entry32",
  "EntryForwarder",
  "TypeInfo"
};

static const char *  __FASTCALL__ entryTypeLX(unsigned char type)
{
   if(type < 6) return __e32type[type];
   else         return "Unknown";
}

static void  __FASTCALL__ entrypaintLX(TWindow* w,const LX_ENTRY *nam)
{
 if(!nam->b32_type)
 {
   w->goto_xy(35,4);
   w->printf("Unused");
 }
 else
 {
   w->goto_xy(1,1);
   w->printf(
	    "Entry type: %s\n"
	    "Object number : %hd\n"
	    "Flags: %02XH\n"
	    ,entryTypeLX(nam->b32_type)
	    ,nam->b32_obj
	    ,(int)nam->entry.e32_flags);
   if(nam->b32_type != 4)
   {
     w->printf(
	      "   [%c] Exported Entry\n"
	      "   [%c] Used Shared Data\n"
	      "   %02XH - parameter word count mask\n"
	      ,Gebool((nam->entry.e32_flags & 0x01) == 0x01)
	      ,Gebool((nam->entry.e32_flags & 0x02) == 0x02)
	      ,(int)(nam->entry.e32_flags >> 2));
   }
   else
   {
     w->printf(
	      "\n"
	      "   [%c] Import by ordinal\n"
	      "\n"
	      ,Gebool((nam->entry.e32_flags & 0x01) == 0x01));
   }
   if(nam->b32_type == 1)
   {
     w->printf(
	      "Entry offset : %04hXH\n"
	      "\n"
	      ,nam->entry.e32_variant.e32_offset.offset16);
   }
   else
      if(nam->b32_type == 3)
      {
	w->printf(
		"Entry offset : %08XH\n"
		 "\n"
		 ,nam->entry.e32_variant.e32_offset.offset32);
      }
      else
      if(nam->b32_type == 2)
      {
       w->printf(
		"Callgate offset : %04hXH\n"
		"Callgate selector : %04hXH\n"
		,nam->entry.e32_variant.e32_callgate.offset
		,nam->entry.e32_variant.e32_callgate.callgate);
      }
      else
       if(nam->b32_type == 4)
       {
	 w->printf(
		  "Module ordinal number : %04hXH\n"
		  "Proc name offset or ordinal : %04hXH\n"
		  ,nam->entry.e32_variant.e32_fwd.modord
		  ,nam->entry.e32_variant.e32_fwd.value);
       }
   }
}

static void __FASTCALL__ PaintEntriesLX(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const LX_ENTRY ** nam = (const LX_ENTRY **)names;
 win->freeze();
 win->clear();
 sprintf(buffer," Entries Table [ %u / %u ] ",start + 1,nlist);
 win->set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
 win->set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
 entrypaintLX(win,nam[start]);
 win->refresh_full();
}

static bool __FASTCALL__ __ReadMapTblLX(binary_stream& handle,memArray * obj,unsigned n)
{
  size_t i;
  for(i = 0;i < n;i++)
  {
    LX_MAP_TABLE mt;
    char stmp[80];
    if(IsKbdTerminate() || handle.eof()) break;
    lxReadPageDesc(handle,&mt,i+1);
    sprintf(stmp,"Off=%08lXH Siz=%04hXH Flg:%04hXH=%s",
		 (unsigned long)mt.o32_pagedataoffset,
		 mt.o32_pagesize,
		 mt.o32_pageflags,
		 (const char *)lxeGetMapAttr(mt.o32_pageflags));
    if(!ma_AddString(obj,(const char *)stmp,true)) break;
  }
  return true;
}
#if 0
static bool  __FASTCALL__ __ReadIterTblLX(binary_stream& handle,memArray * obj,unsigned n)
{
 int i;
  for(i = 0;i < n;i++)
  {
    LX_ITER lxi;
    if(IsKbdTerminate() || handle->eof()) break;
    handle->read(&lxi,sizeof(LX_ITER));
    if(!ma_AddData(obj,&lxi,sizeof(LX_ITER),true)) break;
  }
  return true;
}
#endif
static __filesize_t __FASTCALL__ ShowMapTableLX()
{
    __filesize_t fpos = BMGetCurrFilePos();
    int ret;
    std::string title = " Map of pages ";
    ssize_t nnames = (unsigned)lxe.lx.lxPageCount;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadMapTblLX(bmbioHandle(),obj,nnames);
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

__filesize_t __FASTCALL__ ShowEntriesLX()
{
 binary_stream& handle = *lx_cache;
 __filesize_t fpos;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 if(!lxe.lx.lxEntryTableOffset) { beye_context().NotifyBox(NOT_ENTRY," Entry Table "); return fpos; }
 handle.seek(lxe.lx.lxEntryTableOffset + beye_context().headshift,binary_stream::Seek_Set);
 if(!(obj = ma_Build(0,true))) goto exit;
 if(__ReadEntriesLX(handle,obj))
 {
    int ret;
    if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY," Entry Table "); goto bye; }
    ret = PageBox(70,8,(const any_t**)obj->data,obj->nItems,PaintEntriesLX);
    if(ret != -1)  fpos = LXType == FILE_LX ? CalcEntryLX((const LX_ENTRY*)obj->data[ret]) : CalcEntryLE((const LX_ENTRY*)obj->data[ret]);
 }
 bye:
 ma_Destroy(obj);
 exit:
 return fpos;
}

const char * ResourceGrNamesLX[] =
{
  "RESERVED 0",
  "Mouse pointer share",
  "BITMAP",
  "MENU",
  "DIALOG",
  "STRINGTABLE",
  "FONTDIR",
  "FONT",
  "ACCELTABLE",
  "RCDATA",
  "Err msg table",
  "DlgInclude",
  "VKey table",
  "Key to UGL table",
  "Glyph to char table",
  "Display info",
  "FKA short",
  "FKA long",
  "Help table",
  "Help subtable",
  "DBCS font drive dir",
  "DBCS font drive"
};

static bool  __FASTCALL__ __ReadResourceGroupLX(binary_stream& handle,memArray *obj,unsigned nitems,long * addr)
{
 unsigned i;
 LXResource lxr;
 for(i = 0;i < nitems;i++)
 {
    char stmp[81];
    handle.read(&lxr,sizeof(LXResource));
    addr[i] = lxr.offset;
    if(IsKbdTerminate() || handle.eof()) break;
    sprintf(stmp,"%6hu = ",lxr.nameID);
    if(lxr.typeID < sizeof(ResourceGrNamesLX)/sizeof(char *)) strcat(stmp,ResourceGrNamesLX[lxr.typeID]);
    else  sprintf(&stmp[strlen(stmp)],"Unknown < %04hXH >",lxr.typeID);
    sprintf(&stmp[strlen(stmp)]," obj=%04hXH.%08lXH"
			       ,lxr.object
			       ,(unsigned long)lxr.offset);
    if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

static __filesize_t __FASTCALL__ ShowResourceLX()
{
 __filesize_t fpos;
 binary_stream& handle = *lx_cache;
 memArray * obj;
 long * raddr;
 unsigned nrgroup;
 fpos = BMGetCurrFilePos();
 handle.seek((__fileoff_t)beye_context().headshift + lxe.lx.lxResourceTableOffset,binary_stream::Seek_Set);
 nrgroup = (unsigned)lxe.lx.lxNumberResourceTableEntries;
 if(!nrgroup) { beye_context().NotifyBox(NOT_ENTRY," Resources "); return fpos; }
 if(!(obj = ma_Build(nrgroup,true))) goto exit;
 if(!(raddr  = new long [nrgroup])) return fpos;
 if(__ReadResourceGroupLX(handle,obj,nrgroup,raddr))
 {
   ma_Display(obj," Resource groups : ",LB_SORTABLE,-1);
 }
 ma_Destroy(obj);
 delete raddr;
 exit:
 return fpos;
}

__filesize_t __FASTCALL__ ShowModRefLX()
{
    std::string title = MOD_REFER;
    ssize_t nnames = (unsigned)lxe.lx.lxImportModuleTableEntries;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadModRefNamesLX(bmbioHandle(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ ShowResNamLX()
{
    __filesize_t fpos = BMGetCurrFilePos();
    int ret;
    unsigned ordinal;
    std::string title = RES_NAMES;
    ssize_t nnames = LXRNamesNumItems(bmbioHandle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXRNamesReadItems(bmbioHandle(),obj,nnames);
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
    if(ret != -1) fpos = CalcEntryBungleLX(ordinal,true);
    return fpos;
}

static __filesize_t __FASTCALL__ ShowNResNmLX()
{
    __filesize_t fpos = BMGetCurrFilePos();
    int ret;
    unsigned ordinal;
    std::string title = NORES_NAMES;
    ssize_t nnames = LXNRNamesNumItems(bmbioHandle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXNRNamesReadItems(bmbioHandle(),obj,nnames);
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
    if(ret != -1) fpos = CalcEntryBungleLX(ordinal,true);
    return fpos;
}

__filesize_t __FASTCALL__ ShowImpProcLXLE()
{
    std::string title = IMPPROC_TABLE;
    ssize_t nnames = LXImpNamesNumItems(bmbioHandle());
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = LXImpNamesReadItems(bmbioHandle(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return BMGetCurrFilePos();
}

static bool __FASTCALL__ isLX()
{
   char id[4];
   beye_context().headshift = IsNewExe();
   bmReadBufferEx(id,sizeof(id),beye_context().headshift,binary_stream::Seek_Set);
   if(id[0] == 'L' && id[1] == 'X' && id[2] == 0 && id[3] == 0) return true;
   return false;
}

static void __FASTCALL__ LXinit(CodeGuider& code_guider)
{
    UNUSED(code_guider);
   binary_stream& main_handle = bmbioHandle();
   LXType = FILE_LX;
   bmReadBufferEx(&lxe.lx,sizeof(LXHEADER),beye_context().headshift,binary_stream::Seek_Set);
   if((lx_cache = main_handle.dup()) == &bNull) lx_cache = &main_handle;
}

static void __FASTCALL__ LXdestroy()
{
   binary_stream& main_handle = bmbioHandle();
   if(lx_cache != &bNull && lx_cache != &main_handle) delete lx_cache;
}

static __filesize_t __FASTCALL__ LXHelp()
{
  hlpDisplay(10005);
  return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ lxVA2PA(__filesize_t va)
{
 /* First we must determine object number for given virtual address */
  binary_stream& handle = *lx_cache;
  unsigned long i,diff,oidx;
  __filesize_t rva,pa;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  handle.seek(lxe.lx.lxObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
  pa = oidx = 0; /* means: error */
  for(i = 0;i < lxe.lx.lxObjectCount;i++)
  {
    handle.read((any_t*)&lo,sizeof(LX_OBJECT));
    if(lo.o32_base <= va && va < lo.o32_base + lo.o32_size)
    {
      oidx = i+1;
      break;
    }
  }
  /* Secondly we must determine page within object */
  if(oidx)
  {
    rva = va - lo.o32_base;
    i = rva / lxe.lx.lxPageSize;
    diff = rva - i*lxe.lx.lxPageSize;
    lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
    pa = __calcPageEntry(&mt) + diff;
  }
  return pa;
}

static __filesize_t __FASTCALL__ lxPA2VA(__filesize_t pa)
{
 /* First we must determine page for given physical address */
  binary_stream& handle = *lx_cache;
  unsigned long i,pidx,pagentry;
  __filesize_t rva,va;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  pagentry = va = pidx = 0;
  for(i = 0;i < lxe.lx.lxPageCount;i++)
  {
    lxReadPageDesc(handle,&mt,i+1);
    pagentry = __calcPageEntry(&mt);
    if(pagentry <= pa && pa < pagentry + mt.o32_pagesize)
    {
      pidx = i+1;
      break;
    }
  }
  /* Secondly we must determine object number for given physical address */
  if(pidx)
  {
    rva = pa - pagentry + (pidx-1)*lxe.lx.lxPageSize;
    handle.seek(lxe.lx.lxObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
    for(i = 0;i < lxe.lx.lxObjectCount;i++)
    {
      handle.read((any_t*)&lo,sizeof(LX_OBJECT));
      if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize)
      {
	va = lo.o32_base + rva;
	break;
      }
    }
  }
  return va;
}

static int __FASTCALL__ lxBitness(__filesize_t pa)
{
 /* First we must determine page for given physical address */
  binary_stream& handle = *lx_cache;
  unsigned long i,pidx,pagentry;
  int ret;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  pagentry = pidx = 0;
  ret = DAB_USE16;
  for(i = 0;i < lxe.lx.lxPageCount;i++)
  {
    lxReadPageDesc(handle,&mt,i+1);
    pagentry = __calcPageEntry(&mt);
    if(pagentry <= pa && pa < pagentry + mt.o32_pagesize)
    {
      pidx = i+1;
      break;
    }
  }
  /* Secondly we must determine object number for given physical address */
  if(pidx)
  {
    handle.seek(lxe.lx.lxObjectTableOffset + beye_context().headshift,binary_stream::Seek_Set);
    for(i = 0;i < lxe.lx.lxObjectCount;i++)
    {
      handle.read((any_t*)&lo,sizeof(LX_OBJECT));
      if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize)
      {
	ret = (lo.o32_flags & 0x00002000L) == 0x00002000L ? DAB_USE32 : DAB_USE16;
	break;
      }
    }
  }
  return ret;
}

static bool __FASTCALL__ lxAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos >= beye_context().headshift && cfpos < beye_context().headshift + sizeof(LXHEADER))
  {
    strcpy(addr,"LXH :");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift));
  }
  else
  if(cfpos >= beye_context().headshift + lxe.lx.lxObjectTableOffset &&
     cfpos <  beye_context().headshift + lxe.lx.lxObjectTableOffset + sizeof(LX_OBJECT)*lxe.lx.lxObjectCount)
  {
    strcpy(addr,"LXOD:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - lxe.lx.lxObjectTableOffset));
  }
  else
  if(cfpos >= beye_context().headshift + lxe.lx.lxObjectPageTableOffset &&
     cfpos <  beye_context().headshift + lxe.lx.lxObjectPageTableOffset + sizeof(LX_MAP_TABLE)*lxe.lx.lxPageCount)
  {
    strcpy(addr,"LXPD:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - lxe.lx.lxObjectPageTableOffset));
  }
  else
   if((res=lxPA2VA(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = false;
  return bret;
}

static int __FASTCALL__ lxPlatform() { return DISASM_CPU_IX86; }

extern const REGISTRY_BIN lxTable =
{
  "LX (Linear eXecutable)",
  { "LXhelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "ResTbl", "LXHead", "MapTbl", "Object" },
  { LXHelp, ShowModRefLX, ShowResNamLX, ShowNResNmLX, ShowImpProcLXLE, ShowEntriesLX, ShowResourceLX, ShowNewHeaderLX, ShowMapTableLX, ShowObjectsLX },
  isLX, LXinit, LXdestroy,
  NULL,
  NULL,
  lxPlatform,
  lxBitness,
  NULL,
  lxAddressResolv,
  lxVA2PA,
  lxPA2VA,
  NULL,
  NULL
};
} // namespace	usr
