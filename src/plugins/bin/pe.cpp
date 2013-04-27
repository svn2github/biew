#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/pe.c
 * @brief       This file contains implementation of PE (Portable Executable)
 *              file format decoder.
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
 * @note        Some useful patches
**/
#include <limits>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "beye.h"
#include "colorset.h"
#include "plugins/bin/pe.h"
#include "plugins/disasm.h"
#include "bin_util.h"
#include "codeguid.h"
#include "bmfile.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))

static int is_64bit;

typedef union {
  PE32HEADER   pe32;
  PE32P_HEADER pe32p;
}PE32X_HEADER;

#define PE32_HDR(e,FIELD) (is_64bit?(((PE32P_HEADER *)&e.pe32p)->FIELD):(((PE32HEADER *)&e.pe32)->FIELD))
#define PE32_HDR_SIZE() (is_64bit?sizeof(PE32P_HEADER):sizeof(PE32HEADER))

static PE_ADDR * peVA = NULL;
static PE32X_HEADER pe32;
static PEHEADER pe;
static PERVA *peDir;

#define PE_HDR_SIZE() (sizeof(PEHEADER) + PE32_HDR_SIZE())

static BFile* pe_cache = &bNull;
static BFile* pe_cache1 = &bNull;
static BFile* pe_cache2 = &bNull;
static BFile* pe_cache3 = &bNull;
static BFile* pe_cache4 = &bNull;

static bool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ pe_ReadPubNameList(BFile& handle,void (__FASTCALL__ *mem_out)(const char *));
static __filesize_t __FASTCALL__ peVA2PA(__filesize_t va);
static __filesize_t __FASTCALL__ pePA2VA(__filesize_t pa);
static __fileoff_t __NEAR__ CalcOverlayOffset( void );

static __filesize_t __NEAR__ __FASTCALL__ CalcPEObjectEntry(__fileoff_t offset)
{
 __filesize_t intp;
 intp = offset / PE32_HDR(pe32,peFileAlign);
 if(offset % PE32_HDR(pe32,peFileAlign)) offset = ( offset / intp ) * intp;
 return offset;
}

static __filesize_t __NEAR__ __FASTCALL__ RVA2Phys(__filesize_t rva)
{
 int i;
 __filesize_t npages,poff,obj_rva,pphys,ret,size;
 for(i = pe.peObjects - 1;i >= 0;i--)
 {
   if(rva >= peVA[i].rva) break;
   if(IsKbdTerminate()) return 0;
 }
 if (i < 0) return rva;         // low RVAs fix -XF
 pphys = peVA[i].phys;
 obj_rva = peVA[i].rva;
 /* Added by Kostya Nosov <k-nosov@yandex.ru> */
 if (i < pe.peObjects - 1)
 {
   size = peVA[i+1].phys - pphys;
   if (rva - obj_rva > size) return 0;
 }
 /** each page is 4096 bytes
     each object can contain several pages
     it now contains previous object that contains this page */
 if(!pphys) return 0;
 npages = (rva - obj_rva) / (__filesize_t)4096;
 poff = (rva - obj_rva) % (__filesize_t)4096;
 ret = pphys + npages*4096UL + poff;
 return ret;
}

static __filesize_t __NEAR__ __FASTCALL__ fioReadDWord(BFile& handle,__filesize_t offset,int origin)
{
 handle.seek(offset,origin);
 return handle.read_dword();
}

static unsigned __NEAR__ __FASTCALL__ fioReadWord(BFile& handle,__filesize_t offset,int origin)
{
 handle.seek(offset,origin);
 return handle.read_word();
}

static __filesize_t __NEAR__ __FASTCALL__ fioReadDWord2Phys(BFile& handle,__filesize_t offset,int origin)
{
 unsigned long dword;
 dword = fioReadDWord(handle,offset,origin);
 return RVA2Phys(dword);
}

static const char * __NEAR__ __FASTCALL__ PECPUType(void)
{
    static const struct {
       int code;
       const char *name;
    } pe_cpu[] = {
       {0x014C, "Intel 80386"},
//       {0x014D, "Intel 80486"},
//       {0x014E, "Intel 80586"},
//       {0x014F, "Intel 80686"},
//       {0x0150, "Intel 80786"},
       {0x0162, "MIPS R3000"},
       {0x0166, "MIPS R4000"},
       {0x0168, "MIPS R10000"},
       {0x0169, "MIPS WCE v2"},
       {0x0184, "DEC Alpha"},
       {0x01A2, "SH3"},
       {0x01A3, "SH3DSP"},
       {0x01A4, "SH3E"},
       {0x01A6, "SH4"},
       {0x01A8, "SH5"},
       {0x01C0, "ARM"},
       {0x01C2, "ARM Thumb"},
       {0x01D3, "AM33"},
       {0x01F0, "IBM PowerPC"},
       {0x01F1, "IBM PowerPC FP"},
       {0x0200, "Intel IA-64"},
       {0x0266, "MIPS16"},
       {0x0284, "DEC Alpha 64"},
       {0x0366, "MIPSFPU"},
       {0x0466, "MIPSFPU16"},
       {0x0520, "Tricore"},
       {0x0CEF, "CEF"},
       {0x0EBC, "EFI Byte Code"},
       {0x8664, "AMD64"},
       {0x9041, "M32R"},
       {0xC0EE, "CEE"},
       {0x0000, "Unknown"},
    };
    unsigned i;

    for(i=0; i<(sizeof(pe_cpu)/sizeof(pe_cpu[0])); i++) {
       if(pe.peCPUType == pe_cpu[i].code)
	  return pe_cpu[i].name;
    }

    return "Unknown";
}

static __filesize_t entryPE = 0L;
static __fileoff_t overlayPE = -1L;

