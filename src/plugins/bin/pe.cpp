#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
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

#include "colorset.h"
#include "plugins/bin/pe.h"
#include "plugins/disasm.h"
#include "bin_util.h"
#include "codeguid.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "mz.h"
#include "beye.h"

namespace	usr {
    struct RELOC_PE {
	uint64_t modidx;
	union {
	    uint64_t funcidx; /** if modidx != -1 */
	    uint64_t type;    /** if modidx == -1 */
	}import;
	uint64_t laddr; /** lookup addr */
	uint64_t reserved;
    };

    class PE_Parser : public MZ_Parser {
	public:
	    PE_Parser(binary_stream&,CodeGuider&);
	    virtual ~PE_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F8();
	    virtual __filesize_t	action_F9();
	    virtual __filesize_t	action_F10();

	    virtual bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(char *,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	    virtual __filesize_t	get_public_symbol(char *str,unsigned cb_str,unsigned *_class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
							__filesize_t *start,__filesize_t *end,int *_class,int *bitness);
	private:
	    void			pe_ReadPubName(binary_stream&b_cache,const struct PubName *it,char *buff,unsigned cb_buff);
	    bool			BuildReferStrPE(char *str,RELOC_PE *rpe,int flags);
	    RELOC_PE*			__found_RPE(__filesize_t laddr);
	    void			BuildPERefChain();
	    static tCompare		compare_pe_reloc_s(const any_t *e1,const any_t *e2);
	    bool			PEReadRVAs(binary_stream&handle,memArray *obj,unsigned nnames);
	    __filesize_t		CalcEntryPE(unsigned ordinal,bool dispmsg);
	    unsigned			PEExportNumItems(binary_stream&handle);
	    bool			PEExportReadItems(binary_stream&handle,memArray *obj,unsigned nnames);
	    unsigned			__peReadASCIIZName(binary_stream&handle,__filesize_t offset,char *buff,unsigned cb_buff);
	    static __filesize_t		RVA2Phys(__filesize_t rva);
	    void			writeExportVA(__filesize_t va,binary_stream&handle,char *buf,unsigned long bufsize);
	    bool			__ReadImpContPE(binary_stream&handle,memArray *obj,unsigned nnames);
	    unsigned			GetImpCountPE(binary_stream&handle);
	    unsigned			__ReadImportPE(binary_stream&handle,__filesize_t phys,memArray *obj,unsigned nnames);
	    unsigned			GetImportCountPE(binary_stream&handle,__filesize_t phys);
	    static bool			__ReadObjectsPE(binary_stream&handle,memArray *obj,unsigned n);
	    static void __FASTCALL__	ObjPaintPE(TWindow *win,const any_t **names,unsigned start,unsigned nlist);
	    static void			PaintNewHeaderPE(TWindow *win,const any_t **ptr,unsigned npage,unsigned tpage);
	    static void			PaintNewHeaderPE_2(TWindow *w);
	    static void			PaintNewHeaderPE_1(TWindow *w);
	    static const char*		PECPUType();
	    __filesize_t		CalcPEObjectEntry(__fileoff_t offset);
	    bool			FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
	    static __fileoff_t		CalcOverlayOffset(__filesize_t);
	    void			pe_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
	    unsigned			fioReadWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin);
	    __filesize_t		fioReadDWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin);
	    __filesize_t		fioReadDWord2Phys(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin);
	    void			ShowModContextPE(const std::string& title);

	    binary_stream*	pe_cache1;
	    binary_stream*	pe_cache2;
	    binary_stream*	pe_cache3;
	    binary_stream*	pe_cache4;
	    linearArray*	CurrPEChain;

	    __filesize_t	addr_shift_pe;

	    ExportTablePE	et;
	    PERVA*		peDir;
	    linearArray*	PubNames;

	    static void			(* const pephead[])(TWindow *);
    };
static const char* txt[]={ "PEHelp", "Import", "Export","", "","","", "PEHead", "Dir   ", "Object" };
const char* PE_Parser::prompt(unsigned idx) const { return txt[idx]; }

#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))

static __fileoff_t overlayPE = -1L;
static __filesize_t entryPE = 0;

static int is_64bit;
static binary_stream* pe_cache=&bNull;

typedef union {
  PE32HEADER   pe32;
  PE32P_HEADER pe32p;
}PE32X_HEADER;

static PE32X_HEADER pe32;
static PEHEADER pe;
static PE_ADDR* peVA;

