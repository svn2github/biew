#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/ne.c
 * @brief       This file contains implementation of NE (New Executable) file
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
#include <algorithm>
#include <set>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "colorset.h"
#include "plugins/disasm.h"
#include "plugins/bin/ne.h"
#include "udn.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "codeguid.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"
#include "ne.h"
#include "beye.h"

namespace	usr {
static const char* txt[]={ "NEHelp", "ModRef", "ResNam", "NRsNam", "", "Entry ", "ResTbl", "NE Hdr", "", "SegDef" };
const char* NE_Parser::prompt(unsigned idx) const { return txt[idx]; }

const char* NE_Parser::__nedata[] =
{
  "NOAUTODATA",
  "SINGLEDATA",
  "MULTIPLEDATA",
  "(SINGLE & MULTIPLE DATA)"
};

const char * __neExeType[] =
{
   "Unknown (Any)",
   "OS/2",
   "Windows",
   "DOS4",
   "Windows Dev386"
};

const char* NE_Parser::__getNEType(unsigned type) const
{
  if(type > 4) type = 0;
  return __neExeType[type];
}

const char * PMWinAPI[4] =
{
  "Text or Windowing API not declared.",
  "(NOTWINDOWCOMPAT). Full screen character-mode application.",
  "(WINDOWCOMPAT). Text window character mode application.",
  "(WINDOWAPI). Windowing application."
};

const char* NE_Parser::GetPMWinAPI(unsigned flag)
{
 flag >>= 8;
 flag &= 0x0003;
 return PMWinAPI[flag];
}

void NE_Parser::PaintNewHeaderNE_1(TWindow& w) const
{
  w.printf(
	   "Signature                      = '%c%c'\n"
	   "Linker Version.Revision        = %hd.%hd\n"
	   "Offset of Entry Table          = %XH\n"
	   "Length of Entry Table          = %hu  [ bytes ]\n"
	   "< 32-bit Checksum  >           = %08lXH\n"
	   "NE Flags :                     = [%04hXH]\n"
	   "   Contest DATA in EXE: %s\n"
	   "   [%c]  < Per-process library initialization (INITINSTANCE) >\n"
	   "   [%c]  Runs in protected mode only (PROTMODE)\n"
	   "   [%c]  Win: [LIM 3.2 used fow Windows] OS/2: [8086 instruction]\n"
	   "   [%c]  Win: [Multi instance EMS memory] OS/2: [80286 instructions]\n"
	   "   [%c]  Win: [DLL Global memory above EMS] OS/2: [80386 instructions]\n"
	   "   [%c]  OS/2: Floating point instructrions\n"
	   "   %s\n"
	   "   [%c]  First segment contains code that loads API (Bound Family/API)\n"
	   "   [%c]  Linker detects errors at link time, but still creates module\n"
	   "   [%c]  Win: [Module must located in EMS] OS/2: [SMP mode disabled]\n"
	   "   [%c]  Module is library (DLL)\n"
	   "DS (DGROUP)                    = %hu\n"
	   "HEAPSIZE                       = %hu\n"
	   "STACKSIZE                      = %hu\n"
	   "CS : IP                        = %04hXH:%04hXH"
	   ,ne.neSignature[0],ne.neSignature[1]
	   ,(int)ne.neLinkerVersion,(int)ne.neLinkerRevision
	   ,ne.neOffsetEntryTable
	   ,ne.neLengthEntryTable
	   ,ne.neChecksum
	   ,ne.neContestEXE
	   ,__nedata[ne.neContestEXE & 0x03]
	   ,Gebool( ne.neContestEXE & 0x0004 )
	   ,Gebool( ne.neContestEXE & 0x0008 )
	   ,Gebool( ne.neContestEXE & 0x0010 )
	   ,Gebool( ne.neContestEXE & 0x0020 )
	   ,Gebool( ne.neContestEXE & 0x0040 )
	   ,Gebool( ne.neContestEXE & 0x0040 )
	   ,GetPMWinAPI(ne.neContestEXE)
	   ,Gebool( ne.neContestEXE & 0x0800 )
	   ,Gebool( ne.neContestEXE & 0x2000 )
	   ,Gebool( ne.neContestEXE & 0x4000 )
	   ,Gebool( ne.neContestEXE & 0x8000 )
	   ,ne.neAutoDataSegmentCount
	   ,ne.neHeapSize
	   ,ne.neStackSize
	   ,(unsigned)(ne.neCSIPvalue >> 16),(unsigned)(ne.neCSIPvalue & 0xFFFF));
}

static __filesize_t entryNE;

void NE_Parser::PaintNewHeaderNE_2(TWindow& w) const
{
  w.printf(
	   "SS : SP                        = %04hXH:%04hXH\n"
	   "Segment Table Count            = %hu\n"
	   "Module Reference Table Count   = %hu\n"
	   "Length Non Resident Name Table = %hu\n"
	   "Segment Table Offset           = %hXH\n"
	   "Resource Table Offset          = %hXH\n"
	   "Resident Name Table Offset     = %hXH\n"
	   "Module Reference Table Offset  = %hXH\n"
	   "Import Table Offset            = %hXH\n"
	   "Non Resident Name Table Offset = %08lXH\n"
	   "Moveable Entry Point Count     = %hu\n"
	   "Logical Sector Shift Count     = %hu\n"
	   "Resource Segment Count         = %hu\n"
	   "Operating System               = %s\n"
	   "[%c] - Support for long file names\n"
	   "[%c] - Windows 2.x API runs in prot. mode\n"
	   "[%c] - Windows 2.x API getting prot. font\n"
	   "[%c] - WLO application on OS/2\n"
	   ,(unsigned short)(ne.neSSSPvalue >> 16),(unsigned short)(ne.neSSSPvalue & 0xFFFF)
	   ,(short)ne.neSegmentTableCount
	   ,ne.neModuleReferenceTableCount
	   ,ne.neLengthNonResidentNameTable
	   ,ne.neOffsetSegmentTable
	   ,ne.neOffsetResourceTable
	   ,ne.neOffsetResidentNameTable
	   ,ne.neOffsetModuleReferenceTable
	   ,ne.neOffsetImportTable
	   ,ne.neOffsetNonResidentNameTable
	   ,ne.neMoveableEntryPointCount
	   ,ne.neLogicalSectorShiftCount
	   ,ne.neResourceSegmentCount
	   ,__getNEType(ne.neOperatingSystem)
	   ,Gebool(ne.neFlagsOther & 0x01)
	   ,Gebool(ne.neFlagsOther & 0x02)
	   ,Gebool(ne.neFlagsOther & 0x02)
	   ,Gebool(ne.neFlagsOther & 0x80));
  if(ne.neOperatingSystem == 2)
  {
	/* Correction by Olivier Mengu\u00e9*/
    unsigned char high,low;
    high = ne.neWindowsVersion >> 8;
    low  = ne.neWindowsVersion & 0xFF;
	/* End of correction */
    w.set_color(dialog_cset.addinfo);
    w.printf("Offset of Fast Load Area       = %04hXH"
	     ,ne.neOffsetFastLoadArea); w.clreol();
    w.printf("\nLength of Fast Load Area       = %hu"
	     ,ne.neLengthFastLoadArea); w.clreol();
    w.printf("\nWindows version                = %02hu.%02hu"
	     ,(unsigned int)high,(unsigned int)low); w.clreol();
    w.printf("\n");
  }
  w.set_color(dialog_cset.entry);
  w.printf(">Entry Point   %s = %08XH",( ne.neContestEXE & 32768L ) ? "[ LibEntry ]   " : "[ EXEEntry ] ",entryNE);
  w.clreol();
  w.set_color(dialog_cset.main);
}

void (NE_Parser::*NE_Parser::nephead[])(TWindow& w) const =
{
    &NE_Parser::PaintNewHeaderNE_1,
    &NE_Parser::PaintNewHeaderNE_2
};

void NE_Parser::PaintNewHeaderNE(TWindow& win,const std::vector<std::string>& ptr,unsigned tpage) const
{
    char text[80];
    win.freeze();
    win.clear();
    sprintf(text," New Executable Header [%d/%d] ",ptr.size() + 1,tpage);
    win.set_title(text,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    if(tpage < 2) {
	win.goto_xy(1,1);
	(this->*nephead[tpage])(win);
    }
    win.refresh_full();
}

__filesize_t NE_Parser::action_F8()
{
    __fileoff_t pos;
    unsigned CS,IP;
    CS = (unsigned)((ne.neCSIPvalue) >> 16);  /** segment number */
    IP = (unsigned)(ne.neCSIPvalue & 0xFFFF); /** offset within segment */
    entryNE = CalcEntryPointNE(CS,IP);
    pos = beye_context().tell();
    std::vector<std::string> v;
    v.push_back("");
    v.push_back("");
    if(PageBox(70,22,v,*this,&NE_Parser::PaintNewHeaderNE) != -1 && entryNE) pos = entryNE;
    return pos;
}

void NE_Parser::entpaintNE(TWindow& w,const ENTRY& nam,unsigned flags) const
{
    w.goto_xy(1,1);
    w.printf(
	   "Entry Point for %s segment\n"
	   "Entry point is %s EXPORTED\n"
	   "The Entry %s uses SHARED data segment\n"
	   "Numbers of word that compose the stack %u\n"
	   "Segment offset = %XH bytes\n"
	   "Segment number = %u"
	   ,(unsigned char)(nam.eFixed) == 0xFF ? "MOVEABLE" : "FIXED"
	   ,flags & 0x0001 ? "  " : "NO"
	   ,flags & 0x0002 ? "  " : "NO"
	   ,(flags & 0xFFF4 >> 2)
	   ,nam.eSegOff
	   ,((unsigned)((unsigned char)(nam.eSegNum))));
}

void NE_Parser::paintdummyentryNE(TWindow& w) const
{
    w.goto_xy(1,3);
    w.printf("   Entry point not present ( Dummy bungle )");
}

void NE_Parser::SegPaintNE(TWindow& win,const std::vector<SEGDEF>& names,unsigned start) const
{
    char buffer[81];
    unsigned flags = names[start].sdFlags;
    win.freeze();
    win.clear();
    sprintf(buffer," Segment Table [ %u / %u ] ",start + 1,names.size());
    win.set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    win.goto_xy(1,1);
    if(names[start].sdOffset)
	win.printf("Relative offset from begining in sectors     = %04hXH\n",names[start].sdOffset);
    else
	win.printf("No data of segment in the file\n");
    win.printf(
	  "Length of segments                           = %hu bytes\n"
	  "Minimum allocated memory for segment         = %hu bytes\n"
	  "Segment is :                                   %s\n"
	  " [%c] - Loader has allocated memory for segment\n"
	  " [%c] - Iterated segment\n"
	  "Segment is :                                   %s\n"
	  "Segment is :                                   %s\n"
	  "Segment is :                                   %s\n"
	  "Segment is :                                   %s\n"
	  " [%c] - Segment contains relocation data\n"
	  "Segment is :                                   %s\n"
	  "IOPL :                                         %hu\n"
	  "Segment is :                                   %s\n"
	  "Segment bitness :                              %d\n"
	  " [%c] - Huge memory segment (sizes is sector units)\n"
	  " [%c] - GDT allocation requested"
	  ,names[start].sdLength ? names[start].sdLength : (unsigned short)0xFFFF
	  ,names[start].sdMinMemory ? names[start].sdMinMemory : (unsigned short)0xFFFF
	  ,(flags & 0x0001) ? "DATA" : "CODE"
	  ,Gebool((flags & 0x0002) == 0x0002)
	  ,Gebool((flags & 0x0004) == 0x0004)
	  ,(flags & 0x0010) ? "MOVEABLE" : "FIXED"
	  ,(flags & 0x0020) ? "PURE" : "IMPURE"
	  ,(flags & 0x0040) ? "PRELOAD" : "LOADONCALL"
	  ,(flags & 0x0080) ? (flags & 0x0001) ? "READONLY" : "EXECUTEONLY" : (flags & 0x0001) ? "READWRITE" : "DEBUGABLE"
	  ,Gebool((flags & 0x0100) == 0x0100)
	  ,(flags & 0x0200) ? (flags & 0x0001) ? "EXPAND DOWN" : "CONFORMING" : (flags & 0x0001) ? "EXPAND UP" : "NOCONFORMING"
	  ,(unsigned)(((flags & 0x0C00) >> 10) & 0x03)
	  ,(flags & 0x1000) ? "DISCARDABLE" : "NONDISCARDABLE"
	  ,(flags & 0x2000) ? 32 : 16
	  ,Gebool((flags & 0x4000) == 0x0400)
	  ,Gebool((flags & 0x8000) == 0x0800));
    win.refresh_full();
}

void NE_Parser::EntPaintNE(TWindow& win,const std::vector<ENTRY>& names,unsigned start) const
{
    char buffer[81];
    unsigned flags = names[start].eFlags;
    win.freeze();
    win.clear();
    sprintf(buffer," Entry Point [ %u / %u ] ",start + 1,names.size());
    win.set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    if(names[start].eFixed) entpaintNE(win,names[start],flags);
    else paintdummyentryNE(win);
    win.refresh_full();
}

std::vector<std::string> NE_Parser::__ReadModRefNamesNE(binary_stream& handle)
{
    std::vector<std::string> rc;
    unsigned i;
    uint_fast16_t offTable;
    handle.seek(ne.neOffsetModuleReferenceTable + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < ne.neModuleReferenceTableCount;i++) {
	__filesize_t NameOff;
	unsigned char length;
	__filesize_t fp;
	char stmp[256];
	offTable = handle.read(type_word);
	fp = handle.tell();
	NameOff = (__fileoff_t)headshift() + offTable + ne.neOffsetImportTable;
	handle.seek(NameOff,binary_stream::Seek_Set);
	length = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(stmp,length);
	stmp[length] = 0;
	handle.seek(fp,binary_stream::Seek_Set);
	rc.push_back(stmp);
    }
    return rc;
}

__filesize_t NE_Parser::action_F2()
{
    binary_stream& handle = *ne_cache;
    int ret;
    unsigned nnames;
    __filesize_t fret;
    TWindow * w;
    fret = beye_context().tell();
    handle.seek(0L,binary_stream::Seek_Set);
    if(!(nnames = ne.neModuleReferenceTableCount)) { beye_context().NotifyBox(NOT_ENTRY,MOD_REFER); return fret; }
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadModRefNamesNE(handle);
    delete w;
    while(1) {
	ret = ListBox(objs,MOD_REFER,LB_SELECTIVE | LB_SORTABLE,-1);
	if(ret != -1) ShowProcListNE(ret);
	else break;
   }
    return fret;
}

bool NE_Parser::isPresent(const std::vector<std::string>& objs,const std::string& _tmpl) const
{
    unsigned i;
    bool ret = false;
    size_t nentry = objs.size();
    for(i = 0;i < nentry;i++) {
	if(_tmpl==objs[i]) { ret = true; break; }
    }
    return ret;
}

std::vector<std::string> NE_Parser::__ReadProcListNE(binary_stream& handle,int modno)
{
    std::vector<std::string> rc;
    unsigned i;
    std::string buff;
    SEGDEF tsd;
    modno++;

    handle.seek(headshift()+ne.neOffsetSegmentTable,binary_stream::Seek_Set);
    for(i = 0;i < ne.neSegmentTableCount;i++) {
	handle.read(&tsd,sizeof(SEGDEF));
	if(tsd.sdLength && tsd.sdOffset && tsd.sdFlags & 0x0100) {
	    __filesize_t spos;
	    uint_fast16_t j,nrelocs;
	    RELOC_NE rne;
	    spos = handle.tell();
	    handle.seek(((__fileoff_t)(tsd.sdOffset) << ne.neLogicalSectorShiftCount) + tsd.sdLength,binary_stream::Seek_Set);
	    nrelocs = handle.read(type_word);
	    for(j = 0;j < nrelocs;j++) {
		handle.read(&rne,sizeof(RELOC_NE));
		if((rne.Type & 3) && rne.idx == modno) {
		    if((rne.Type & 3) == 1) {
			char stmp[256];
			sprintf(stmp,"< By ordinal >   @%hu",rne.ordinal);
			buff=stmp;
		    } else buff=rd_ImpName(rne.ordinal,true);
		    if(!isPresent(rc,buff)) {
			if(IsKbdTerminate()) goto exit;
			rc.push_back(buff);
		    }
		}
	    }
	    handle.seek(spos,binary_stream::Seek_Set);
	}
    }
exit:
    return rc;
}

void NE_Parser::ShowProcListNE( int modno )
{
    binary_stream& handle = *ne_cache;
    char ptitle[80];
    std::string name;
    TWindow *w;
    handle.seek(0L,binary_stream::Seek_Set);
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadProcListNE(handle,modno);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,MOD_REFER); return; }
    name=rd_ImpName(modno+1,false);
    sprintf(ptitle,"%s%s ",IMPPROC_TABLE,name.c_str());
    ListBox(objs,ptitle,LB_SORTABLE,-1);
}