static void __NEAR__ PaintNewHeaderPE_1( void )
{
  const char *fmt;
  time_t tval;
  entryPE = RVA2Phys(pe.peEntryPointRVA);
  tval = pe.peTimeDataStamp;
  twPrintF("Signature                      = '%c%c' (Type: %04X)\n"
	   "Required CPU Type              = %s\n"
	   "Number of object entries       = %hu\n"
	   "Time/Data Stamp                = %s"
	   "NT header size                 = %hu bytes\n"
	   "Image flags :                    [%04hXH]\n"
	   "    [%c] < Relocation info stripped >\n"
	   "    [%c] Image is executable\n"
	   "    [%c] < Line number stripped >\n"
	   "    [%c] < Local symbols stripped >\n"
	   "    [%c] < Minimal object >\n"
	   "    [%c] < Update object >\n"
	   "    [%c] < 16 bit word machine >\n"
	   "    [%c] < 32 bit word machine >\n"
	   "    [%c] Fixed\n"
	   "    [%c] < System file >\n"
	   "    [%c] Library image\n"
	   "Linker version                 = %u.%02u\n"
	   ,pe.peSignature[0],pe.peSignature[1],pe.peMagic
	   ,PECPUType()
	   ,pe.peObjects
	   ,ctime(&tval)
	   ,pe.peNTHdrSize
	   ,pe.peFlags
	   ,Gebool(pe.peFlags & 0x0001)
	   ,Gebool(pe.peFlags & 0x0002)
	   ,Gebool(pe.peFlags & 0x0004)
	   ,Gebool(pe.peFlags & 0x0008)
	   ,Gebool(pe.peFlags & 0x0010)
	   ,Gebool(pe.peFlags & 0x0020)
	   ,Gebool(pe.peFlags & 0x0040)
	   ,Gebool(pe.peFlags & 0x0100)
	   ,Gebool(pe.peFlags & 0x0200)
	   ,Gebool(pe.peFlags & 0x1000)
	   ,Gebool(pe.peFlags & 0x2000)
	   ,(int)pe.peLMajor,(int)pe.peLMinor);
  twSetColorAttr(dialog_cset.entry);
  twPrintF("EntryPoint RVA    %s = %08lXH (Offset: %08lXH)",pe.peFlags & 0x2000 ? "[ LibEntry ]" : "[ EXEEntry ]",pe.peEntryPointRVA,entryPE); twClrEOL();
  twSetColorAttr(dialog_cset.main);
  if(is_64bit)
    fmt = "\nImage base                   = %016llXH\n"
	  "Object aligning                = %08lXH";
  else
    fmt = "\nImage base                   = %08lXH\n"
	  "Object aligning                = %08lXH";
  twPrintF(fmt
	   ,PE32_HDR(pe32,peImageBase)
	   ,PE32_HDR(pe32,peObjectAlign));
}

static void __NEAR__ PaintNewHeaderPE_2( void )
{
  const char *fmt;
  static const char * subSystem[] =
  {
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows Character",
    "OS/2 GUI",
    "OS/2 Character",
    "Posix GUI",
    "Posix Character",
    "Win9x Driver",
    "Windows CE",
    "EFI application",
    "EFI boot service driver",
    "EFI runtime driver",
    "EFI ROM",
    "X-Box",
  };

  twPrintF(
	   "Size of Text                   = %08lXH\n"
	   "Size of Data                   = %08lXH\n"
	   "Size of BSS                    = %08lXH\n"
	   "File align                     = %08lXH\n"
	   "OS/User/Subsystem version      = %hu.%hu/%hu.%hu/%hu.%hu\n"
	   "Image size                     = %lu bytes\n"
	   "Header size                    = %lu bytes\n"
	   "File checksum                  = %08lXH\n"
	   "Subsystem                      = %s\n"
	   "DLL Flags :                      [%04hXH]\n"
	   " [%c] Per-Process Library initialization\n"
	   " [%c] Per-Process Library termination\n"
	   " [%c] Per-Thread  Library initialization\n"
	   " [%c] Per-Thread  Library termination\n"
	   "Number of directory entries    = %lu bytes\n"
	   ,pe.peSizeOfText
	   ,pe.peSizeOfData
	   ,pe.peSizeOfBSS
	   ,PE32_HDR(pe32,peFileAlign)
	   ,PE32_HDR(pe32,peOSMajor),PE32_HDR(pe32,peOSMinor),PE32_HDR(pe32,peUserMajor),PE32_HDR(pe32,peUserMinor),PE32_HDR(pe32,peSubSystMajor),PE32_HDR(pe32,peSubSystMinor)
	   ,PE32_HDR(pe32,peImageSize)
	   ,PE32_HDR(pe32,peHeaderSize)
	   ,PE32_HDR(pe32,peFileChecksum)
	   ,PE32_HDR(pe32,peSubSystem) < ARRAY_SIZE(subSystem) ? subSystem[PE32_HDR(pe32,peSubSystem)] : "Unknown"
	   ,PE32_HDR(pe32,peDLLFlags)
	   ,Gebool(PE32_HDR(pe32,peDLLFlags) & 0x0001)
	   ,Gebool(PE32_HDR(pe32,peDLLFlags) & 0x0002)
	   ,Gebool(PE32_HDR(pe32,peDLLFlags) & 0x0004)
	   ,Gebool(PE32_HDR(pe32,peDLLFlags) & 0x0008)
	   ,PE32_HDR(pe32,peDirSize));
   if(is_64bit)
    fmt=
	   "Stack reserve size             = %llu bytes\n"
	   "Stack commit size              = %llu bytes\n"
	   "Heap reserve size              = %llu bytes\n"
	   "Heap commit size               = %llu bytes";
   else
    fmt=
	   "Stack reserve size             = %lu bytes\n"
	   "Stack commit size              = %lu bytes\n"
	   "Heap reserve size              = %lu bytes\n"
	   "Heap commit size               = %lu bytes";
   twPrintF(fmt
	   ,PE32_HDR(pe32,peStackReserveSize)
	   ,PE32_HDR(pe32,peStackCommitSize)
	   ,PE32_HDR(pe32,peHeapReserveSize)
	   ,PE32_HDR(pe32,peHeapCommitSize));
  if (CalcOverlayOffset() != -1) {
    entryPE = overlayPE;
    twSetColorAttr(dialog_cset.entry);
    twPrintF("\nOverlay                        = %08lXH", entryPE); twClrEOL();
    twSetColorAttr(dialog_cset.main);
  }
}

static void (__NEAR__ * const pephead[])(void) = /* [dBorca] the table is const, not the any_t*/
{
    PaintNewHeaderPE_1,
    PaintNewHeaderPE_2
};

static void __FASTCALL__ PaintNewHeaderPE(TWindow * win,const any_t**ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  twUseWin(win);
  twFreezeWin(win);
  twClearWin();
  sprintf(text," Portable Executable Header [%d/%d] ",npage + 1,tpage);
  twSetTitleAttr(win,text,TW_TMODE_CENTER,dialog_cset.title);
  twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
  if(npage < 2)
  {
    twGotoXY(1,1);
    (*(pephead[npage]))();
  }
  twRefreshFullWin(win);
}