#define PE32_HDR(e,FIELD) (is_64bit?(((PE32P_HEADER *)&e.pe32p)->FIELD):(((PE32HEADER *)&e.pe32)->FIELD))
#define PE32_HDR_SIZE() (is_64bit?sizeof(PE32P_HEADER):sizeof(PE32HEADER))
#define PE_HDR_SIZE() (sizeof(PEHEADER) + PE32_HDR_SIZE())

__filesize_t PE_Parser::CalcPEObjectEntry(__fileoff_t offset)
{
 __filesize_t intp;
 intp = offset / PE32_HDR(pe32,peFileAlign);
 if(offset % PE32_HDR(pe32,peFileAlign)) offset = ( offset / intp ) * intp;
 return offset;
}

__filesize_t PE_Parser::RVA2Phys(__filesize_t rva)
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

__filesize_t PE_Parser::fioReadDWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin)
{
 handle.seek(offset,origin);
 return handle.read(type_dword);
}

unsigned PE_Parser::fioReadWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin)
{
 handle.seek(offset,origin);
 return handle.read(type_word);
}

__filesize_t PE_Parser::fioReadDWord2Phys(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin)
{
 unsigned long dword;
 dword = fioReadDWord(handle,offset,origin);
 return RVA2Phys(dword);
}

const char* PE_Parser::PECPUType()
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

