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
#include <sstream>
#include <iomanip>
#include <limits>
#include <set>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "colorset.h"
#include "plugins/bin/pe.h"
#include "plugins/disasm.h"
#include "udn.h"
#include "codeguid.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "bconsole.h"
#include "listbox.h"
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
	bool operator<(const RELOC_PE& rhs) const { return laddr<rhs.laddr; }
    };

    struct PE32X_HEADER {
	uint64_t	peImageBase;
	uint32_t	peObjectAlign;
	uint32_t	peFileAlign;
	uint16_t	peOSMajor;
	uint16_t	peOSMinor;
	uint16_t	peUserMajor;
	uint16_t	peUserMinor;
	uint16_t	peSubSystMajor;
	uint16_t	peSubSystMinor;
	uint32_t	peReserv9;
	uint32_t	peImageSize;
	uint32_t	peHeaderSize;
	uint32_t	peFileChecksum;
	uint16_t	peSubSystem;
	uint16_t	peDLLFlags;
	uint64_t	peStackReserveSize;
	uint64_t	peStackCommitSize;
	uint64_t	peHeapReserveSize;
	uint64_t	peHeapCommitSize;
	uint32_t	peReserv10;
	uint32_t	peDirSize;
    };

    class PE_Reader : public Opaque {
	public:
	    PE_Reader(binary_stream& h):handle(h) {}
	    virtual ~PE_Reader() {}

	    void init() { pe32x=read_pe32x_header(); }

	    PE32X_HEADER&	header() { return pe32x; }
	    virtual size_t	header_size() const = 0;
	protected:
	    virtual PE32X_HEADER	read_pe32x_header() const = 0;
	    PE32X_HEADER	pe32x;
	    binary_stream&	handle;
    };

    class PE32_Reader : public PE_Reader {
	public:
	    PE32_Reader(binary_stream& h):PE_Reader(h) {}
	    virtual ~PE32_Reader() {}

	    virtual size_t	header_size() const;
	protected:
	    virtual PE32X_HEADER	read_pe32x_header() const;
    };

    class PE64_Reader : public PE_Reader {
	public:
	    PE64_Reader(binary_stream& h):PE_Reader(h) {}
	    virtual ~PE64_Reader() {}

	    virtual size_t	header_size() const;
	protected:
	    virtual PE32X_HEADER	read_pe32x_header() const;
    };

    class PE_Parser : public MZ_Parser {
	public:
	    PE_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~PE_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F8();
	    virtual __filesize_t	action_F9();
	    virtual __filesize_t	action_F10();

	    virtual std::string		bind(const DisMode& _parent,__filesize_t shift,Bin_Format::bind_type flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual Bin_Format::bitness	query_bitness(__filesize_t) const;
	    virtual std::string		address_resolving(__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev);
	    virtual Object_Info		get_object_attribute(__filesize_t pa);
	private:
	    std::string			pe_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    std::string			BuildReferStrPE(const RELOC_PE& rpe,int flags) const;
	    std::set<RELOC_PE>::const_iterator	__found_RPE(__filesize_t laddr) const;
	    void			BuildPERefChain();
	    std::vector<std::string>	PEReadRVAs() const;
	    __filesize_t		CalcEntryPE(unsigned ordinal,bool dispmsg) const;
	    unsigned			PEExportNumItems(binary_stream& handle);
	    std::vector<std::string>	PEExportReadItems(binary_stream& handle,size_t nnames) const;
	    std::string			__peReadASCIIZName(binary_stream& handle,__filesize_t offset) const;
	    __filesize_t		RVA2Phys(__filesize_t rva) const;
	    std::string			writeExportVA(__filesize_t va,binary_stream&handle) const;
	    std::vector<std::string>	__ReadImpContPE(binary_stream& handle,size_t nnames) const;
	    unsigned			GetImpCountPE(binary_stream& handle) const;
	    std::vector<std::string>	__ReadImportPE(binary_stream& handle,__filesize_t phys,size_t nnames) const;
	    unsigned			GetImportCountPE(binary_stream& handle,__filesize_t phys) const;
	    std::vector<PE_OBJECT>	__ReadObjectsPE(binary_stream& handle,size_t n) const;
	    void			ObjPaintPE(TWindow& win,const std::vector<PE_OBJECT>& names,unsigned start) const;
	    void			PaintNewHeaderPE(TWindow& win,const std::vector<std::string>& ptr,unsigned tpage) const;
	    void			PaintNewHeaderPE_2(TWindow& w,__filesize_t&) const;
	    void			PaintNewHeaderPE_1(TWindow& w,__filesize_t&) const;
	    std::string			PECPUType() const __CONST_FUNC__;
	    __filesize_t		CalcPEObjectEntry(__fileoff_t offset) const;
	    Symbol_Info			FindPubName(__filesize_t pa) const;
	    __fileoff_t			CalcOverlayOffset(__filesize_t) const;
	    void			pe_ReadPubNameList(binary_stream& handle);
	    unsigned			fioReadWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const;
	    __filesize_t		fioReadDWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const;
	    __filesize_t		fioReadDWord2Phys(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const;
	    void			ShowModContextPE(const std::string& title) const;

	    binary_stream*	pe_cache1;
	    binary_stream*	pe_cache2;
	    binary_stream*	pe_cache3;
	    binary_stream*	pe_cache4;
	    std::set<RELOC_PE>	CurrPEChain;

	    __filesize_t	addr_shift_pe;

	    ExportTablePE	et;
	    PERVA*		peDir;
	    std::set<symbolic_information>	PubNames;
	    PE_HEADER		pe;
	    PE_Reader*		reader;

	    __fileoff_t		overlayPE;

	    bool		is_64bit;
	    PE_ADDR*		peVA;
	    binary_stream*	pe_cache;

	    static void			(PE_Parser::*pephead[])(TWindow&,__filesize_t&) const;
    };
static const char* txt[]={ "PEHelp", "Import", "Export","", "","","", "PEHead", "Dir   ", "Object" };
const char* PE_Parser::prompt(unsigned idx) const { return txt[idx]; }
static __filesize_t	entryPE;

template<class T>
inline size_t array_size(const T& x) { return sizeof(T)/sizeof(x[0]); }

__filesize_t PE_Parser::CalcPEObjectEntry(__fileoff_t offset) const
{
 __filesize_t intp;
 intp = offset / reader->header().peFileAlign;
 if(offset % reader->header().peFileAlign) offset = ( offset / intp ) * intp;
 return offset;
}

__filesize_t PE_Parser::RVA2Phys(__filesize_t rva) const
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

__filesize_t PE_Parser::fioReadDWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const
{
 handle.seek(offset,origin);
 return handle.read(type_dword);
}

unsigned PE_Parser::fioReadWord(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const
{
 handle.seek(offset,origin);
 return handle.read(type_word);
}

__filesize_t PE_Parser::fioReadDWord2Phys(binary_stream& handle,__filesize_t offset,binary_stream::e_seek origin) const
{
 unsigned long dword;
 dword = fioReadDWord(handle,offset,origin);
 return RVA2Phys(dword);
}

std::string PE_Parser::PECPUType() const
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

void PE_Parser::PaintNewHeaderPE_1(TWindow& w,__filesize_t& entry_PE) const
{
  const char *fmt;
  time_t tval;
  entry_PE = RVA2Phys(pe.peEntryPointRVA);
  tval = pe.peTimeDataStamp;
  w.printf(
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
	   ,PECPUType().c_str()
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
  w.set_color(dialog_cset.entry);
  w.printf("EntryPoint RVA    %s = %08lXH (Offset: %08lXH)",pe.peFlags & 0x2000 ? "[ LibEntry ]" : "[ EXEEntry ]",pe.peEntryPointRVA,entry_PE); w.clreol();
  w.set_color(dialog_cset.main);
  if(is_64bit)
    fmt = "\nImage base                   = %016llXH\n"
	  "Object aligning                = %08lXH";
  else
    fmt = "\nImage base                   = %08lXH\n"
	  "Object aligning                = %08lXH";
  w.printf(fmt
	   ,reader->header().peImageBase
	   ,reader->header().peObjectAlign);
}

void PE_Parser::PaintNewHeaderPE_2(TWindow& w,__filesize_t& entry_PE) const
{
  const char *fmt;
  static const char* subSystem[] =
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

  w.printf(
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
	   ,reader->header().peFileAlign
	   ,reader->header().peOSMajor,reader->header().peOSMinor,reader->header().peUserMajor
	   ,reader->header().peUserMinor,reader->header().peSubSystMajor,reader->header().peSubSystMinor
	   ,reader->header().peImageSize
	   ,reader->header().peHeaderSize
	   ,reader->header().peFileChecksum
	   ,reader->header().peSubSystem < array_size(subSystem) ? subSystem[reader->header().peSubSystem] : "Unknown"
	   ,reader->header().peDLLFlags
	   ,Gebool(reader->header().peDLLFlags & 0x0001)
	   ,Gebool(reader->header().peDLLFlags & 0x0002)
	   ,Gebool(reader->header().peDLLFlags & 0x0004)
	   ,Gebool(reader->header().peDLLFlags & 0x0008)
	   ,reader->header().peDirSize);
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
   w.printf(fmt
	   ,reader->header().peStackReserveSize
	   ,reader->header().peStackCommitSize
	   ,reader->header().peHeapReserveSize
	   ,reader->header().peHeapCommitSize);
  if ((entry_PE=CalcOverlayOffset(MZ_Parser::is_new_exe(bctx().sc_bm_file()))) != -1) {
    w.set_color(dialog_cset.entry);
    w.printf("\nOverlay                        = %08lXH", entry_PE); w.clreol();
    w.set_color(dialog_cset.main);
  }
}

void (PE_Parser::*PE_Parser::pephead[])(TWindow&,__filesize_t&) const = /* [dBorca] the table is const, not the any_t*/
{
    &PE_Parser::PaintNewHeaderPE_1,
    &PE_Parser::PaintNewHeaderPE_2
};

void PE_Parser::PaintNewHeaderPE(TWindow& win,const std::vector<std::string>& ptr,unsigned tpage) const
{
    std::ostringstream oss;
    win.freeze();
    win.clear();
    oss<<" Portable Executable Header ["<<tpage<<"/"<<ptr.size() + 1<<"] ";
    win.set_title(oss.str(),TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    if(tpage < 2) {
	win.goto_xy(1,1);
	(this->*pephead[tpage])(win,entryPE);
    }
    win.refresh_full();
}

__filesize_t PE_Parser::action_F8()
{
    __fileoff_t fpos;
    fpos = bctx().tell();
    std::vector<std::string> v;
    v.push_back("");
    v.push_back("");
    if(PageBox(70,21,v,*this,&PE_Parser::PaintNewHeaderPE) != -1 && entryPE && entryPE < main_handle().flength()) fpos = entryPE;
    return fpos;
}

void PE_Parser::ObjPaintPE(TWindow& win,const std::vector<PE_OBJECT>& names,unsigned start) const
{
    char buffer[9];
    win.freeze();
    win.clear();
    std::ostringstream oss;
    oss<<" Object Table [ "<<(start + 1)<<" / "<<names.size()<<" ] ";
    win.set_title(oss.str(),TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    win.goto_xy(1,1);
    const PE_OBJECT& nam = names[start];

    memcpy(buffer, nam.oName, 8);
    buffer[8] = 0;

    win.printf(
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
	  ,nam.oVirtualSize
	  ,nam.oRVA
	  ,nam.oPhysicalSize
	  ,nam.oPhysicalOffset
	  ,nam.oNReloc
	  ,nam.oRelocPtr
	  ,nam.oNLineNumb
	  ,nam.oLineNumbPtr
	  ,nam.oFlags

	  ,Gebool(nam.oFlags & 0x00000020UL), Gebool(nam.oFlags & 0x10000000UL)
	  ,Gebool(nam.oFlags & 0x00000040UL), Gebool(nam.oFlags & 0x20000000UL)
	  ,Gebool(nam.oFlags & 0x00000080UL), Gebool(nam.oFlags & 0x40000000UL)
	  ,Gebool(nam.oFlags & 0x00001000UL), Gebool(nam.oFlags & 0x80000000UL)
	  ,Gebool(nam.oFlags & 0x00000200UL)
	  ,Gebool(nam.oFlags & 0x00000800UL)
	  ,Gebool(nam.oFlags & 0x01000000UL)
	  ,Gebool(nam.oFlags & 0x02000000UL)
	  ,Gebool(nam.oFlags & 0x04000000UL)
	  ,Gebool(nam.oFlags & 0x08000000UL)

	  ,nam.oFlags&0x00F00000 ? 1 << (((nam.oFlags&0x00F00000)>>20)-1) : 0
	  ,nam.oFlags&0x00F00000 ? "byte(s)" : "(default)");

    win.refresh_full();
}

std::vector<PE_OBJECT> PE_Parser::__ReadObjectsPE(binary_stream& handle,size_t n) const
{
    std::vector<PE_OBJECT> rc;
    unsigned i;
    for(i = 0;i < n;i++) {
	PE_OBJECT po;
	if(IsKbdTerminate() || handle.eof()) break;
	binary_packet bp=handle.read(sizeof(PE_OBJECT)); memcpy(&po,bp.data(),bp.size());
	rc.push_back(po);
    }
    return rc;
}

__fileoff_t PE_Parser::CalcOverlayOffset(__filesize_t ___headshift) const
{
    __filesize_t overlay_PE=overlayPE;
    if (overlay_PE == -1 && pe.peObjects) {
	pe_cache->seek(0x18 + pe.peNTHdrSize + ___headshift, binary_stream::Seek_Set);
	std::vector<PE_OBJECT> objs = __ReadObjectsPE(*pe_cache, pe.peObjects);
	if (!objs.empty()) {
	    int i;
	    for (i = 0; i < pe.peObjects; i++) {
		PE_OBJECT& o = objs[i];
		__fileoff_t end = o.oPhysicalOffset + ((o.oPhysicalSize + (reader->header().peFileAlign - 1)) & ~(reader->header().peFileAlign - 1));
		if (overlay_PE < end) overlay_PE = end;
	    }
	}
    }
    return overlay_PE;
}

__filesize_t PE_Parser::action_F10()
{
    __filesize_t fpos;
    binary_stream& handle = *pe_cache;
    unsigned nnames;
    fpos = bctx().tell();
    nnames = pe.peObjects;
    if(!nnames) { bctx().NotifyBox(NOT_ENTRY," Objects Table "); return fpos; }
    handle.seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
    std::vector<PE_OBJECT> objs = __ReadObjectsPE(handle,nnames);
    if(!objs.empty()) {
	int ret = PageBox(70,19,objs,*this,&PE_Parser::ObjPaintPE);
	if(ret != -1)  fpos = CalcPEObjectEntry(objs[ret].oPhysicalOffset);
    }
    return fpos;
}

unsigned PE_Parser::GetImportCountPE(binary_stream& handle,__filesize_t phys) const
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
std::string PE_Parser::__peReadASCIIZName(binary_stream& handle,__filesize_t offset) const
{
    std::string rc;
    unsigned j;
    char ch;
    __filesize_t fpos;
    fpos = handle.tell();
    j = 0;
    handle.seek(offset,binary_stream::Seek_Set);
    while(1) {
	ch = handle.read(type_byte);
	if(!ch || handle.eof()) break;
	rc[j++] = ch;
    }
    handle.seek(fpos,binary_stream::Seek_Set);
    return rc;
}

std::vector<std::string> PE_Parser::__ReadImportPE(binary_stream& handle,__filesize_t phys,size_t nnames) const
{
    std::vector<std::string> rc;
    size_t i;
    __filesize_t fpos = handle.tell();
    __filesize_t rva,addr;
    handle.seek(phys,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	std::string tmp;
	bool is_eof;
	rva = fioReadDWord(handle,12L,binary_stream::Seek_Cur);
	handle.seek(4L,binary_stream::Seek_Cur);
	addr = RVA2Phys(rva);
	tmp=__peReadASCIIZName(handle,addr);
	if(IsKbdTerminate()) break;
	is_eof = handle.eof();
	rc.push_back(is_eof ? CORRUPT_BIN_MSG : tmp);
	if(is_eof) break;
    }
    handle.seek(fpos,binary_stream::Seek_Set);
    return rc;
}

unsigned PE_Parser::GetImpCountPE(binary_stream& handle) const
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

std::vector<std::string> PE_Parser:: __ReadImpContPE(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    size_t i;
    unsigned VA;
    uint64_t Hint;
    int cond;
    __filesize_t rphys;
    handle.seek(addr_shift_pe,binary_stream::Seek_Set);
    VA = pa2va(addr_shift_pe);
    for(i = 0;i < nnames;i++) {
	std::string stmp;
	bool is_eof;
	std::ostringstream oss;
	oss<<"."<<std::hex<<std::setfill('0')<<std::setw(8)<<VA<<": ";
	stmp=oss.str();
	VA += 4;
	Hint = is_64bit?handle.read(type_qword):handle.read(type_dword);
	cond=0;
	if(is_64bit) { if(Hint & 0x8000000000000000ULL) cond = 1; }
	else         { if(Hint & 0x0000000080000000ULL) cond = 1; }
	if(!cond) {
	    rphys = RVA2Phys(is_64bit?(Hint&0x7FFFFFFFFFFFFFFFULL):(Hint&0x7FFFFFFFUL));
	    if(rphys > main_handle().flength() || handle.eof())
		rc.push_back(CORRUPT_BIN_MSG);
	    else stmp=__peReadASCIIZName(handle,rphys+2);
	} else {
	    oss.str("");
	    oss<<"< By ordinal >   @";
	    if(is_64bit) oss <<(uint64_t)(Hint & 0x7FFFFFFFFFFFFFFFULL);
	    else oss<<(uint32_t)(Hint & 0x7FFFFFFFUL);
	    stmp=oss.str();
	}
	is_eof = handle.eof();
	rc.push_back(is_eof ? CORRUPT_BIN_MSG : stmp);
	if(IsKbdTerminate() || is_eof) break;
    }
    return rc;
}

void PE_Parser::ShowModContextPE(const std::string& title) const {
    ssize_t nnames = GetImpCountPE(main_handle());
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadImpContPE(main_handle(),nnames);
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return;
}

__filesize_t PE_Parser::action_F2()
{
    binary_stream& handle = *pe_cache;
    std::ostringstream oss;
    unsigned nnames;
    __filesize_t phys,fret;
    fret = bctx().tell();
    if(!peDir[PE_IMPORT].rva) { not_found: bctx().NotifyBox(NOT_ENTRY," Module References "); return fret; }
    handle.seek(0L,binary_stream::Seek_Set);
    phys = RVA2Phys(peDir[PE_IMPORT].rva);
    if(!(nnames = GetImportCountPE(handle,phys))) goto not_found;
    std::vector<std::string> objs = __ReadImportPE(handle,phys,nnames);
    if(!objs.empty()) {
	int i;
	i = 0;
	ListBox lb(bctx());
	while(1) {
	    ImportDirPE imp_pe;
	    unsigned long magic;

	    i = lb.run(objs,MOD_REFER,ListBox::Selective,i);
	    if(i == -1) break;
	    oss<<IMPPROC_TABLE<<objs[i]<<" ";
	    handle.seek(phys + i*sizeof(ImportDirPE),binary_stream::Seek_Set);
	    binary_packet bp=handle.read(sizeof(ImportDirPE)); memcpy(&imp_pe,bp.data(),bp.size());
	    if(handle.eof()) break;
	    if(!(imp_pe.idMajVer == 0 && imp_pe.idMinVer == 0 && imp_pe.idDateTime != 0xFFFFFFFFUL))
		magic = imp_pe.idFlags;
	    else magic = imp_pe.idLookupTableRVA;
	    addr_shift_pe = magic ? RVA2Phys(magic) : magic;
	    ShowModContextPE(oss.str());
	}
    }
    return fret;
}

std::string PE_Parser::writeExportVA(__filesize_t va, binary_stream& handle) const
{
    std::string rc;
    // check for forwarded export
    if (va>=peDir[PE_EXPORT].rva && va<peDir[PE_EXPORT].rva+peDir[PE_EXPORT].size)
	rc=__peReadASCIIZName(handle, RVA2Phys(va));
    // normal export
    else {
	std::ostringstream oss;
	oss<<"."<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)(va + reader->header().peImageBase);
	rc=oss.str();
    }
    return rc;
}

std::vector<std::string> PE_Parser::PEExportReadItems(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    __filesize_t nameaddr,expaddr,nameptr;
    unsigned long *addr;
    unsigned i,ord;
    std::ostringstream oss;
    std::string buff;

    nameptr = RVA2Phys(et.etNamePtrTableRVA);
    expaddr = RVA2Phys(et.etOrdinalTableRVA);
    if(!(addr = new unsigned long[nnames])) return rc;
    handle.seek(RVA2Phys(et.etAddressTableRVA),binary_stream::Seek_Set);
    binary_packet bp=handle.read(sizeof(unsigned long)*nnames); memcpy(addr,bp.data(),bp.size());

    for(i = 0;i < et.etNumNamePtrs;i++) {
	std::string stmp;
	bool is_eof;
	nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,binary_stream::Seek_Set);
	stmp=__peReadASCIIZName(handle,nameaddr);
	if(IsKbdTerminate()) break;
	ord = fioReadWord(handle,expaddr + i*2,binary_stream::Seek_Set);
	is_eof = handle.eof();
	oss.str("");
	oss<<(char)ListBox::Ord_Delimiter<<std::left<<std::setw(9)<<(unsigned long)(ord+(unsigned long)et.etOrdinalBase);
	buff=oss.str();
	buff+=writeExportVA(addr[ord], handle);
	addr[ord] = 0;
	stmp=buff;
	rc.push_back(is_eof ? CORRUPT_BIN_MSG : stmp);  // -XF removed PFree(stmp)
	if(is_eof) break;
    }

    for(i = 0;i < nnames;i++) {
	if(addr[i]) {
	    ord = i+et.etOrdinalBase;
	    oss.str("");
	    oss<<" < by ordinal > "<<(char)ListBox::Ord_Delimiter<<std::left<<std::setw(9)<<(unsigned long)ord<<" ";
	    buff=oss.str();
	    buff+=writeExportVA(addr[i], handle);
	    rc.push_back(buff);
	}
    }

    delete addr;
    return rc;
}

unsigned PE_Parser::PEExportNumItems(binary_stream& handle)
{
  __filesize_t addr;
  if(!peDir[PE_EXPORT].rva) return 0;
  addr = RVA2Phys(peDir[PE_EXPORT].rva);
  handle.seek(addr,binary_stream::Seek_Set);
  binary_packet bp=handle.read(sizeof(et)); memcpy(&et,bp.data(),bp.size());
  return (unsigned)(et.etNumEATEntries);
}

__filesize_t  PE_Parser::CalcEntryPE(unsigned ordinal,bool dispmsg) const
{
 __filesize_t fret,rva;
 unsigned ord;
 binary_stream& handle = *pe_cache1;
 fret = bctx().tell();
 {
   __filesize_t eret;
   rva = RVA2Phys(et.etAddressTableRVA);
   ord = (unsigned)ordinal - (unsigned)et.etOrdinalBase;
   eret = fioReadDWord2Phys(handle,rva + 4*ord,binary_stream::Seek_Set);
   if(eret && eret < main_handle().flength()) fret = eret;
   else if(dispmsg) bctx().ErrMessageBox(NO_ENTRY,BAD_ENTRY);
 }
 return fret;
}

__filesize_t PE_Parser::action_F3()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    unsigned ordinal;
    __filesize_t addr;
    std::string exp_nam;
    std::string exp_buf;
    std::ostringstream oss;
    fpos = bctx().tell();
    exp_nam=EXP_TABLE;
    if(peDir[PE_EXPORT].rva) {
	addr = RVA2Phys(peDir[PE_EXPORT].rva);
	main_handle().seek(addr,binary_stream::Seek_Set);
	binary_packet bp=main_handle().read(sizeof(et)); memcpy(&et,bp.data(),bp.size());
	if(et.etNameRVA) {
	    char sftime[80];
	    struct tm * tm;
	    time_t tval;
	    exp_buf=__peReadASCIIZName(main_handle(),RVA2Phys(et.etNameRVA));
	    if(exp_buf.length() > 50) exp_buf=exp_buf.substr(50)+"...";
	    tval = et.etDateTime;
	    tm = localtime(&tval);
	    strftime(sftime,sizeof(sftime),"%x",tm);
	    oss<<" "<<exp_buf<<" (ver="<<std::hex<<std::setfill('0')<<std::setw(4)<<et.etMajVer
		<<"."<<std::hex<<std::setfill('0')<<std::setw(4)<<et.etMinVer
		<<" "<<sftime<<") ";
	    exp_nam+=oss.str();
	}
    }
    std::string title = exp_nam;
    ssize_t nnames = PEExportNumItems(main_handle());
    ListBox::flags flags = ListBox::Selective | ListBox::Sortable;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = PEExportReadItems(main_handle(),nnames);
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
    if(ret != -1) fpos = CalcEntryPE(ordinal,true);
    return fpos;
}

std::vector<std::string> PE_Parser::PEReadRVAs() const
{
    std::vector<std::string> rc;
    unsigned i;
    static const char *rvaNames[] = {
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

    std::ostringstream oss;
    for (i=0; i<reader->header().peDirSize; i++) {
	oss.str("");
	oss<<(i<array_size(rvaNames) ? rvaNames[i] : "Unknown             ")<<"  "
	    <<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)peDir[i].rva<<"  "
	    <<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)peDir[i].size;
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t PE_Parser::action_F9()
{
    __filesize_t fpos = bctx().tell();
    int ret;
    std::string title = " Directory Entry       RVA           size ";
    ListBox::flags flags = ListBox::Selective | ListBox::UseAcc;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = PEReadRVAs();
    delete w;
    ListBox lb(bctx());
    if(objs.empty()) { bctx().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
exit:
    if (ret!=-1 && peDir[ret].rva) fpos = RVA2Phys(peDir[ret].rva);
    return fpos;
}

/***************************************************************************/
/************************  FOR PE  *****************************************/
/***************************************************************************/
void PE_Parser::BuildPERefChain()
{
  __filesize_t  phys,cpos;
  unsigned long i,j;
  RELOC_PE rel;
  ImportDirPE ipe;
  unsigned nnames;
  TWindow *w;

  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(PubNames.empty()) pe_ReadPubNameList(main_handle());
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
    binary_packet bp=handle.read(sizeof(ImportDirPE)); memcpy(&ipe,bp.data(),bp.size());
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
	CurrPEChain.insert(rel);
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
	CurrPEChain.insert(rel);
      }
      if(is_eof) break;
    }
  }
//  la_Sort(CurrPEChain,compare_pe_reloc_s);
  delete w;
}

std::set<RELOC_PE>::const_iterator PE_Parser::__found_RPE(__filesize_t laddr) const
{
  RELOC_PE key;
  key.laddr = laddr;
  return CurrPEChain.find(key);
}

std::string PE_Parser::BuildReferStrPE(const RELOC_PE& rpe,int flags) const
{
    std::string str;
    binary_stream& handle=*pe_cache,&handle2=*pe_cache4,&handle3=*pe_cache3;
    __filesize_t phys,rva;
    unsigned long magic;
    uint64_t Hint;
    ImportDirPE ipe;
    char buff[400];
    binary_packet bp(1);

    phys = RVA2Phys(peDir[PE_IMPORT].rva);
    handle.seek(phys + 20L*rpe.modidx,binary_stream::Seek_Set);
    rva = fioReadDWord(handle,12L,binary_stream::Seek_Cur);
    if(rpe.modidx != std::numeric_limits<uint64_t>::max()) {
	char *is_ext;
	if(flags & Bin_Format::Use_Type) str+=" off32";
	handle2.seek(RVA2Phys(rva),binary_stream::Seek_Set);
	bp=handle2.read(400); memcpy(buff,bp.data(),bp.size());
	buff[399] = 0;
	/*
	    Removing extension .dll from import name.
	    Modified by "Kostya Nosov" <k-nosov@yandex.ru>
	*/
	is_ext = strrchr(buff,'.');
	if(is_ext && !strcmp(is_ext,".dll")) {
	    *is_ext = 0;
	    str+=" ";
	    str+=buff;
	    str+=".";
	} else {
	    std::ostringstream oss;
	    oss<<" <"<<buff<<">.";
	    str+=oss.str();
	}
	handle3.seek(phys + rpe.modidx*sizeof(ImportDirPE),binary_stream::Seek_Set);
	bp=handle3.read(sizeof(ImportDirPE)); memcpy(&ipe,bp.data(),bp.size());
	if(!(ipe.idMajVer == 0 && ipe.idMinVer == 0 && ipe.idDateTime != 0xFFFFFFFFUL))
				  magic = ipe.idFlags;
	else                         magic = ipe.idLookupTableRVA;
	if(magic == 0) magic = ipe.idLookupTableRVA; /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
	magic = magic ? RVA2Phys(magic) : magic;
	if(magic) {
	    int cond;
	    handle.seek(magic + rpe.import.funcidx*sizeof(long),binary_stream::Seek_Set);
	    Hint = is_64bit?handle.read(type_qword):handle.read(type_dword);
	    cond=0;
	    if(is_64bit) { if(Hint & 0x8000000000000000ULL) cond=1; }
	    else         { if(Hint & 0x80000000UL) cond=1; }
	    if(cond) {
		/* TODO: is really to have ORDINAL > 0x7fffffff ? */
		std::ostringstream oss;
		oss<<"@"<<(Hint & 0x7FFFFFFFUL);
		str+=oss.str();
	    } else {
		uint64_t hint_off;
		if(is_64bit) hint_off=Hint & 0x7FFFFFFFFFFFFFFFULL;
		else         hint_off=Hint & 0x7FFFFFFFUL;
		phys = RVA2Phys(hint_off);
		if(phys > main_handle().flength()) str+="???";
		else {
		    handle2.seek(phys + 2,binary_stream::Seek_Set);
		    bp=handle2.read(400); memcpy(buff,bp.data(),bp.size());
		    buff[399] = 0;
		    str+=buff;
		}
	    }
	}
    } else { /** internal refs */
	unsigned long delta,value,point_to;
	const char *pe_how;
	handle3.seek(rpe.laddr,binary_stream::Seek_Set);
	value = handle.read(type_dword);
	delta = reader->header().peImageBase;
	point_to = 0;
	switch(rpe.import.type) {
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
	std::ostringstream oss;
	if(!(flags & Bin_Format::Save_Virt)) {
	    str+="*this.";
	    if(flags & Bin_Format::Use_Type) str+=pe_how;
	    /** if out of physical image */
	    oss<<std::hex<<std::setfill('0')<<std::setw(8)<<delta;
	    str+=oss.str();
	    if(flags & Bin_Format::Use_Type) str+=")";
	} else { oss<<std::hex<<std::setfill('0')<<std::setw(8)<<value; str+=oss.str(); }
    }
    return str;
}

std::string PE_Parser::bind(const DisMode& parent,__filesize_t ulShift,Bin_Format::bind_type flags,int codelen,__filesize_t r_sh)
{
    std::set<RELOC_PE>::const_iterator rpe;
    std::string str;
    binary_stream* b_cache;
    UNUSED(codelen);
    b_cache = pe_cache3;
    if(flags & Bin_Format::Try_Pic) return str;
    if(peDir[PE_IMPORT].rva || peDir[PE_FIXUP].rva) {
	uint32_t id;
	main_handle().seek(ulShift,binary_stream::Seek_Set);
	id = main_handle().read(type_dword);
	b_cache->seek(RVA2Phys(id - reader->header().peImageBase),
	    binary_stream::Seek_Set);
	if(CurrPEChain.empty()) BuildPERefChain();
	rpe = __found_RPE(b_cache->read(type_dword));
	if(rpe==CurrPEChain.end()) rpe = __found_RPE(ulShift);
	if(rpe!=CurrPEChain.end()) str = BuildReferStrPE(*rpe,flags);
    }
    if(str.empty() && (flags & Bin_Format::Try_Label)) {
	if(PubNames.empty()) pe_ReadPubNameList(main_handle());
	Symbol_Info rc = FindPubName(r_sh);
	if(rc.pa!=Plugin::Bad_Address) {
	    str=rc.name;
	    if(!DumpMode && !EditMode) code_guider().add_go_address(parent,str,r_sh);
        }
    }
    return str;
}

PE_Parser::PE_Parser(BeyeContext& b,binary_stream& h,CodeGuider& __code_guider,udn& u)
	:MZ_Parser(b,h,__code_guider,u)
	,pe_cache1(&h)
	,pe_cache2(&h)
	,pe_cache3(&h)
	,pe_cache4(&h)
	,overlayPE(-1L)
	,pe_cache(&h)
{
    char id[2];
    binary_packet bp(1);
    if(headshift()) {
	main_handle().seek(headshift(),binary_stream::Seek_Set);
	bp=main_handle().read(sizeof(id)); memcpy(id,bp.data(),bp.size());
	if(!(id[0] == 'P' && id[1] == 'E')) throw bad_format_exception();
    }
    else throw bad_format_exception();

    int i;

    main_handle().seek(headshift(),binary_stream::Seek_Set);
    bp=main_handle().read(sizeof(PE_HEADER)); memcpy(&pe,bp.data(),bp.size());
    is_64bit = pe.peMagic==0x20B?1:0;
    if(is_64bit) reader = new(zeromem) PE64_Reader(h);
    else	 reader = new(zeromem) PE32_Reader(h);
    reader->init();

    peDir = new PERVA[reader->header().peDirSize];
    bp=main_handle().read(sizeof(PERVA)*reader->header().peDirSize); memcpy(peDir,bp.data(),bp.size());

    peVA = new PE_ADDR[pe.peObjects];

    main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < pe.peObjects;i++) {
	main_handle().seek(12,binary_stream::Seek_Set);
	peVA[i].rva = main_handle().read(type_dword);
	main_handle().seek(4,binary_stream::Seek_Set);
	peVA[i].phys = main_handle().read(type_dword);
	main_handle().seek(16L,binary_stream::Seek_Cur);
    }

    binary_stream& __main_handle = main_handle();
    pe_cache = __main_handle.dup();
    pe_cache1 = __main_handle.dup();
    pe_cache2 = __main_handle.dup();
    pe_cache3 = __main_handle.dup();
    pe_cache4 = __main_handle.dup();
}

PE_Parser::~PE_Parser()
{
  binary_stream& __main_handle = main_handle();
  if(peVA) delete peVA;
  if(peDir) delete peDir;
  if(pe_cache != &__main_handle) delete pe_cache;
  if(pe_cache1 != &__main_handle) delete pe_cache1;
  if(pe_cache2 != &__main_handle) delete pe_cache2;
  if(pe_cache3 != &__main_handle) delete pe_cache3;
  if(pe_cache4 != &__main_handle) delete pe_cache4;
}

Bin_Format::bitness PE_Parser::query_bitness(__filesize_t off) const
{
   if(off >= headshift())
   {
     return (pe.peFlags & 0x0040) ? Bin_Format::Use16 :
	    (pe.peFlags & 0x0100) ? Bin_Format::Use32 : Bin_Format::Use64;
   }
   else return Bin_Format::Use16;
}

__filesize_t PE_Parser::action_F1()
{
    Beye_Help bhelp(bctx());
    if(bhelp.open(true)) {
	bhelp.run(10009);
	bhelp.close();
    }
    return bctx().tell();
}

std::string PE_Parser::address_resolving(__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
    uint32_t res;
    std::ostringstream oss;
    if(cfpos >= headshift() && cfpos < headshift() + reader->header_size() + reader->header().peDirSize*sizeof(PERVA))
	oss<<"PEH :"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift());
    else if(cfpos >= headshift() + pe.peNTHdrSize + 0x18 &&
	cfpos <  headshift() + pe.peNTHdrSize + 0x18 + pe.peObjects*sizeof(PE_OBJECT))
	oss<<"PEOD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos - headshift() - pe.peNTHdrSize - 0x18);
    else if((res=pa2va(cfpos))!=0) /* Added by "Kostya Nosov" <k-nosov@yandex.ru> */
	oss<<"."<<std::hex<<std::setfill('0')<<std::setw(8)<<res;
    return oss.str();
}

__filesize_t PE_Parser::va2pa(__filesize_t va) const
{
    return va >= reader->header().peImageBase ? RVA2Phys(va-reader->header().peImageBase) : 0L;
}

__filesize_t PE_Parser::pa2va(__filesize_t pa) const
{
    int i;
    __filesize_t ret_addr;
    main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
    ret_addr = 0;
    for(i = 0;i < pe.peObjects;i++) {
	PE_OBJECT po;
	__filesize_t obj_pa;
	if(IsKbdTerminate() || main_handle().eof()) break;
	binary_packet bp=main_handle().read(sizeof(PE_OBJECT)); memcpy(&po,bp.data(),bp.size());
	obj_pa = CalcPEObjectEntry(po.oPhysicalOffset);
	if(pa >= obj_pa && pa < obj_pa + po.oPhysicalSize) {
	    ret_addr = po.oRVA + (pa - obj_pa) + reader->header().peImageBase;
	    break;
	}
    }
    return ret_addr;
}

std::string PE_Parser::pe_ReadPubName(binary_stream& b_cache,const symbolic_information& it) const
{
    return __peReadASCIIZName(b_cache,it.nameoff);
}

Symbol_Info PE_Parser::FindPubName(__filesize_t pa) const
{
    Symbol_Info rc;
    symbolic_information key;
    std::set<symbolic_information>::const_iterator it;
    key.pa = pa;
    it = PubNames.find(key);
    if(it!=PubNames.end()) {
	rc.pa=pa;
	rc.name=pe_ReadPubName(*pe_cache4,*it);
	return rc;
    }
    return _udn().find(pa);
}

void PE_Parser::pe_ReadPubNameList(binary_stream& handle)
{
  unsigned long i,nitems,expaddr,nameptr,nameaddr,entry_pa;
  unsigned ord;
  symbolic_information pn;
  binary_stream& b_cache = *pe_cache4;
  nitems = PEExportNumItems(handle);
  expaddr  = RVA2Phys(et.etOrdinalTableRVA);
  nameptr = RVA2Phys(et.etNamePtrTableRVA);
  for(i = 0;i < nitems;i++)
  {
    nameaddr = fioReadDWord2Phys(handle,nameptr + 4*i,binary_stream::Seek_Set);
    ord = fioReadWord(b_cache,expaddr + i*2,binary_stream::Seek_Set) + (unsigned)et.etOrdinalBase;
    entry_pa = CalcEntryPE((unsigned)ord,false);
    pn.pa = entry_pa;
    pn.nameoff = nameaddr;
    pn.attr = Symbol_Info::Global;
    PubNames.insert(pn);
  }
}

Symbol_Info PE_Parser::get_public_symbol(__filesize_t pa,bool as_prev)
{
    Symbol_Info rc;
    if(PubNames.empty()) pe_ReadPubNameList(*pe_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    rc=find_symbolic_information(PubNames,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	rc.name=pe_ReadPubName(*pe_cache,*idx);
    }
    return rc;
}

Object_Info PE_Parser::get_object_attribute(__filesize_t pa)
{
    Object_Info rc;
    unsigned i,nitems;
    rc.start = 0;
    rc.end = main_handle().flength();
    rc._class = Object_Info::NoObject;
    rc.bitness = query_bitness(pa);
    rc.number = 0;
    nitems = pe.peObjects;
    main_handle().seek(0x18 + pe.peNTHdrSize + headshift(),binary_stream::Seek_Set);
    for(i = 0;i < nitems;i++) {
	PE_OBJECT po;
	if(IsKbdTerminate() || main_handle().eof()) break;
	binary_packet bp=main_handle().read(sizeof(PE_OBJECT)); memcpy(&po,bp.data(),bp.size());
	if(pa >= rc.start && pa < po.oPhysicalOffset) {
	    /** means between two objects */
	    rc.end = po.oPhysicalOffset;
	    rc.number = 0;
	    break;
	}
	if(pa >= po.oPhysicalOffset && pa < po.oPhysicalOffset + po.oPhysicalSize) {
	    rc.start = po.oPhysicalOffset;
	    rc.end = rc.start + po.oPhysicalSize;
	    rc._class = po.oFlags & 0x00000020L ? Object_Info::Code : Object_Info::Data;
	    rc.name=(char*)po.oName;
	    rc.number = i+1;
	    break;
	}
	rc.start = po.oPhysicalOffset + po.oPhysicalSize;
    }
    return rc;
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

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) PE_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info pe_info = {
    "PE (Portable Executable)",	/**< plugin name */
    query_interface
};

PE32X_HEADER PE32_Reader::read_pe32x_header() const {
    PE32X_HEADER rc;
    handle.read(type_dword); // skip peBaseOfData
    rc.peImageBase=handle.read(type_dword);
    rc.peObjectAlign=handle.read(type_dword);
    rc.peFileAlign=handle.read(type_dword);
    rc.peOSMajor=handle.read(type_word);
    rc.peOSMinor=handle.read(type_word);
    rc.peUserMajor=handle.read(type_word);
    rc.peUserMinor=handle.read(type_word);
    rc.peSubSystMajor=handle.read(type_word);
    rc.peSubSystMinor=handle.read(type_word);
    rc.peReserv9=handle.read(type_dword);
    rc.peImageSize=handle.read(type_dword);
    rc.peHeaderSize=handle.read(type_dword);
    rc.peFileChecksum=handle.read(type_dword);
    rc.peSubSystem=handle.read(type_word);
    rc.peDLLFlags=handle.read(type_word);
    rc.peStackReserveSize=handle.read(type_dword);
    rc.peStackCommitSize=handle.read(type_dword);
    rc.peHeapReserveSize=handle.read(type_dword);
    rc.peHeapCommitSize=handle.read(type_dword);
    rc.peReserv10=handle.read(type_dword);
    rc.peDirSize=handle.read(type_dword);
    return rc;
}
size_t PE32_Reader::header_size() const { return sizeof(PE32_HEADER); }

PE32X_HEADER PE64_Reader::read_pe32x_header() const {
    PE32X_HEADER rc;
    rc.peImageBase=handle.read(type_qword);
    rc.peObjectAlign=handle.read(type_dword);
    rc.peFileAlign=handle.read(type_dword);
    rc.peOSMajor=handle.read(type_word);
    rc.peOSMinor=handle.read(type_word);
    rc.peUserMajor=handle.read(type_word);
    rc.peUserMinor=handle.read(type_word);
    rc.peSubSystMajor=handle.read(type_word);
    rc.peSubSystMinor=handle.read(type_word);
    rc.peReserv9=handle.read(type_dword);
    rc.peImageSize=handle.read(type_dword);
    rc.peHeaderSize=handle.read(type_dword);
    rc.peFileChecksum=handle.read(type_dword);
    rc.peSubSystem=handle.read(type_word);
    rc.peDLLFlags=handle.read(type_word);
    rc.peStackReserveSize=handle.read(type_qword);
    rc.peStackCommitSize=handle.read(type_qword);
    rc.peHeapReserveSize=handle.read(type_qword);
    rc.peHeapCommitSize=handle.read(type_qword);
    rc.peReserv10=handle.read(type_dword);
    rc.peDirSize=handle.read(type_dword);
    return rc;
}
size_t PE64_Reader::header_size() const { return sizeof(PE32P_HEADER); }
} // namespace	usr