std::vector<std::string> NE_Parser::RNamesReadItems(binary_stream& handle,size_t nnames,__filesize_t offset)
{
    std::vector<std::string> rc;
    unsigned char length;
    unsigned Ordinal;
    unsigned i;
    char stmp[300]; /* max255 + @ordinal */
    handle.seek(offset,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	length = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(stmp,length);
	Ordinal = handle.read(type_word);
	sprintf(&stmp[length],"%c%-5u",LB_ORD_DELIMITER, Ordinal);
	rc.push_back(stmp);
    }
    return rc;
}

std::vector<std::string> NE_Parser::NERNamesReadItems(binary_stream& handle,size_t nnames)
{
    return RNamesReadItems(handle,nnames,ne.neOffsetResidentNameTable + headshift());
}

std::vector<std::string> NE_Parser::NENRNamesReadItems(binary_stream& handle,size_t nnames)
{
    return RNamesReadItems(handle,nnames,ne.neOffsetNonResidentNameTable);
}

std::vector<SEGDEF> NE_Parser::__ReadSegTableNE(binary_stream& handle,size_t nnames)
{
    std::vector<SEGDEF> rc;
    unsigned i;
    for(i = 0;i < nnames;i++) {
	SEGDEF sd;
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(&sd,sizeof(SEGDEF));
	rc.push_back(sd);
    }
    return rc;
}