void PE_Parser::PaintNewHeaderPE_1(TWindow* w)
{
  const char *fmt;
  time_t tval;
  entryPE = RVA2Phys(pe.peEntryPointRVA);
  tval = pe.peTimeDataStamp;
  w->printf(
	   "Signature                      = '%c%c' (Type: %04X)\n"
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
  w->set_color(dialog_cset.entry);
  w->printf("EntryPoint RVA    %s = %08lXH (Offset: %08lXH)",pe.peFlags & 0x2000 ? "[ LibEntry ]" : "[ EXEEntry ]",pe.peEntryPointRVA,entryPE); w->clreol();
  w->set_color(dialog_cset.main);
  if(is_64bit)
    fmt = "\nImage base                   = %016llXH\n"
	  "Object aligning                = %08lXH";
  else
    fmt = "\nImage base                   = %08lXH\n"
	  "Object aligning                = %08lXH";
  w->printf(fmt
	   ,PE32_HDR(pe32,peImageBase)
	   ,PE32_HDR(pe32,peObjectAlign));
}

void PE_Parser::PaintNewHeaderPE_2(TWindow* w)
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

  w->printf(
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
   w->printf(fmt
	   ,PE32_HDR(pe32,peStackReserveSize)
	   ,PE32_HDR(pe32,peStackCommitSize)
	   ,PE32_HDR(pe32,peHeapReserveSize)
	   ,PE32_HDR(pe32,peHeapCommitSize));
  if (CalcOverlayOffset(MZ_Parser::is_new_exe(beye_context().sc_bm_file())) != -1) {
    entryPE = overlayPE;
    w->set_color(dialog_cset.entry);
    w->printf("\nOverlay                        = %08lXH", entryPE); w->clreol();
    w->set_color(dialog_cset.main);
  }
}

void (* const PE_Parser::pephead[])(TWindow*) = /* [dBorca] the table is const, not the any_t*/
{
    PaintNewHeaderPE_1,
    PaintNewHeaderPE_2
};

void PE_Parser::PaintNewHeaderPE(TWindow * win,const any_t**ptr,unsigned npage,unsigned tpage)
{
  char text[80];
  UNUSED(ptr);
  win->freeze();
  win->clear();
  sprintf(text," Portable Executable Header [%d/%d] ",npage + 1,tpage);
  win->set_title(text,TWindow::TMode_Center,dialog_cset.title);
  win->set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
  if(npage < 2)
  {
    win->goto_xy(1,1);
    (*(pephead[npage]))(win);
  }
  win->refresh_full();
}

__filesize_t PE_Parser::action_F8()
{
 __fileoff_t fpos;
 fpos = beye_context().tell();
 if(PageBox(70,21,NULL,2,PaintNewHeaderPE) != -1 && entryPE && entryPE < main_handle().flength()) fpos = entryPE;
 return fpos;
}

void PE_Parser::ObjPaintPE(TWindow * win,const any_t** names,unsigned start,unsigned nlist)
{
 char buffer[81];
 const PE_OBJECT ** nam = (const PE_OBJECT **)names;
 const PE_OBJECT *  objs = nam[start];
 win->freeze();
 win->clear();
 sprintf(buffer," Object Table [ %u / %u ] ",start + 1,nlist);
 win->set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
 win->set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
 win->goto_xy(1,1);

 memcpy(buffer, objs->oName, 8);
 buffer[8] = 0;

 win->printf(
	  "Object Name                    = %8s\n"
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

 win->refresh_full();
}

bool PE_Parser::__ReadObjectsPE(binary_stream& handle,memArray * obj,unsigned n)
{
 unsigned i;
  for(i = 0;i < n;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || handle.eof()) break;
    handle.read(&po,sizeof(PE_OBJECT));
    if(!ma_AddData(obj,&po,sizeof(PE_OBJECT),true)) break;
  }
  return true;
}

__fileoff_t PE_Parser::CalcOverlayOffset(__filesize_t headshift)
{
  if (overlayPE == -1 && pe.peObjects) {
    memArray *obj;
    if ((obj = ma_Build(pe.peObjects, true))) {
      pe_cache->seek(0x18 + pe.peNTHdrSize + headshift, binary_stream::Seek_Set);
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

__filesize_t PE_Parser::action_F10()
{
 __filesize_t fpos;
 binary_stream& handle = *pe_cache;
 unsigned nnames;
 memArray * obj;
 fpos = beye_context().tell();
 nnames = pe.peObjects;
 if(!nnames) { beye_context().NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
 if(!(obj = ma_Build(nnames,true))) return fpos;
 handle.seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
 if(__ReadObjectsPE(handle,obj,nnames))
 {
  int ret;
    ret = PageBox(70,19,(const any_t**)obj->data,obj->nItems,ObjPaintPE);
    if(ret != -1)  fpos = CalcPEObjectEntry(((PE_OBJECT *)obj->data[ret])->oPhysicalOffset);
 }
 ma_Destroy(obj);
 return fpos;
}

unsigned PE_Parser::GetImportCountPE(binary_stream& handle,__filesize_t phys)
{
  unsigned count;
  __filesize_t fpos = handle.tell();
  unsigned long ctrl;
  count = 0;
  handle.seek(phys,binary_stream::Seek_Set);
  while(1)
  {
    ctrl = fioReadDWord(handle,12L,binary_stream::Seek_Cur);
    handle.seek(4L,binary_stream::Seek_Cur);
    if(ctrl == 0 || count > 0xFFFD || IsKbdTerminate() || handle.eof()) break;
    count++;
  }
  handle.seek(fpos,binary_stream::Seek_Set);
  return count;
}

/* returns really readed number of characters */
unsigned PE_Parser::__peReadASCIIZName(binary_stream& handle,__filesize_t offset,char *buff, unsigned cb_buff)
{
  unsigned j;
  char ch;
  __filesize_t fpos;
  fpos = handle.tell();
  j = 0;
  handle.seek(offset,binary_stream::Seek_Set);
  while(1)
  {
    ch = handle.read(type_byte);
    buff[j++] = ch;
    if(!ch || j >= cb_buff || handle.eof()) break;
  }
  if(j) buff[j-1]=0;
  handle.seek(fpos,binary_stream::Seek_Set);
  return j;
}

unsigned  PE_Parser::__ReadImportPE(binary_stream& handle,__filesize_t phys,memArray *obj,unsigned nnames)
{
  unsigned i;
  __filesize_t fpos = handle.tell();
  __filesize_t rva,addr;
  handle.seek(phys,binary_stream::Seek_Set);
  for(i = 0;i < nnames;i++)
  {
    char tmp[300];
    bool is_eof;
    rva = fioReadDWord(handle,12L,binary_stream::Seek_Cur);
    handle.seek(4L,binary_stream::Seek_Cur);
    addr = RVA2Phys(rva);
    __peReadASCIIZName(handle,addr,tmp,sizeof(tmp));
    if(IsKbdTerminate()) break;
    is_eof = handle.eof();
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : tmp,true)) break;
    if(is_eof) break;
  }
  handle.seek(fpos,binary_stream::Seek_Set);
  return obj->nItems;
}

unsigned PE_Parser::GetImpCountPE(binary_stream& handle)
{
 unsigned count;
 uint64_t Hint;
 count = 0;
 if(addr_shift_pe)
 {
   handle.seek(addr_shift_pe,binary_stream::Seek_Set);
   while(1)
   {
     Hint = is_64bit ? handle.read(type_qword):handle.read(type_dword);
     if(Hint == 0ULL || count > 0xFFFD || IsKbdTerminate() || handle.eof()) break;
     count++;
   }
 }
 return count;
}

bool PE_Parser:: __ReadImpContPE(binary_stream& handle,memArray * obj,unsigned nnames)
{
  unsigned i,VA;
  uint64_t Hint;
  int cond;
  __filesize_t rphys;
  handle.seek(addr_shift_pe,binary_stream::Seek_Set);
  VA = pa2va(addr_shift_pe);
  for(i = 0;i < nnames;i++)
  {
    char stmp[300];
    bool is_eof;
    sprintf(stmp,".%08X: ", VA);
    VA += 4;
    Hint = is_64bit?handle.read(type_qword):handle.read(type_dword);
    cond=0;
    if(is_64bit) { if(Hint & 0x8000000000000000ULL) cond = 1; }
    else         { if(Hint & 0x0000000080000000ULL) cond = 1; }
    if(!cond)
    {
      rphys = RVA2Phys(is_64bit?(Hint&0x7FFFFFFFFFFFFFFFULL):(Hint&0x7FFFFFFFUL));
      if(rphys > main_handle().flength() || handle.eof())
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

void PE_Parser::ShowModContextPE(const std::string& title) {
    ssize_t nnames = GetImpCountPE(main_handle());
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadImpContPE(main_handle(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return;
}

__filesize_t PE_Parser::action_F2()
{
    binary_stream& handle = *pe_cache;
    char petitle[80];
    memArray* obj;
    unsigned nnames;
    __filesize_t phys,fret;
    fret = beye_context().tell();
    if(!peDir[PE_IMPORT].rva) { not_found: beye_context().NotifyBox(NOT_ENTRY," Module References "); return fret; }
    handle.seek(0L,binary_stream::Seek_Set);
    phys = RVA2Phys(peDir[PE_IMPORT].rva);
    if(!(nnames = GetImportCountPE(handle,phys))) goto not_found;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    if(__ReadImportPE(handle,phys,obj,nnames)) {
	int i;
	i = 0;
	while(1) {
	    ImportDirPE imp_pe;
	    unsigned long magic;

	    i = ma_Display(obj,MOD_REFER,LB_SELECTIVE,i);
	    if(i == -1) break;
	    sprintf(petitle,"%s%s ",IMPPROC_TABLE,(char *)obj->data[i]);
	    handle.seek(phys + i*sizeof(ImportDirPE),binary_stream::Seek_Set);
	    handle.read(&imp_pe,sizeof(ImportDirPE));
	    if(handle.eof()) break;
	    if(!(imp_pe.idMajVer == 0 && imp_pe.idMinVer == 0 && imp_pe.idDateTime != 0xFFFFFFFFUL))
		magic = imp_pe.idFlags;
	    else magic = imp_pe.idLookupTableRVA;
	    addr_shift_pe = magic ? RVA2Phys(magic) : magic;
	    ShowModContextPE(petitle);
	}
    }
    ma_Destroy(obj);
    exit:
    return fret;
}

void PE_Parser::writeExportVA(__filesize_t va, binary_stream& handle, char *buf, unsigned long bufsize)
{
    // check for forwarded export
    if (va>=peDir[PE_EXPORT].rva && va<peDir[PE_EXPORT].rva+peDir[PE_EXPORT].size)
	__peReadASCIIZName(handle, RVA2Phys(va), buf, bufsize);
    // normal export
    else
    sprintf(buf, ".%08lX", (unsigned long)(va + PE32_HDR(pe32,peImageBase)));
}

bool PE_Parser::PEExportReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
{
  __filesize_t nameaddr,expaddr,nameptr;
  unsigned long *addr;
  unsigned i,ord;
  char buff[80];

  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  expaddr = RVA2Phys(et.etOrdinalTableRVA);
  if(!(addr = new unsigned long[nnames])) return true;
  handle.seek(RVA2Phys(et.etAddressTableRVA),binary_stream::Seek_Set);
  handle.read(addr,sizeof(unsigned long)*nnames);

  for(i = 0;i < et.etNumNamePtrs;i++)
  {
    char stmp[300];
    bool is_eof;
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,binary_stream::Seek_Set);
    __peReadASCIIZName(handle,nameaddr, stmp, sizeof(stmp)-sizeof(buff)-1);
    if(IsKbdTerminate()) break;
    ord = fioReadWord(handle,expaddr + i*2,binary_stream::Seek_Set);
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

unsigned PE_Parser::PEExportNumItems(binary_stream& handle)
{
  __filesize_t addr;
  if(!peDir[PE_EXPORT].rva) return 0;
  addr = RVA2Phys(peDir[PE_EXPORT].rva);
  handle.seek(addr,binary_stream::Seek_Set);
  handle.read((any_t*)&et,sizeof(et));
  return (unsigned)(et.etNumEATEntries);
}

__filesize_t  PE_Parser::CalcEntryPE(unsigned ordinal,bool dispmsg)
{
 __filesize_t fret,rva;
 unsigned ord;
 binary_stream& handle = *pe_cache1;
 fret = beye_context().tell();
 {
   __filesize_t eret;
   rva = RVA2Phys(et.etAddressTableRVA);
   ord = (unsigned)ordinal - (unsigned)et.etOrdinalBase;
   eret = fioReadDWord2Phys(handle,rva + 4*ord,binary_stream::Seek_Set);
   if(eret && eret < main_handle().flength()) fret = eret;
   else if(dispmsg) beye_context().ErrMessageBox(NO_ENTRY,BAD_ENTRY);
 }
 return fret;
}

__filesize_t PE_Parser::action_F3()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    unsigned ordinal;
    __filesize_t addr;
    char exp_nam[256], exp_buf[300];
    fpos = beye_context().tell();
    strcpy(exp_nam,EXP_TABLE);
    if(peDir[PE_EXPORT].rva) {
	addr = RVA2Phys(peDir[PE_EXPORT].rva);
	main_handle().seek(addr,binary_stream::Seek_Set);
	main_handle().read((any_t*)&et,sizeof(et));
	if(et.etNameRVA) {
	    char sftime[80];
	    struct tm * tm;
	    time_t tval;
	    __peReadASCIIZName(main_handle(),RVA2Phys(et.etNameRVA),exp_buf, sizeof(exp_buf));
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
    std::string title = exp_nam;
    ssize_t nnames = PEExportNumItems(main_handle());
    int flags = LB_SELECTIVE | LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = PEExportReadItems(main_handle(),obj,nnames);
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
    if(ret != -1) fpos = CalcEntryPE(ordinal,true);
    return fpos;
}

bool PE_Parser::PEReadRVAs(binary_stream& handle, memArray * obj, unsigned nnames)
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

__filesize_t PE_Parser::action_F9()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " Directory Entry       RVA           size ";
    ssize_t nnames = PE32_HDR(pe32,peDirSize);
    int flags = LB_SELECTIVE | LB_USEACC;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = PEReadRVAs(main_handle(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if (ret!=-1 && peDir[ret].rva) fpos = RVA2Phys(peDir[ret].rva);
    return fpos;
}

/***************************************************************************/
/************************  FOR PE  *****************************************/
/***************************************************************************/
tCompare PE_Parser::compare_pe_reloc_s(const any_t*e1,const any_t*e2)
{
  const RELOC_PE  *p1, *p2;
  p1 = (const RELOC_PE  *)e1;
  p2 = (const RELOC_PE  *)e2;
  return ((p1->laddr) < (p2->laddr) ? -1 : (p1->laddr) > (p2->laddr) ? 1 : 0);
}

void PE_Parser::BuildPERefChain()
{
  __filesize_t  phys,cpos;
  unsigned long i,j;
  RELOC_PE rel;
  ImportDirPE ipe;
  unsigned nnames;
  TWindow *w;
  if(!(CurrPEChain = la_Build(0,sizeof(RELOC_PE),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) pe_ReadPubNameList(main_handle(),MemOutBox);
  w->goto_xy(1,1);
  w->puts(BUILD_REFS);
  binary_stream& handle = *pe_cache;
  handle.seek(0L,binary_stream::Seek_Set);
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
    handle.seek(phys + i*sizeof(ImportDirPE),binary_stream::Seek_Set);
    handle.read(&ipe,sizeof(ImportDirPE));
    if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
				 magic = ipe.idLookupTableRVA;
    else                         magic = ipe.idFlags;
    if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
    addr_shift_pe = magic ? RVA2Phys(magic) : magic;
    if(addr_shift_pe)
    {
      bool is_eof;
      handle.seek(addr_shift_pe,binary_stream::Seek_Set);
      j = 0;
      is_eof = false;
      while(1)
      {
	Hint = is_64bit?handle.read(type_qword):handle.read(type_dword);
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
    handle.seek(phys,binary_stream::Seek_Set);
    cpos = handle.tell();
    while(handle.tell() < cpos + peDir[PE_FIXUP].size)
    {
      uint16_t typeoff;
      __filesize_t page,physoff,size,ccpos;
      bool is_eof;
      ccpos = handle.tell();
      page = handle.read(type_dword);
      physoff = RVA2Phys(page);
      size = handle.read(type_dword);
      is_eof = false;
      while(handle.tell() < ccpos + size)
      {
	typeoff = handle.read(type_word);
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
  delete w;
}

RELOC_PE* PE_Parser::__found_RPE(__filesize_t laddr)
{
  RELOC_PE key;
  if(!CurrPEChain) BuildPERefChain();
  key.laddr = laddr;
  return (RELOC_PE*)la_Find(CurrPEChain,&key,compare_pe_reloc_s);
}

bool PE_Parser::BuildReferStrPE(char *str,RELOC_PE  *rpe,int flags)
{
   binary_stream& handle=*pe_cache,&handle2=*pe_cache4,&handle3=*pe_cache3;
   __filesize_t phys,rva;
   bool retrf;
   unsigned long magic;
   uint64_t Hint;
   ImportDirPE ipe;
   char buff[400];
   phys = RVA2Phys(peDir[PE_IMPORT].rva);
   handle.seek(phys + 20L*rpe->modidx,binary_stream::Seek_Set);
   rva = fioReadDWord(handle,12L,binary_stream::Seek_Cur);
   retrf = true;
   if(rpe->modidx != std::numeric_limits<uint64_t>::max())
   {
     char *is_ext;
     if(flags & APREF_USE_TYPE) strcat(str," off32");
     handle2.seek(RVA2Phys(rva),binary_stream::Seek_Set);
     handle2.read(buff,400);
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
     handle3.seek(phys + rpe->modidx*sizeof(ImportDirPE),binary_stream::Seek_Set);
     handle3.read(&ipe,sizeof(ImportDirPE));
     if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
				  magic = ipe.idFlags;
     else                         magic = ipe.idLookupTableRVA;
     if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
     magic = magic ? RVA2Phys(magic) : magic;
     if(magic)
     {
       int cond;
       handle.seek(magic + rpe->import.funcidx*sizeof(long),binary_stream::Seek_Set);
       Hint = is_64bit?handle.read(type_qword):handle.read(type_dword);
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
	 if(phys > main_handle().flength()) strcat(str,"???");
	 else
	 {
	   handle2.seek(phys + 2,binary_stream::Seek_Set);
	   handle2.read(buff,400);
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
     handle3.seek(rpe->laddr,binary_stream::Seek_Set);
     value = handle.read(type_dword);
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
		  point_to = va2pa(value);
		  pe_how = "((off32)";
		  break;
	  case 4: /** HIGHADJUST */
		  handle.seek(value,binary_stream::Seek_Set);
		  value = handle.read(type_dword);
		  point_to = va2pa(value);
		  pe_how = "((full32)";
		  break;
	  case 5: /** MIPS JUMP ADDR */
		  pe_how = "((mips)";
		  break;
     }
     delta = point_to ? point_to : value-delta;
     if(!(flags & APREF_SAVE_VIRT))
     {
       strcat(str,"*this.");
       if(flags & APREF_USE_TYPE) strcat(str,pe_how);
       /** if out of physical image */
       strcat(str,Get8Digit(delta));
       if(flags & APREF_USE_TYPE)  strcat(str,")");
       retrf=true;
     }
     else strcat(str,Get8Digit(value));
   }
   return retrf;
}

bool PE_Parser::bind(const DisMode& parent,char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_PE  *rpe;
  bool retrf;
  binary_stream* b_cache;
  char buff[400];
  UNUSED(codelen);
  b_cache = pe_cache3;
  retrf = false;
  if(flags & APREF_TRY_PIC) return false;
  if(peDir[PE_IMPORT].rva || peDir[PE_FIXUP].rva)
  {
    uint32_t id;
    main_handle().seek(ulShift,binary_stream::Seek_Set);
    id = main_handle().read(type_dword);
    b_cache->seek(RVA2Phys(id - PE32_HDR(pe32,peImageBase)),
	    binary_stream::Seek_Set);
    rpe = __found_RPE(b_cache->read(type_dword));
    if(!rpe) rpe = __found_RPE(ulShift);
    if(rpe)  retrf = BuildReferStrPE(str,rpe,flags);
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(!PubNames) pe_ReadPubNameList(main_handle(),MemOutBox);
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) code_guider().add_go_address(parent,str,r_sh);
       retrf = true;
     }
  }
  return retrf;
}

PE_Parser::PE_Parser(binary_stream& h,CodeGuider& __code_guider)
	:MZ_Parser(h,__code_guider)
	,pe_cache1(&bNull)
	,pe_cache2(&bNull)
	,pe_cache3(&bNull)
	,pe_cache4(&bNull)
{
   int i;

    main_handle().seek(headshift(),binary_stream::Seek_Set);
    main_handle().read(&pe,sizeof(PEHEADER));
    is_64bit = pe.peMagic==0x20B?1:0;
    main_handle().read(&pe32,PE32_HDR_SIZE());

   if(!(peDir = new PERVA[PE32_HDR(pe32,peDirSize)]))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   main_handle().read(peDir, sizeof(PERVA)*PE32_HDR(pe32,peDirSize));

   if(!(peVA = new PE_ADDR[pe.peObjects]))
   {
     MemOutBox("PE initialization");
     exit(EXIT_FAILURE);
   }
   main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
   for(i = 0;i < pe.peObjects;i++)
   {
     main_handle().seek(12,binary_stream::Seek_Set);
     peVA[i].rva = main_handle().read(type_dword);
     main_handle().seek(4,binary_stream::Seek_Set);
     peVA[i].phys = main_handle().read(type_dword);
     main_handle().seek(16L,binary_stream::Seek_Cur);
   }

   binary_stream& __main_handle = main_handle();
   if((pe_cache = __main_handle.dup()) == &bNull) pe_cache = &__main_handle;
   if((pe_cache1 = __main_handle.dup()) == &bNull) pe_cache1 = &__main_handle;
   if((pe_cache2 = __main_handle.dup()) == &bNull) pe_cache2 = &__main_handle;
   if((pe_cache3 = __main_handle.dup()) == &bNull) pe_cache3 = &__main_handle;
   if((pe_cache4 = __main_handle.dup()) == &bNull) pe_cache4 = &__main_handle;
}

PE_Parser::~PE_Parser()
{
  binary_stream& __main_handle = main_handle();
  if(peVA) delete peVA;
  if(peDir) delete peDir;
  if(PubNames) { la_Destroy(PubNames); PubNames = 0; }
  if(CurrPEChain) { la_Destroy(CurrPEChain); CurrPEChain = 0; } /* Fixed by "Kostya Nosov" <k-nosov@yandex.ru> */
  if(pe_cache != &bNull && pe_cache != &__main_handle) delete pe_cache;
  if(pe_cache1 != &bNull && pe_cache1 != &__main_handle) delete pe_cache1;
  if(pe_cache2 != &bNull && pe_cache2 != &__main_handle) delete pe_cache2;
  if(pe_cache3 != &bNull && pe_cache3 != &__main_handle) delete pe_cache3;
  if(pe_cache4 != &bNull && pe_cache4 != &__main_handle) delete pe_cache4;
}

int PE_Parser::query_bitness(__filesize_t off) const
{
   if(off >= headshift())
   {
     return (pe.peFlags & 0x0040) ? DAB_USE16 :
	    (pe.peFlags & 0x0100) ? DAB_USE32 : DAB_USE64;
   }
   else return DAB_USE16;
}

__filesize_t PE_Parser::action_F1()
{
  hlpDisplay(10009);
  return beye_context().tell();
}

bool PE_Parser::address_resolving(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
 bool bret = true;
 uint32_t res;
 if(cfpos >= headshift() && cfpos < headshift() + PE_HDR_SIZE() + PE32_HDR(pe32,peDirSize)*sizeof(PERVA))
 {
    strcpy(addr,"PEH :");
    strcpy(&addr[5],Get4Digit(cfpos - headshift()));
 }
 else
 if(cfpos >= headshift() + pe.peNTHdrSize + 0x18 &&
    cfpos <  headshift() + pe.peNTHdrSize + 0x18 + pe.peObjects*sizeof(PE_OBJECT))
 {
    strcpy(addr,"PEOD:");
    strcpy(&addr[5],Get4Digit(cfpos - headshift() - pe.peNTHdrSize - 0x18));
 }
 else  /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
   if((res=pa2va(cfpos))!=0)
   {
     addr[0] = '.';
     strcpy(&addr[1],Get8Digit(res));
   }
   else bret = false;
  return bret;
}

__filesize_t PE_Parser::va2pa(__filesize_t va)
{
  return va >= PE32_HDR(pe32,peImageBase) ? RVA2Phys(va-PE32_HDR(pe32,peImageBase)) : 0L;
}

__filesize_t PE_Parser::pa2va(__filesize_t pa)
{
  int i;
  __filesize_t ret_addr;
  main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
  ret_addr = 0;
  for(i = 0;i < pe.peObjects;i++)
  {
    PE_OBJECT po;
    __filesize_t obj_pa;
    if(IsKbdTerminate() || main_handle().eof()) break;
    main_handle().read(&po,sizeof(PE_OBJECT));
    obj_pa = CalcPEObjectEntry(po.oPhysicalOffset);
    if(pa >= obj_pa && pa < obj_pa + po.oPhysicalSize)
    {
      ret_addr = po.oRVA + (pa - obj_pa) + PE32_HDR(pe32,peImageBase);
      break;
    }
  }
  return ret_addr;
}

void PE_Parser::pe_ReadPubName(binary_stream& b_cache,const struct PubName *it,
			   char *buff,unsigned cb_buff)
{
    char stmp[300];
    __peReadASCIIZName(b_cache,it->nameoff,stmp, sizeof(stmp));
    strncpy(buff,stmp,cb_buff);
    buff[cb_buff-1] = 0;
}

bool PE_Parser::FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  struct PubName *ret,key;
  key.pa = pa;
  if(!PubNames) pe_ReadPubNameList(*pe_cache4,MemOutBox);
  ret = (PubName*)la_Find(PubNames,&key,fmtComparePubNames);
  if(ret)
  {
    pe_ReadPubName(*pe_cache4,ret,buff,cb_buff);
    return true;
  }
  return udnFindName(pa,buff,cb_buff);
}

void PE_Parser::pe_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
  unsigned long i,nitems,expaddr,nameptr,nameaddr,entry_pa;
  unsigned ord;
  struct PubName pn;
  binary_stream& b_cache = (pe_cache4 == &bNull) ? handle : *pe_cache4;
  nitems = PEExportNumItems(handle);
  if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
  expaddr  = RVA2Phys(et.etOrdinalTableRVA);
  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  for(i = 0;i < nitems;i++)
  {
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,binary_stream::Seek_Set);
    ord = fioReadWord(b_cache,expaddr + i*2,binary_stream::Seek_Set) + (unsigned)et.etOrdinalBase;
    entry_pa = CalcEntryPE((unsigned)ord,false);
    pn.pa = entry_pa;
    pn.nameoff = nameaddr;
    pn.attr = SC_GLOBAL;
    if(!la_AddData(PubNames,&pn,mem_out)) break;
  }
  la_Sort(PubNames,fmtComparePubNames);
}

__filesize_t PE_Parser::get_public_symbol(char *str,unsigned cb_str,unsigned *func_class,
			  __filesize_t pa,bool as_prev)
{
    __filesize_t fpos;
    size_t idx;
    if(!PubNames) pe_ReadPubNameList(*pe_cache,NULL);
    fpos=fmtGetPubSym(*func_class,pa,as_prev,PubNames,idx);
    if(idx!=std::numeric_limits<size_t>::max()) {
	struct PubName *it;
	it = &((struct PubName  *)PubNames->data)[idx];
	pe_ReadPubName(*pe_cache,it,str,cb_str);
	str[cb_str-1] = 0;
    }
    return fpos;
}

unsigned PE_Parser::get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
		      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned i,nitems,ret;
  *start = 0;
  *end = main_handle().flength();
  *_class = OC_NOOBJECT;
  *bitness = query_bitness(pa);
  name[0] = 0;
  nitems = pe.peObjects;
  ret = 0;
  main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
  for(i = 0;i < nitems;i++)
  {
    PE_OBJECT po;
    if(IsKbdTerminate() || main_handle().eof()) break;
    main_handle().read(&po,sizeof(PE_OBJECT));
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

int PE_Parser::query_platform() const {
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

static bool probe(binary_stream& main_handle) {
   char id[2];
   __filesize_t headshift = MZ_Parser::is_new_exe(main_handle);
   if(headshift)
   {
     main_handle.seek(headshift,binary_stream::Seek_Set);
     main_handle.read(id,sizeof(id));
     if(id[0] == 'P' && id[1] == 'E') return true;
   }
   return false;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent) { return new(zeromem) PE_Parser(h,_parent); }
extern const Binary_Parser_Info pe_info = {
    "PE (Portable Executable)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