static __filesize_t __FASTCALL__ ShowNewHeaderPE( void )
{
 __fileoff_t fpos;
 fpos = BMGetCurrFilePos();
 if(PageBox(70,21,NULL,2,PaintNewHeaderPE) != -1 && entryPE && entryPE < bmGetFLength()) fpos = entryPE;
 return fpos;
}

static void __FASTCALL__ ObjPaintPE(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const PE_OBJECT ** nam = (const PE_OBJECT **)names;
 const PE_OBJECT *  objs = nam[start];
 twUseWin(win);
 twFreezeWin(win);
 twClearWin();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 twSetTitleAttr(win,buffer,TW_TMODE_CENTER,dialog_cset.title);
 twSetFooterAttr(win,PAGEBOX_SUB,TW_TMODE_RIGHT,dialog_cset.selfooter);
 twGotoXY(1,1);

 memcpy(buffer, objs->oName, 8);
 buffer[8] = 0;

 twPrintF("Object Name                    = %8s\n"
	  "Virtual Size                   = %lX bytes\n"
	  "RVA (relative virtual address) = %08lX\n"
	  "Physical size                  = %08lX bytes\n"
	  "Physical offset                = %08lX bytes\n"
	  "<Relocations>                  = %hu at %08lX\n"
	  "<Line numbers>                 = %hu at %08lX\n"
	  "FLAGS: %lX\n"
	  "   [%c] Executable code          "   "   [%c] Shared object\n"
	  "   [%c] Initialized data         "   "   [%c] Executable object\n"
	  "   [%c] Uninitialized data       "   "   [%c] Readable object\n"
	  "   [%c] Contains COMDAT          "   "   [%c] Writable object\n"
	  "   [%c] Contains comments or other info\n"
	  "   [%c] Won't become part of the image\n"
	  "   [%c] Contains extended relocations\n"
	  "   [%c] Discardable as needed\n"
	  "   [%c] Must not be cashed\n"
	  "   [%c] Not pageable\n"
	  "   Alignment                   = %u %s\n"

	  ,buffer
	  ,objs->oVirtualSize
	  ,objs->oRVA
	  ,objs->oPhysicalSize
	  ,objs->oPhysicalOffset
	  ,objs->oNReloc
	  ,objs->oRelocPtr
	  ,objs->oNLineNumb
	  ,objs->oLineNumbPtr
	  ,objs->oFlags

	  ,Gebool(objs->oFlags & 0x00000020UL), Gebool(objs->oFlags & 0x10000000UL)
	  ,Gebool(objs->oFlags & 0x00000040UL), Gebool(objs->oFlags & 0x20000000UL)
	  ,Gebool(objs->oFlags & 0x00000080UL), Gebool(objs->oFlags & 0x40000000UL)
	  ,Gebool(objs->oFlags & 0x00001000UL), Gebool(objs->oFlags & 0x80000000UL)
	  ,Gebool(objs->oFlags & 0x00000200UL)
	  ,Gebool(objs->oFlags & 0x00000800UL)
	  ,Gebool(objs->oFlags & 0x01000000UL)
	  ,Gebool(objs->oFlags & 0x02000000UL)
	  ,Gebool(objs->oFlags & 0x04000000UL)
	  ,Gebool(objs->oFlags & 0x08000000UL)

	  ,objs->oFlags&0x00F00000 ? 1 << (((objs->oFlags&0x00F00000)>>20)-1) : 0
	  ,objs->oFlags&0x00F00000 ? "byte(s)" : "(default)");

 twRefreshFullWin(win);
}

static bool __NEAR__ __FASTCALL__ __ReadObjectsPE(BFile& handle,memArray * obj,unsigned n)
{
 unsigned i;
  for(i = 0;i < n;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || handle.eof()) break;
    handle.read_buffer(&po,sizeof(PE_OBJECT));
    if(!ma_AddData(obj,&po,sizeof(PE_OBJECT),true)) break;
  }
  return true;
}

static __fileoff_t __NEAR__ CalcOverlayOffset( void )
{
  if (overlayPE == -1 && pe.peObjects) {
    memArray *obj;
    if ((obj = ma_Build(pe.peObjects, true))) {
      pe_cache->seek(0x18 + pe.peNTHdrSize + beye_context().headshift, SEEK_SET);
      if (__ReadObjectsPE(*pe_cache, obj, pe.peObjects)) {
	int i;
	for (i = 0; i < pe.peObjects; i++) {
	  PE_OBJECT *o = (PE_OBJECT *)obj->data[i];
	  __fileoff_t end = o->oPhysicalOffset + ((o->oPhysicalSize + (PE32_HDR(pe32,peFileAlign) - 1)) & ~(PE32_HDR(pe32,peFileAlign) - 1));
	  if (overlayPE < end) {
	    overlayPE = end;
	  }
	}
      }
      ma_Destroy(obj);
    }
  }
  return overlayPE;
}

static __filesize_t __FASTCALL__ ShowObjectsPE( void )
{
 __filesize_t fpos;
 BFile& handle = *pe_cache;
 unsigned nnames;
 memArray * obj;
 fpos = BMGetCurrFilePos();
 nnames = pe.peObjects;
 if(!nnames) { NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,true))) return fpos;
 handle.seek(0x18 + pe.peNTHdrSize + beye_context().headshift,SEEK_SET);
 if(__ReadObjectsPE(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,19,(const any_t**)obj->data,obj->nItems,ObjPaintPE);
    if(ret != -1)  fpos = CalcPEObjectEntry(((PE_OBJECT *)obj->data[ret])->oPhysicalOffset);
 }
 ma_Destroy(obj);
 return fpos;
}

static unsigned __NEAR__ __FASTCALL__ GetImportCountPE(BFile& handle,__filesize_t phys)
{
  unsigned count;
  __filesize_t fpos = handle.tell();
  unsigned long ctrl;
  count = 0;
  handle.seek(phys,SEEKF_START);
  while(1)
  {
    ctrl = fioReadDWord(handle,12L,SEEKF_CUR);
    handle.seek(4L,SEEKF_CUR);
    if(ctrl == 0 || count > 0xFFFD || IsKbdTerminate() || handle.eof()) break;
    count++;
  }
  handle.seek(fpos,SEEKF_START);
  return count;
}