unsigned NE_Parser::GetNamCountNE(binary_stream& handle,__filesize_t offset )
{
 unsigned i;
 i = 0;
 if(!offset) return 0;
 handle.seek(offset,binary_stream::Seek_Set);
 while(1)
 {
   unsigned char l;
   l = handle.read(type_byte);
   if(l == 0 || handle.eof()) break;
   handle.seek(l + 2,binary_stream::Seek_Cur);
   i++;
   if(i > 0xFFFD) break;
 }
 return i;
}

unsigned NE_Parser::NERNamesNumItems(binary_stream& handle)
{
   return GetNamCountNE(handle,headshift() + ne.neOffsetResidentNameTable);
}

unsigned NE_Parser::NENRNamesNumItems(binary_stream& handle)
{
   return GetNamCountNE(handle,ne.neOffsetNonResidentNameTable);
}

void NE_Parser::ReadEntryItemNE(binary_stream& handle,ENTRY * obj,unsigned char etype)
{
 obj->eFixed = etype;
 if(etype)
 {
  if(etype == 0xFF)
  {
      obj->eFlags = handle.read(type_byte);
      handle.seek(2,binary_stream::Seek_Cur); /** int 3F */
      obj->eSegNum = handle.read(type_byte);
      obj->eSegOff = handle.read(type_word);
  }
  else
  {
     obj->eFlags = handle.read(type_byte);
     obj->eSegOff = handle.read(type_word);
  }
 }
 if(etype != 0xFE && etype != 0xFF) obj->eSegNum = etype;
}

