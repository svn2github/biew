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
#include <sstream>
#include <iomanip>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colorset.h"
#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "udn.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "bconsole.h"
#include "listbox.h"
#include "libbeye/kbd_code.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
static const char* txt[]={ "LXhelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "ResTbl", "LXHead", "MapTbl", "Object" };
const char* LX_Parser::prompt(unsigned idx) const { return txt[idx]; }

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

std::string LX_Parser::GetOrderingLX(unsigned char type) const
{
 if(type < 2) return LXordering[type];
 else         return "";
}

std::string LX_Parser::GetCPUTypeLX(int num) const
{
 if(num > 5) num = 0;
 return LXcputype[num];
}

std::string LX_Parser::GetOSTypeLX(int num) const
{
  if(num > 4) num = 0;
  return LXostype[num];
}

std::string LX_Parser::__getOSModType(char type) const
{
  return __osModType[type & 0x07];
}

void LX_Parser::PaintNewHeaderLX_1(TWindow& w) const
{
  w.printf(
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
	   ,(int)lxe.lx.lxByteOrdering,GetOrderingLX(lxe.lx.lxByteOrdering).c_str()
	   ,(int)lxe.lx.lxWordOrdering,GetOrderingLX(lxe.lx.lxWordOrdering).c_str()
	   ,lxe.lx.lxFormatLevel
	   ,GetOSTypeLX(lxe.lx.lxOSType).c_str()
	   ,GetCPUTypeLX(lxe.lx.lxCPUType).c_str()
	   ,(unsigned short)(lxe.lx.lxModuleVersion >> 16),(unsigned)(unsigned short)(lxe.lx.lxModuleVersion)
	   ,lxe.lx.lxModuleFlags
	   ,NE_Parser::__nedata[lxe.lx.lxModuleFlags & 0x0000003]
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000004L) == 0x00000004L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000010L) == 0x00000010L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00000020L) == 0x00000020L)
	   ,NE_Parser::GetPMWinAPI((unsigned)(lxe.lx.lxModuleFlags)).c_str()
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00002000L) == 0x00002000L)
	   ,__getOSModType(((lxe.lx.lxModuleFlags & 0x00038000L) >> 15) & 0x07).c_str()
	   ,Gebool((lxe.lx.lxModuleFlags & 0x00080000L) == 0x00080000L)
	   ,Gebool((lxe.lx.lxModuleFlags & 0x40000000L) == 0x40000000L)
	   ,lxe.lx.lxPageCount
	   ,lxe.lx.lxEIPObjectNumbers
	   ,lxe.lx.lxEIP
	   ,lxe.lx.lxESPObjectNumbers
	   ,lxe.lx.lxESP);
}