/* returns really readed number of characters */
unsigned __NEAR__ __FASTCALL__ __peReadASCIIZName(BFile& handle,__filesize_t offset,char *buff, unsigned cb_buff)
{
  unsigned j;
  char ch;
  __filesize_t fpos;
  fpos = handle.tell();
  j = 0;
  handle.seek(offset,SEEKF_START);
  while(1)
  {
    ch = handle.read_byte();
    buff[j++] = ch;
    if(!ch || j >= cb_buff || handle.eof()) break;
  }
  if(j) buff[j-1]=0;
  handle.seek(fpos,SEEKF_START);
  return j;
}

static unsigned __NEAR__ __FASTCALL__ __ReadImportPE(BFile& handle,__filesize_t phys,memArray *obj,unsigned nnames)
{
  unsigned i;
  __filesize_t fpos = handle.tell();
  __filesize_t rva,addr;
  handle.seek(phys,SEEKF_START);
  for(i = 0;i < nnames;i++)
  {
    char tmp[300];
    bool is_eof;
    rva = fioReadDWord(handle,12L,SEEKF_CUR);
    handle.seek(4L,SEEKF_CUR);
    addr = RVA2Phys(rva);
    __peReadASCIIZName(handle,addr,tmp,sizeof(tmp));
    if(IsKbdTerminate()) break;
    is_eof = handle.eof();
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : tmp,true)) break;
    if(is_eof) break;
  }
  handle.seek(fpos,SEEKF_START);
  return obj->nItems;
}

static __filesize_t addr_shift_pe = 0L;

static unsigned __FASTCALL__ GetImpCountPE(BFile& handle)
{
 unsigned count;
 uint64_t Hint;
 count = 0;
 if(addr_shift_pe)
 {
   handle.seek(addr_shift_pe,SEEKF_START);
   while(1)
   {
     Hint = is_64bit ? handle.read_qword():handle.read_dword();
     if(Hint == 0ULL || count > 0xFFFD || IsKbdTerminate() || handle.eof()) break;
     count++;
   }
 }
 return count;
}

static bool __FASTCALL__  __ReadImpContPE(BFile& handle,memArray * obj,unsigned nnames)
{
  unsigned i,VA;
  uint64_t Hint;
  int cond;
  __filesize_t rphys;
  handle.seek(addr_shift_pe,SEEKF_START);
  VA = pePA2VA(addr_shift_pe);
  for(i = 0;i < nnames;i++)
  {
    char stmp[300];
    bool is_eof;
    sprintf(stmp,".%08X: ", VA);
    VA += 4;
    Hint = is_64bit?handle.read_qword():handle.read_dword();
    cond=0;
    if(is_64bit) { if(Hint & 0x8000000000000000ULL) cond = 1; }
    else         { if(Hint & 0x0000000080000000ULL) cond = 1; }
    if(!cond)
    {
      rphys = RVA2Phys(is_64bit?(Hint&0x7FFFFFFFFFFFFFFFULL):(Hint&0x7FFFFFFFUL));
      if(rphys > bmGetFLength() || handle.eof())
      {
	if(!ma_AddString(obj,CORRUPT_BIN_MSG,true)) break;
      }
      else
      {
	char tmp[256];
	__peReadASCIIZName(handle,rphys+2,tmp,sizeof(tmp));
	strcat(stmp,tmp);
      }
    }
    else
    {
      char tmp[80];
      if(is_64bit) sprintf(tmp,"< By ordinal >   @%llu",Hint & 0x7FFFFFFFFFFFFFFFULL);
      else         sprintf(tmp,"< By ordinal >   @%lu" ,(uint32_t)(Hint & 0x7FFFFFFFUL));
      strcat(stmp,tmp);
    }
    is_eof = handle.eof();
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,true)) break;
    if(IsKbdTerminate() || is_eof) break;
  }
  return true;
}

static __filesize_t __FASTCALL__ ShowModRefPE( void )
{
  BFile& handle = *pe_cache;
  char petitle[80];
  memArray * obj;
  unsigned nnames;
  __filesize_t phys,fret;
  fret = BMGetCurrFilePos();
  if(!peDir[PE_IMPORT].rva) { not_found: NotifyBox(NOT_ENTRY," Module References "); return fret; }
  handle.seek(0L,SEEK_SET);
  phys = RVA2Phys(peDir[PE_IMPORT].rva);
  if(!(nnames = GetImportCountPE(handle,phys))) goto not_found;
  if(!(obj = ma_Build(nnames,true))) goto exit;
  if(__ReadImportPE(handle,phys,obj,nnames))
  {
    int i;
    i = 0;
    while(1)
    {
     ImportDirPE imp_pe;
     unsigned long magic;

     i = ma_Display(obj,MOD_REFER,LB_SELECTIVE,i);
     if(i == -1) break;
     sprintf(petitle,"%s%s ",IMPPROC_TABLE,(char *)obj->data[i]);
     handle.seek(phys + i*sizeof(ImportDirPE),SEEKF_START);
     handle.read_buffer(&imp_pe,sizeof(ImportDirPE));
     if(handle.eof()) break;
     if(!(imp_pe.idMajVer == 0 && imp_pe.idMinVer == 0 && imp_pe.idDateTime != 0xFFFFFFFFUL))
				   magic = imp_pe.idFlags;
     else                          magic = imp_pe.idLookupTableRVA;
     addr_shift_pe = magic ? RVA2Phys(magic) : magic;
     fmtShowList(GetImpCountPE,__ReadImpContPE,petitle,0,NULL);
    }
  }
  ma_Destroy(obj);
  exit:
  return fret;
}

static ExportTablePE et;

static inline void writeExportVA(__filesize_t va, BFile& handle, char *buf, unsigned long bufsize)
{
    // check for forwarded export
    if (va>=peDir[PE_EXPORT].rva && va<peDir[PE_EXPORT].rva+peDir[PE_EXPORT].size)
	__peReadASCIIZName(handle, RVA2Phys(va), buf, bufsize);
    // normal export
    else
    sprintf(buf, ".%08lX", (unsigned long)(va + PE32_HDR(pe32,peImageBase)));
}