void NE_Parser::SkipEntryItemNE(binary_stream& handle,unsigned char etype)
{
 if(etype)
 {
  if(etype == 0xFF) handle.seek(6,binary_stream::Seek_Cur); /** moveable */
  else
   handle.seek(3,binary_stream::Seek_Cur); /** fixed */
 }
}

bool NE_Parser::ReadEntryNE(ENTRY * obj,unsigned entnum)
{
  binary_stream& handle = *ne_cache1;
  unsigned i,j;
  unsigned char nentry,etype;
  handle.seek((__fileoff_t)headshift() + ne.neOffsetEntryTable,binary_stream::Seek_Set);
  i = 0;
  while(1)
  {
     nentry = handle.read(type_byte);
     if(nentry == 0 || handle.eof()) break;
     etype = handle.read(type_byte);
     for(j = 0;j < nentry;j++,i++)
     {
       if(i == entnum - 1)
       {
	 ReadEntryItemNE(handle,(ENTRY *)obj,etype);
	 return true;
       }
       else SkipEntryItemNE(handle,etype);
     }
  }
  return false;
}

bool NE_Parser::ReadSegDefNE(SEGDEF * obj,unsigned segnum) const
{
 binary_stream* handle;
  handle = ne_cache3;
  if(segnum > ne.neSegmentTableCount || !segnum) return false;
  handle->seek((__fileoff_t)headshift() + ne.neOffsetSegmentTable + (segnum - 1)*sizeof(SEGDEF),binary_stream::Seek_Set);
  handle->read((any_t*)obj,sizeof(SEGDEF));
  return true;
}

__filesize_t  NE_Parser::CalcEntryPointNE( unsigned segnum, unsigned offset )
{
  SEGDEF seg;
  __filesize_t shift;
  if(!ReadSegDefNE(&seg,segnum)) return 0;
  shift = seg.sdOffset ? (((unsigned long)seg.sdOffset)<<ne.neLogicalSectorShiftCount) + offset : 0L;
  return shift;
}

__filesize_t  NE_Parser::CalcEntryNE(unsigned ord,bool dispmsg)
{
  ENTRY entr;
  SEGDEF segd;
  int segnum;
  if(!ReadEntryNE(&entr,ord)) { if(dispmsg) beye_context().ErrMessageBox(NOT_ENTRY,""); return 0L; }
  if(entr.eFixed == 0xFE)
  {
    char outs[100];
    if(dispmsg)
    {
      sprintf(outs,"This entry is constant : %04hXH",entr.eSegOff);
      beye_context().TMessageBox(outs,"Virtual entry");
    }
    return 0L;
  }
  else                    segnum = entr.eSegNum;
  if(ReadSegDefNE(&segd,segnum))
  {
    return segd.sdOffset ? (((__filesize_t)segd.sdOffset)<<ne.neLogicalSectorShiftCount) + entr.eSegOff : 0L;
  }
  else if(dispmsg) beye_context().ErrMessageBox(NO_ENTRY,BAD_ENTRY);
  return 0L;
}

__filesize_t NE_Parser::action_F10()
{
    binary_stream& handle = *ne_cache;
    unsigned nnames;
    __filesize_t fpos;
    nnames = ne.neSegmentTableCount;
    fpos = beye_context().tell();
    if(!nnames) { beye_context().NotifyBox(NOT_ENTRY," Segment Definition "); return fpos; }
    handle.seek((__fileoff_t)headshift() + ne.neOffsetSegmentTable,binary_stream::Seek_Set);
    std::vector<SEGDEF> objs = __ReadSegTableNE(handle,nnames);
    if(!objs.empty()) {
	int i = PageBox(65,17,objs,*this,&NE_Parser::SegPaintNE) + 1;
	if(i > 0) fpos = (__filesize_t)objs[i-1].sdOffset<<ne.neLogicalSectorShiftCount;
    }
    return fpos;
}

std::vector<ENTRY> NE_Parser::__ReadEntryTableNE(binary_stream& handle)
{
    std::vector<ENTRY> rc;
    unsigned i;
    unsigned char j,nentry;
    i = 0;
    while(1) {
	unsigned char etype;
	nentry = handle.read(type_byte);
	if(nentry == 0 || handle.eof()) break;
	etype = handle.read(type_byte);
	for(j = 0;j < nentry;j++,i++) {
	    ENTRY ent;
	    if(IsKbdTerminate()) break;
	    ReadEntryItemNE(handle,&ent,etype);
	    rc.push_back(ent);
	}
    }
    return rc;
}

unsigned NE_Parser::GetEntryCountNE()
{
 binary_stream& handle = *ne_cache;
 unsigned i,j;
 unsigned char nentry;
 handle.seek((__fileoff_t)headshift() + ne.neOffsetEntryTable,binary_stream::Seek_Set);
 i = 0;
 while(1)
 {
   unsigned char etype;
   nentry = handle.read(type_byte);
   if(nentry == 0 || handle.eof()) break; /** end of table */
   else
   {
     etype = handle.read(type_byte);
     for(j = 0;j < nentry;j++,i++) { SkipEntryItemNE(handle,etype); if(i > 0xFFFD || IsKbdTerminate()) goto exit; }
   }
 }
 exit:
 return i;
}

__filesize_t NE_Parser::action_F6()
{
    binary_stream& handle = *ne_cache;
    unsigned nnames;
    __filesize_t fpos;
    nnames = GetEntryCountNE();
    fpos = beye_context().tell();
    if(!nnames) { beye_context().NotifyBox(NOT_ENTRY," Entries "); return fpos; }
    handle.seek((__fileoff_t)headshift() + ne.neOffsetEntryTable,binary_stream::Seek_Set);
    std::vector<ENTRY> objs = __ReadEntryTableNE(handle);
    if(!objs.empty()) {
	int i = PageBox(50,6,objs,*this,&NE_Parser::EntPaintNE) + 1;
	if(i > 0)  fpos = CalcEntryNE(i,true);
    }
    return fpos;
}