void LX_Parser::PaintNewHeaderLX_2(TWindow& w) const
{
  w.printf(
	   "Page size                        = %08lXH\n"
	   "Page offset shift                = %08lXH\n"
	   "Fixup section size               = %08lXH\n"
	   ,lxe.lx.lxPageSize
	   ,lxe.lx.lxPageOffsetShift
	   ,lxe.lx.lxFixupSectionSize);
  w.printf("Page checksum                    = %08lXH\n",lxe.lx.lxFixupSectionChecksum);
  w.printf(
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

void LX_Parser::PaintNewHeaderLX_3(TWindow& w) const
{
  w.printf(
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
    w.printf(
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
  w.set_color(dialog_cset.entry);
  w.printf(">Entry Point                     = %08lXH",LXEntryPoint);
  w.clreol();
  w.set_color(dialog_cset.main);
}

void (LX_Parser::*LX_Parser::lxphead[])(TWindow&) const =
{
    &LX_Parser::PaintNewHeaderLX_1,
    &LX_Parser::PaintNewHeaderLX_2,
    &LX_Parser::PaintNewHeaderLX_3
};

void LX_Parser::PaintNewHeaderLX(TWindow& win,const std::vector<std::string>& ptr,unsigned npage) const
{
    UNUSED(ptr);
    win.freeze();
    win.clear();
    std::ostringstream oss;
    oss<<" Linear eXecutable Header ["<<(npage+1)<<"/"<<ptr.size()<<"] ";
    win.set_title(oss.str(),TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    if(npage < 3) {
	win.goto_xy(1,1);
	(this->*lxphead[npage])(win);
    }
    win.refresh_full();
}

__filesize_t LX_Parser::action_F8()
{
    __filesize_t fpos;
    LXEntryPoint = CalcEntryPoint(lxe.lx.lxEIPObjectNumbers,lxe.lx.lxEIP);
    if(LXEntryPoint == FILESIZE_MAX) LXEntryPoint = 0;
    fpos = bctx().tell();
    std::vector<std::string> v;
    v.push_back("");
    v.push_back("");
    v.push_back("");
    if(PageBox(70,21,v,*this,&LX_Parser::PaintNewHeaderLX) != -1) {
	if(LXEntryPoint) fpos = LXEntryPoint;
    }
    return fpos;
}

unsigned LX_Parser::LXRNamesNumItems(binary_stream& handle) const
{
    return NE_Parser::GetNamCountNE(handle,headshift() + lxe.lx.lxResidentNameTableOffset);
}

unsigned LX_Parser::LXNRNamesNumItems(binary_stream& handle) const
{
    return NE_Parser::GetNamCountNE(handle,lxe.lx.lxNonResidentNameTableOffset);
}

std::vector<std::string> LX_Parser::LXRNamesReadItems(binary_stream& handle,size_t nnames) const
{
    return NE_Parser::RNamesReadItems(handle,nnames,lxe.lx.lxResidentNameTableOffset + headshift());
}

unsigned LX_Parser::LXImpNamesNumItems(binary_stream& handle) const
{
  __filesize_t fpos;
  unsigned char len;
  unsigned count;
  handle.seek(lxe.lx.lxImportProcedureTableOffset + headshift(),binary_stream::Seek_Set);
  fpos = handle.tell();
  count = 0;
  while(fpos < lxe.lx.lxFixupSectionSize + lxe.lx.lxFixupPageTableOffset + headshift())
  {
    len = handle.read(type_byte);
    handle.seek(len,binary_stream::Seek_Cur);
    fpos = handle.tell();
    if(handle.eof()) break;
    count++;
  }
  return count;
}

std::vector<std::string> LX_Parser::LXImpNamesReadItems(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    size_t i;
    unsigned char byte;
    handle.seek(lxe.lx.lxImportProcedureTableOffset + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	char nam[256];
	byte = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(nam,byte);
	nam[byte] = 0;
	rc.push_back(nam);
    }
    return rc;
}

std::vector<std::string> LX_Parser::LXNRNamesReadItems(binary_stream& handle,size_t nnames) const
{
    return NE_Parser::RNamesReadItems(handle,nnames,lxe.lx.lxNonResidentNameTableOffset);
}

std::vector<std::string> LX_Parser::__ReadModRefNamesLX(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned i;
    unsigned char byte;
    handle.seek(lxe.lx.lxImportModuleTableOffset + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	char nam[256];
	byte = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(nam,byte);
	nam[byte] = 0;
	rc.push_back(nam);
    }
    return rc;
}

void LX_Parser::objpaintLX(TWindow& w,const LX_OBJECT& nam) const
{
    w.goto_xy(1,1);
    w.printf(
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
	  ,nam.o32_size
	  ,nam.o32_base
	  ,nam.o32_flags
	  ,Gebool((nam.o32_flags & 0x00000001L) == 0x00000001L)
	  ,Gebool((nam.o32_flags & 0x00000002L) == 0x00000002L)
	  ,Gebool((nam.o32_flags & 0x00000004L) == 0x00000004L)
	  ,Gebool((nam.o32_flags & 0x00000008L) == 0x00000008L)
	  ,Gebool((nam.o32_flags & 0x00000010L) == 0x00000010L)
	  ,Gebool((nam.o32_flags & 0x00000020L) == 0x00000020L)
	  ,Gebool((nam.o32_flags & 0x00000040L) == 0x00000040L)
	  ,Gebool((nam.o32_flags & 0x00000080L) == 0x00000080L)
	  ,Gebool((nam.o32_flags & 0x00000100L) == 0x00000100L)
	  ,Gebool((nam.o32_flags & 0x00000200L) == 0x00000200L)
	  ,Gebool((nam.o32_flags & 0x00000400L) == 0x00000400L)
	  ,Gebool((nam.o32_flags & 0x00001000L) == 0x00001000L)
	  ,Gebool((nam.o32_flags & 0x00002000L) == 0x00002000L)
	  ,Gebool((nam.o32_flags & 0x00004000L) == 0x00004000L)
	  ,Gebool((nam.o32_flags & 0x00008000L) == 0x00008000L)
	  ,Gebool((nam.o32_flags & 0x00010000L) == 0x00010000L)
	  ,nam.o32_pagemap
	  ,nam.o32_mapsize);
}

void LX_Parser::ObjPaintLX(TWindow& win,const std::vector<LX_OBJECT>& names,unsigned start) const
{
    win.freeze();
    win.clear();
    std::ostringstream oss;
    oss<<" Object Table [ "<<start + 1<<" / "<<names.size()<<" ] ";
    win.set_title(oss.str(),TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    objpaintLX(win,names[start]);
    win.refresh_full();
}

std::vector<LX_OBJECT> LX_Parser::__ReadObjectsLX(binary_stream& handle,size_t n) const
{
    std::vector<LX_OBJECT> rc;
    size_t i;
    for(i = 0;i < n;i++) {
	LX_OBJECT lxo;
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(&lxo,sizeof(LX_OBJECT));
	rc.push_back(lxo);
    }
    return rc;
}

std::vector<LX_ENTRY> LX_Parser::__ReadEntriesLX(binary_stream& handle) const
{
    std::vector<LX_ENTRY> rc;
    unsigned i;
    unsigned char j;
    unsigned char cnt,type;
    uint_fast16_t numobj = 0;
    LX_ENTRY _lxe;
    i = 0;
    while(1) {
	bool is_eof;
	is_eof = false;
	cnt = handle.read(type_byte);
	type = handle.read(type_byte);
	if(!cnt) break;
	if(type) numobj = handle.read(type_word);
	for(j = 0;j < cnt;j++,i++) {
	    char size;
	    switch(type) {
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
	    if(size) {
		_lxe.b32_obj = numobj;
		_lxe.entry.e32_flags = handle.read(type_byte);
		handle.read(&_lxe.entry.e32_variant,size);
	    }
	    rc.push_back(_lxe);
	}
	if(is_eof) break;
    }
exit:
    return rc;
}

void LX_Parser::lxReadPageDesc(binary_stream& handle,LX_MAP_TABLE *mt,unsigned long pageidx) const
{
  handle.seek(headshift()+lxe.lx.lxObjectPageTableOffset+
	  sizeof(LX_MAP_TABLE)*(pageidx - 1),binary_stream::Seek_Set);
  handle.read((any_t*)mt,sizeof(LX_MAP_TABLE));
}

__filesize_t LX_Parser::__calcPageEntry(LX_MAP_TABLE *mt) const
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

__filesize_t LX_Parser::CalcPageEntry(unsigned long pageidx) const
{
  binary_stream& handle = *lx_cache;
  LX_MAP_TABLE mt;
  if(!pageidx) return -1;
  lxReadPageDesc(handle,&mt,pageidx);
  return __calcPageEntry(&mt);
}

__filesize_t LX_Parser::CalcEntryPoint(unsigned long objnum,__filesize_t _offset) const
{
  binary_stream& handle = *lx_cache;
  unsigned long i,diff;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  if(!objnum) return bctx().tell();
  handle.seek(lxe.lx.lxObjectTableOffset + headshift(),binary_stream::Seek_Set);
  handle.seek(sizeof(LX_OBJECT)*(objnum - 1),binary_stream::Seek_Cur);
  handle.read((any_t*)&lo,sizeof(LX_OBJECT));
  i = _offset / lxe.lx.lxPageSize;
  diff = _offset - i*lxe.lx.lxPageSize;
  lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
  return __calcPageEntry(&mt) + diff;
}

std::string LX_Parser::ReadLXLEImpMod(__filesize_t offtable,unsigned num) const
{
    binary_stream* handle;
    unsigned i;
    unsigned char len;
    char buff[256];
    handle = lx_cache;
    handle->seek(offtable,binary_stream::Seek_Set);
    for(i = 1;i < num;i++) {
	len = handle->read(type_byte);
	handle->seek(len,binary_stream::Seek_Cur);
    }
    len = handle->read(type_byte);
    handle->read((any_t*)buff,len);
    buff[len] = 0;
    return buff;
}

std::string LX_Parser::ReadLXLEImpName(__filesize_t offtable,unsigned num) const
{
    binary_stream* handle;
    unsigned char len;
    char buff[256];
    handle = lx_cache;
    handle->seek(offtable+num,binary_stream::Seek_Set);
    len = handle->read(type_byte);
    handle->read((any_t*)buff,len);
    buff[len] = 0;
    return buff;
}

void LX_Parser::ShowFwdModOrdLX(const LX_ENTRY& lxent) const
{
    std::string buff;
    buff[0] = 0;
    buff=ReadLXLEImpMod(lxe.lx.lxImportModuleTableOffset + headshift(),lxent.entry.e32_variant.e32_fwd.modord);
    buff+=".";
    if((lxent.entry.e32_flags & 0x01) == 0x01) {
	std::ostringstream oss;
	oss<<"@"<<(unsigned)lxent.entry.e32_variant.e32_fwd.value;
	buff+=oss.str();
    }
    else buff+=ReadLXLEImpName(lxe.lx.lxImportProcedureTableOffset + headshift(),(unsigned)lxent.entry.e32_variant.e32_fwd.value);
    bctx().TMessageBox(buff," Forwarder entry point ");
}

__filesize_t LX_Parser::CalcEntryLX(const LX_ENTRY& lxent) const
{
    __filesize_t ret;
    ret = bctx().tell();
    switch(lxent.b32_type) {
	case 1: ret = CalcEntryPoint(lxent.b32_obj,lxent.entry.e32_variant.e32_offset.offset16);
		      break;
	case 2: ret = CalcEntryPoint(lxent.b32_obj,lxent.entry.e32_variant.e32_callgate.offset);
		      break;
	case 3: ret = CalcEntryPoint(lxent.b32_obj,lxent.entry.e32_variant.e32_offset.offset32);
		      break;
	case 4: ShowFwdModOrdLX(lxent);
	case 5:
	default: break;
    }
    return ret;
}

__filesize_t LX_Parser::CalcEntryBungleLX(unsigned ordinal,bool dispmsg) const
{
  binary_stream* handle;
  bool found;
  unsigned i;
  unsigned char j;
  unsigned char cnt,type;
  uint_fast16_t numobj = 0;
  LX_ENTRY lxent;
  __filesize_t ret;
  ret = bctx().tell();
  handle = lx_cache;
  handle->seek(lxe.lx.lxEntryTableOffset + headshift(),binary_stream::Seek_Set);
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
 if(found) ret = CalcEntryLX(lxent);
 else      if(dispmsg) bctx().ErrMessageBox(NOT_ENTRY,"");
 return ret;
}

__filesize_t LX_Parser::action_F10()
{
    binary_stream& handle = *lx_cache;
    __filesize_t fpos;
    unsigned nnames;
    fpos = bctx().tell();
    nnames = (unsigned)lxe.lx.lxObjectCount;
    if(!nnames) { bctx().NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
    handle.seek(lxe.lx.lxObjectTableOffset + headshift(),binary_stream::Seek_Set);
    std::vector<LX_OBJECT> objs = __ReadObjectsLX(handle,nnames);
    if(!objs.empty()) {
	int ret;
	ret = PageBox(70,20,objs,*this,&LX_Parser::ObjPaintLX);
	if(ret != -1)  fpos = CalcPageEntry(objs[ret].o32_pagemap);
    }
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

std::string LX_Parser::lxeGetMapAttr(unsigned long attr) const
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

std::string LX_Parser::entryTypeLX(unsigned char type) const
{
   if(type < 6) return __e32type[type];
   else         return "Unknown";
}

void LX_Parser::entrypaintLX(TWindow& w,const LX_ENTRY& nam) const
{
    if(!nam.b32_type) {
	w.goto_xy(35,4);
	w.printf("Unused");
    } else {
	w.goto_xy(1,1);
	w.printf(
	    "Entry type: %s\n"
	    "Object number : %hd\n"
	    "Flags: %02XH\n"
	    ,entryTypeLX(nam.b32_type).c_str()
	    ,nam.b32_obj
	    ,(int)nam.entry.e32_flags);
	if(nam.b32_type != 4) {
	    w.printf(
	      "   [%c] Exported Entry\n"
	      "   [%c] Used Shared Data\n"
	      "   %02XH - parameter word count mask\n"
	      ,Gebool((nam.entry.e32_flags & 0x01) == 0x01)
	      ,Gebool((nam.entry.e32_flags & 0x02) == 0x02)
	      ,(int)(nam.entry.e32_flags >> 2));
	} else {
	    w.printf(
	      "\n"
	      "   [%c] Import by ordinal\n"
	      "\n"
	      ,Gebool((nam.entry.e32_flags & 0x01) == 0x01));
	}
	if(nam.b32_type == 1) {
	    w.printf(
	      "Entry offset : %04hXH\n"
	      "\n"
	      ,nam.entry.e32_variant.e32_offset.offset16);
	} else if(nam.b32_type == 3) {
	    w.printf(
		"Entry offset : %08XH\n"
		 "\n"
		 ,nam.entry.e32_variant.e32_offset.offset32);
	} else if(nam.b32_type == 2) {
	    w.printf(
		"Callgate offset : %04hXH\n"
		"Callgate selector : %04hXH\n"
		,nam.entry.e32_variant.e32_callgate.offset
		,nam.entry.e32_variant.e32_callgate.callgate);
	} else if(nam.b32_type == 4) {
	    w.printf(
		  "Module ordinal number : %04hXH\n"
		  "Proc name offset or ordinal : %04hXH\n"
		  ,nam.entry.e32_variant.e32_fwd.modord
		  ,nam.entry.e32_variant.e32_fwd.value);
	}
    }
}

void LX_Parser::PaintEntriesLX(TWindow& win,const std::vector<LX_ENTRY>& names,unsigned start) const
{
    std::ostringstream oss;
    win.freeze();
    win.clear();
    oss<<" Entries Table [ "<<(start + 1)<<" / "<<names.size()<<" ] ";
    win.set_title(oss.str(),TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    entrypaintLX(win,names[start]);
    win.refresh_full();
}

std::vector<std::string> LX_Parser::__ReadMapTblLX(binary_stream& handle,size_t n) const
{
    std::vector<std::string> rc;
    size_t i;
    for(i = 0;i < n;i++) {
	LX_MAP_TABLE mt;
	std::ostringstream oss;
	if(IsKbdTerminate() || handle.eof()) break;
	lxReadPageDesc(handle,&mt,i+1);
	oss<<"off="<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)mt.o32_pagedataoffset
	    <<"H Siz="<<std::hex<<std::setfill('0')<<std::setw(4)<<mt.o32_pagesize
	    <<"H Flg:"<<std::hex<<std::setfill('0')<<std::setw(4)<<mt.o32_pageflags
	    <<"H="<<lxeGetMapAttr(mt.o32_pageflags);
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t LX_Parser::action_F9()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    std::string title = " Map of pages ";
    ssize_t nnames = (unsigned)lxe.lx.lxPageCount;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadMapTblLX(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
exit:
    if(ret != -1) fpos = CalcPageEntry(ret + 1);
    return fpos;
}

__filesize_t LX_Parser::action_F6()
{
    binary_stream& handle = *lx_cache;
    __filesize_t fpos;
    fpos = bctx().tell();
    if(!lxe.lx.lxEntryTableOffset) { bctx().NotifyBox(NOT_ENTRY," Entry Table "); return fpos; }
    handle.seek(lxe.lx.lxEntryTableOffset + headshift(),binary_stream::Seek_Set);
    std::vector<LX_ENTRY> objs = __ReadEntriesLX(handle);
    int ret;
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY," Entry Table "); goto bye; }
    ret = PageBox(70,8,objs,*this,&LX_Parser::PaintEntriesLX);
    if(ret != -1)  fpos = CalcEntryLX(objs[ret]);
bye:
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

std::vector<std::string> LX_Parser::__ReadResourceGroupLX(binary_stream& handle,size_t nitems,long* addr)
{
    std::vector<std::string> rc;
    unsigned i;
    LXResource lxr;
    std::ostringstream oss;
    for(i = 0;i < nitems;i++) {
	handle.read(&lxr,sizeof(LXResource));
	addr[i] = lxr.offset;
	if(IsKbdTerminate() || handle.eof()) break;
	oss.str("");
	oss<<std::setw(6)<<lxr.nameID<<" = ";
	if(lxr.typeID < sizeof(ResourceGrNamesLX)/sizeof(char *)) oss<<ResourceGrNamesLX[lxr.typeID];
	else  oss<<"Unknown < "<<std::hex<<std::setfill('0')<<std::setw(4)<<lxr.typeID<<"H >";
	oss<<" obj="<<std::hex<<std::setfill('0')<<std::setw(4)<<lxr.object<<"H."<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)lxr.offset<<"H";
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t LX_Parser::action_F7()
{
    __filesize_t fpos;
    binary_stream& handle = *lx_cache;
    long* raddr;
    unsigned nrgroup;
    fpos = bctx().tell();
    handle.seek((__fileoff_t)headshift() + lxe.lx.lxResourceTableOffset,binary_stream::Seek_Set);
    nrgroup = (unsigned)lxe.lx.lxNumberResourceTableEntries;
    if(!nrgroup) { bctx().NotifyBox(NOT_ENTRY," Resources "); return fpos; }
    if(!(raddr  = new long [nrgroup])) return fpos;
    std::vector<std::string> objs = __ReadResourceGroupLX(handle,nrgroup,raddr);
    ListBox lb(bctx());
    if(!objs.empty()) lb.run(objs," Resource groups : ",ListBox::Sortable,-1);
    delete raddr;
    return fpos;
}

__filesize_t LX_Parser::action_F2()
{
    std::string title = MOD_REFER;
    ssize_t nnames = (unsigned)lxe.lx.lxImportModuleTableEntries;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadModRefNamesLX(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return bctx().tell();
}

__filesize_t LX_Parser::action_F3()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    unsigned ordinal;
    std::string title = RES_NAMES;
    ssize_t nnames = LXRNamesNumItems(main_handle());
    ListBox::flags flags = ListBox::Selective | ListBox::Sortable;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = LXRNamesReadItems(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
    if(ret != -1) {
	const char* cptr;
	char buff[40];
	cptr = strrchr(objs[ret].c_str(),ListBox::Ord_Delimiter);
	cptr++;
	strcpy(buff,cptr);
	ordinal = atoi(buff);
    }
exit:
    if(ret != -1) fpos = CalcEntryBungleLX(ordinal,true);
    return fpos;
}

__filesize_t LX_Parser::action_F4()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    unsigned ordinal;
    std::string title = NORES_NAMES;
    ssize_t nnames = LXNRNamesNumItems(main_handle());
    ListBox::flags flags = ListBox::Selective | ListBox::Sortable;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = LXNRNamesReadItems(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
    if(ret != -1) {
	const char* cptr;
	char buff[40];
	cptr = strrchr(objs[ret].c_str(),ListBox::Ord_Delimiter);
	cptr++;
	strcpy(buff,cptr);
	ordinal = atoi(buff);
    }
exit:
    if(ret != -1) fpos = CalcEntryBungleLX(ordinal,true);
    return fpos;
}

__filesize_t LX_Parser::action_F5()
{
    std::string title = IMPPROC_TABLE;
    ssize_t nnames = LXImpNamesNumItems(main_handle());
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = LXImpNamesReadItems(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return bctx().tell();
}

LX_Parser::LX_Parser(BeyeContext& b,binary_stream& h,CodeGuider& __code_guider,udn& u)
	:MZ_Parser(b,h,__code_guider,u)
{
    char id[4];
    main_handle().seek(headshift(),binary_stream::Seek_Set);
    main_handle().read(id,sizeof(id));
    if(!(id[0] == 'L' && id[1] == 'X' && id[2] == 0 && id[3] == 0)) throw bad_format_exception();

    main_handle().seek(headshift(),binary_stream::Seek_Set);
    main_handle().read(&lxe.lx,sizeof(LXHEADER));
    lx_cache = main_handle().dup();
}

LX_Parser::~LX_Parser()
{
   binary_stream& _mh = main_handle();
   if(lx_cache != &_mh) delete lx_cache;
}

__filesize_t LX_Parser::action_F1()
{
    Beye_Help bhelp(bctx());
    if(bhelp.open(true)) {
	bhelp.run(10005);
	bhelp.close();
    }
    return bctx().tell();
}

__filesize_t LX_Parser::va2pa(__filesize_t va) const
{
    /* First we must determine object number for given virtual address */
    binary_stream& handle = *lx_cache;
    unsigned long i,diff,oidx;
    __filesize_t rva,pa;
    LX_OBJECT lo;
    LX_MAP_TABLE mt;
    handle.seek(lxe.lx.lxObjectTableOffset + headshift(),binary_stream::Seek_Set);
    pa = oidx = 0; /* means: error */
    for(i = 0;i < lxe.lx.lxObjectCount;i++) {
	handle.read((any_t*)&lo,sizeof(LX_OBJECT));
	if(lo.o32_base <= va && va < lo.o32_base + lo.o32_size)  {
	    oidx = i+1;
	    break;
	}
    }
    /* Secondly we must determine page within object */
    if(oidx) {
	rva = va - lo.o32_base;
	i = rva / lxe.lx.lxPageSize;
	diff = rva - i*lxe.lx.lxPageSize;
	lxReadPageDesc(handle,&mt,i+lo.o32_pagemap);
	pa = __calcPageEntry(&mt) + diff;
    }
    return pa;
}

__filesize_t LX_Parser::pa2va(__filesize_t pa) const
{
    /* First we must determine page for given physical address */
    binary_stream& handle = *lx_cache;
    unsigned long i,pidx,pagentry;
    __filesize_t rva,va;
    LX_OBJECT lo;
    LX_MAP_TABLE mt;
    pagentry = va = pidx = 0;
    for(i = 0;i < lxe.lx.lxPageCount;i++) {
	lxReadPageDesc(handle,&mt,i+1);
	pagentry = __calcPageEntry(&mt);
	if(pagentry <= pa && pa < pagentry + mt.o32_pagesize) {
	    pidx = i+1;
	    break;
	}
    }
    /* Secondly we must determine object number for given physical address */
    if(pidx) {
	rva = pa - pagentry + (pidx-1)*lxe.lx.lxPageSize;
	handle.seek(lxe.lx.lxObjectTableOffset + headshift(),binary_stream::Seek_Set);
	for(i = 0;i < lxe.lx.lxObjectCount;i++) {
	    handle.read((any_t*)&lo,sizeof(LX_OBJECT));
	    if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize) {
		va = lo.o32_base + rva;
		break;
	    }
	}
    }
    return va;
}

Bin_Format::bitness LX_Parser::query_bitness(__filesize_t pa) const
{
 /* First we must determine page for given physical address */
  binary_stream& handle = *lx_cache;
  unsigned long i,pidx,pagentry;
  Bin_Format::bitness ret;
  LX_OBJECT lo;
  LX_MAP_TABLE mt;
  pagentry = pidx = 0;
  ret = Bin_Format::Use16;
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
    handle.seek(lxe.lx.lxObjectTableOffset + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < lxe.lx.lxObjectCount;i++)
    {
      handle.read((any_t*)&lo,sizeof(LX_OBJECT));
      if(lo.o32_pagemap <= pidx && pidx < lo.o32_pagemap + lo.o32_mapsize)
      {
	ret = (lo.o32_flags & 0x00002000L) == 0x00002000L ? Bin_Format::Use32 : Bin_Format::Use16;
	break;
      }
    }
  }
  return ret;
}

std::string LX_Parser::address_resolving(__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
    uint32_t res;
    std::ostringstream oss;
    if(cfpos >= headshift() && cfpos < headshift() + sizeof(LXHEADER)) oss<<"LXH :"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift());
    else if(cfpos >= headshift() + lxe.lx.lxObjectTableOffset &&
		cfpos <  headshift() + lxe.lx.lxObjectTableOffset + sizeof(LX_OBJECT)*lxe.lx.lxObjectCount)
	    oss<<"LXOD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift() - lxe.lx.lxObjectTableOffset);
    else if(cfpos >= headshift() + lxe.lx.lxObjectPageTableOffset &&
	    cfpos <  headshift() + lxe.lx.lxObjectPageTableOffset + sizeof(LX_MAP_TABLE)*lxe.lx.lxPageCount)
	    oss<<"LXPD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift() - lxe.lx.lxObjectPageTableOffset);
    else if((res=pa2va(cfpos))!=0) oss<<"."<<std::hex<<std::setfill('0')<<std::setw(8)<<res;
    return oss.str();
}

int LX_Parser::query_platform() const { return DISASM_CPU_IX86; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) LX_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info lx_info = {
    "LX (Linear eXecutable)",	/**< plugin name */
    query_interface
};
} // namespace	usr