static bool __FASTCALL__ PEExportReadItems(BFile& handle,memArray * obj,unsigned nnames)
{
  __filesize_t nameaddr,expaddr,nameptr;
  unsigned long *addr;
  unsigned i,ord;
  char buff[80];

  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  expaddr = RVA2Phys(et.etOrdinalTableRVA);
  if(!(addr = new unsigned long[nnames])) return true;
  handle.seek(RVA2Phys(et.etAddressTableRVA),SEEKF_START);
  handle.read_buffer(addr,sizeof(unsigned long)*nnames);

  for(i = 0;i < et.etNumNamePtrs;i++)
  {
    char stmp[300];
    bool is_eof;
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,SEEKF_START);
    __peReadASCIIZName(handle,nameaddr, stmp, sizeof(stmp)-sizeof(buff)-1);
    if(IsKbdTerminate()) break;
    ord = fioReadWord(handle,expaddr + i*2,SEEKF_START);
    is_eof = handle.eof();
    sprintf(buff,"%c%-9lu ", LB_ORD_DELIMITER, ord+(unsigned long)et.etOrdinalBase);
    writeExportVA(addr[ord], handle, buff+11, sizeof(buff)-11);
    addr[ord] = 0;
    strcat(stmp,buff);
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,true)) break;  // -XF removed PFree(stmp)
    if(is_eof) break;
  }

  for(i = 0;i < nnames;i++)
  {
    if(addr[i])
    {
      ord = i+et.etOrdinalBase;
      sprintf(buff," < by ordinal > %c%-9lu ",LB_ORD_DELIMITER, (unsigned long)ord);
      writeExportVA(addr[i], handle, buff+27, sizeof(buff)-27);
      if(!ma_AddString(obj,buff,true)) break;
    }
  }

  delete addr;
  return true;
}

static unsigned __FASTCALL__ PEExportNumItems(BFile& handle)
{
  __filesize_t addr;
  if(!peDir[PE_EXPORT].rva) return 0;
  addr = RVA2Phys(peDir[PE_EXPORT].rva);
  handle.seek(addr,SEEKF_START);
  handle.read_buffer((any_t*)&et,sizeof(et));
  return (unsigned)(et.etNumEATEntries);
}

static __filesize_t __NEAR__ __FASTCALL__ CalcEntryPE(unsigned ordinal,bool dispmsg)
{
 __filesize_t fret,rva;
 unsigned ord;
 BFile& handle = *pe_cache1;
 fret = BMGetCurrFilePos();
 {
   __filesize_t eret;
   rva = RVA2Phys(et.etAddressTableRVA);
   ord = (unsigned)ordinal - (unsigned)et.etOrdinalBase;
   eret = fioReadDWord2Phys(handle,rva + 4*ord,SEEKF_START);
   if(eret && eret < bmGetFLength()) fret = eret;
   else if(dispmsg) ErrMessageBox(NO_ENTRY,BAD_ENTRY);
 }
 return fret;
}

static __filesize_t __FASTCALL__ ShowExpNamPE( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  unsigned ordinal;
  __filesize_t addr;
  char exp_nam[256], exp_buf[300];
  fpos = BMGetCurrFilePos();
  strcpy(exp_nam,EXP_TABLE);
  if(peDir[PE_EXPORT].rva)
  {
    addr = RVA2Phys(peDir[PE_EXPORT].rva);
    bmSeek(addr,SEEKF_START);
    bmReadBuffer((any_t*)&et,sizeof(et));
    if(et.etNameRVA)
    {
      char sftime[80];
      struct tm * tm;
      time_t tval;
      __peReadASCIIZName(bmbioHandle(),RVA2Phys(et.etNameRVA),exp_buf, sizeof(exp_buf));
      if(strlen(exp_buf) > 50) strcpy(&exp_buf[50],"...");
      tval = et.etDateTime;
      tm = localtime(&tval);
      strftime(sftime,sizeof(sftime),"%x",tm);
      sprintf(exp_nam," %s (ver=%hX.%hX %s) "
	      ,exp_buf
	      ,et.etMajVer, et.etMinVer
	      ,sftime);
    }
  }
  ret = fmtShowList(PEExportNumItems,PEExportReadItems,
		    exp_nam,
		    LB_SELECTIVE | LB_SORTABLE,&ordinal);
  if(ret != -1)
  {
    fpos = CalcEntryPE(ordinal,true);
  }
  return fpos;
}


static bool __FASTCALL__ PEReadRVAs(BFile& handle, memArray * obj, unsigned nnames)
{
  unsigned i;
  static const char *rvaNames[] =
  {
      "~Export Table        ",
      "~Import Table        ",
      "~Resource Table      ",
      "E~xception Table     ",
      "Sec~urity Table      ",
      "Re~location Table    ",
      "~Debug Information   ",
      "Image De~scription   ",
      "~Machine Specific    ",
      "~Thread Local Storage",
      "Load Confi~guration  ",
      "~Bound Import Table  ",
      "Import ~Adress Table ",
      "Dela~y Import Table  ",
      "~COM+                ",
      "Reser~ved            "
  };
  UNUSED(handle);
  UNUSED(nnames);
  for (i=0; i<PE32_HDR(pe32,peDirSize); i++)
  {
    char foo[80];

    sprintf(foo, "%s  %08lX  %8lu",
	i<ARRAY_SIZE(rvaNames) ? rvaNames[i] : "Unknown             ",
	(unsigned long)peDir[i].rva,
	(unsigned long)peDir[i].size);
    if (!ma_AddString(obj, foo, true))
	return false;
  }

  return true;
}

static unsigned __FASTCALL__ PENumRVAs(BFile& handle)
{
  UNUSED(handle);
  return PE32_HDR(pe32,peDirSize);
}

static __filesize_t __FASTCALL__ ShowPERVAs( void )
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  ret = fmtShowList(PENumRVAs, PEReadRVAs, " Directory Entry       RVA           size ", LB_SELECTIVE|LB_USEACC, NULL);
  if (ret!=-1 && peDir[ret].rva)
    fpos = RVA2Phys(peDir[ret].rva);

  return fpos;
}

/***************************************************************************/
/************************  FOR PE  *****************************************/
/***************************************************************************/

typedef struct tagRELOC_PE
{
  uint64_t modidx;
  union
  {
    uint64_t funcidx; /** if modidx != -1 */
    uint64_t type;    /** if modidx == -1 */
  }import;
  uint64_t laddr; /** lookup addr */
  uint64_t reserved;
}RELOC_PE;

static linearArray *CurrPEChain = NULL;