const char * ResourceGrNames[] =
{
  "RESERVED 0",
  "CURSOR",
  "BITMAP",
  "ICON",
  "MENU",
  "DIALOG",
  "STRINGTABLE",
  "FONTDIR",
  "FONT",
  "ACCELERATOR",
  "RCDATA",
  "RESERVED 11",
  "GROUP CURSOR",
  "RESERVED 13",
  "GROUP ICON",
  "NAME TABLE",
  "VERSIONINFO"
};

char* NE_Parser::GetResourceIDNE(binary_stream& handle,unsigned rid,__filesize_t BegResTab)
{
 static char buff[30];
 unsigned char nByte;
 if(rid & 0x8000) sprintf(buff,"%hi",rid & 0x7FFF);
 else
 {
   __filesize_t pos;
   pos = handle.tell();
   handle.seek(BegResTab + rid,binary_stream::Seek_Set);
   nByte = handle.read(type_byte);
   if(nByte > 26)
   {
     handle.read(buff,26);
     strcat(buff,"...");
     nByte = 29;
   }
   else if(nByte) handle.read(buff,nByte);
   buff[nByte] = 0;
   handle.seek(pos,binary_stream::Seek_Set);
 }
 return buff;
}

std::vector<std::string> NE_Parser::__ReadResourceGroupNE(binary_stream& handle,size_t nitems,long * addr)
{
    std::vector<std::string> rc;
    unsigned i,j;
    uint_fast16_t rcAlign,rTypeID,rcount;
    unsigned long BegResTab;
    char buff[81];
    BegResTab = handle.tell();
    rcAlign = handle.read(type_word);
    for(i = 0;i < nitems;i++) {
	addr[i++] = handle.tell();
	rTypeID = handle.read(type_word);
	rcount = handle.read(type_word);
	handle.seek(4,binary_stream::Seek_Cur);
	if(IsKbdTerminate() || handle.eof()) break;
	if(rTypeID & 0x8000) {
	    rTypeID &= 0x7FFF;
	    if(rTypeID < 17) strcpy(buff,ResourceGrNames[rTypeID]);
	    else             sprintf(buff,"< Ordinal type: %04hXH >",rTypeID);
	}
	else  sprintf(buff,"\"%s\"",GetResourceIDNE(handle,rTypeID,BegResTab));
	rc.push_back(buff);
	for(j = 0;j < rcount;j++)  {
	    NAMEINFO nam;
	    char stmp[81];
	    if(IsKbdTerminate() || handle.eof()) break;
	    handle.read(&nam,sizeof(NAMEINFO));
	    addr[i++] = ((unsigned long)nam.rnOffset)<<rcAlign;
	    sprintf(stmp," %s <length: %04hXH> %s %s %s",
		   GetResourceIDNE(handle,nam.rnID,BegResTab),
		   (unsigned)((unsigned long)nam.rnLength)<<rcAlign,
		   ((nam.rnFlags & 0x0010) ? "MOVEABLE" : "FIXED"),
		   ((nam.rnFlags & 0x0020) ? "PURE"     : "IMPURE"),
		   ((nam.rnFlags & 0x0040) ? "PRELOAD"  : "LOADONCALL")
		   );
	    rc.push_back(stmp);
	}
	i--;
    }
    return rc;
}

unsigned int NE_Parser::GetResourceGroupCountNE(binary_stream& handle)
{
 uint_fast16_t rcount, rTypeID;
 int count = 0;
 __filesize_t pos;
 if(ne.neOffsetResourceTable == ne.neOffsetResidentNameTable) return 0;
 pos = handle.tell();
 handle.seek(2L,binary_stream::Seek_Cur); /** rcAlign */
 while(1)
 {
   rTypeID = handle.read(type_word);
   if(rTypeID)
   {
     rcount = handle.read(type_word);
     handle.seek(rcount*sizeof(NAMEINFO) + 4,binary_stream::Seek_Cur);
     count += rcount + 1;
     if(count > 0xF000 || IsKbdTerminate() || handle.eof()) break;
   }
   else break;
 }
 handle.seek(pos,binary_stream::Seek_Set);
 return count;
}

__filesize_t NE_Parser::action_F7()
{
    __filesize_t fpos;
    binary_stream& handle = *ne_cache;
    long * raddr;
    unsigned nrgroup;
    fpos = beye_context().tell();
    handle.seek((__fileoff_t)headshift() + ne.neOffsetResourceTable,binary_stream::Seek_Set);
    if(!(nrgroup = GetResourceGroupCountNE(handle))) { beye_context().NotifyBox(NOT_ENTRY," Resources "); return fpos; }
    if(!(raddr  = new long [nrgroup])) return fpos;
    std::vector<std::string> objs = __ReadResourceGroupNE(handle,nrgroup,raddr);
    if(!objs.empty()) {
	int i = ListBox(objs," Resource groups : ",LB_SELECTIVE,-1);
	if(i != -1) fpos = raddr[i];
    }
    delete raddr;
    return fpos;
}

