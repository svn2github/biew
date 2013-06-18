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
#include <sstream>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "plugins/disasm.h"
#include "plugins/bin/lx_le.h"
#include "udn.h"
#include "beyehelp.h"
#include "listbox.h"
#include "tstrings.h"
#include "bconsole.h"
#include "libbeye/kbd_code.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
static const char* txt[]={ "LEHelp", "Import", "ResNam", "NRsNam", "ImpNam", "Entry ", "", "LEHead", "MapTbl", "Object" };
const char* LE_Parser::prompt(unsigned idx) const { return txt[idx]; }

std::vector<std::string> LE_Parser::__ReadMapTblLE(binary_stream& handle,size_t n) const
{
    std::vector<std::string> rc;
    size_t i;
    handle.seek(lxe.le.leObjectPageMapTableOffset + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < n;i++) {
	LE_PAGE lep;
	std::ostringstream oss;
	if(IsKbdTerminate() || handle.eof()) break;
	binary_packet bp=handle.read(sizeof(LE_PAGE)); memcpy(&lep,bp.data(),bp.size());
	oss<<"#="<<std::hex<<std::setfill('0')<<std::setw(8)<<(long)lep.number<<"H Flags: "<<std::hex<<std::setfill('0')<<std::setw(4)<<lep.flags
	    <<"h = "<<lxeGetMapAttr(lep.flags);
	rc.push_back(oss.str());
    }
    return rc;
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
    binary_packet bp=handle->read(sizeof(LE_PAGE)); memcpy(&mt,bp.data(),bp.size());
    if(handle->eof()) break;
    if(mt.number == pageidx)
    {
      found = true;
      break;
    }
  }
  if(found) return __calcPageEntryLE((LE_PAGE*)&mt,pageidx - 1);
  else      return bctx().tell();
}

__filesize_t LE_Parser::CalcEntryPoint(unsigned long objnum,__filesize_t _offset) const
{
    binary_stream* handle;
    unsigned long i,start,pidx,j;
    __filesize_t ret,pageoff;
    LX_OBJECT lo;
    LE_PAGE mt;
    binary_packet bp(1);
    if(!objnum) return -1;
    handle = lx_cache;
    handle->seek(lxe.le.leObjectTableOffset + headshift(),binary_stream::Seek_Set);
    handle->seek(sizeof(LX_OBJECT)*(objnum - 1),binary_stream::Seek_Cur);
    bp=handle->read(sizeof(LX_OBJECT)); memcpy(&lo,bp.data(),bp.size());
/*  if((lo.o32_flags & 0x00002000L) == 0x00002000L) USE16 = 0;
  else                                            USE16 = 0xFF; */
    pageoff = lxe.le.leObjectPageMapTableOffset + headshift();
    start = 0;
    ret = -1;
    for(i = 0;i < lo.o32_mapsize;i++) {
	bool is_eof;
	is_eof = false;
	if(_offset >= start && _offset < start + lxe.le.lePageSize) {
	    bool found;
	    handle->seek(pageoff,binary_stream::Seek_Set);
	    pidx = i + lo.o32_pagemap;
	    found = false;
	    for(j = 0;j < lxe.le.lePageCount;j++) {
		bp=handle->read(sizeof(LE_PAGE)); memcpy(&mt,bp.data(),bp.size());
		if((is_eof = handle->eof()) != 0) break;
		if(mt.number == pidx) { found = true; break; }
	    }
	    if(found) ret = __calcPageEntryLE((LE_PAGE*)&mt,pidx - 1) + _offset - start;
	    else      ret = bctx().tell();
	    break;
	}
	if(is_eof) break;
	start += lxe.le.lePageSize;
    }
    return ret;
}

__filesize_t LE_Parser::CalcEntryLE(const LX_ENTRY& lxent) const
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