static tCompare __FASTCALL__ compare_pe_reloc_s(const void __HUGE__ *e1,const void __HUGE__ *e2)
{
  const RELOC_PE __HUGE__ *p1,__HUGE__ *p2;
  p1 = (const RELOC_PE __HUGE__ *)e1;
  p2 = (const RELOC_PE __HUGE__ *)e2;
  return ((p1->laddr) < (p2->laddr) ? -1 : (p1->laddr) > (p2->laddr) ? 1 : 0);
}

static void __NEAR__ __FASTCALL__ BuildPERefChain( void )
{
  __filesize_t  phys,cpos;
  unsigned long i,j;
  RELOC_PE rel;
  ImportDirPE ipe;
  unsigned nnames;
  TWindow *w,*usd;
  usd = twUsedWin();
  if(!(CurrPEChain = la_Build(0,sizeof(RELOC_PE),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) pe_ReadPubNameList(bmbioHandle(),MemOutBox);
  twUseWin(w);
  twGotoXY(1,1);
  twPutS(BUILD_REFS);
  twUseWin(usd);
  BFile& handle = *pe_cache;
  handle.seek(0L,SEEK_SET);
  /**
     building references chain for external
  */
  if(peDir[PE_IMPORT].rva)
  {
    phys = RVA2Phys(peDir[PE_IMPORT].rva);
    nnames = GetImportCountPE(handle,phys);
  }
  else
  {
    phys = 0;
    nnames = 0;
  }
  for(i = 0;i < nnames;i++)
  {
    unsigned long magic,Hint;
    handle.seek(phys + i*sizeof(ImportDirPE),SEEKF_START);
    handle.read_buffer(&ipe,sizeof(ImportDirPE));
    if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
				 magic = ipe.idLookupTableRVA;
    else                         magic = ipe.idFlags;
    if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
    addr_shift_pe = magic ? RVA2Phys(magic) : magic;
    if(addr_shift_pe)
    {
      bool is_eof;
      handle.seek(addr_shift_pe,SEEKF_START);
      j = 0;
      is_eof = false;
      while(1)
      {
	Hint = is_64bit?handle.read_qword():handle.read_dword();
	is_eof = handle.eof();
	if(!Hint || IsKbdTerminate() || is_eof) break;
	rel.modidx = i;
	rel.laddr = Hint;
	rel.import.funcidx = j;
	if(!la_AddData(CurrPEChain,&rel,MemOutBox)) goto bye;
	j++;
      }
      if(is_eof) break;
    }
  }
  /**
     building references chain for internal
  */
  if(peDir[PE_FIXUP].size)
  {
    phys = RVA2Phys(peDir[PE_FIXUP].rva);
    handle.seek(phys,SEEKF_START);
    cpos = handle.tell();
    while(handle.tell() < cpos + peDir[PE_FIXUP].size)
    {
      uint16_t typeoff;
      __filesize_t page,physoff,size,ccpos;
      bool is_eof;
      ccpos = handle.tell();
      page = handle.read_dword();
      physoff = RVA2Phys(page);
      size = handle.read_dword();
      is_eof = false;
      while(handle.tell() < ccpos + size)
      {
	typeoff = handle.read_word();
	is_eof = handle.eof();
	if(IsKbdTerminate() || is_eof) break;
	rel.modidx = std::numeric_limits<uint64_t>::max();
	rel.import.type = typeoff >> 12;
	rel.laddr = physoff + (typeoff & 0x0FFF);
	if(!la_AddData(CurrPEChain,&rel,MemOutBox)) goto bye;
      }
      if(is_eof) break;
    }
  }
  bye:
  la_Sort(CurrPEChain,compare_pe_reloc_s);
  CloseWnd(w);
}

static RELOC_PE __HUGE__ * __NEAR__ __FASTCALL__ __found_RPE(__filesize_t laddr)
{
  RELOC_PE key;
  if(!CurrPEChain) BuildPERefChain();
  key.laddr = laddr;
  return (RELOC_PE*)la_Find(CurrPEChain,&key,compare_pe_reloc_s);
}

static __filesize_t __NEAR__ __FASTCALL__ BuildReferStrPE(char *str,RELOC_PE __HUGE__ *rpe,int flags)
{
   BFile& handle=*pe_cache,&handle2=*pe_cache4,&handle3=*pe_cache3;
   __filesize_t phys,rva,retrf;
   unsigned long magic;
   uint64_t Hint;
   ImportDirPE ipe;
   char buff[400];
   phys = RVA2Phys(peDir[PE_IMPORT].rva);
   handle.seek(phys + 20L*rpe->modidx,SEEKF_START);
   rva = fioReadDWord(handle,12L,SEEKF_CUR);
   retrf = RAPREF_DONE;
   if(rpe->modidx != std::numeric_limits<uint64_t>::max())
   {
     char *is_ext;
     if(flags & APREF_USE_TYPE) strcat(str," off32");
     handle2.seek(RVA2Phys(rva),SEEKF_START);
     handle2.read_buffer(buff,400);
     buff[399] = 0;
     /*
	Removing extension .dll from import name.
	Modified by "Kostya Nosov" <k-nosov@yandex.ru>
     */
     is_ext = strrchr(buff,'.');
     if(is_ext && !strcmp(is_ext,".dll"))
     {
	  *is_ext = 0;
	  sprintf(&str[strlen(str)]," %s.",buff);
     }
     else
       sprintf(&str[strlen(str)]," <%s>.",buff);
     handle3.seek(phys + rpe->modidx*sizeof(ImportDirPE),SEEKF_START);
     handle3.read_buffer(&ipe,sizeof(ImportDirPE));
     if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
				  magic = ipe.idFlags;
     else                         magic = ipe.idLookupTableRVA;
     if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
     magic = magic ? RVA2Phys(magic) : magic;
     if(magic)
     {
       int cond;
       handle.seek(magic + rpe->import.funcidx*sizeof(long),SEEKF_START);
       Hint = is_64bit?handle.read_qword():handle.read_dword();
       cond=0;
       if(is_64bit) { if(Hint & 0x8000000000000000ULL) cond=1; }
       else         { if(Hint & 0x80000000UL) cond=1; }
       if(cond)
       {
	 /* TODO: is really to have ORDINAL > 0x7fffffff ? */
	 char dig[15];
	 sprintf(dig,"@%lu",Hint & 0x7FFFFFFFUL);
	 strcat(str,dig);
       }
       else
       {
	 uint64_t hint_off;
	 if(is_64bit) hint_off=Hint & 0x7FFFFFFFFFFFFFFFULL;
	 else         hint_off=Hint & 0x7FFFFFFFUL;
	 phys = RVA2Phys(hint_off);
	 if(phys > bmGetFLength()) strcat(str,"???");
	 else
	 {
	   handle2.seek(phys + 2,SEEKF_START);
	   handle2.read_buffer(buff,400);
	   buff[399] = 0;
	   strcat(str,buff);
	 }
       }
     }
   }
   else /** internal refs */
   {
     unsigned long delta,value,point_to;
     const char *pe_how;
     handle3.seek(rpe->laddr,SEEKF_START);
     value = handle.read_dword();
     delta = PE32_HDR(pe32,peImageBase);
     point_to = 0;
     switch(rpe->import.type)
     {
	  default:
	  case 0: /** FULL, fixup is skipped */
		  pe_how = "(";
		  point_to = value;
		  break;
	  case 1: /** HIGH 16-bit */
		  pe_how = "((high16)";
		  break;
	  case 2: /** LOW 16-bit */
		  pe_how = "((low16)";
		  break;
	  case 3: /** HIGHLOW */
		  point_to = peVA2PA(value);
		  pe_how = "((off32)";
		  break;
	  case 4: /** HIGHADJUST */
		  handle.seek(value,SEEK_SET);
		  value = handle.read_dword();
		  point_to = peVA2PA(value);
		  pe_how = "((full32)";
		  break;
	  case 5: /** MIPS JUMP ADDR */
		  pe_how = "((mips)";
		  break;
     }
     retrf = point_to ? point_to : value-delta;
     if(!(flags & APREF_SAVE_VIRT))
     {
       strcat(str,"*this.");
       if(flags & APREF_USE_TYPE) strcat(str,pe_how);
       /** if out of physical image */
       strcat(str,Get8Digit(retrf));
       if(flags & APREF_USE_TYPE)  strcat(str,")");
     }
     else strcat(str,Get8Digit(value));
   }
   return retrf;
}

static unsigned long __FASTCALL__ AppendPERef(char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_PE __HUGE__ *rpe;
  __filesize_t retrf;
  BFile* b_cache;
  char buff[400];
  UNUSED(codelen);
  b_cache = pe_cache3;
  retrf = RAPREF_NONE;
  if(flags & APREF_TRY_PIC) return RAPREF_NONE;
  if(peDir[PE_IMPORT].rva || peDir[PE_FIXUP].rva)
  {
    b_cache->seek(RVA2Phys(bmReadDWordEx(ulShift,SEEK_SET) - PE32_HDR(pe32,peImageBase)),
	    SEEK_SET);
    rpe = __found_RPE(b_cache->read_dword());
    if(!rpe) rpe = __found_RPE(ulShift);
    if(rpe)  retrf = BuildReferStrPE(str,rpe,flags);
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(!PubNames) pe_ReadPubNameList(bmbioHandle(),MemOutBox);
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) GidAddGoAddress(str,r_sh);
       retrf = RAPREF_DONE;
     }
  }
  return retrf;
}