__filesize_t NE_Parser::action_F3()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    unsigned ordinal;
    std::string title = RES_NAMES;
    ssize_t nnames = NERNamesNumItems(main_handle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = NERNamesReadItems(main_handle(),nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
    if(ret != -1) {
	const char* cptr;
	char buff[40];
	cptr = strrchr(objs[ret].c_str(),LB_ORD_DELIMITER);
	cptr++;
	strcpy(buff,cptr);
	ordinal = atoi(buff);
    }
exit:
    if(ret != -1) fpos = CalcEntryNE(ordinal,true);
    return fpos;
}

__filesize_t NE_Parser::action_F4()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    unsigned ordinal;
    std::string title = NORES_NAMES;
    ssize_t nnames = NENRNamesNumItems(main_handle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = NENRNamesReadItems(main_handle(),nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
    if(ret != -1) {
	const char* cptr;
	char buff[40];
	cptr = strrchr(objs[ret].c_str(),LB_ORD_DELIMITER);
	cptr++;
	strcpy(buff,cptr);
	ordinal = atoi(buff);
    }
exit:
    if(ret != -1) fpos = CalcEntryNE(ordinal,true);
    return fpos;
}

/***************************************************************************/
/************************ RELOCATION FOR NE  *******************************/
/***************************************************************************/
static unsigned CurrChainSegment = 0xFFFF;
static unsigned long CurrSegmentStart = 0;
static unsigned long CurrSegmentLength = 0;
static int           CurrSegmentHasReloc = -1;
static std::set<NERefChain> CurrNEChain;
static char __codelen,__type;

void NE_Parser::BuildNERefChain(__filesize_t segoff,__filesize_t slength)
{
  unsigned nchains,i;
  __filesize_t reloc_off;
  TWindow * w;
//  if(!(CurrNEChain = la_Build(0,sizeof(NERefChain),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  w->goto_xy(1,1);
  w->printf(" Building reference chains for segment #%u",CurrChainSegment);
  if(PubNames.empty()) ne_ReadPubNameList(main_handle());
  reloc_off = segoff + slength;

  main_handle().seek(reloc_off,binary_stream::Seek_Set);
  nchains =main_handle().read(type_word);
  reloc_off += 2;
  for(i = 0;i < nchains;i++)
  {
     __filesize_t this_reloc_off;
     RELOC_NE rne;
     NERefChain nrc;
     this_reloc_off = reloc_off + i*sizeof(RELOC_NE);
     main_handle().seek(this_reloc_off,binary_stream::Seek_Set);
     main_handle().read(&rne,sizeof(RELOC_NE));
     if(IsKbdTerminate() || main_handle().eof()) break;
     nrc.offset = rne.RefOff;
     nrc.number = i;
     CurrNEChain.insert(nrc);
     if(!(rne.Type & 0x04) && rne.AddrType) {/** if not additive fixup and not byte fixup */
       while(1) {
	 unsigned next_off;
	 std::set<NERefChain>::const_reverse_iterator it = CurrNEChain.rbegin();
	 main_handle().seek(segoff + (__filesize_t)(*it).offset,binary_stream::Seek_Set);
	 next_off =main_handle().read(type_word);
	 if(main_handle().eof()) break;
	 if(next_off > slength || next_off == 0xFFFF || next_off == (*it).offset) break;
	 nrc.offset = next_off;
	 nrc.number = i;
	 CurrNEChain.insert(nrc);
       }
     }
  }
//  la_Sort(CurrNEChain,compare_chains);
  delete w;
}

RELOC_NE* NE_Parser::__found_RNE(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen)
{
  NERefChain key;
  std::set<NERefChain>::const_iterator it;
  static RELOC_NE rne;
  if(segnum != CurrChainSegment || CurrNEChain.empty())
  {
    CurrNEChain.clear();
    CurrChainSegment = segnum;
    BuildNERefChain(segoff,slength);
  }
  key.offset = keyoff;
  __codelen = codelen;
  it = CurrNEChain.find(key);
  if(it!=CurrNEChain.end()) {
    main_handle().seek(segoff + slength + 2 + sizeof(RELOC_NE)*(*it).number,binary_stream::Seek_Set);
    main_handle().read(&rne,sizeof(rne));
    return &rne;
  }
  else      return 0;
}

RELOC_NE* NE_Parser::__found_RNE_spec(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen,int type)
{
  NERefChain key;
  std::set<NERefChain>::const_iterator it;
  static RELOC_NE rne;
  if(segnum != CurrChainSegment || CurrNEChain.empty())
  {
    CurrNEChain.clear();
    CurrChainSegment = segnum;
    BuildNERefChain(segoff,slength);
  }
  key.offset = keyoff;
  __codelen = codelen;
  __type = type;
  it = CurrNEChain.find(key);
  if(it!=CurrNEChain.end())
  {
    binary_stream* b_cache;
    b_cache = ne_cache;
    b_cache->seek(segoff + slength + 2 + sizeof(RELOC_NE)*(*it).number,binary_stream::Seek_Set);
    b_cache->read(&rne,sizeof(rne));
    return &rne;
  }
  else      return 0;
}

unsigned NE_Parser::__findSpecType(__filesize_t sstart,__filesize_t ssize,unsigned segnum,__filesize_t target,char codelen,char type,unsigned defval)
{
   unsigned ret;
   RELOC_NE * rne;
   rne = __found_RNE_spec(sstart,ssize,segnum,(unsigned)(target - sstart),codelen,type);
   if(rne) ret = rne->ordinal;
   else    ret = defval;
   return ret;
}

std::string NE_Parser::rdImpNameNELX(unsigned idx,bool useasoff,__filesize_t OffTable)
{
  unsigned char len;
  __filesize_t name_off;
  binary_stream* b_cache;
  b_cache = ne_cache2;
  name_off = OffTable;
  if(!useasoff)
  {
    __filesize_t ref_off;
    ref_off = (__filesize_t)headshift()
	      + ne.neOffsetModuleReferenceTable
	      + (idx - 1)*2;
	main_handle().seek(ref_off,binary_stream::Seek_Set);
	name_off += main_handle().read(type_word);
  }
  else name_off += idx;
  b_cache->seek(name_off,binary_stream::Seek_Set);
  len = b_cache->read(type_byte);
  char buff[len+1];
  b_cache->read(buff,len);
  buff[len] = 0;
  return buff;
}

std::string NE_Parser::rd_ImpName(unsigned idx,bool useasoff)
{
  return rdImpNameNELX(idx,useasoff,headshift() + ne.neOffsetImportTable);
}

bool NE_Parser::BuildReferStrNE(const DisMode& parent,std::string& str,RELOC_NE *rne,int flags,__filesize_t ulShift)
{
  std::string buff;
  char stmp[4096];
  const char *pref;
  bool retrf;
  char reflen;
  bool need_virt;
  reflen = 0;
  pref = "";
  retrf = false;
  need_virt = (flags & APREF_SAVE_VIRT);
  switch(rne->AddrType)
  {
      case 0: reflen = 1; pref = "(b) "; break;
      case 2: reflen = 2; pref = "seg "; break;
      case 3: reflen = 4; pref = "seg:off16 "; break;
      case 5: reflen = 2; pref = "off16 "; break;
      case 11: reflen = 6; pref = "seg:off32 "; break;
      case 13: reflen = 4; pref = "off32 "; break;
      default: break;
  }
  if(flags & APREF_USE_TYPE) str+=pref;
  if((rne->Type & 3) == 1 || (rne->Type & 3) == 2) /** imported type */
  {
    retrf = true;
    buff=rd_ImpName(rne->idx,0);
    sprintf(stmp,"<%s>.",buff.c_str());
    str+=stmp;
    if((rne->Type & 3) == 1) { sprintf(stmp,"@%hu",rne->ordinal); str+=stmp; }
    else
    {
      str+=rd_ImpName(rne->ordinal,true);
    }
  }
  else
   if((rne->Type & 3) == 0)
   {
     if(rne->idx == 0x00FF && rne->AddrType != 2)
     {
       __filesize_t ea;
       ea = CalcEntryNE(rne->ordinal,false);
       if(FindPubName(buff,ea))
	  str+=buff;
       else
       {
	 retrf = ea?true:false;
	 sprintf(stmp,"(*this).@%hu",rne->ordinal);
	 str+=stmp;
       }
       if(!DumpMode && !EditMode && !(flags & APREF_USE_TYPE)) code_guider().add_go_address(parent,str,ea);
     }
     else
     {
       __filesize_t ep;
       ep = CalcEntryPointNE(rne->idx,rne->ordinal);
       if(FindPubName(buff,ep))
	 str+=buff;
       else
       {
	 if(need_virt) sprintf(stmp,".%08lX",(unsigned long)pa2va(ep));
	 else sprintf(stmp,"(*this).seg<#%hu>:%sH",rne->idx,Get4Digit(rne->ordinal));
	 str+=stmp;
	 retrf = ep?true:false;
       }
       if(!DumpMode && !EditMode && !(flags & APREF_USE_TYPE)) code_guider().add_go_address(parent,str,ep);
     }
   }
   else
   {
      str+="?OSFIXUP?";
   }
   if((rne->Type & 4) == 4)
   {
     __filesize_t data;
     if(reflen && reflen <= 4)
     {
       str+="+";
       main_handle().seek(ulShift,binary_stream::Seek_Set);
       data = main_handle().read(type_dword);
       str+=(reflen == 1)? Get2Digit(data) : (reflen == 2) ? Get4Digit(data) : Get8Digit(data);
     }
     else str+=",<add>";
   }
   return retrf;
}

bool NE_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
    unsigned i;
    __filesize_t segpos,slength;
    std::string buff;
    if(flags & APREF_TRY_PIC) return false;
    if(ulShift >= CurrSegmentStart && ulShift <= CurrSegmentStart + CurrSegmentLength)
    {
       i = CurrChainSegment - 1;
       if(CurrSegmentHasReloc) goto Direct;
       else                    goto TryLabel;
    }
    for(i = 0;i < ne.neSegmentTableCount;i++)
    {
      SEGDEF sd;
      ReadSegDefNE(&sd,i + 1);
      segpos = ((__filesize_t)sd.sdOffset) << ne.neLogicalSectorShiftCount;
      if(!segpos) continue;
      slength = sd.sdLength;
      if((sd.sdFlags & 0x4000) == 0x4000) slength <<= ne.neLogicalSectorShiftCount;
      if(ulShift >= segpos && ulShift <= segpos + slength) /** we insize segment */
      {
	 RELOC_NE *rne;
	 CurrSegmentStart = segpos;
	 CurrSegmentLength = slength;
	 CurrSegmentHasReloc = (sd.sdFlags >> 8) & 1;
	 if(!CurrSegmentHasReloc) return false;
	 Direct:
	 rne = __found_RNE(CurrSegmentStart,CurrSegmentLength,i + 1,(unsigned)(ulShift - CurrSegmentStart),codelen);
	 if(rne)
	 {
	    if(rne->AddrType == 2)
	    {
	      main_handle().seek(ulShift,binary_stream::Seek_Set);
	      rne->ordinal = main_handle().read(type_word);
	      rne->ordinal = __findSpecType(CurrSegmentStart,CurrSegmentLength,i + 1,ulShift,codelen,5,rne->ordinal);
	    }
	    if(rne->AddrType == 5)
	    {
	      rne->idx    = __findSpecType(CurrSegmentStart,CurrSegmentLength,i + 1,ulShift,codelen,2,rne->idx);
	    }
	    return BuildReferStrNE(parent,str,rne,flags,ulShift);
	 }
	 else
	 {
	   TryLabel:
	   if(flags & APREF_TRY_LABEL)
	   {
	      if(FindPubName(buff,r_sh))
	      {
		str+=buff;
		if(!DumpMode && !EditMode) code_guider().add_go_address(parent,str,r_sh);
		return true;
	      }
	   }
	 }
	 return false;
      }
    }
  return false;
}

/** return false if unsuccess true otherwise */
bool NE_Parser::ReadPubNames(binary_stream& handle,__filesize_t offset)
{
 symbolic_information pnam;
 ENTRY ent;
 unsigned ord,i = 0;
 __filesize_t noff;
 bool ret;
 if(!offset) return false;
 ret = true;
 handle.seek(offset,binary_stream::Seek_Set);
 while(1)
 {
   unsigned char l;
   noff = handle.tell();
   l = handle.read(type_byte);
   if(l == 0 || handle.eof()) { ret = true; break; }
   handle.seek(l,binary_stream::Seek_Cur);
   ord = handle.read(type_word);
   if(ord)
   {
     if(ReadEntryNE(&ent,ord))
     {
       pnam.pa = CalcEntryNE(ord,false);
       pnam.nameoff = noff;
       pnam.attr = SC_GLOBAL;
     }
     else
     {
       ret = false;
       break;
     }
     if(pnam.pa) PubNames.insert(pnam);
   }
   i++;
   if(i > 0xFFFD || handle.eof()) { ret = false; break; }
 }
 return ret;
}

std::string NE_Parser::ne_ReadPubName(binary_stream& b_cache,const symbolic_information& it)
{
    unsigned char rlen;
    b_cache.seek(it.nameoff,binary_stream::Seek_Set);
    rlen = b_cache.read(type_byte);
    char stmp[rlen+1];
    b_cache.read(stmp,rlen);
    stmp[rlen] = 0;
    return stmp;
}

bool NE_Parser::FindPubName(std::string& buff,__filesize_t pa)
{
    symbolic_information key;
    std::set<symbolic_information>::const_iterator it;
    key.pa = pa;
    if(PubNames.empty()) ne_ReadPubNameList(*ne_cache2);
    it = PubNames.find(key);
    if(it!=PubNames.end()) {
	buff=ne_ReadPubName(*ne_cache2,*it);
	return true;
    }
    return _udn().find(pa,buff);
}

void NE_Parser::ne_ReadPubNameList(binary_stream& handle)
{
   {
     ReadPubNames(handle,headshift() + ne.neOffsetResidentNameTable);
     ReadPubNames(handle,ne.neOffsetNonResidentNameTable);
//     if(PubNames->nItems)
//       la_Sort(PubNames,fmtComparePubNames);
   }
}

NE_Parser::NE_Parser(binary_stream& h,CodeGuider& __code_guider,udn& u)
	:MZ_Parser(h,__code_guider,u)
{
    h.seek(headshift(),binary_stream::Seek_Set);
    h.read(&ne,sizeof(NEHEADER));
    ne_cache3 = h.dup();
    ne_cache1 = h.dup();
    ne_cache = h.dup();
    ne_cache2 = h.dup();
}

NE_Parser::~NE_Parser()
{
  binary_stream& h = main_handle();
  CurrNEChain.clear();
  PubNames.clear();
  if(ne_cache != &h) delete ne_cache;
  if(ne_cache2 != &h) delete ne_cache2;
  if(ne_cache3 != &h) delete ne_cache3;
  if(ne_cache1 != &h) delete ne_cache1;
}

__filesize_t NE_Parser::action_F1()
{
  hlpDisplay(10006);
  return beye_context().tell();
}

__filesize_t NE_Parser::va2pa(__filesize_t va)
{
  SEGDEF nesd;
  uint_fast16_t seg,off;
  seg = (va >> 16) & 0xFFFFU;
  off = va & 0xFFFFU;
  if(!ReadSegDefNE(&nesd,seg)) return 0L;
  return nesd.sdOffset ? (((__filesize_t)nesd.sdOffset)<<ne.neLogicalSectorShiftCount) + off : 0;
}

__filesize_t NE_Parser::pa2va(__filesize_t pa)
{
  unsigned i,segcount = ne.neSegmentTableCount;
  __filesize_t currseg_st,nextseg_st;
  for(i = 0;i < segcount-1;i++)
  {
    SEGDEF nesd_c/*,nesd_n*/;
    if(!ReadSegDefNE(&nesd_c,i+1)) return 0L;
    currseg_st = (((__filesize_t)nesd_c.sdOffset)<<ne.neLogicalSectorShiftCount);
    if(!currseg_st) continue;
/*    if(!ReadSegDefNE(&nesd_n,i+2)) goto it_seg;*/
/*    nextseg_st = (((unsigned long)nesd_n.sdOffset)<<ne.neLogicalSectorShiftCount);*/
    nextseg_st = currseg_st + nesd_c.sdLength;
    if(pa >= currseg_st && pa < nextseg_st)
    {
/*      it_seg:*/
      return ((__filesize_t)(i+1) << 16) + (unsigned)(pa - currseg_st);
    }
/*
    if(i == segcount-2 && pa >= nextseg_st)
      return ((__filesize_t)(i+2) << 16) + (unsigned)(pa - nextseg_st);
*/
  }
  return 0L;
}

__filesize_t NE_Parser::get_public_symbol(std::string& str,unsigned& func_class,
			  __filesize_t pa,bool as_prev)
{
    __filesize_t fpos;
    if(PubNames.empty()) ne_ReadPubNameList(*ne_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    fpos=find_symbolic_information(PubNames,func_class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	str=ne_ReadPubName(*ne_cache,*idx);
    }
    return fpos;
}

unsigned NE_Parser::__get_object_attribute(__filesize_t pa,std::string& name,
		      __filesize_t& start,__filesize_t& end,int& _class,int& bitness) const
{
  __filesize_t currseg_st;
  unsigned i,segcount = ne.neSegmentTableCount,ret;
  bool found;
  start = 0;
  end = main_handle().flength();
  _class = OC_NOOBJECT;
  bitness = DAB_USE16;
  name.clear();
  ret = 0;
  main_handle().seek((__fileoff_t)headshift() + ne.neOffsetSegmentTable,binary_stream::Seek_Set);
  found = false;
  for(i = 0;i < segcount;i++)
  {
    SEGDEF nesd_c;
    if(!ReadSegDefNE(&nesd_c,i+1)) return 0L;
    currseg_st = (((__filesize_t)nesd_c.sdOffset)<<ne.neLogicalSectorShiftCount);
    if(!currseg_st) { start = end; continue; } /** BSS segment */
    if(pa < currseg_st)
    {
      start = end;
      end = currseg_st;
      found = true;
      ret = i;
      break;
    }
    if(pa >= currseg_st && pa < currseg_st + nesd_c.sdLength)
    {
      start = currseg_st;
      end = start + nesd_c.sdLength;
      _class = nesd_c.sdFlags & 0x01 ? OC_DATA : OC_CODE;
      bitness = nesd_c.sdFlags & 0x2000 ? DAB_USE32 : DAB_USE16;
      ret = i+1;
      found = true;
      break;
    }
    start = currseg_st;
    end = currseg_st + nesd_c.sdLength;
  }
  if(!found)
  {
    start = end;
    end = main_handle().flength();
  }
  return ret;
}

unsigned NE_Parser::get_object_attribute(__filesize_t pa,std::string& name,
		      __filesize_t& start,__filesize_t& end,int& _class,int& bitness) {
    return __get_object_attribute(pa,name,start,end,_class,bitness);
}

int NE_Parser::query_bitness(__filesize_t pa) const
{
  static __filesize_t st = 0,end = 0;
  std::string name;
  int _class;
  static int bitness;
  if(!(pa >= st && pa < end))
  {
    __get_object_attribute(pa,name,st,end,_class,bitness);
  }
  return bitness;
}

bool NE_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos >= headshift() && cfpos < headshift() + sizeof(NEHEADER))
  {
     addr="NEH :";
     addr+=Get4Digit(cfpos - headshift());
  }
  else
  if(cfpos >= headshift() + ne.neOffsetSegmentTable &&
     cfpos <  headshift() + ne.neOffsetSegmentTable + ne.neSegmentTableCount*sizeof(SEGDEF))
  {
    addr="NESD:";
    addr+=Get4Digit(cfpos - headshift() - ne.neOffsetSegmentTable);
  }
  else
  if(cfpos >= headshift() + ne.neOffsetEntryTable &&
     cfpos <  headshift() + ne.neOffsetEntryTable + ne.neLengthEntryTable)
  {
    addr="NEET:";
    addr+=Get4Digit(cfpos - headshift() - ne.neOffsetEntryTable);
  }
  else
    if((res=pa2va(cfpos))!=0)
    {
      addr = ".";
      addr+=Get8Digit(res);
    }
    else bret = false;
  return bret;
}

int NE_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe(binary_stream& main_handle) {
    char id[2];
    __filesize_t headshift = MZ_Parser::is_new_exe(main_handle);
    if(headshift) {
	main_handle.seek(headshift,binary_stream::Seek_Set);
	main_handle.read(id,sizeof(id));
	if(id[0] == 'N' && id[1] == 'E') return true;
    }
    return false;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) NE_Parser(h,_parent,u); }
extern const Binary_Parser_Info ne_info = {
    "NE (New Exe)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