__filesize_t LE_Parser::CalcEntryBungleLE(unsigned ordinal,bool dispmsg) const
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
    handle->seek(lxe.le.leEntryTableOffset + headshift(),binary_stream::Seek_Set);
    i = 0;
    found = false;
    while(1) {
	cnt = handle->read(type_byte);
	type = handle->read(type_byte);
	if(!cnt) break;
	if(type) numobj = handle->read(type_word);
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
	    if(i == ordinal - 1) {
		lxent.b32_type = type;
		found = true;
		if(size) {
		    lxent.b32_obj = numobj;
		    lxent.entry.e32_flags = handle->read(type_byte);
		    binary_packet bp=handle->read(size); memcpy(&lxent.entry.e32_variant,bp.data(),bp.size());
		}
		break;
	    } else if(size) handle->seek(size + sizeof(char),binary_stream::Seek_Cur);
	    if(handle->eof()) break;
	}
	if(found) break;
    }
    if(found) ret = CalcEntryLE(lxent);
    else      if(dispmsg) bctx().ErrMessageBox(NOT_ENTRY,"");
    return ret;
}

__filesize_t LE_Parser::action_F10()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    std::string title = " Map of pages ";
    ssize_t nnames = (unsigned)lxe.le.lePageCount;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadMapTblLE(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
exit:
    if(ret != -1) fpos = CalcPageEntry(ret + 1);
    return fpos;
}

__filesize_t LE_Parser::action_F3()
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
	cptr = strrchr(objs[ret].c_str(),ListBox::Ord_Delimiter);
	cptr++;
	std::istringstream iss(cptr);
	iss>>ordinal;
    }
exit:
    if(ret != -1) fpos = CalcEntryBungleLE(ordinal,true);
    return fpos;
}

__filesize_t LE_Parser::action_F4()
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
	cptr = strrchr(objs[ret].c_str(),ListBox::Ord_Delimiter);
	cptr++;
	std::istringstream iss(cptr);
	iss>>ordinal;
    }
exit:
    if(ret != -1) fpos = CalcEntryBungleLE(ordinal,true);
    return fpos;
}

LE_Parser::LE_Parser(BeyeContext& b,binary_stream& h,CodeGuider& __code_guider,udn& u)
	:LX_Parser(b,h,__code_guider,u)
{
    binary_packet bp(1);
    if(headshift()) {
	char id[2];
	main_handle().seek(headshift(),binary_stream::Seek_Set);
	bp=main_handle().read(sizeof(id)); memcpy(id,bp.data(),bp.size());
	if(!(id[0] == 'L' && id[1] == 'E')) throw bad_format_exception();
    } else throw bad_format_exception();
    main_handle().seek(headshift(),binary_stream::Seek_Set);
    bp=main_handle().read(sizeof(LEHEADER)); memcpy(&lxe.le,bp.data(),bp.size());
}

LE_Parser::~LE_Parser()
{
}

__filesize_t LE_Parser::action_F1()
{
    Beye_Help bhelp(bctx());
    if(bhelp.open(true)) {
	bhelp.run(10004);
	bhelp.close();
    }
    return bctx().tell();
}

std::string LE_Parser::address_resolving(__filesize_t cfpos)
{
    std::ostringstream oss;
     /* Since this function is used in references resolving of disassembler
	it must be seriously optimized for speed. */
    if(cfpos >= headshift() && cfpos < headshift() + sizeof(LEHEADER)) oss<<"LEH :"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift());
    else if(cfpos >= headshift() + lxe.le.leObjectTableOffset &&
	    cfpos <  headshift() + lxe.le.leObjectTableOffset + sizeof(LX_OBJECT)*lxe.le.leObjectCount)
		oss<<"LEOD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift() - lxe.le.leObjectTableOffset);
    else if(cfpos >= headshift() + lxe.le.leObjectPageMapTableOffset &&
		cfpos <  headshift() + lxe.le.leObjectPageMapTableOffset + sizeof(LE_PAGE)*lxe.le.lePageCount)
	oss<<"LEPD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift() - lxe.le.leObjectPageMapTableOffset);
    return oss.str();
}

int LE_Parser::query_platform() const { return DISASM_CPU_IX86; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) LE_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info le_info = {
    "LE (Linear Executable)",	/**< plugin name */
    query_interface
};
} // namespace	usr