static bool __FASTCALL__ IsPE( void )
{
   char id[2];
   beye_context().headshift = IsNewExe();
   if(beye_context().headshift)
   {
     bmReadBufferEx(id,sizeof(id),beye_context().headshift,SEEKF_START);
     if(id[0] == 'P' && id[1] == 'E') return true;
   }
   return false;
}

static void __FASTCALL__ initPE( void )
{
   int i;

   bmReadBufferEx(&pe,sizeof(PEHEADER),beye_context().headshift,SEEKF_START);
   is_64bit = pe.peMagic==0x20B?1:0;
   bmReadBufferEx(&pe32,PE32_HDR_SIZE(),beye_context().headshift+sizeof(PEHEADER),SEEKF_START);

   if(!(peDir = new PERVA[PE32_HDR(pe32,peDirSize)]))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   bmReadBuffer(peDir, sizeof(PERVA)*PE32_HDR(pe32,peDirSize));

   if(!(peVA = new PE_ADDR[pe.peObjects]))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   bmSeek(0x18 + pe.peNTHdrSize + beye_context().headshift,SEEKF_START);
   for(i = 0;i < pe.peObjects;i++)
   {
     peVA[i].rva = bmReadDWordEx(12L,SEEKF_CUR);
     peVA[i].phys = bmReadDWordEx(4L,SEEKF_CUR);
     bmSeek(16L,SEEKF_CUR);
   }

   BFile& main_handle = bmbioHandle();
   if((pe_cache = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache = &main_handle;
   if((pe_cache1 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache1 = &main_handle;
   if((pe_cache2 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache2 = &main_handle;
   if((pe_cache3 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache3 = &main_handle;
   if((pe_cache4 = main_handle.dup_ex(BBIO_SMALL_CACHE_SIZE)) == &bNull) pe_cache4 = &main_handle;
}

static void __FASTCALL__ destroyPE( void )
{
  BFile& main_handle = bmbioHandle();
  if(peVA) delete peVA;
  if(peDir) delete peDir;
  if(PubNames) { la_Destroy(PubNames); PubNames = 0; }
  if(CurrPEChain) { la_Destroy(CurrPEChain); CurrPEChain = 0; } /* Fixed by "Kostya Nosov" <k-nosov@yandex.ru> */
  if(pe_cache != &bNull && pe_cache != &main_handle) delete pe_cache;
  if(pe_cache1 != &bNull && pe_cache1 != &main_handle) delete pe_cache1;
  if(pe_cache2 != &bNull && pe_cache2 != &main_handle) delete pe_cache2;
  if(pe_cache3 != &bNull && pe_cache3 != &main_handle) delete pe_cache3;
  if(pe_cache4 != &bNull && pe_cache4 != &main_handle) delete pe_cache4;
}

static int __FASTCALL__ bitnessPE(__filesize_t off)
{
   if(off >= beye_context().headshift)
   {
     return (pe.peFlags & 0x0040) ? DAB_USE16 :
	    (pe.peFlags & 0x0100) ? DAB_USE32 : DAB_USE64;
   }
   else return DAB_USE16;
}

static __filesize_t __FASTCALL__ PEHelp( void )
{
  hlpDisplay(10009);
  return BMGetCurrFilePos();
}

static bool __FASTCALL__ peAddressResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 bool bret = true;
 uint32_t res;
 if(cfpos >= beye_context().headshift && cfpos < beye_context().headshift + PE_HDR_SIZE() + PE32_HDR(pe32,peDirSize)*sizeof(PERVA))
 {
    strcpy(addr,"PEH :");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift));
 }
 else
 if(cfpos >= beye_context().headshift + pe.peNTHdrSize + 0x18 &&
    cfpos <  beye_context().headshift + pe.peNTHdrSize + 0x18 + pe.peObjects*sizeof(PE_OBJECT))
 {
    strcpy(addr,"PEOD:");
    strcpy(&addr[5],Get4Digit(cfpos - beye_context().headshift - pe.peNTHdrSize - 0x18));
 }
 else  /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
   if((res=pePA2VA(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = false;
  return bret;
}

static __filesize_t __FASTCALL__ peVA2PA(__filesize_t va)
{
  return va >= PE32_HDR(pe32,peImageBase) ? RVA2Phys(va-PE32_HDR(pe32,peImageBase)) : 0L;
}

static __filesize_t __FASTCALL__ pePA2VA(__filesize_t pa)
{
  int i;
  __filesize_t ret_addr;
  bmSeek(0x18 + pe.peNTHdrSize + beye_context().headshift,SEEK_SET);
  ret_addr = 0;
  for(i = 0;i < pe.peObjects;i++)
  {
    PE_OBJECT po;
    __filesize_t obj_pa;
    if(IsKbdTerminate() || bmEOF()) break;
    bmReadBuffer(&po,sizeof(PE_OBJECT));
    obj_pa = CalcPEObjectEntry(po.oPhysicalOffset);
    if(pa >= obj_pa && pa < obj_pa + po.oPhysicalSize)
    {
      ret_addr = po.oRVA + (pa - obj_pa) + PE32_HDR(pe32,peImageBase);
      break;
    }
  }
  return ret_addr;
}

static void __FASTCALL__ pe_ReadPubName(BFile& b_cache,const struct PubName *it,
			   char *buff,unsigned cb_buff)
{
    char stmp[300];
    __peReadASCIIZName(b_cache,it->nameoff,stmp, sizeof(stmp));
    strncpy(buff,stmp,cb_buff);
    buff[cb_buff-1] = 0;
}

static bool __NEAR__ __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  return fmtFindPubName(*pe_cache4,buff,cb_buff,pa,
			pe_ReadPubNameList,
			pe_ReadPubName);
}

static void __FASTCALL__ pe_ReadPubNameList(BFile& handle,void (__FASTCALL__ *mem_out)(const char *))
{
  unsigned long i,nitems,expaddr,nameptr,nameaddr,entry_pa;
  unsigned ord;
  struct PubName pn;
  BFile& b_cache = (pe_cache4 == &bNull) ? handle : *pe_cache4;
  nitems = PEExportNumItems(handle);
  if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
  expaddr  = RVA2Phys(et.etOrdinalTableRVA);
  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  for(i = 0;i < nitems;i++)
  {
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,SEEKF_START);
    ord = fioReadWord(b_cache,expaddr + i*2,SEEKF_START) + (unsigned)et.etOrdinalBase;
    entry_pa = CalcEntryPE((unsigned)ord,false);
    pn.pa = entry_pa;
    pn.nameoff = nameaddr;
    pn.attr = SC_GLOBAL;
    if(!la_AddData(PubNames,&pn,mem_out)) break;
  }
  la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ peGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
			  __filesize_t pa,bool as_prev)
{
  return fmtGetPubSym(*pe_cache,str,cb_str,func_class,pa,as_prev,
		      pe_ReadPubNameList,
		      pe_ReadPubName);
}

static unsigned __FASTCALL__ peGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
		      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned i,nitems,ret;
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = bitnessPE(pa);
  name[0] = 0;
  nitems = pe.peObjects;
  ret = 0;
  bmSeek(0x18 + pe.peNTHdrSize + beye_context().headshift,SEEK_SET);
  for(i = 0;i < nitems;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || bmEOF()) break;
    bmReadBuffer(&po,sizeof(PE_OBJECT));
    if(pa >= *start && pa < po.oPhysicalOffset)
    {
      /** means between two objects */
      *end = po.oPhysicalOffset;
      ret = 0;
      break;
    }
    if(pa >= po.oPhysicalOffset && pa < po.oPhysicalOffset + po.oPhysicalSize)
    {
      *start = po.oPhysicalOffset;
      *end = *start + po.oPhysicalSize;
      *_class = po.oFlags & 0x00000020L ? OC_CODE : OC_DATA;
      strncpy(name,(const char *)po.oName,cb_name);
      ret = i+1;
      break;
    }
    *start = po.oPhysicalOffset + po.oPhysicalSize;
  }
  return ret;
}

static int __FASTCALL__ pePlatform( void ) {
    unsigned id;
    switch(pe.peCPUType) {
	case 0x8664: /*AMD64*/
	case 0x014C:
	case 0x014D:
	case 0x014E:
	case 0x014F: id = DISASM_CPU_IX86; break;
	case 0x01C0:
	case 0x01C2: id = DISASM_CPU_ARM; break;
	case 0x01F0:
	case 0x01F1: id = DISASM_CPU_PPC; break;
	case 0x0162:
	case 0x0166:
	case 0x0168:
	case 0x0169:
	case 0x0266:
	case 0x0366:
	case 0x0466: id = DISASM_CPU_MIPS; break;
	case 0x01A2:
	case 0x01A3:
	case 0x01A4:
	case 0x01A6:
	case 0x01A8: id = DISASM_CPU_SH; break;
	case 0x0200: id = DISASM_CPU_IA64; break;
	case 0x0284: id = DISASM_CPU_ALPHA; break;
	default: id = DISASM_DATA; break;
    }
    return id;
}

extern const REGISTRY_BIN peTable =
{
  "PE (Portable Executable)",
  { "PEHelp", "Import", "Export", NULL, NULL, NULL, NULL, "PEHead", "Dir   ", "Object" },
  { PEHelp, ShowModRefPE, ShowExpNamPE, NULL, NULL, NULL, NULL, ShowNewHeaderPE, ShowPERVAs, ShowObjectsPE },
  IsPE, initPE, destroyPE,
  NULL,
  AppendPERef,
  fmtSetState,
  pePlatform,
  bitnessPE,
  NULL,
  peAddressResolv,
  peVA2PA,
  pePA2VA,
  peGetPubSym,
  peGetObjAttr,
  NULL,
  NULL
};
} // namespace beye
