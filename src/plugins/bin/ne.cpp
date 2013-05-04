#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "beye.h"
#include "colorset.h"
#include "plugins/disasm.h"
#include "plugins/bin/ne.h"
#include "bin_util.h"
#include "bmfile.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "codeguid.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
static CodeGuider* code_guider;
static NEHEADER ne;

static BFile* ne_cache = &bNull;
static BFile* ne_cache1 = &bNull;
static BFile* ne_cache2 = &bNull;
static BFile* ne_cache3 = &bNull;

static __filesize_t  __FASTCALL__ CalcEntryPointNE( unsigned,unsigned );
static void __FASTCALL__ ne_ReadPubNameList(BFile& handle,void (__FASTCALL__ *mem_out)(const std::string&));
static bool   __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ rd_ImpName(char *buff,int blen,unsigned idx,bool useasoff);
static __filesize_t __FASTCALL__ nePA2VA(__filesize_t pa);

const char * __nedata[] =
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

static const char * __FASTCALL__ __getNEType(unsigned type)
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

const char * __FASTCALL__ GetPMWinAPI(unsigned flag)
{
 flag >>= 8;
 flag &= 0x0003;
 return PMWinAPI[flag];
}

static void  PaintNewHeaderNE_1(TWindow* w)
{
  twPrintF(w,
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

static void  PaintNewHeaderNE_2(TWindow* w)
{
  twPrintF(w,
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
    twSetColorAttr(w,dialog_cset.addinfo);
    twPrintF(w,"Offset of Fast Load Area       = %04hXH"
	     ,ne.neOffsetFastLoadArea); twClrEOL(w);
    twPrintF(w,"\nLength of Fast Load Area       = %hu"
	     ,ne.neLengthFastLoadArea); twClrEOL(w);
    twPrintF(w,"\nWindows version                = %02hu.%02hu"
	     ,(unsigned int)high,(unsigned int)low); twClrEOL(w);
    twPrintF(w,"\n");
  }
  twSetColorAttr(w,dialog_cset.entry);
  twPrintF(w,">Entry Point   %s = %08XH",( ne.neContestEXE & 32768L ) ? "[ LibEntry ]   " : "[ EXEEntry ] ",entryNE);
  twClrEOL(w);
  twSetColorAttr(w,dialog_cset.main);
}

static void ( * nephead[])(TWindow* w) =
{
  PaintNewHeaderNE_1,
  PaintNewHeaderNE_2
};

static void __FASTCALL__ PaintNewHeaderNE(TWindow * win,const any_t**ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  twFocusWin(win);
  twFreezeWin(win);
  twClearWin(win);
  sprintf(text," New Executable Header [%d/%d] ",npage + 1,tpage);
  twSetTitleAttr(win,text,TW_TMODE_CENTER,dialog_cset.title);
  twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
  if(npage < 2)
  {
    twGotoXY(win,1,1);
    (*(nephead[npage]))(win);
  }
  twRefreshFullWin(win);
}

static __filesize_t __FASTCALL__ ShowNewHeaderNE( void )
{
  __fileoff_t pos;
  unsigned CS,IP;
  CS = (unsigned)((ne.neCSIPvalue) >> 16);  /** segment number */
  IP = (unsigned)(ne.neCSIPvalue & 0xFFFF); /** offset within segment */
  entryNE = CalcEntryPointNE(CS,IP);
  pos = BMGetCurrFilePos();
  if(PageBox(70,22,NULL,2,PaintNewHeaderNE) != -1 && entryNE) pos = entryNE;
  return pos;
}

static void  __FASTCALL__ entpaintNE(TWindow* w,const ENTRY *nam,unsigned flags)
{
  twGotoXY(w,1,1);
  twPrintF(w,
	   "Entry Point for %s segment\n"
	   "Entry point is %s EXPORTED\n"
	   "The Entry %s uses SHARED data segment\n"
	   "Numbers of word that compose the stack %u\n"
	   "Segment offset = %XH bytes\n"
	   "Segment number = %u"
	   ,(unsigned char)(nam->eFixed) == 0xFF ? "MOVEABLE" : "FIXED"
	   ,flags & 0x0001 ? "  " : "NO"
	   ,flags & 0x0002 ? "  " : "NO"
	   ,(flags & 0xFFF4 >> 2)
	   ,nam->eSegOff
	   ,((unsigned)((unsigned char)(nam->eSegNum))));
}

static void  __FASTCALL__ paintdummyentryNE(TWindow* w)
{
    twGotoXY(w,1,3);
    twPrintF(w,"   Entry point not present ( Dummy bungle )");
}

static void __FASTCALL__ SegPaintNE(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const SEGDEF ** nam = (const SEGDEF **)names;
 unsigned flags = nam[start]->sdFlags;
 twFocusWin(win);
 twFreezeWin(win);
 twClearWin(win);
 sprintf(buffer," Segment Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 twGotoXY(win,1,1);
 if(nam[start]->sdOffset)
   twPrintF(win,"Relative offset from begining in sectors     = %04hXH\n"
	    ,nam[start]->sdOffset);
 else
   twPrintF(win,"No data of segment in the file\n");
 twPrintF(win,
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
	  ,nam[start]->sdLength ? nam[start]->sdLength : (unsigned short)0xFFFF
	  ,nam[start]->sdMinMemory ? nam[start]->sdMinMemory : (unsigned short)0xFFFF
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
 twRefreshFullWin(win);
}

static void __FASTCALL__ EntPaintNE(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const ENTRY ** nam = (const ENTRY **)names;
 unsigned flags = nam[start]->eFlags;
 twFocusWin(win);
 twFreezeWin(win);
 twClearWin(win);
 sprintf(buffer," Entry Point [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 if(nam[start]->eFixed) entpaintNE(win,nam[start],flags);
 else paintdummyentryNE(win);
 twRefreshFullWin(win);
}

static bool __FASTCALL__ __ReadModRefNamesNE(BFile& handle,memArray * obj)
{
 unsigned i;
 uint_fast16_t offTable;
 handle.seek(ne.neOffsetModuleReferenceTable + beye_context().headshift,SEEKF_START);
 for(i = 0;i < ne.neModuleReferenceTableCount;i++)
 {
   __filesize_t NameOff;
   unsigned char length;
   __filesize_t fp;
   char stmp[256];
   offTable = handle.read_word();
   fp = handle.tell();
   NameOff = (__fileoff_t)beye_context().headshift + offTable + ne.neOffsetImportTable;
   handle.seek(NameOff,SEEKF_START);
   length = handle.read_byte();
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read_buffer(stmp,length);
   stmp[length] = 0;
   handle.seek(fp,SEEKF_START);
   if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

static void __FASTCALL__ ShowProcListNE(int);

static __filesize_t __FASTCALL__ ShowModRefNE( void )
{
 BFile& handle = *ne_cache;
 int ret;
 bool bval;
 unsigned nnames;
 __filesize_t fret;
 memArray * obj;
 TWindow * w;
 fret = BMGetCurrFilePos();
 handle.seek(0L,SEEK_SET);
 if(!(nnames = ne.neModuleReferenceTableCount)) { NotifyBox(NOT_ENTRY,MOD_REFER); return fret; }
 if(!(obj = ma_Build(nnames,true))) goto exit;
 w = PleaseWaitWnd();
 bval = __ReadModRefNamesNE(handle,obj);
 CloseWnd(w);
 if(bval)
 {
   while(1)
   {
     ret = ma_Display(obj,MOD_REFER,LB_SELECTIVE | LB_SORTABLE,-1);
     if(ret != -1)
     {
       ShowProcListNE(ret);
     }
     else break;
   }
 }
 ma_Destroy(obj);
 exit:
 return fret;
}

static bool  __FASTCALL__ isPresent(memArray *arr,unsigned nentry,char *_tmpl)
{
   unsigned i;
   bool ret = false;
   if(nentry)
   {
     for(i = 0;i < nentry;i++)
     {
       if(strcmp((const char *)arr->data[i],_tmpl) == 0) { ret = true; break; }
     }
   }
   return ret;
}


static bool __FASTCALL__ __ReadProcListNE(BFile& handle,memArray * obj,int modno)
{
  unsigned i,count;
  char buff[256];
  SEGDEF tsd;
  modno++;
  count = 0;

  handle.seek(beye_context().headshift+ne.neOffsetSegmentTable,SEEKF_START);
  for(i = 0;i < ne.neSegmentTableCount;i++)
  {
    handle.read_buffer(&tsd,sizeof(SEGDEF));
    if(tsd.sdLength && tsd.sdOffset && tsd.sdFlags & 0x0100)
    {
      __filesize_t spos;
      uint_fast16_t j,nrelocs;
      RELOC_NE rne;
      spos = handle.tell();
      handle.seek(((__fileoff_t)(tsd.sdOffset) << ne.neLogicalSectorShiftCount) + tsd.sdLength,SEEKF_START);
      nrelocs = handle.read_word();
      for(j = 0;j < nrelocs;j++)
      {
	 handle.read_buffer(&rne,sizeof(RELOC_NE));
	 if((rne.Type & 3) && rne.idx == modno)
	 {
	   if((rne.Type & 3) == 1)
	   {
	     sprintf(buff,"< By ordinal >   @%hu",rne.ordinal);
	   }
	   else
	   {
	      rd_ImpName(buff,sizeof(buff),rne.ordinal,true);
	   }
	   if(!isPresent(obj,count,buff))
	   {
	     if(IsKbdTerminate()) goto exit;
	     if(!ma_AddString(obj,buff,true)) goto exit;
	   }
	 }
      }
      handle.seek(spos,SEEKF_START);
    }
  }
  exit:
  return true;
}

static void __FASTCALL__ ShowProcListNE( int modno )
{
 BFile& handle = *ne_cache;
 char ptitle[80],name[50];
 bool __bool;
 memArray* obj;
 TWindow *w;
 handle.seek(0L,SEEK_SET);
 w = PleaseWaitWnd();
 if(!(obj = ma_Build(0,true))) return;
 __bool = __ReadProcListNE(handle,obj,modno);
 CloseWnd(w);
 if(__bool)
 {
     if(!obj->nItems)  { NotifyBox(NOT_ENTRY,MOD_REFER); return; }
     rd_ImpName(name,sizeof(name),modno+1,false);
     sprintf(ptitle,"%s%s ",IMPPROC_TABLE,name);
     ma_Display(obj,ptitle,LB_SORTABLE,-1);
 }
 ma_Destroy(obj);
}
#if 0
static int RNRprevind = -3;
static long RNRprevshift = 0;

static unsigned __FASTCALL__ RNameReadFull(BFile& handle,char * names,unsigned nindex,unsigned long offset)
{
 unsigned char length;
 uint_fast16_t Ordinal;
 unsigned i;
 if(RNRprevind == (nindex - 1) && RNRprevshift)  handle->seek(RNRprevshift,SEEKF_START);
 else
 {
   handle->seek(offset,SEEKF_START);
   for(i = 0;i < nindex;i++)
   {
     length = handle->read_byte();
     handle->seek(length + 2,SEEKF_CUR);
   }
 }
 length = handle->read_byte();
 handle->read_buffer((any_t*)names,length);
 names[length] = 0;
 Ordinal = handle->read_word();
 RNRprevind = nindex;
 RNRprevshift = handle->tell();
 return Ordinal;
}

static unsigned __FASTCALL__ ResNameReadFull(BFile& handle,char * names,unsigned nindex)
{
  return RNameReadFull(handle,names,nindex,ne.neOffsetResidentNameTable + beye_context().headshift);
}

static unsigned __FASTCALL__ NResNameReadFull(BFile& handle,char * names,unsigned nindex)
{
  return RNameReadFull(handle,names,nindex,ne.neOffsetNonResidentNameTable);
}
#endif
bool __FASTCALL__ RNamesReadItems(BFile& handle,memArray * obj,unsigned nnames,__filesize_t offset)
{
 unsigned char length;
 unsigned Ordinal;
 unsigned i;
 char stmp[300]; /* max255 + @ordinal */
 handle.seek(offset,SEEKF_START);
 for(i = 0;i < nnames;i++)
 {
   length = handle.read_byte();
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read_buffer(stmp,length);
   Ordinal = handle.read_word();
   sprintf(&stmp[length],"%c%-5u",LB_ORD_DELIMITER, Ordinal);
   if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

static bool __FASTCALL__ NERNamesReadItems(BFile& handle,memArray * names,unsigned nnames)
{
   return RNamesReadItems(handle,names,nnames,ne.neOffsetResidentNameTable + beye_context().headshift);
}

static bool __FASTCALL__ NENRNamesReadItems(BFile& handle,memArray * names,unsigned nnames)
{
   return RNamesReadItems(handle,names,nnames,ne.neOffsetNonResidentNameTable);
}

static bool  __FASTCALL__ __ReadSegTableNE(BFile& handle,memArray * obj,unsigned nnames)
{
 unsigned i;
 for(i = 0;i < nnames;i++)
 {
   SEGDEF sd;
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read_buffer(&sd,sizeof(SEGDEF));
   if(!ma_AddData(obj,&sd,sizeof(SEGDEF),true)) break;
 }
 return true;
}

unsigned __FASTCALL__ GetNamCountNE(BFile& handle,__filesize_t offset )
{
 unsigned i;
 i = 0;
 if(!offset) return 0;
 handle.seek(offset,SEEKF_START);
 while(1)
 {
   unsigned char l;
   l = handle.read_byte();
   if(l == 0 || handle.eof()) break;
   handle.seek(l + 2,SEEKF_CUR);
   i++;
   if(i > 0xFFFD) break;
 }
 return i;
}

static unsigned __FASTCALL__ NERNamesNumItems(BFile& handle)
{
   return GetNamCountNE(handle,beye_context().headshift + ne.neOffsetResidentNameTable);
}

static unsigned __FASTCALL__ NENRNamesNumItems(BFile& handle)
{
   return GetNamCountNE(handle,ne.neOffsetNonResidentNameTable);
}

static void __FASTCALL__ ReadEntryItemNE(BFile& handle,ENTRY * obj,unsigned char etype)
{
 obj->eFixed = etype;
 if(etype)
 {
  if(etype == 0xFF)
  {
      obj->eFlags = handle.read_byte();
      handle.seek(2,SEEKF_CUR); /** int 3F */
      obj->eSegNum = handle.read_byte();
      obj->eSegOff = handle.read_word();
  }
  else
  {
     obj->eFlags = handle.read_byte();
     obj->eSegOff = handle.read_word();
  }
 }
 if(etype != 0xFE && etype != 0xFF) obj->eSegNum = etype;
}

static void  __FASTCALL__ SkipEntryItemNE(BFile& handle,unsigned char etype)
{
 if(etype)
 {
  if(etype == 0xFF) handle.seek(6,SEEKF_CUR); /** moveable */
  else
   handle.seek(3,SEEKF_CUR); /** fixed */
 }
}

static bool __FASTCALL__ ReadEntryNE(ENTRY * obj,unsigned entnum)
{
  BFile& handle = *ne_cache1;
  unsigned i,j;
  unsigned char nentry,etype;
  handle.seek((__fileoff_t)beye_context().headshift + ne.neOffsetEntryTable,SEEK_SET);
  i = 0;
  while(1)
  {
     nentry = handle.read_byte();
     if(nentry == 0 || handle.eof()) break;
     etype = handle.read_byte();
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

static bool __FASTCALL__ ReadSegDefNE(SEGDEF * obj,unsigned segnum)
{
 BFile* handle;
  handle = ne_cache3;
  if(segnum > ne.neSegmentTableCount || !segnum) return false;
  handle->seek((__fileoff_t)beye_context().headshift + ne.neOffsetSegmentTable + (segnum - 1)*sizeof(SEGDEF),BFile::Seek_Set);
  handle->read_buffer((any_t*)obj,sizeof(SEGDEF));
  return true;
}

static __filesize_t  __FASTCALL__ CalcEntryPointNE( unsigned segnum, unsigned offset )
{
  SEGDEF seg;
  __filesize_t shift;
  if(!ReadSegDefNE(&seg,segnum)) return 0;
  shift = seg.sdOffset ? (((unsigned long)seg.sdOffset)<<ne.neLogicalSectorShiftCount) + offset : 0L;
  return shift;
}

static __filesize_t  __FASTCALL__ CalcEntryNE(unsigned ord,bool dispmsg)
{
  ENTRY entr;
  SEGDEF segd;
  int segnum;
  if(!ReadEntryNE(&entr,ord)) { if(dispmsg) ErrMessageBox(NOT_ENTRY,NULL); return 0L; }
  if(entr.eFixed == 0xFE)
  {
    char outs[100];
    if(dispmsg)
    {
      sprintf(outs,"This entry is constant : %04hXH",entr.eSegOff);
      TMessageBox(outs,"Virtual entry");
    }
    return 0L;
  }
  else                    segnum = entr.eSegNum;
  if(ReadSegDefNE(&segd,segnum))
  {
    return segd.sdOffset ? (((__filesize_t)segd.sdOffset)<<ne.neLogicalSectorShiftCount) + entr.eSegOff : 0L;
  }
  else if(dispmsg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
  return 0L;
}

static __filesize_t __FASTCALL__ ShowSegDefNE( void )
{
 BFile& handle = *ne_cache;
 unsigned nnames;
 __filesize_t fpos;
 memArray * obj;
 nnames = ne.neSegmentTableCount;
 fpos = BMGetCurrFilePos();
 if(!nnames) { NotifyBox(NOT_ENTRY," Segment Definition "); return fpos; }
 if(!(obj = ma_Build(nnames,true))) return fpos;
 handle.seek((__fileoff_t)beye_context().headshift + ne.neOffsetSegmentTable,SEEK_SET);
 if(__ReadSegTableNE(handle,obj,nnames))
 {
    int i;
    i = PageBox(65,17,(const any_t**)obj->data,obj->nItems,SegPaintNE) + 1;
    if(i > 0)
    {
      fpos = ((__filesize_t)((const SEGDEF *)obj->data[i-1])->sdOffset)<<ne.neLogicalSectorShiftCount;
    }
 }
 ma_Destroy(obj);
 return fpos;
}

static bool  __FASTCALL__ __ReadEntryTableNE(BFile& handle,memArray * obj)
{
 unsigned i;
 unsigned char j,nentry;
 i = 0;
 while(1)
 {
   unsigned char etype;
   nentry = handle.read_byte();
   if(nentry == 0 || handle.eof()) break;
   etype = handle.read_byte();
   for(j = 0;j < nentry;j++,i++)
   {
     ENTRY ent;
     if(IsKbdTerminate()) break;
     ReadEntryItemNE(handle,&ent,etype);
     if(!ma_AddData(obj,&ent,sizeof(ENTRY),true)) break;
   }
 }
 return true;
}

static unsigned __FASTCALL__ GetEntryCountNE( void )
{
 BFile& handle = *ne_cache;
 unsigned i,j;
 unsigned char nentry;
 handle.seek((__fileoff_t)beye_context().headshift + ne.neOffsetEntryTable,SEEK_SET);
 i = 0;
 while(1)
 {
   unsigned char etype;
   nentry = handle.read_byte();
   if(nentry == 0 || handle.eof()) break; /** end of table */
   else
   {
     etype = handle.read_byte();
     for(j = 0;j < nentry;j++,i++) { SkipEntryItemNE(handle,etype); if(i > 0xFFFD || IsKbdTerminate()) goto exit; }
   }
 }
 exit:
 return i;
}

static __filesize_t __FASTCALL__ ShowEntriesNE( void )
{
 BFile& handle = *ne_cache;
 unsigned nnames;
 __filesize_t fpos;
 memArray * obj;
 nnames = GetEntryCountNE();
 fpos = BMGetCurrFilePos();
 if(!nnames) { NotifyBox(NOT_ENTRY," Entries "); return fpos; }
 if(!(obj = ma_Build(nnames,true))) return fpos;
 handle.seek((__fileoff_t)beye_context().headshift + ne.neOffsetEntryTable,SEEK_SET);
 if(__ReadEntryTableNE(handle,obj))
 {
  int i;
    i = PageBox(50,6,(const any_t**)obj->data,obj->nItems,EntPaintNE) + 1;
    if(i > 0)  fpos = CalcEntryNE(i,true);
 }
 ma_Destroy(obj);
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

static char *  __FASTCALL__ GetResourceIDNE(BFile& handle,unsigned rid,__filesize_t BegResTab)
{
 static char buff[30];
 unsigned char nByte;
 if(rid & 0x8000) sprintf(buff,"%hi",rid & 0x7FFF);
 else
 {
   __filesize_t pos;
   pos = handle.tell();
   handle.seek(BegResTab + rid,SEEKF_START);
   nByte = handle.read_byte();
   if(nByte > 26)
   {
     handle.read_buffer(buff,26);
     strcat(buff,"...");
     nByte = 29;
   }
   else if(nByte) handle.read_buffer(buff,nByte);
   buff[nByte] = 0;
   handle.seek(pos,SEEKF_START);
 }
 return buff;
}

static bool  __FASTCALL__ __ReadResourceGroupNE(BFile& handle,memArray *obj,unsigned nitems,long * addr)
{
 unsigned i,j;
 uint_fast16_t rcAlign,rTypeID,rcount;
 unsigned long BegResTab;
 char buff[81];
 BegResTab = handle.tell();
 rcAlign = handle.read_word();
 for(i = 0;i < nitems;i++)
 {
    addr[i++] = handle.tell();
    rTypeID = handle.read_word();
    rcount = handle.read_word();
    handle.seek(4,SEEKF_CUR);
    if(IsKbdTerminate() || handle.eof()) break;
    if(rTypeID & 0x8000)
    {
      rTypeID &= 0x7FFF;
      if(rTypeID < 17) strcpy(buff,ResourceGrNames[rTypeID]);
      else             sprintf(buff,"< Ordinal type: %04hXH >",rTypeID);
    }
    else  sprintf(buff,"\"%s\"",GetResourceIDNE(handle,rTypeID,BegResTab));
    if(!ma_AddString(obj,buff,true)) break;
    for(j = 0;j < rcount;j++)
    {
      NAMEINFO nam;
      char stmp[81];
      if(IsKbdTerminate() || handle.eof()) break;
      handle.read_buffer(&nam,sizeof(NAMEINFO));
      addr[i++] = ((unsigned long)nam.rnOffset)<<rcAlign;
      sprintf(stmp," %s <length: %04hXH> %s %s %s",
		   GetResourceIDNE(handle,nam.rnID,BegResTab),
		   (unsigned)((unsigned long)nam.rnLength)<<rcAlign,
		   ((nam.rnFlags & 0x0010) ? "MOVEABLE" : "FIXED"),
		   ((nam.rnFlags & 0x0020) ? "PURE"     : "IMPURE"),
		   ((nam.rnFlags & 0x0040) ? "PRELOAD"  : "LOADONCALL")
		   );
      if(!ma_AddString(obj,stmp,true)) goto exit;
    }
    i--;
 }
 exit:
 return true;
}

static unsigned int  __FASTCALL__ GetResourceGroupCountNE(BFile& handle)
{
 uint_fast16_t rcount, rTypeID;
 int count = 0;
 __filesize_t pos;
 if(ne.neOffsetResourceTable == ne.neOffsetResidentNameTable) return 0;
 pos = handle.tell();
 handle.seek(2L,SEEKF_CUR); /** rcAlign */
 while(1)
 {
   rTypeID = handle.read_word();
   if(rTypeID)
   {
     rcount = handle.read_word();
     handle.seek(rcount*sizeof(NAMEINFO) + 4,SEEKF_CUR);
     count += rcount + 1;
     if(count > 0xF000 || IsKbdTerminate() || handle.eof()) break;
   }
   else break;
 }
 handle.seek(pos,SEEKF_START);
 return count;
}

static __filesize_t __FASTCALL__ ShowResourcesNE( void )
{
 __filesize_t fpos;
 BFile& handle = *ne_cache;
 memArray* rgroup;
 long * raddr;
 unsigned nrgroup;
 fpos = BMGetCurrFilePos();
 handle.seek((__fileoff_t)beye_context().headshift + ne.neOffsetResourceTable,SEEK_SET);
 if(!(nrgroup = GetResourceGroupCountNE(handle))) { NotifyBox(NOT_ENTRY," Resources "); return fpos; }
 if(!(rgroup = ma_Build(nrgroup,true))) goto exit;
 if(!(raddr  = new long [nrgroup])) return fpos;
 if(__ReadResourceGroupNE(handle,rgroup,nrgroup,raddr))
 {
  int i;
   i = ma_Display(rgroup," Resource groups : ",LB_SELECTIVE,-1);
   if(i != -1) fpos = raddr[i];
 }
 delete raddr;
 ma_Destroy(rgroup);
 exit:
 return fpos;
}

static __filesize_t __FASTCALL__ ShowResNamNE( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  ret = fmtShowList(NERNamesNumItems,NERNamesReadItems,
		    RES_NAMES,
		    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryNE(ordinal,true);
  }
  return fpos;
}

static __filesize_t __FASTCALL__ ShowNResNmNE( void )
{
  __filesize_t fpos;
  fpos = BMGetCurrFilePos();
  {
    int ret;
    unsigned ordinal;
    ret = fmtShowList(NENRNamesNumItems,NENRNamesReadItems,
		      NORES_NAMES,
		      LB_SELECTIVE | LB_SORTABLE,&ordinal);
    if(ret != -1)
    {
      fpos = CalcEntryNE(ordinal,true);
    }
  }
  return fpos;
}

static bool __FASTCALL__ IsNEFormat( void )
{
   char id[2];
   beye_context().headshift = IsNewExe();
   if(beye_context().headshift)
   {
     bmReadBufferEx(id,sizeof(id),beye_context().headshift,SEEKF_START);
     if(id[0] == 'N' && id[1] == 'E') return true;
   }
   return false;
}

/***************************************************************************/
/************************ RELOCATION FOR NE  *******************************/
/***************************************************************************/

typedef struct tagNERefChain
{
  unsigned offset;
  unsigned number;
}NERefChain;

static unsigned CurrChainSegment = 0xFFFF;
static unsigned long CurrSegmentStart = 0;
static unsigned long CurrSegmentLength = 0;
static int           CurrSegmentHasReloc = -1;
static linearArray *CurrNEChain = NULL;
static char __codelen,__type;

static tCompare __FASTCALL__ compare_chains(const any_t*v1,const any_t*v2)
{
  const NERefChain  * c1, * c2;
  c1 = (const NERefChain  *)v1;
  c2 = (const NERefChain  *)v2;
  return __CmpLong__(c1->offset,c2->offset);
}


static void  __FASTCALL__ BuildNERefChain(__filesize_t segoff,__filesize_t slength)
{
  unsigned nchains,i;
  __filesize_t reloc_off;
  TWindow * w,*usd;
  usd = twFocusedWin();
  if(!(CurrNEChain = la_Build(0,sizeof(NERefChain),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  twFocusWin(w);
  twGotoXY(w,1,1);
  twPrintF(w," Building reference chains for segment #%u",CurrChainSegment);
  twFocusWin(usd);
  if(!PubNames) ne_ReadPubNameList(bmbioHandle(),MemOutBox);
  reloc_off = segoff + slength;
  nchains = bmReadWordEx(reloc_off,SEEKF_START);
  reloc_off += 2;
  for(i = 0;i < nchains;i++)
  {
     __filesize_t this_reloc_off;
     RELOC_NE rne;
     NERefChain nrc;
     this_reloc_off = reloc_off + i*sizeof(RELOC_NE);
     bmReadBufferEx(&rne,sizeof(RELOC_NE),this_reloc_off,SEEKF_START);
     if(IsKbdTerminate() || bmEOF()) break;
     nrc.offset = rne.RefOff;
     nrc.number = i;
     if(!la_AddData(CurrNEChain,&nrc,MemOutBox)) { OutOfMem: break; }
     if(!(rne.Type & 0x04) && rne.AddrType) /** if not additive fixup and not byte fixup */
     {
       while(1)
       {
	 unsigned next_off;
	 next_off = bmReadWordEx(segoff + (__filesize_t)((NERefChain  *)CurrNEChain->data)[CurrNEChain->nItems - 1].offset,SEEKF_START);
	 if(bmEOF()) break;
	 if(next_off > slength || next_off == 0xFFFF || next_off == ((NERefChain  *)CurrNEChain->data)[CurrNEChain->nItems - 1].offset) break;
	 nrc.offset = next_off;
	 nrc.number = i;
	 if(!la_AddData(CurrNEChain,&nrc,MemOutBox)) goto OutOfMem;
       }
     }
  }
  la_Sort(CurrNEChain,compare_chains);
  CloseWnd(w);
}

static tCompare __FASTCALL__ compare_ne_spec(const any_t*e1,const any_t*e2)
{
  const NERefChain  *r1, *r2;
  RELOC_NE rne;
  tCompare ret;
  r1 = reinterpret_cast<const NERefChain*>(e1);
  r2 = reinterpret_cast<const NERefChain*>(e2);
  if(r2->offset >= r1->offset && r2->offset < r1->offset + __codelen)
  {
    bmReadBufferEx(&rne,sizeof(RELOC_NE),CurrSegmentStart + CurrSegmentLength + 2 + sizeof(RELOC_NE)*r2->number,SEEKF_START);
    if(rne.Type == __type)  return 0;
  }
  if(r1->offset < r2->offset) ret = -1;
  else                        ret = 1;
  return ret;
}

static tCompare __FASTCALL__ compare_ne(const any_t*e1,const any_t*e2)
{
  const NERefChain  *r1, *r2;
  int ret;
  r1 = reinterpret_cast<const NERefChain*>(e1);
  r2 = reinterpret_cast<const NERefChain*>(e2);
  if(r2->offset >= r1->offset && r2->offset < r1->offset + __codelen) ret = 0;
  else
    if(r1->offset < r2->offset) ret = -1;
    else                        ret = 1;
  return ret;
}

static RELOC_NE *  __FASTCALL__ __found_RNE(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen)
{
  NERefChain *found,key;
  static RELOC_NE rne;
  if(segnum != CurrChainSegment || !CurrNEChain)
  {
    if(CurrNEChain) la_Destroy(CurrNEChain);
    CurrChainSegment = segnum;
    BuildNERefChain(segoff,slength);
  }
  key.offset = keyoff;
  __codelen = codelen;
  found = (NERefChain*)la_Find(CurrNEChain,&key,compare_ne);
  if(found) { bmReadBufferEx(&rne,sizeof(rne),segoff + slength + 2 + sizeof(RELOC_NE)*found->number,SEEKF_START); return &rne; }
  else      return 0;
}

static RELOC_NE *  __FASTCALL__ __found_RNE_spec(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen,int type)
{
  NERefChain *found,key;
  static RELOC_NE rne;
  if(segnum != CurrChainSegment || !CurrNEChain)
  {
    if(CurrNEChain) la_Destroy(CurrNEChain);
    CurrChainSegment = segnum;
    BuildNERefChain(segoff,slength);
  }
  key.offset = keyoff;
  __codelen = codelen;
  __type = type;
  found = (NERefChain*)la_Find(CurrNEChain,&key,compare_ne_spec);
  if(found)
  {
    BFile* b_cache;
    b_cache = ne_cache;
    b_cache->seek(segoff + slength + 2 + sizeof(RELOC_NE)*found->number,SEEKF_START);
    b_cache->read_buffer(&rne,sizeof(rne));
    return &rne;
  }
  else      return 0;
}

static unsigned  __FASTCALL__ __findSpecType(__filesize_t sstart,__filesize_t ssize,unsigned segnum,__filesize_t target,char codelen,char type,unsigned defval)
{
   unsigned ret;
   RELOC_NE * rne;
   rne = __found_RNE_spec(sstart,ssize,segnum,(unsigned)(target - sstart),codelen,type);
   if(rne) ret = rne->ordinal;
   else    ret = defval;
   return ret;
}

static void  __FASTCALL__ rdImpNameNELX(char *buff,int blen,unsigned idx,bool useasoff,__filesize_t OffTable)
{
  unsigned char len;
  __filesize_t name_off;
  BFile* b_cache;
  b_cache = ne_cache2;
  name_off = OffTable;
  if(!useasoff)
  {
    __filesize_t ref_off;
    ref_off = (__filesize_t)beye_context().headshift
	      + ne.neOffsetModuleReferenceTable
	      + (idx - 1)*2;
    name_off += bmReadWordEx(ref_off,SEEKF_START);
  }
  else name_off += idx;
  b_cache->seek(name_off,SEEKF_START);
  len = b_cache->read_byte();
  len = len > blen ? blen : len;
  b_cache->read_buffer(buff,len);
  buff[len] = 0;
}

static void __FASTCALL__ rd_ImpName(char *buff,int blen,unsigned idx,bool useasoff)
{
  rdImpNameNELX(buff,blen,idx,useasoff,beye_context().headshift + ne.neOffsetImportTable);
}

static bool  __FASTCALL__ BuildReferStrNE(const DisMode& parent,char *str,RELOC_NE *rne,int flags,__filesize_t ulShift)
{
  char buff[256];
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
  if(flags & APREF_USE_TYPE) strcat(str,pref);
  if((rne->Type & 3) == 1 || (rne->Type & 3) == 2) /** imported type */
  {
    retrf = true;
    rd_ImpName(buff,sizeof(buff),rne->idx,0);
    sprintf(&str[strlen(str)],"<%s>.",buff);
    if((rne->Type & 3) == 1) sprintf(&str[strlen(str)],"@%hu",rne->ordinal);
    else
    {
      rd_ImpName(buff,sizeof(buff),rne->ordinal,true);
      strcat(str,buff);
    }
  }
  else
   if((rne->Type & 3) == 0)
   {
     if(rne->idx == 0x00FF && rne->AddrType != 2)
     {
       __filesize_t ea;
       ea = CalcEntryNE(rne->ordinal,false);
       if(FindPubName(buff,sizeof(buff),ea))
	  sprintf(&str[strlen(str)],"%s",buff);
       else
       {
	 retrf = ea?true:false;
	 sprintf(&str[strlen(str)],"(*this).@%hu",rne->ordinal);
       }
       if(!DumpMode && !EditMode && !(flags & APREF_USE_TYPE)) code_guider->add_go_address(parent,str,ea);
     }
     else
     {
       __filesize_t ep;
       ep = CalcEntryPointNE(rne->idx,rne->ordinal);
       if(FindPubName(buff,sizeof(buff),ep))
	 sprintf(&str[strlen(str)],"%s",buff);
       else
       {
	 if(need_virt) sprintf(&str[strlen(str)],".%08lX",(unsigned long)nePA2VA(ep));
	 else sprintf(&str[strlen(str)],"(*this).seg<#%hu>:%sH",rne->idx,Get4Digit(rne->ordinal));
	 retrf = ep?true:false;
       }
       if(!DumpMode && !EditMode && !(flags & APREF_USE_TYPE)) code_guider->add_go_address(parent,str,ep);
     }
   }
   else
   {
      strcat(str,"?OSFIXUP?");
   }
   if((rne->Type & 4) == 4)
   {
     __filesize_t data;
     if(reflen && reflen <= 4)
     {
       strcat(str,"+");
       data = bmReadDWordEx(ulShift,SEEKF_START);
       strcat(str,reflen == 1 ? Get2Digit(data) : reflen == 2 ? Get4Digit(data) : Get8Digit(data));
     }
     else strcat(str,",<add>");
   }
   return retrf;
}

static bool __FASTCALL__ AppendNERef(const DisMode& parent,char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
    unsigned i;
    __filesize_t segpos,slength;
    char buff[256];
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
	      rne->ordinal = bmReadWordEx(ulShift,SEEKF_START);
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
	      if(FindPubName(buff,sizeof(buff),r_sh))
	      {
		strcat(str,buff);
		if(!DumpMode && !EditMode) code_guider->add_go_address(parent,str,r_sh);
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
static bool  __FASTCALL__ ReadPubNames(BFile& handle,__filesize_t offset,void (__FASTCALL__ *mem_out)(const std::string&))
{
 struct PubName pnam;
 ENTRY ent;
 unsigned ord,i = 0;
 __filesize_t noff;
 bool ret;
 if(!offset) return false;
 ret = true;
 handle.seek(offset,SEEKF_START);
 while(1)
 {
   unsigned char l;
   noff = handle.tell();
   l = handle.read_byte();
   if(l == 0 || handle.eof()) { ret = true; break; }
   handle.seek(l,SEEKF_CUR);
   ord = handle.read_word();
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
     if(pnam.pa)
     {
       if(!la_AddData(PubNames,&pnam,mem_out))
       {
	 ret = false;
	 break;
       }
     }
   }
   i++;
   if(i > 0xFFFD || handle.eof()) { ret = false; break; }
 }
 return ret;
}

static void __FASTCALL__ ne_ReadPubName(BFile& b_cache,const struct PubName *it,
			   char *buff,unsigned cb_buff)
{
    unsigned char rlen;
      b_cache.seek(it->nameoff,SEEK_SET);
      rlen = b_cache.read_byte();
      rlen = std::min(unsigned(rlen),cb_buff-1);
      b_cache.read_buffer(buff,rlen);
      buff[rlen] = 0;
}

static bool  __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  return fmtFindPubName(*ne_cache2,buff,cb_buff,pa,
			ne_ReadPubNameList,
			ne_ReadPubName);
}


static void __FASTCALL__ ne_ReadPubNameList(BFile& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
   if((PubNames = la_Build(0,sizeof(struct PubName),mem_out)) != NULL)
   {
     ReadPubNames(handle,beye_context().headshift + ne.neOffsetResidentNameTable,mem_out);
     ReadPubNames(handle,ne.neOffsetNonResidentNameTable,mem_out);
     if(PubNames->nItems)
       la_Sort(PubNames,fmtComparePubNames);
   }
}

static void __FASTCALL__ NE_init(CodeGuider& _code_guider)
{
    code_guider=&_code_guider;
   BFile& main_handle = bmbioHandle();
   bmReadBufferEx(&ne,sizeof(NEHEADER),beye_context().headshift,SEEKF_START);
   if((ne_cache3 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) ne_cache3 = &main_handle;
   if((ne_cache1 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) ne_cache2 = &main_handle;
   if((ne_cache = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) ne_cache = &main_handle;
   if((ne_cache2 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) ne_cache2 = &main_handle;
}

static void __FASTCALL__ NE_destroy( void )
{
  BFile& main_handle = bmbioHandle();
  if(CurrNEChain) { la_Destroy(CurrNEChain); CurrNEChain = 0; }
  if(PubNames)  { la_Destroy(PubNames); PubNames = 0; }
  if(ne_cache != &bNull && ne_cache != &main_handle) delete ne_cache;
  if(ne_cache2 != &bNull && ne_cache2 != &main_handle) delete ne_cache2;
  if(ne_cache3 != &bNull && ne_cache3 != &main_handle) delete ne_cache3;
  if(ne_cache1 != &bNull && ne_cache1 != &main_handle) delete ne_cache1;
}

static __filesize_t __FASTCALL__ NEHelp( void )
{
  hlpDisplay(10006);
  return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ neVA2PA(__filesize_t va)
{
  SEGDEF nesd;
  uint_fast16_t seg,off;
  seg = (va >> 16) & 0xFFFFU;
  off = va & 0xFFFFU;
  if(!ReadSegDefNE(&nesd,seg)) return 0L;
  return nesd.sdOffset ? (((__filesize_t)nesd.sdOffset)<<ne.neLogicalSectorShiftCount) + off : 0;
}

static __filesize_t __FASTCALL__ nePA2VA(__filesize_t pa)
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

static __filesize_t __FASTCALL__ neGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
			  __filesize_t pa,bool as_prev)
{
  return fmtGetPubSym(*ne_cache,str,cb_str,func_class,pa,as_prev,
		      ne_ReadPubNameList,
		      ne_ReadPubName);
}

static unsigned __FASTCALL__ neGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
		      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  __filesize_t currseg_st;
  unsigned i,segcount = ne.neSegmentTableCount,ret;
  unsigned bio_opt;
  bool found;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = DAB_USE16;
  name[0] = 0;
  ret = 0;
  bio_opt = bmbioHandle().get_optimization();
  bmbioHandle().set_optimization(bio_opt | BFile::Opt_Db);
  bmSeek((__fileoff_t)beye_context().headshift + ne.neOffsetSegmentTable,SEEK_SET);
  found = false;
  for(i = 0;i < segcount;i++)
  {
    SEGDEF nesd_c;
    if(!ReadSegDefNE(&nesd_c,i+1)) return 0L;
    currseg_st = (((__filesize_t)nesd_c.sdOffset)<<ne.neLogicalSectorShiftCount);
    if(!currseg_st) { *start = *end; continue; } /** BSS segment */
    if(pa < currseg_st)
    {
      *start = *end;
      *end = currseg_st;
      found = true;
      ret = i;
      break;
    }
    if(pa >= currseg_st && pa < currseg_st + nesd_c.sdLength)
    {
      *start = currseg_st;
      *end = *start + nesd_c.sdLength;
      *_class = nesd_c.sdFlags & 0x01 ? OC_DATA : OC_CODE;
      *bitness = nesd_c.sdFlags & 0x2000 ? DAB_USE32 : DAB_USE16;
      ret = i+1;
      found = true;
      break;
    }
    *start = currseg_st;
    *end = currseg_st + nesd_c.sdLength;
  }
  if(!found)
  {
    *start = *end;
    *end = bmGetFLength();
  }
  bmbioHandle().set_optimization(bio_opt);
  return ret;
}

static int __FASTCALL__ bitnessNE(__filesize_t pa)
{
  static __filesize_t st = 0,end = 0;
  char name[1];
  int _class;
  static int bitness;
  if(!(pa >= st && pa < end))
  {
    neGetObjAttr(pa,name,sizeof(name),&st,&end,&_class,&bitness);
  }
  return bitness;
}

static bool __FASTCALL__ neAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos >= beye_context().headshift && cfpos < beye_context().headshift + sizeof(NEHEADER))
  {
     strcpy(addr,"NEH :");
     strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift));
  }
  else
  if(cfpos >= beye_context().headshift + ne.neOffsetSegmentTable &&
     cfpos <  beye_context().headshift + ne.neOffsetSegmentTable + ne.neSegmentTableCount*sizeof(SEGDEF))
  {
    strcpy(addr,"NESD:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - ne.neOffsetSegmentTable));
  }
  else
  if(cfpos >= beye_context().headshift + ne.neOffsetEntryTable &&
     cfpos <  beye_context().headshift + ne.neOffsetEntryTable + ne.neLengthEntryTable)
  {
    strcpy(addr,"NEET:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - ne.neOffsetEntryTable));
  }
  else
    if((res=nePA2VA(cfpos))!=0)
    {
      addr[0] = '.';
      strcpy(&addr[1],Get8Digit(res));
    }
    else bret = false;
  return bret;
}

static int __FASTCALL__ platformNE( void ) { return DISASM_CPU_IX86; }

extern const REGISTRY_BIN neTable =
{
  "NE (New Exe)",
  { "NEHelp", "ModRef", "ResNam", "NRsNam", NULL, "Entry ", "ResTbl", "NE Hdr", NULL, "SegDef" },
  { NEHelp, ShowModRefNE, ShowResNamNE, ShowNResNmNE, NULL, ShowEntriesNE, ShowResourcesNE, ShowNewHeaderNE, NULL, ShowSegDefNE },
  IsNEFormat, NE_init, NE_destroy,
  NULL,
  AppendNERef,
  platformNE,
  bitnessNE,
  NULL,
  neAddressResolv,
  neVA2PA,
  nePA2VA,
  neGetPubSym,
  neGetObjAttr
};
} // namespace beye
