#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/elf386.c
 * @brief       This file contains implementation of ELF-32 file format decoder.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    A short ELF hacker's guide from author of beye.
    ===============================================
    IMHO ELF is too complex format and contains surplusing information
    like SECTION HEADERS and PROGRAM HEADERS.
    As a rule, in normal ELF files both types are present.
    But there is minor number of non-standard ELF files where
    some type of headers are missing (or section or program ones).
    In case when SECTION HEADERS are lost, executable files and shared
    objects are workable, but BFD utilities can not handle such file
    properly, and so it will be no longer possible to link something
    against such file (but beye works with such files properly).
    In case when PROGRAM HEADERS are lost, executable files and shared
    objects are not workable, but RELOCATABLE OBJECTS will be normally
    linked to target executable or shared object.
    See also:

    http://www.caldera.com/developers/gabi/
    http://segfault.net/~scut/cpu/generic/
    http://www.linuxassembly.org

    ELFkickers This distribution is a collection of programs that are generally
	       unrelated, except in that they all deal with the ELF file format.

    teensy     tools that are provided as exhibitions of just how compressed a
	       Linux ELF executable can be and still function.

    These can be found at:
    http://www.muppetlabs.com/~breadbox/software/
*/
#include <algorithm>
#include <vector>
#include <string>
#include <set>

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "colorset.h"
#include "plugins/bin/elf.h"
#include "plugins/disasm.h"
#include "udn.h"
#include "codeguid.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    struct VA_map {
	__filesize_t va;
	__filesize_t size;
	__filesize_t foff;
	__filesize_t nameoff;
	__filesize_t flags;
    };
    struct Elf_Reloc {
	__filesize_t	offset;
	__filesize_t	info;
	__filesize_t	addend;
	__filesize_t	sh_idx;

	bool operator<(const Elf_Reloc& rhs) const { return offset<rhs.offset; }
    };

    class ELF_Parser;
    class Elf_Arch : public Opaque {
	public:
	    Elf_Arch(ELF_Parser& p,Elf_Reader& r):_parent(p),_reader(r) {}
	    virtual ~Elf_Arch() {}
	
	    virtual void		read_reloc(binary_stream& handle,Elf_Reloc&) const;
	    virtual bool		build_refer_str(binary_stream& handle,std::string& str,const Elf_Reloc& erl,int flags,unsigned codelen,__filesize_t defval) const;

	    virtual std::string		read_nametable(binary_stream& cache,__filesize_t off) const;
	    virtual std::string		read_nametableex(binary_stream& cache,__filesize_t off,__filesize_t shtbl) const;
	protected:
	    Elf_Reader&			reader() const { return _reader; }
	    ELF_Parser&			parent() const { return _parent; }
	    bool			read_reloc_name(binary_stream& handle,const Elf_Reloc& erl,std::string& buff) const;
	    inline uint16_t		bioRead12(binary_stream& handle2) const { return handle2.read(type_word)&0x0FFFUL; };
	    inline uint32_t		bioRead19(binary_stream& handle2) const { return handle2.read(type_dword)&0x0007FFFFUL; };
	    inline uint32_t		bioRead22(binary_stream& handle2) const { return handle2.read(type_dword)&0x003FFFFFUL; };
	    inline uint32_t		bioRead24(binary_stream& handle2) const { return handle2.read(type_dword)&0x00FFFFFFUL; };
	    inline uint32_t		bioRead25(binary_stream& handle2) const { return handle2.read(type_dword)&0x01FFFFFFUL; };
	    inline uint32_t		bioRead30(binary_stream& handle2) const { return handle2.read(type_dword)&0x3FFFFFFFUL; };
	private:
	    ELF_Parser&		_parent;
	    Elf_Reader&		_reader;
    };

    class Elf_Ppc : public Elf_Arch {
	public:
	    Elf_Ppc(ELF_Parser& p,Elf_Reader& r):Elf_Arch(p,r) {}
	    virtual ~Elf_Ppc() {}
	
	    virtual void		read_reloc(binary_stream& handle,Elf_Reloc&) const;
	    virtual bool		build_refer_str(binary_stream& handle,std::string& str,const Elf_Reloc& erl,int flags,unsigned codelen,__filesize_t defval) const;
    };

    class Elf_Arm : public Elf_Arch {
	public:
	    Elf_Arm(ELF_Parser& p,Elf_Reader& r):Elf_Arch(p,r) {}
	    virtual ~Elf_Arm() {}
	
	    virtual void		read_reloc(binary_stream& handle,Elf_Reloc&) const;
	    virtual bool		build_refer_str(binary_stream& handle,std::string& str,const Elf_Reloc& erl,int flags,unsigned codelen,__filesize_t defval) const;
    };

    class Elf_i386 : public Elf_Arch {
	public:
	    Elf_i386(ELF_Parser& p,Elf_Reader& r):Elf_Arch(p,r) {}
	    virtual ~Elf_i386() {}
	
	    virtual void		read_reloc(binary_stream& handle,Elf_Reloc&) const;
	    virtual bool		build_refer_str(binary_stream& handle,std::string& str,const Elf_Reloc& erl,int flags,unsigned codelen,__filesize_t defval) const;
    };

    class Elf_x86_64 : public Elf_Arch {
	public:
	    Elf_x86_64(ELF_Parser& p,Elf_Reader& r):Elf_Arch(p,r) {}
	    virtual ~Elf_x86_64() {}
	
	    virtual void		read_reloc(binary_stream& handle,Elf_Reloc&) const;
	    virtual bool		build_refer_str(binary_stream& handle,std::string& str,const Elf_Reloc& erl,int flags,unsigned codelen,__filesize_t defval) const;
    };

    class ELF_Parser : public Binary_Parser {
	public:
	    ELF_Parser(binary_stream&,CodeGuider&,udn&);
	    virtual ~ELF_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F7();
	    virtual __filesize_t	action_F9();
	    virtual __filesize_t	action_F10();

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual int			query_endian(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual __filesize_t	get_public_symbol(std::string& str,unsigned& _class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,std::string& name,
							__filesize_t& start,__filesize_t& end,int& _class,int& bitness);
	protected:
	    friend class Elf_Arch;
	    __filesize_t		get_active_shtbl() const { return active_shtbl; }
	    void			set_active_shtbl(__filesize_t t) { active_shtbl=t; }
	    bool			is_sections_present() const { return IsSectionsPresent; }
	    unsigned long		sym_ent_size() const { return __elfSymEntSize; }
	    __filesize_t		findPHEntry(unsigned long type,unsigned& nitems) const;
	    bool			FindPubName(std::string& buff,__filesize_t pa) const;
	private:
	    std::string			elf_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    void			__elfReadSegments(std::map<__filesize_t,VA_map>& to,bool is_virt) const;
	    void			displayELFdyninfo(__filesize_t f_off,unsigned nitems) const;
	    std::set<Elf_Reloc>::const_iterator	__found_ElfRel(__filesize_t offset);
	    void			buildElf386RelChain();
	    void			__elfReadRelaSection(__filesize_t offset,__filesize_t size,__filesize_t sh_link,__filesize_t info,__filesize_t entsize);
	    void			__elfReadRelSection(__filesize_t offset,__filesize_t size,__filesize_t sh_link,__filesize_t info,__filesize_t entsize);
	    __filesize_t		get_f_offset(__filesize_t r_offset,__filesize_t sh_link) const;
	    __filesize_t		displayELFdyntab(__filesize_t dynptr,unsigned long nitem,long entsize) const;
	    __filesize_t		displayELFsymtab() const;
	    __filesize_t		__calcSymEntry(binary_stream&handle,__filesize_t num,bool display_msg) const;
	    std::vector<std::string>	__elfReadDynTab(binary_stream&handle,size_t ntbl,__filesize_t entsize) const;
	    std::vector<std::string>	__elfReadSymTab(binary_stream&handle,size_t nsym) const;
	    bool			ELF_IS_SECTION_PHYSICAL(unsigned sec_num) const;
	    std::string			elf_SymTabShNdx(unsigned idx) const;
	    std::string			elf_SymTabBind(char type) const;
	    std::string			elf_SymTabType(char type) const;
	    std::vector<std::string>	__elfReadSecHdr(binary_stream&handle,size_t nnames) const;
	    std::string			elf_encode_sh_type(long sh_type) const;
	    std::vector<std::string>	__elfReadPrgHdr(binary_stream&handle,size_t nnames) const;
	    std::string			elf_encode_p_type(long p_type) const;
	    std::string			elf_osabi(unsigned char id) const;
	    std::string			elf_version(unsigned long id) const;
	    std::string			elf_machine(unsigned id,unsigned& disasm) const;
	    std::string			elf_otype(unsigned id) const;
	    std::string			elf_data(unsigned char id) const;
	    std::string			elf_class(unsigned char id) const;
	    __filesize_t		findSHEntry(binary_stream&b_cache,unsigned long type,unsigned long& nitems,__filesize_t& link,unsigned long& ent_size) const;
	    __filesize_t		findPHPubSyms(unsigned long& number,unsigned long& ent_size,__filesize_t& act_shtbl) const;
	    __filesize_t		findPHDynEntry(unsigned long type,__filesize_t dynptr,unsigned long nitems) const;
	    void			elf_ReadPubNameList(binary_stream& handle);

	    bool		is_msbf; /* is most significand byte first */
	    bool		is_64bit;
	    Elf_Reader*		elf_reader;
	    Elf_Arch*		elf_arch;

	    __filesize_t	active_shtbl;
	    __filesize_t	elf_min_va;
	    unsigned long	__elfNumSymTab;
	    __filesize_t	__elfSymShTbl;
	    unsigned long	__elfSymEntSize;
	    __filesize_t	__elfSymPtr;
	    bool		IsSectionsPresent;

	    binary_stream*	namecache;
	    binary_stream*	elfcache;

	    std::map<__filesize_t,VA_map>	va_map_virt;
	    std::map<__filesize_t,VA_map>	va_map_phys;
	    std::set<Elf_Reloc>			CurrElfChain;
	    std::set<symbolic_information>	PubNames;

	    binary_stream&	main_handle;
	    CodeGuider&		code_guider;
	    udn&		_udn;
    };
static const char* txt[]={ "ELFhlp", "DynInf", "DynSec", "", "", "", "SymTab", "", "SecHdr", "PrgDef" };
const char* ELF_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t  ELF_Parser::findPHEntry(unsigned long type,unsigned& nitems) const
{
    __filesize_t fpos,dynptr;
    Elf_Phdr phdr;
    size_t i,limit;
    fpos = main_handle.tell();
    dynptr = 0;
    nitems = 0;
    limit = elf_reader->ehdr().e_phnum;
    for(i = 0;i < limit;i++) {
	phdr=elf_reader->read_phdr(main_handle,elf_reader->ehdr().e_phoff + i*elf_reader->ehdr().e_phentsize);
	if(main_handle.eof()) break;
	if(phdr.p_type == type) {
	    dynptr = phdr.p_offset;
	    nitems = phdr.p_filesz/elf_reader->dyn_size();
	    break;
	}
    }
    main_handle.seek(fpos,binary_stream::Seek_Set);
    return dynptr;
}

__filesize_t ELF_Parser::findPHDynEntry(unsigned long type,
					__filesize_t dynptr,
					unsigned long nitems) const
{
  unsigned i;
  __filesize_t fpos;
  bool is_found = false;
  Elf_Dyn dyntab;
  fpos = main_handle.tell();
  main_handle.seek(dynptr,binary_stream::Seek_Set);
  for(i = 0;i < nitems;i++)
  {
    dyntab=elf_reader->read_dyn(main_handle,dynptr);
    if(main_handle.eof()) break;
    dynptr += elf_reader->dyn_size();
    if(dyntab.d_tag == type) { is_found = true; break; }
  }
  main_handle.seek(fpos,binary_stream::Seek_Set);
  return is_found ? dyntab.d_un.d_ptr : 0L;
}

__filesize_t ELF_Parser::findPHPubSyms(unsigned long& number,
					unsigned long& ent_size,
					__filesize_t& act_shtbl) const
{
    __filesize_t fpos, dynptr, dyn_ptr;
    unsigned i, nitems;
    fpos = main_handle.tell();
    ent_size = UINT_MAX;
    /* Here we did an attempt to detect symbol table even if sections header
       are lost */
    dyn_ptr = dynptr = findPHEntry(PT_DYNAMIC,nitems);
    if(dynptr) {
	dynptr = va2pa(findPHDynEntry(DT_SYMTAB,dyn_ptr,nitems));
	if(dynptr) {
	    act_shtbl = va2pa(findPHDynEntry(DT_STRTAB,dyn_ptr,nitems));
	    ent_size = findPHDynEntry(DT_SYMENT,dyn_ptr,nitems);
       /* Only way to determine size of symbol table entries it's find
	   nearest section that follows DT_SYMTAB.
	   FixME: It dangerous technique. May be there exists most safety way ?
	*/
	    __filesize_t _fpos,dptr,max_val,cur_ptr;
	    Elf_Dyn dyntab;
	    _fpos = main_handle.tell();
	    dptr = dyn_ptr;
	    main_handle.seek(dptr,binary_stream::Seek_Set);
	    max_val = main_handle.flength(); /* if section is last */
	    for(i = 0;i < nitems;i++) {
		dyntab=elf_reader->read_dyn(main_handle,dptr);
		if(main_handle.eof()) break;
		dptr += elf_reader->dyn_size();
		cur_ptr = va2pa(dyntab.d_un.d_ptr);
		if(cur_ptr > dynptr && cur_ptr < max_val) max_val = cur_ptr;
	    }
	    main_handle.seek(_fpos,binary_stream::Seek_Set);
	    number = (max_val - dynptr) / ent_size;
	}
    }
    main_handle.seek(fpos, binary_stream::Seek_Set);
    return dynptr;
}

__filesize_t ELF_Parser::findSHEntry(binary_stream& b_cache, unsigned long type,
				unsigned long& nitems,__filesize_t& link,
				unsigned long& ent_size) const
{
    Elf_Shdr shdr;
    __filesize_t fpos, tableptr;
    size_t i, limit;
    fpos = b_cache.tell();
    tableptr = 0L;
    limit = elf_reader->ehdr().e_shnum;
    for(i = 0;i < limit;i++) {
	shdr=elf_reader->read_shdr(b_cache,elf_reader->ehdr().e_shoff + i*elf_reader->ehdr().e_shentsize);
	if(b_cache.eof()) break;
	if(shdr.sh_type == type) {
	    tableptr = shdr.sh_offset;
	    nitems = shdr.sh_size/shdr.sh_entsize;
	    link = shdr.sh_link;
	    ent_size = shdr.sh_entsize;
	    break;
	}
    }
    b_cache.seek(fpos, binary_stream::Seek_Set);
    return tableptr;
}

__filesize_t ELF_Parser::va2pa(__filesize_t va) const
{
    if(!va_map_virt.empty())
    for(std::map<__filesize_t,VA_map>::const_iterator it = va_map_virt.begin();it!=va_map_virt.end();it++) {
	if(va >= (*it).first && va < (*it).first + (*it).second.size) return (*it).second.foff + (va - (*it).first);
    }
    return 0L;
}

__filesize_t ELF_Parser::pa2va(__filesize_t pa) const
{
    __filesize_t ret;
    ret = 0L;
    for(std::map<__filesize_t,VA_map>::const_iterator it = va_map_phys.begin();it!=va_map_phys.end();it++) {
	if(pa >= (*it).first && pa < (*it).first + (*it).second.size) {
	    ret = (*it).second.va + (pa - (*it).first);
	    break;
	}
    }
    return ret;
}

std::string ELF_Parser::elf_class(unsigned char id) const
{
  switch(id)
  {
    case ELFCLASSNONE:	return "Invalid";
    case ELFCLASS32:	return "32-bit";
    case ELFCLASS64:	return "64-bit";
    default:		return "Unknown";
  }
}

std::string ELF_Parser::elf_data(unsigned char id) const
{
  switch(id)
  {
    case ELFDATANONE:	return "Invalid";
    case ELFDATA2LSB:	return "LSB - little endian";
    case ELFDATA2MSB:	return "MSB - big endian";
    default:		return "Unknown";
  }
}

std::string ELF_Parser::elf_otype(unsigned id) const
{
  switch(id)
  {
    case ET_NONE:	return "none";
    case ET_REL:	return "relocatable";
    case ET_EXEC:	return "executable";
    case ET_DYN:	return "shared object";
    case ET_CORE:	return "core";
    case ET_LOOS:	return "OS-specific low";
    case ET_HIOS:	return "OS-specific high";
    case ET_LOPROC:	return "processor-specific low";
    case ET_HIPROC:	return "processor-specific high";
    default:		return "Unknown";
  }
}

/*
    only common machine types are used, add remaining if needed
*/

std::string ELF_Parser::elf_machine(unsigned id,unsigned& disasm) const
{
    disasm=DISASM_DATA;
    switch(id) {
	case EM_NONE:	return "None";
	case EM_M32:	return "AT&T WE32100";
	case EM_SPARC:	disasm = DISASM_CPU_SPARC; return "Sun SPARC";
	case EM_386:	disasm = DISASM_CPU_IX86; return "Intel 386";
	case EM_68K:	disasm = DISASM_CPU_PPC; return "Motorola m68k";
	case EM_88K:	disasm = DISASM_CPU_PPC; return "Motorola m88k";
	case EM_860:	return "Intel 80860";
	case EM_MIPS:	disasm = DISASM_CPU_MIPS; return "MIPS I";
	case EM_S370:	return "IBM System/370";
	case EM_MIPS_RS3_LE:disasm = DISASM_CPU_MIPS; return "MIPS R3000";
	case EM_PARISC:	return "HP PA-RISC";
	case EM_SPARC32PLUS:disasm = DISASM_CPU_SPARC; return "SPARC v8plus";
	case EM_960:	return "Intel 80960";
	case EM_PPC:	disasm = DISASM_CPU_PPC; return "Power PC 32-bit";
	case EM_PPC64:	disasm = DISASM_CPU_PPC; return "Power PC 64-bit";
	case EM_S390:	return "IBM System/390";
	case EM_ADSP:	return "Atmel ADSP";
	case EM_V800:	return "NEC V800";
	case EM_FR20:	return "Fujitsu FR20";
	case EM_RH32:	return "TRW RH-32";
	case EM_RCE:	return "Motorola RCE";
	case EM_ARM:	disasm=DISASM_CPU_ARM; return "ARM";
	case EM_ALPHA:	disasm = DISASM_CPU_ALPHA; return "DEC Alpha";
	case EM_SH:	disasm = DISASM_CPU_SH; return "Hitachi SH";
	case EM_SPARCV9:disasm = DISASM_CPU_SPARC; return "SPARC v9 64-bit";
	case EM_TRICORE:return "Siemens TriCore embedded processor";
	case EM_ARC:	return "Argonaut RISC Core";
	case EM_H8_300:	return "Hitachi H8/300";
	case EM_H8_300H:return "Hitachi H8/300H";
	case EM_H8S:	return "Hitachi H8S";
	case EM_H8_500:	return "Hitachi H8/500";
	case EM_IA_64:	disasm = DISASM_CPU_IA64; return "Intel IA-64";
	case EM_MIPS_X:	disasm = DISASM_CPU_MIPS; return "Stanford MIPS-X";
	case EM_COLDFIRE:return "Motorola ColdFire";
	case EM_68HC12:	return "Motorola M68HC12";
	case EM_MMA:	return "Fujitsu MMA Multimedia Accelerator";
	case EM_PCP:	return "Siemens PCP";
	case EM_NCPU:	return "Sony nCPU embedded RISC processor";
	case EM_NDR1:	return "Denso NDR1 microprocessor";
	case EM_STARCORE:return "Motorola StarCore processor";
	case EM_ME16:	return "Toyota ME16 processor";
	case EM_ST100:	return "STMicroelectronics ST100 processor";
	case EM_TINYJ:	return "Advanced Logic Corp. TinyJ";
	case EM_X86_64:	disasm = DISASM_CPU_IX86; return "AMD x86-64";
	case EM_PDSP:	return "Sony DSP Processor";
	case EM_PDP10:	return "DEC PDP-10";
	case EM_PDP11:	return "DEC PDP-11";
	case EM_FX66:	return "Siemens FX66 microcontroller";
	case EM_ST9PLUS:return "STMicroelectronics ST9+ 8/16 bit microcontroller";
	case EM_ST7:	return "STMicroelectronics ST7 8-bit microcontroller";
	case EM_68HC16:	return "Motorola MC68HC16 Microcontroller";
	case EM_68HC11:	return "Motorola MC68HC11 Microcontroller";
	case EM_68HC08:	return "Motorola MC68HC08 Microcontroller";
	case EM_68HC05:	return "Motorola MC68HC05 Microcontroller";
	case EM_SVX:	return "Silicon Graphics SVx";
	case EM_ST19:	return "STMicroelectronics ST19 8-bit microcontroller";
	case EM_VAX:	return "DEC VAX";
	case EM_CRIS:	return "Axis Comm. 32-bit embedded processor";
	case EM_JAVELIN:return "Infineon Tech. 32-bit embedded processor";
	case EM_FIREPATH:return "Element 14 64-bit DSP Processor";
	case EM_ZSP:	return "LSI Logic 16-bit DSP Processor";
	case EM_MMIX:	return "Donald Knuth's educational 64-bit processor";
	case EM_HUANY:	return "Harvard University machine-independent object files";
	case EM_PRISM:	return "SiTera Prism";
	case EM_AVR:	disasm=DISASM_CPU_AVR; return "Atmel AVR 8-bit";
	case EM_FR30:	return "Fujitsu FR30";
	case EM_D10V:	return "Mitsubishi D10V";
	case EM_D30V:	return "Mitsubishi D30V";
	case EM_V850:	return "NEC v850";
	case EM_M32R:	return "Mitsubishi M32R";
	case EM_MN10300:return "Matsushita MN10300";
	case EM_MN10200:return "Matsushita MN10200";
	case EM_PJ:	return "picoJava";
	case EM_OPENRISC:return "OpenRISC 32-bit embedded processor";
	case EM_ARC_A5:	return "ARC Cores Tangent-A5";
	case EM_XTENSA:	return "Tensilica Xtensa Architecture";
	case EM_VIDEOCORE:return "Alphamosaic VideoCore processor";
	case EM_TMM_GPP:return "Thompson Multimedia General Purpose Processor";
	case EM_NS32K:	return "National Semiconductor 32000 series";
	case EM_TPC:	return "Tenor Network TPC processor";
	case EM_SNP1K:	return "Trebia SNP 1000 processor";
	case EM_IP2K:	return "Ubicom IP2022 micro controller";
	case EM_CR:	return "National Semiconductor CompactRISC";
	case EM_MSP430:	return "TI msp430 micro controller";
	case EM_BLACKFIN:return "ADI Blackfin";
	case EM_ALTERA_NIOS2: return "Altera Nios II soft-core processor";
	case EM_CRX:	return "National Semiconductor CRX";
	case EM_XGATE:	return "Motorola XGATE embedded processor";
	case EM_C166:	return "Infineon C16x/XC16x processor";
	case EM_M16C:	return "Renesas M16C series microprocessors";
	case EM_DSPIC30F:return "Microchip Technology dsPIC30F Digital Signal Controller";
	case EM_CE:	return "Freescale Communication Engine RISC core";
	case EM_M32C:	return "Renesas M32C series microprocessors";
	case EM_TSK3000:return "Altium TSK3000 core";
	case EM_RS08:	return "Freescale RS08 embedded processor";
	case EM_ECOG2:	return "Cyan Technology eCOG2 microprocessor";
	case EM_SCORE:	return "Sunplus Score";
	case EM_DSP24:	return "New Japan Radio (NJR) 24-bit DSP Processor";
	case EM_VIDEOCORE3:return "Broadcom VideoCore III processor";
	case EM_LATTICEMICO32: return "RISC processor for Lattice FPGA architecture";
	case EM_SE_C17:	return "Seiko Epson C17 family";
	case EM_MMDSP_PLUS:return "STMicroelectronics 64bit VLIW Data Signal Processor";
	case EM_CYPRESS_M8C:return "Cypress M8C microprocessor";
	case EM_R32C:	return "Renesas R32C series microprocessors";
	case EM_TRIMEDIA:return "NXP Semiconductors TriMedia architecture family";
	case EM_QDSP6:	return "QUALCOMM DSP6 Processor";
	case EM_8051:	return "Intel 8051 and variants";
	case EM_STXP7X:	return "STMicroelectronics STxP7x family";
	case EM_NDS32:	return "Andes Technology compact code size embedded RISC processor family";
	case EM_ECOG1X:	return "Cyan Technology eCOG1X family";
	case EM_MAXQ30:	return "Dallas Semiconductor MAXQ30 Core Micro-controllers";
	case EM_XIMO16:	return "New Japan Radio (NJR) 16-bit DSP Processor";
	case EM_MANIK:	return "M2000 Reconfigurable RISC Microprocessor";
	case EM_CRAYNV2:disasm = DISASM_CPU_CRAY; return "Cray Inc. NV2 vector architecture";
	case EM_RX:	return "Renesas RX family";
	case EM_METAG:	return "Imagination Technologies META processor architecture";
	case EM_MCST_ELBRUS:return "MCST Elbrus general purpose hardware architecture";
	case EM_ECOG16:	return "Cyan Technology eCOG16 family";
	case EM_CR16:	return "National Semiconductor CompactRISC 16-bit processor";
	default:	return "Unknown";
    }
}

std::string ELF_Parser::elf_version(unsigned long id) const
{
  switch(id)
  {
    case EV_NONE:    return "Invalid";
    case EV_CURRENT: return "Current";
    default:         return "Unknown";
  }
}

std::string ELF_Parser::elf_osabi(unsigned char id) const
{
  switch(id)
  {
    case ELFOSABI_SYSV:		return "UNIX System V";
    case ELFOSABI_HPUX:		return "HP-UX";
    case ELFOSABI_NETBSD:	return "NetBSD";
    case ELFOSABI_LINUX:	return "GNU/Linux";
    case ELFOSABI_HURD:		return "GNU/Hurd";
    case ELFOSABI_86OPEN:	return "86Open";
    case ELFOSABI_SOLARIS:	return "Solaris";
    case ELFOSABI_MONTEREY:	return "Monterey";
    case ELFOSABI_IRIX:		return "IRIX";
    case ELFOSABI_FREEBSD:	return "FreeBSD";
    case ELFOSABI_TRU64:	return "TRU64 UNIX";
    case ELFOSABI_MODESTO:	return "Novell Modesto";
    case ELFOSABI_OPENBSD:	return "OpenBSD";
    case ELFOSABI_ARM:		return "ARM";
    case ELFOSABI_STANDALONE:	return "Standalone (embedded) application";
    default:			return "Unknown";
  }
}

__filesize_t ELF_Parser::show_header() const
{
  __filesize_t fpos;
  TWindow *w;
  char hdr[81];
  unsigned keycode,dummy;
  __filesize_t entrya;
  fpos = beye_context().tell();
  entrya = va2pa(elf_reader->ehdr().e_entry);
  sprintf(hdr," ELF (Executable and Linking Format) ");
  w = CrtDlgWndnls(hdr,74,18);
  w->goto_xy(1,1);
  w->printf(
	   "Signature                         = %02X %02X %02X %02XH (%c%c%c%c)\n"
	   "File class                        = %02XH (%s)\n"
	   "Data encoding                     = %02XH (%s)\n"
	   "ELF header version                = %02XH (%s)\n"
	   "Operating system / ABI            = %02XH (%s)\n"
	   "ABI version                       = %02XH (%d)\n"
	   "Object file type                  = %04XH (%s)\n"
	   "Machine architecture              = %04XH (%s)\n"
	   "Object file version               = %08lXH (%s)\n"
	    ,elf_reader->ehdr().e_ident[EI_MAG0],	elf_reader->ehdr().e_ident[EI_MAG1]
	    ,elf_reader->ehdr().e_ident[EI_MAG2],	elf_reader->ehdr().e_ident[EI_MAG3]
	    ,elf_reader->ehdr().e_ident[EI_MAG0],	elf_reader->ehdr().e_ident[EI_MAG1]
	    ,elf_reader->ehdr().e_ident[EI_MAG2],	elf_reader->ehdr().e_ident[EI_MAG3]
	    ,elf_reader->ehdr().e_ident[EI_CLASS],	elf_class(elf_reader->ehdr().e_ident[EI_CLASS]).c_str()
	    ,elf_reader->ehdr().e_ident[EI_DATA],	elf_data(elf_reader->ehdr().e_ident[EI_DATA]).c_str()
	    ,elf_reader->ehdr().e_ident[EI_VERSION],	elf_version(elf_reader->ehdr().e_ident[EI_VERSION]).c_str()
	    ,elf_reader->ehdr().e_ident[EI_OSABI],	elf_osabi(elf_reader->ehdr().e_ident[EI_OSABI]).c_str()
	    ,elf_reader->ehdr().e_ident[EI_ABIVERSION],elf_reader->ehdr().e_ident[EI_ABIVERSION]
	    ,elf_reader->ehdr().e_type,	elf_otype(elf_reader->ehdr().e_type).c_str()
	    ,elf_reader->ehdr().e_machine,	elf_machine(elf_reader->ehdr().e_machine,dummy).c_str()
	    ,elf_reader->ehdr().e_version,	elf_version(elf_reader->ehdr().e_version).c_str()
	    );
  w->set_color(dialog_cset.entry);
  if(is_64bit)
    w->printf("Entry point VA             = %016llXH (offset: %016llXH)"
	   ,(unsigned long long)elf_reader->ehdr().e_entry,(unsigned long long)entrya);
  else
    w->printf("Entry point VA                    = %08lXH (offset: %08lXH)"
	   ,(unsigned)elf_reader->ehdr().e_entry,(unsigned)entrya);
  w->clreol(); w->printf("\n");
  w->set_color(dialog_cset.main);
  if(is_64bit)
    w->printf("Program header table offset       = %016llXH\n"
	   "Section header table offset       = %016llXH\n"
	   ,elf_reader->ehdr().e_phoff
	   ,elf_reader->ehdr().e_shoff);
  else
    w->printf("Program header table offset       = %08lXH\n"
	   "Section header table offset       = %08lXH\n"
	   ,elf_reader->ehdr().e_phoff
	   ,elf_reader->ehdr().e_shoff);
  w->printf(
	   "Processor specific flag           = %08lXH\n"
	   "ELF header size (bytes)           = %04XH\n"
	   "Program header table entry size   = %04XH\n"
	   "Program header table entry count  = %04XH\n"
	   "Section header table entry size   = %04XH\n"
	   "Section header table entry count  = %04XH\n"
	   "Section header string table index = %04XH"
	   ,elf_reader->ehdr().e_flags
	   ,elf_reader->ehdr().e_ehsize
	   ,elf_reader->ehdr().e_phentsize
	   ,elf_reader->ehdr().e_phnum
	   ,elf_reader->ehdr().e_shentsize
	   ,elf_reader->ehdr().e_shnum
	   ,elf_reader->ehdr().e_shstrndx);
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = entrya; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  delete w;
  return fpos;
}

std::string ELF_Parser::elf_encode_p_type(long p_type) const
{
   switch(p_type)
   {
      case PT_NULL: return "Unusable";
      case PT_LOAD: return "Loadable";
      case PT_DYNAMIC: return "Dynalinking";
      case PT_INTERP: return "Interpreter";
      case PT_NOTE:  return "Auxiliary";
      case PT_SHLIB: return "Unspecified";
      case PT_PHDR:  return "header itself";
      case PT_NUM: return "Number of types";
      case PT_LOPROC: return "Low processor";
      case PT_HIPROC: return "High processor";
      default:  return "Unknown";
   }
}

std::vector<std::string> ELF_Parser::__elfReadPrgHdr(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    size_t i;
    handle.seek(elf_reader->ehdr().e_phoff,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	__filesize_t fp;
	char stmp[80];
	Elf_Phdr phdr;
	if(IsKbdTerminate() || handle.eof()) break;
	fp = handle.tell();
	phdr=elf_reader->read_phdr(handle,fp);
	handle.seek(fp+elf_reader->ehdr().e_phentsize,binary_stream::Seek_Set);
	sprintf(stmp,"%-15s %08lX %08lX %08lX %08lX %08lX %c%c%c %08lX",
		elf_encode_p_type(phdr.p_type).c_str(),
		(unsigned long)phdr.p_offset,
		(unsigned long)phdr.p_vaddr,
		(unsigned long)phdr.p_paddr,
		(unsigned long)phdr.p_filesz,
		(unsigned long)phdr.p_memsz,
		(phdr.p_flags & PF_X) == PF_X ? 'X' : ' ',
		(phdr.p_flags & PF_W) == PF_W ? 'W' : ' ',
		(phdr.p_flags & PF_R) == PF_R ? 'R' : ' ',
		(unsigned long)phdr.p_align
	);
	rc.push_back(stmp);
    }
    return rc;
}

std::string ELF_Parser::elf_encode_sh_type(long sh_type) const
{
   switch(sh_type)
   {
      case SHT_NULL: return "NULL";
      case SHT_PROGBITS: return "PRGBTS";
      case SHT_SYMTAB: return "SYMTAB";
      case SHT_STRTAB: return "STRTAB";
      case SHT_RELA:  return "RELA";
      case SHT_HASH: return "HSHTAB";
      case SHT_DYNAMIC:  return "DYNLNK";
      case SHT_NOTE: return "NOTES";
      case SHT_NOBITS: return "NOBITS";
      case SHT_REL:  return "REL";
      case SHT_SHLIB: return "UNSPEC";
      case SHT_DYNSYM: return "DYNSYM";
      case SHT_NUM: return "NTYPES";
      case SHT_GNU_verdef: return "VERDEF";
      case SHT_GNU_verneed: return "VERNED";
      case SHT_GNU_versym: return "SYMVER";
      case SHT_LOPROC: return "LOPROC";
      case SHT_HIPROC: return "HIPROC";
      case SHT_LOUSER: return "LOUSER";
      case SHT_HIUSER: return "HIUSER";
      default:  return "UNK";
   }
}

std::vector<std::string> ELF_Parser::__elfReadSecHdr(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    size_t i;
    handle.seek(elf_reader->ehdr().e_shoff,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	 Elf_Shdr shdr;
	 std::string tmp;
	__filesize_t fp;
	char stmp[80];
	if(IsKbdTerminate() || handle.eof()) break;
	fp = handle.tell();
	shdr=elf_reader->read_shdr(handle,fp);
	tmp=elf_arch->read_nametable(*namecache,shdr.sh_name);
	handle.seek(fp+elf_reader->ehdr().e_shentsize,binary_stream::Seek_Set);
	sprintf(stmp,"%-16s %-6s %c%c%c %08lX %08lX %08lX %04hX %04hX %04hX %04hX",
		tmp.c_str(),
		elf_encode_sh_type(shdr.sh_type).c_str(),
		(shdr.sh_flags & SHF_WRITE) == SHF_WRITE ? 'W' : ' ',
		(shdr.sh_flags & SHF_ALLOC) == SHF_ALLOC ? 'A' : ' ',
		(shdr.sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR ? 'X' : ' ',
		(unsigned long)shdr.sh_addr,
		(unsigned long)shdr.sh_offset,
		(unsigned long)shdr.sh_size,
		(uint16_t)shdr.sh_link,
		(uint16_t)shdr.sh_info,
		(uint16_t)shdr.sh_addralign,
		(uint16_t)shdr.sh_entsize
	);
	rc.push_back(stmp);
    }
    return rc;
}

std::string ELF_Parser::elf_SymTabType(char type) const
{
  switch(ELF_ST_TYPE(type))
  {
    case STT_NOTYPE:  return "NoType";
    case STT_OBJECT:  return "Object";
    case STT_FUNC:    return "Func. ";
    case STT_SECTION: return "Sect. ";
    case STT_FILE:    return "File  ";
    case STT_NUM:     return "Number";
    case STT_LOPROC:  return "LoProc";
    case STT_HIPROC:  return "HiProc";
    default: return "Unknwn";
  }
}

std::string ELF_Parser::elf_SymTabBind(char type) const
{
  switch(ELF_ST_BIND(type))
  {
    case STB_LOCAL:  return "Local ";
    case STB_GLOBAL: return "Global";
    case STB_WEAK:   return "Weak  ";
    case STB_NUM:    return "Number";
    case STB_LOPROC: return "LoProc";
    case STB_HIPROC: return "HiProc";
    default: return "Unknwn";
  }
}

std::string ELF_Parser::elf_SymTabShNdx(unsigned idx) const
{
  char ret[10];
  switch(idx)
  {
    case SHN_UNDEF:  return "Undef ";
    case SHN_LOPROC: return "LoProc";
    case SHN_HIPROC: return "HiProc";
    case SHN_ABS:    return "Abs.  ";
    case SHN_COMMON: return "Common";
    case SHN_HIRESERVE: return "HiRes.";
    default: sprintf(ret,"%04XH ",idx); return ret;
  }
}

bool ELF_Parser::ELF_IS_SECTION_PHYSICAL(unsigned sec_num) const
{
  return !(sec_num == SHN_UNDEF || sec_num == SHN_LOPROC ||
	   sec_num == SHN_HIPROC || sec_num == SHN_ABS ||
	   sec_num == SHN_COMMON || sec_num == SHN_HIRESERVE);
}

std::vector<std::string> ELF_Parser::__elfReadSymTab(binary_stream& handle,size_t nsym) const
{
    size_t i;
    std::vector<std::string> rc;
    std::string text;
    handle.seek(__elfSymPtr,binary_stream::Seek_Set);
    for(i = 0;i < nsym;i++) {
	__filesize_t fp;
	char stmp[80];
	Elf_Sym sym;
	if(IsKbdTerminate() || handle.eof()) break;
	fp = handle.tell();
	sym=elf_reader->read_sym(handle,fp);
	handle.seek(fp+__elfSymEntSize,binary_stream::Seek_Set);
	text=elf_arch->read_nametableex(*namecache,sym.st_name,active_shtbl); // !!! HACK
	if(is_64bit)
	    sprintf(stmp,"%-29s %016llX %08lX %04hX %s %s %s"
	       ,text.c_str()
	       ,(unsigned long long)sym.st_value
	       ,(unsigned)sym.st_size
	       ,sym.st_other
	       ,elf_SymTabType(sym.st_info).c_str()
	       ,elf_SymTabBind(sym.st_info).c_str()
	       ,elf_SymTabShNdx(sym.st_shndx).c_str()
	    );
	else
	    sprintf(stmp,"%-37s %08lX %08lX %04hX %s %s %s"
	       ,text.c_str()
	       ,(unsigned)sym.st_value
	       ,(unsigned)sym.st_size
	       ,sym.st_other
	       ,elf_SymTabType(sym.st_info).c_str()
	       ,elf_SymTabBind(sym.st_info).c_str()
	       ,elf_SymTabShNdx(sym.st_shndx).c_str()
	    );
	rc.push_back(stmp);
    }
    return rc;
}

std::vector<std::string> ELF_Parser::__elfReadDynTab(binary_stream& handle,size_t ntbl,__filesize_t entsize) const
{
    size_t i;
    std::vector<std::string> rc;
    std::string sout;
    unsigned len,rborder;
    for(i = 0;i < ntbl;i++) {
	__filesize_t fp;
	std::string stmp;
	Elf_Dyn pdyn;
	fp = handle.tell();
	pdyn=elf_reader->read_dyn(handle,fp);
	handle.seek(fp+entsize,binary_stream::Seek_Set);
	fp = handle.tell();
	/* Note: elf-64 specs requre ELF_XWORD here! But works ELF_WORD !!! */
	sout=elf_arch->read_nametableex(*namecache,pdyn.d_tag,active_shtbl);
	len = sout.length();
	rborder=is_64bit?52:60;
	if(len > rborder-4) len = rborder-7;
	if(IsKbdTerminate() || handle.eof()) break;
	stmp=sout;
	if(len > rborder-4) stmp+="...";
//   if(rlen < rborder) { memset(&stmp[rlen],' ',rborder-rlen); stmp[rborder] = 0; }
	char sbuf[256];
	if(is_64bit) sprintf(sbuf," vma=%016llXH",(unsigned long long)pdyn.d_un.d_val);
	else sprintf(sbuf," vma=%08lXH",(unsigned long)pdyn.d_un.d_val);
	stmp+=sbuf;
	rc.push_back(stmp);
	handle.seek(fp,binary_stream::Seek_Set);
    }
    return rc;
}

__filesize_t ELF_Parser::action_F10()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " type            fileoffs virtaddr physaddr filesize memsize  flg align   ";
    ssize_t nnames = elf_reader->ehdr().e_phnum;
    int flags = LB_SELECTIVE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __elfReadPrgHdr(main_handle,nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
exit:
    if(ret != -1) {
	Elf_Phdr it;
	it=elf_reader->read_phdr(main_handle,elf_reader->ehdr().e_phoff+elf_reader->phdr_size()*ret);
	main_handle.read(&it,sizeof(it));
	fpos = it.p_offset;
    }
    return fpos;
}

__filesize_t ELF_Parser::action_F9()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " name             type   flg virtaddr fileoffs   size   link info algn esiz";
    ssize_t nnames = IsSectionsPresent ? elf_reader->ehdr().e_shnum : 0;
    int flags = LB_SELECTIVE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __elfReadSecHdr(main_handle,nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
exit:
    if(ret != -1) {
	Elf_Shdr it;
	it=elf_reader->read_shdr(main_handle,elf_reader->ehdr().e_shoff+elf_reader->ehdr().e_shentsize*ret);
	fpos = it.sh_offset;
    }
    return fpos;
}

__filesize_t ELF_Parser::__calcSymEntry(binary_stream& handle,__filesize_t num,bool display_msg) const
{
   Elf_Sym it;
   Elf_Shdr sec;
   __filesize_t ffpos,fpos = 0L;
   ffpos = handle.tell();
   it=elf_reader->read_sym(handle,__elfSymPtr+__elfSymEntSize*num);
   // HACK
   sec=elf_reader->read_shdr(handle,elf_reader->ehdr().e_shoff+elf_reader->ehdr().e_shentsize*it.st_shndx);
   handle.seek(ffpos,binary_stream::Seek_Set);
   if(ELF_IS_SECTION_PHYSICAL(it.st_shndx))
/*
   In relocatable files, st_value holds alignment constraints for a
   symbol whose section index is SHN_COMMON.

   In relocatable files, st_value holds a section offset for a defined
   symbol. That is, st_value is an offset from the beginning of the
   section that st_shndx identifies.

   In executable and shared object files, st_value holds a virtual
   address. To make these files' symbols more useful for the dynamic
   linker, the section offset (file interpretation) gives way to a
   virtual address (memory interpretation) for which the section number
   is irrelevant.
*/
     fpos = elf_reader->ehdr().e_type == ET_REL ?
	    sec.sh_offset + it.st_value:
	    va2pa(it.st_value);
   else
     if(display_msg) beye_context().ErrMessageBox(NO_ENTRY,BAD_ENTRY);
   return fpos;
}

__filesize_t ELF_Parser::displayELFsymtab() const
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = " Name                                  Value    Size     Oth. Type   Bind   Sec# ";
    ssize_t nnames = __elfNumSymTab;
    int flags = LB_SELECTIVE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = __elfReadSymTab(main_handle,nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
exit:
    if(ret != -1) {
	__filesize_t ea;
	ea = __calcSymEntry(main_handle,ret,true);
	fpos = ea ? ea : fpos;
    }
    return fpos;
}

__filesize_t ELF_Parser::displayELFdyntab(__filesize_t dynptr,
						unsigned long nitem,
						long entsize) const
{
    __filesize_t fpos;
    binary_stream& handle=*elfcache;
    unsigned ndyn;
    fpos = beye_context().tell();
    ndyn = (unsigned)nitem;
    handle.seek(dynptr,binary_stream::Seek_Set);
    std::vector<std::string> objs = __elfReadDynTab(handle,ndyn,entsize);
    if(!objs.empty()) {
	int ret;
	ret = ListBox(objs," Dynamic section ",LB_SELECTIVE | LB_SORTABLE,-1);
	if(ret != -1) {
	    const char *addr;
	    addr = strstr(objs[ret].c_str(),"vma=");
	    if(addr) {
		__filesize_t addr_probe;
		addr_probe = is_64bit?strtoull(&addr[4],NULL,16):strtoul(&addr[4],NULL,16);
		if(addr_probe && addr_probe >= elf_min_va) {
		    addr_probe = va2pa(addr_probe);
		    if(addr_probe && addr_probe < main_handle.flength()) fpos = addr_probe;
		    else goto not_entry;
		} else goto not_entry;
	    } else {
not_entry:
		beye_context().ErrMessageBox(NOT_ENTRY,"");
	    }
	}
    }
    return fpos;
}

__filesize_t ELF_Parser::action_F7()
{
    __filesize_t fpos;
    fpos = beye_context().tell();
    if(!__elfSymPtr) { beye_context().NotifyBox(NOT_ENTRY," ELF symbol table "); return fpos; }
    active_shtbl = __elfSymShTbl;
    return displayELFsymtab();
}

__filesize_t ELF_Parser::action_F3()
{
    __filesize_t fpos,dynptr;
    unsigned long number;
    unsigned long nitems,ent_size = UINT_MAX;
    fpos = beye_context().tell();
    dynptr = findSHEntry(main_handle, SHT_DYNSYM, number, active_shtbl, ent_size);
    if(!dynptr) {
	dynptr = findPHPubSyms(nitems, ent_size, active_shtbl);
	number = nitems;
    }
    if(!dynptr) { beye_context().NotifyBox(NOT_ENTRY," ELF dynamic symbol table "); return fpos; }
    return displayELFdyntab(dynptr,number,ent_size);
}

/***************************************************************************/
/************************ RELOCATION FOR ELF *******************************/
/***************************************************************************/
__filesize_t ELF_Parser::get_f_offset(__filesize_t r_offset,__filesize_t sh_link) const
{
  /*
    r_offset member gives the location at which to apply the relocation
    action. For a relocatable file, the value is the byte offset from
    the beginning of the section to the storage unit affected by the
    relocation. For an executable file or a shared object, the value is
    the virtual address of the storage unit affected by the relocation.
  */
  __filesize_t f_offset;
  binary_stream& handle = *elfcache;
  switch(elf_reader->ehdr().e_type)
  {
     case ET_REL:
		{
		  Elf_Shdr shdr;
		  __filesize_t fp;
		  fp = handle.tell();
		  shdr=elf_reader->read_shdr(handle,elf_reader->ehdr().e_shoff+sh_link*elf_reader->ehdr().e_shentsize);
		  handle.seek(fp,binary_stream::Seek_Set);
		  f_offset = shdr.sh_offset + r_offset;
		}
     default: f_offset = va2pa(r_offset);
	      break;
  }
  return f_offset;
}

void ELF_Parser::__elfReadRelSection(__filesize_t offset,
					__filesize_t size,
					__filesize_t sh_link,
					__filesize_t info,
					__filesize_t entsize)
{
  binary_stream& handle = *elfcache,&handle2 = *namecache;
  size_t i,nitems;
  Elf_Rel relent;
  __filesize_t fp, sfp, lfp;
  if(!entsize) return;
  fp = handle.tell();
  handle.seek(offset,binary_stream::Seek_Set);
  nitems = (size_t)(size / entsize);
  sfp = handle2.tell();
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    lfp=handle.tell();
    relent=elf_reader->read_rel(handle,lfp);
    handle.seek(lfp+elf_reader->rel_size(),binary_stream::Seek_Set);
    if(entsize > elf_reader->rel_size()) handle.seek(entsize-elf_reader->rel_size(),binary_stream::Seek_Cur);
    erc.offset = get_f_offset(relent.r_offset,info);
    erc.info = relent.r_info;
    /* Entries of type Elf32_Rel store an implicit addend in the
       location to be modified */
    handle2.seek(erc.offset, binary_stream::Seek_Set);
    elf_arch->read_reloc(handle2,erc);
    erc.sh_idx = sh_link;
    CurrElfChain.insert(erc);
//    if(!la_AddData(CurrElfChain,&erc,NULL)) break;
  }
  handle2.seek(sfp,binary_stream::Seek_Set);
  handle.seek(fp,binary_stream::Seek_Set);
}

void ELF_Parser::__elfReadRelaSection(__filesize_t offset,
					__filesize_t size,
					__filesize_t sh_link,
					__filesize_t info,
					__filesize_t entsize)
{
  binary_stream& handle = *elfcache;
  size_t i,nitems;
  Elf_Rela relent;
  __filesize_t fp, lfp;
  if(!entsize) return;
  fp = handle.tell();
  handle.seek(offset,binary_stream::Seek_Set);
  nitems = (size_t)(size / entsize);
  for(i = 0;i < nitems;i++)
  {
    Elf_Reloc erc;
    lfp=handle.tell();
    relent=elf_reader->read_rela(handle,lfp);
    handle.seek(lfp+elf_reader->rela_size(), binary_stream::Seek_Set);
    if(entsize > elf_reader->rela_size()) handle.seek(entsize-elf_reader->rela_size(),binary_stream::Seek_Cur);
    erc.offset = get_f_offset(relent.r_offset,info);
    erc.info = relent.r_info;
    erc.addend = relent.r_addend;
    erc.sh_idx = sh_link;
    CurrElfChain.insert(erc);
//    if(!la_AddData(CurrElfChain,&erc,NULL)) break;
  }
  handle.seek(fp,binary_stream::Seek_Set);
}

void ELF_Parser::buildElf386RelChain()
{
  size_t i,_nitems;
  TWindow *w;
  binary_stream& handle = *elfcache;
  __filesize_t fp;
//  if(!(CurrElfChain = la_Build(0,sizeof(Elf_Reloc),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  w->goto_xy(1,1);
  w->puts(BUILD_REFS);
  fp = handle.tell();
  if(IsSectionsPresent) /* Section headers are present */
  {
    handle.seek(elf_reader->ehdr().e_shoff,binary_stream::Seek_Set);
    _nitems = elf_reader->ehdr().e_shnum;
    for(i = 0;i < _nitems;i++)
    {
      Elf_Shdr shdr;
      __filesize_t _fp;
      if(IsKbdTerminate() || handle.eof()) break;
      _fp=handle.tell();
      shdr=elf_reader->read_shdr(handle,_fp);
      handle.seek(_fp+elf_reader->ehdr().e_shentsize,binary_stream::Seek_Set);
      switch(shdr.sh_type)
      {
	case SHT_REL: __elfReadRelSection(shdr.sh_offset,
					  shdr.sh_size,
					  shdr.sh_link,
					  shdr.sh_info,
					  shdr.sh_entsize);
		      break;
	case SHT_RELA: __elfReadRelaSection(shdr.sh_offset,
					    shdr.sh_size,
					    shdr.sh_link,
					    shdr.sh_info,
					    shdr.sh_entsize);
		      break;
	default: break;
     }
    }
  }
  else if(elf_reader->ehdr().e_type != ET_REL)
  {
    /* If section headers are lost then information can be taken
       from program headers
    */
    __filesize_t dyn_ptr,dynptr,link,type;
    unsigned tsize,nitems;
    dynptr = findPHEntry(PT_DYNAMIC,nitems);
    link = va2pa(findPHDynEntry(DT_SYMTAB,dynptr,nitems));
    if(dynptr)
    {
      dyn_ptr = va2pa(findPHDynEntry(DT_RELA,dynptr,nitems));
      if(dyn_ptr)
      {
	tsize = findPHDynEntry(DT_RELASZ,dynptr,nitems);
	__elfReadRelaSection(dyn_ptr,
			     tsize,
			     link,
			     0,/* only executable can lose sections */
			     sizeof(Elf386_External_Rela));
      }
      dyn_ptr = va2pa(findPHDynEntry(DT_REL,dynptr,nitems));
      if(dyn_ptr)
      {
	tsize = findPHDynEntry(DT_RELSZ,dynptr,nitems);
	__elfReadRelSection(dyn_ptr,
			    tsize,
			    link,
			    0,/* only executable can lose sections */
			    sizeof(Elf386_External_Rel));
      }
      dyn_ptr = va2pa(findPHDynEntry(DT_JMPREL,dynptr,nitems));
      if(dyn_ptr)
      {
	tsize = findPHDynEntry(DT_PLTRELSZ,dynptr,nitems);
	type = findPHDynEntry(DT_PLTREL,dynptr,nitems);
	if(type == DT_REL)
	__elfReadRelSection(dyn_ptr,
			    tsize,
			    link,
			    0,/* only executable can lose sections */
			    sizeof(Elf386_External_Rel));
	else
	__elfReadRelaSection(dyn_ptr,
			     tsize,
			     link,
			     0,/* only executable can lose sections */
			     sizeof(Elf386_External_Rel));
      }
    }
  }
//  la_Sort(CurrElfChain,compare_elf_reloc);
  handle.seek(fp,binary_stream::Seek_Set);
  delete w;
  return;
}

std::set<Elf_Reloc>::const_iterator ELF_Parser::__found_ElfRel(__filesize_t offset)
{
  Elf_Reloc key;
  if(CurrElfChain.empty()) buildElf386RelChain();
  key.offset = offset;
  return CurrElfChain.find(key);
}

static const char* S_INTERPRETER="Interpreter : ";

void ELF_Parser::displayELFdyninfo(__filesize_t f_off,unsigned nitems) const
{
    Elf_Dyn dyntab;
    __filesize_t curroff,stroff;
    unsigned i;
    bool is_add;
    char stmp[80];
    stroff = 0;
    stroff = va2pa(findPHDynEntry(DT_STRTAB,f_off,nitems));
    if(!stroff) { beye_context().NotifyBox(" String information not found!",NULL); return; }
    main_handle.seek(f_off,binary_stream::Seek_Set);
    strcpy(stmp,S_INTERPRETER);
    curroff = findPHEntry(PT_INTERP, i);
    if(curroff) {
	main_handle.seek(curroff,binary_stream::Seek_Set);
	main_handle.read(&stmp[strlen(S_INTERPRETER) - 1],sizeof(stmp)-strlen(S_INTERPRETER)-1);
    }
    std::vector<std::string> objs;
    objs.push_back(stmp);
    main_handle.seek(f_off,binary_stream::Seek_Set);
    for(i = 0;i < nitems;i++) {
	dyntab=elf_reader->read_dyn(main_handle,f_off);
	if(main_handle.eof()) break;
	f_off += elf_reader->dyn_size();
	is_add = true;
	switch(dyntab.d_tag) {
	    case DT_NULL: goto dyn_end;
	    case DT_NEEDED:
		    {
		      strcpy(stmp,"Needed : ");
			main_handle.seek(dyntab.d_un.d_ptr + stroff,binary_stream::Seek_Set);
			main_handle.read(&stmp[strlen(stmp)],70);
		    }
		    break;
	    case DT_SONAME:
		    {
		      strcpy(stmp,"SO name: ");
			main_handle.seek(dyntab.d_un.d_ptr + stroff,binary_stream::Seek_Set);
			main_handle.read(&stmp[strlen(stmp)],70);
		    }
		    break;
	    case DT_RPATH:
		    {
		      strcpy(stmp,"LibPath: ");
			main_handle.seek(dyntab.d_un.d_ptr + stroff,binary_stream::Seek_Set);
			main_handle.read(&stmp[strlen(stmp)],70);
		    }
		    break;
	    default:     is_add = false; break;
	}
	if(is_add) objs.push_back(stmp);
    }
dyn_end:
    ListBox(objs," Dynamic linking information ",LB_SORTABLE,-1);
}

__filesize_t ELF_Parser::action_F2()
{
    __filesize_t dynptr,fpos;
    unsigned number;
    fpos = beye_context().tell();
    dynptr = findPHEntry(PT_DYNAMIC,number);
    if(!dynptr) { beye_context().NotifyBox(NOT_ENTRY," ELF dynamic linking information "); return fpos; }
    displayELFdyninfo(dynptr,number);
    beye_context().bm_file().seek(fpos, binary_stream::Seek_Set);
    return fpos;
}

bool ELF_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  std::string buff;
  bool ret = false;
  std::set<Elf_Reloc>::const_iterator erl;
  __filesize_t defval;
  main_handle.seek(ulShift,binary_stream::Seek_Set);
  switch(codelen) {
    default:
    case 1: defval = main_handle.read(type_byte); break;
    case 2: defval = main_handle.read(type_word); break;
    case 4: defval = main_handle.read(type_dword); break;
    case 8:
	main_handle.seek(ulShift, binary_stream::Seek_Set);
	defval=main_handle.read(type_qword);
	break;
  }
  if(flags & APREF_TRY_PIC)
  {
       __filesize_t off_in_got = defval;
       __filesize_t dynptr, dyn_ent, got_off;
       unsigned nitems;
/** @todo: If "program header" will be lost and will be present "section
    header" only then we should handle such situation propertly too */
       dynptr = findPHEntry(PT_DYNAMIC,nitems);
       if(dynptr)
       {
	 dyn_ent = findPHDynEntry(DT_PLTGOT,dynptr,nitems);
	 if(dyn_ent)
	 {
	   got_off = va2pa(dyn_ent);
	   return bind(parent,str, got_off + off_in_got, flags & ~APREF_TRY_PIC, codelen, r_sh);
	 }
       }
       return false;
  }
  if(PubNames.empty()) elf_ReadPubNameList(main_handle);
  if((erl = __found_ElfRel(ulShift)) != CurrElfChain.end())
  {
    Elf_Reloc r = (*erl);
    ret = elf_arch->build_refer_str(*elfcache,str,r,flags,codelen,defval);
  }
  if(!ret && elf_reader->ehdr().e_type>ET_REL && codelen>=4)
  {
    uint32_t id;
    main_handle.seek(ulShift,binary_stream::Seek_Set);
    id = main_handle.read(type_dword);
    if((erl = __found_ElfRel(va2pa(id))) != CurrElfChain.end())
    {
      Elf_Reloc r = (*erl);
      ret = elf_arch->build_refer_str(*elfcache,str,r,flags,codelen,defval);
    }
  }
  if(!ret)
  {
    buff="FFFFFFFF";
    if(flags & APREF_TRY_LABEL)
    {
       if(FindPubName(buff,r_sh))
       {
	 if(buff.length())
	 {
	   str+=buff;
	   if(!DumpMode && !EditMode) code_guider.add_go_address(parent,str,r_sh);
	   ret = true;
	 }
       }
    }
  }
  return ret;
}

void ELF_Parser::__elfReadSegments(std::map<__filesize_t,VA_map>& to, bool is_virt ) const
{
 Elf_Phdr phdr;
 Elf_Shdr shdr;
 struct VA_map vamap;
 __filesize_t fp;
 unsigned va_map_count;
 bool test;
 size_t i;
   /* We'll try to build section headers first
      since they is used in calculations of objects.
      For translation of virtual address to physical and vise versa
      more preferred to use program headers. But they often are not presented
      and often is unordered by file offsets. */
   if(IsSectionsPresent) /* Section headers are present */
   {
     va_map_count = elf_reader->ehdr().e_shnum;
//     if(!(*to = la_Build(0,sizeof(struct VA_map),MemOutBox)))
//     {
//       exit(EXIT_FAILURE);
//     }
     main_handle.seek(elf_reader->ehdr().e_shoff,binary_stream::Seek_Set);
     for(i = 0;i < va_map_count;i++)
     {
       __filesize_t flg,x_flags;
       fp = main_handle.tell();
       shdr=elf_reader->read_shdr(main_handle,fp);
       main_handle.seek(fp+elf_reader->ehdr().e_shentsize,binary_stream::Seek_Set);
       vamap.va = shdr.sh_addr;
       vamap.size = shdr.sh_size;
       vamap.foff = shdr.sh_offset;
       vamap.nameoff = shdr.sh_name;
       flg = shdr.sh_flags;
       x_flags = 0;
       /* I think - it would be better to use for computation of virtual and
	  physical addresses maps only that sections which occupy memory
	  during execution. All other are rubbish for analyze */
       if((flg & SHF_ALLOC) == SHF_ALLOC)
       {
	 x_flags |= PF_R; /* Ugle: means flags is not empty */
	 if(flg & SHF_WRITE)     x_flags |= PF_W;
	 if(flg & SHF_EXECINSTR) x_flags |= PF_X;
	 vamap.flags = x_flags;
	 test = is_virt ? va2pa(vamap.va) != 0 : pa2va(vamap.foff) != 0;
	 if(!test) {
	  if(is_virt) to.insert(std::make_pair(vamap.va,vamap));
	  else        to.insert(std::make_pair(vamap.foff,vamap));
//	   if(!la_AddData(*to,&vamap,MemOutBox)) exit(EXIT_FAILURE);
	   /** We must sort va_map after adding of each element because ELF section
	       header has unsorted and nested elements */
//	   la_Sort(*to,is_virt ? vamap_comp_virt : vamap_comp_phys);
	 }
       }
     }
   }
   else /* Try to build program headers map */
    if((va_map_count = elf_reader->ehdr().e_phnum) != 0) /* Program headers are present */
    {
//      if(!(*to = la_Build(va_map_count,sizeof(struct VA_map),MemOutBox)))
//      {
//	exit(EXIT_FAILURE);
//      }
      main_handle.seek(elf_reader->ehdr().e_phoff,binary_stream::Seek_Set);
      for(i = 0;i < va_map_count;i++)
      {
	fp = main_handle.tell();
	phdr=elf_reader->read_phdr(main_handle,fp);
	main_handle.seek(fp+elf_reader->ehdr().e_phentsize,binary_stream::Seek_Set);
	vamap.va = phdr.p_vaddr;
	vamap.size = std::max(phdr.p_filesz, phdr.p_memsz);
	vamap.foff = phdr.p_offset;
	vamap.nameoff = phdr.p_type & 0x000000FFUL ? ~phdr.p_type : 0xFFFFFFFFUL;
	vamap.flags = phdr.p_flags;
	test = is_virt ? va2pa(vamap.va) != 0 : pa2va(vamap.foff) != 0;
	if(!test) {
	  if(is_virt) to.insert(std::make_pair(vamap.va,vamap));
	  else        to.insert(std::make_pair(vamap.foff,vamap));
//	  if(!la_AddData(*to,&vamap,MemOutBox))
//	  {
//	    exit(EXIT_FAILURE);
//	  }
	  /** We must sort va_map after adding of each element because ELF program
	      header has unsorted and has nested elements */
//	  la_Sort(*to,is_virt ? vamap_comp_virt : vamap_comp_phys);
	}
      }
    }
}

ELF_Parser::ELF_Parser(binary_stream& h,CodeGuider& _code_guider,udn& u)
	    :Binary_Parser(h,_code_guider,u)
	    ,elf_min_va(FILESIZE_MAX)
	    ,namecache(&h)
	    ,elfcache(&h)
	    ,main_handle(h)
	    ,code_guider(_code_guider)
	    ,_udn(u)
{
    __filesize_t fs;
    uint8_t buf[16];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(buf,16);
    is_msbf = (buf[EI_DATA] == ELFDATA2MSB);
    is_64bit = (buf[EI_CLASS] == ELFCLASS64);
    if(is_64bit)
	elf_reader = new(zeromem) Elf64_Reader(main_handle);
    else
	elf_reader = new(zeromem) Elf32_Reader(main_handle);
    switch(elf_reader->ehdr().e_machine) {
	default:
	case EM_386: elf_arch = new(zeromem) Elf_i386(*this,*elf_reader); break;
	case EM_X86_64: elf_arch = new(zeromem) Elf_x86_64(*this,*elf_reader); break;
	case EM_PPC:
	case EM_PPC64: elf_arch = new(zeromem) Elf_Ppc(*this,*elf_reader); break;
	case EM_ARM: elf_arch = new(zeromem) Elf_Arm(*this,*elf_reader); break;
    }
   fs = main_handle.flength();
   IsSectionsPresent = elf_reader->ehdr().e_shnum != 0 &&
		       elf_reader->ehdr().e_shoff &&
		       elf_reader->ehdr().e_shoff < fs &&
		       elf_reader->ehdr().e_shoff +
		       elf_reader->ehdr().e_shnum*elf_reader->ehdr().e_shentsize <= fs;
   __elfReadSegments(va_map_virt,true);
   __elfReadSegments(va_map_phys,false);
   /** Find min value of virtual address */
   if(!va_map_virt.empty())
   for(std::map<__filesize_t,VA_map>::const_iterator it = va_map_virt.begin(); it!=va_map_virt.end();it++)
   {
     if((*it).first < elf_min_va) elf_min_va = (*it).first;
   }
   namecache = main_handle.dup();
   elfcache = main_handle.dup();
   /** Computing symbol table entry */
   __elfSymPtr = findSHEntry(main_handle, SHT_SYMTAB, __elfNumSymTab, __elfSymShTbl, __elfSymEntSize);
}

ELF_Parser::~ELF_Parser()
{
   if(namecache != &main_handle) delete namecache;
   if(elfcache != &main_handle) delete elfcache;
   delete elf_reader;
}

int ELF_Parser::query_bitness(__filesize_t off) const
{
  UNUSED(off);
  return is_64bit?DAB_USE64:DAB_USE32;
}

__filesize_t ELF_Parser::action_F1()
{
  hlpDisplay(10003);
  return beye_context().tell();
}

bool ELF_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
    bool bret = true;
    __filesize_t res;
    if(cfpos < elf_reader->ehdr_size()) {
	addr="ELFhdr:";
	addr+=Get2Digit(cfpos);
    } else if((res=pa2va(cfpos))!=0) {
	addr = ".";
	addr+=Get8Digit(res);
    }
    else bret = false;
    return bret;
}

bool ELF_Parser::FindPubName(std::string& buff,__filesize_t pa) const
{
  symbolic_information key;
  std::set<symbolic_information>::const_iterator ret;
  key.pa = pa;
  ret = PubNames.find(key);
  if(ret!=PubNames.end()) {
    buff=elf_arch->read_nametableex(*namecache,(*ret).nameoff,(*ret).addinfo);
    return true;
  }
  return _udn.find(pa,buff);
}

void ELF_Parser::elf_ReadPubNameList(binary_stream& handle)
{
  __filesize_t fpos,fp,tableptr,pubname_shtbl;
  unsigned long i,number,ent_size;
  struct symbolic_information epn;
  binary_stream& b_cache = handle;
  fpos = b_cache.tell();
  tableptr = findSHEntry(b_cache, SHT_DYNSYM, number, pubname_shtbl, ent_size);
  if(!tableptr) tableptr = findPHPubSyms(number, ent_size, pubname_shtbl);
  if(tableptr) {
    b_cache.seek(tableptr,binary_stream::Seek_Set);
    for(i = 0;i < number;i++)
    {
     Elf_Dyn pdyn;
     fp = b_cache.tell();
     pdyn=elf_reader->read_dyn(b_cache,fp);
     if(b_cache.eof()) break;
     b_cache.seek(fp+ent_size,binary_stream::Seek_Set);
     epn.nameoff = pdyn.d_tag;
     epn.pa = va2pa(pdyn.d_un.d_val);
     epn.addinfo = pubname_shtbl;
     epn.attr = ELF_ST_INFO(STB_GLOBAL,STT_NOTYPE);
     PubNames.insert(epn);
    }
  }
  /** If present symbolic information we must read it */

  if(__elfNumSymTab)
  {
    handle.seek(__elfSymPtr,binary_stream::Seek_Set);
    for(i = 0;i < __elfNumSymTab;i++)
    {
      Elf_Sym sym;
      fp = handle.tell();
      sym=elf_reader->read_sym(handle,fp);
      if(handle.eof() || IsKbdTerminate()) break;
      handle.seek(fp+__elfSymEntSize,binary_stream::Seek_Set);
      if(ELF_IS_SECTION_PHYSICAL(sym.st_shndx) &&
	 ELF_ST_TYPE(sym.st_info) != STT_SECTION)
      {
	epn.pa = __calcSymEntry(handle,i,false);
	epn.nameoff = sym.st_name;
	epn.addinfo = __elfSymShTbl;
	epn.attr = sym.st_info;
	PubNames.insert(epn);
//	if(!la_AddData(PubNames,&epn,MemOutBox)) break;
      }
    }
  }
//  la_Sort(PubNames,compare_pubnames);
  b_cache.seek(fpos,binary_stream::Seek_Set);
}

std::string ELF_Parser::elf_ReadPubName(binary_stream& b_cache,const symbolic_information& it) const
{
   UNUSED(b_cache);
   return elf_arch->read_nametableex(*namecache,it.nameoff,it.addinfo);
}

__filesize_t ELF_Parser::get_public_symbol(std::string& str,unsigned& func_class,
			   __filesize_t pa,bool as_prev)
{
    __filesize_t fpos;
    if(PubNames.empty()) elf_ReadPubNameList(*elfcache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    fpos=find_symbolic_information(PubNames,func_class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	str=elf_ReadPubName(*elfcache,*idx);
    }
    return fpos;
}

unsigned ELF_Parser::get_object_attribute(__filesize_t pa,std::string& name,
		       __filesize_t& start,__filesize_t& end,int& _class,int& bitness)
{
  unsigned i,ret;
  start = 0;
  end = main_handle.flength();
  _class = OC_NOOBJECT;
  bitness = query_bitness(pa);
  name[0] = 0;
  ret = 0;
  for(std::map<__filesize_t,VA_map>::const_iterator it = va_map_phys.begin();it !=va_map_phys.end();it++) {
    if(!((*it).first && (*it).second.size)) continue;
    if(pa >= start && pa < (*it).first) {
      /** means between two objects */
      end = (*it).first;
      ret = 0;
      break;
    }
    if(pa >= (*it).first && pa < (*it).first + (*it).second.size) {
      start = (*it).first;
      end = start + (*it).second.size;
      if((*it).second.flags)
      {
	if((*it).second.flags & PF_X) _class = OC_CODE;
	else                          _class = OC_DATA;
      }
      else  _class = OC_NOOBJECT;
      name=elf_arch->read_nametable(*namecache,(*it).second.nameoff);
      ret = i+1;
      break;
    }
    start = (*it).first + (*it).second.size;
  }
  return ret;
}

int ELF_Parser::query_platform() const {
    unsigned id;
    elf_machine(elf_reader->ehdr().e_machine,id);
    return id;
}

int ELF_Parser::query_endian(__filesize_t off) const {
 UNUSED(off);
 return is_msbf?DAE_BIG:DAE_LITTLE;
}

static bool probe(binary_stream& main_handle) {
    char id[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(id,sizeof id);
    return IS_ELF(id);
//  [0] == EI_MAG0 && id[1] == EI_MAG1 && id[2] == 'L' && id[3] == 'F';
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) ELF_Parser(h,_parent,u); }
extern const Binary_Parser_Info elf_info = {
    "ELF (Executable and Linking Format)",	/**< plugin name */
    probe,
    query_interface
};

void Elf_Arch::read_reloc(binary_stream& handle2,Elf_Reloc& erc) const { UNUSED(handle2); erc.addend=0; }

void Elf_Arm::read_reloc(binary_stream& handle2,Elf_Reloc& erc) const
{
    switch(reader().R_TYPE(erc.info)) {
      case R_ARM_THM_JUMP6:
      case R_ARM_THM_PC8:
      case R_ARM_ABS8:
      case R_ARM_THM_JUMP8:
	       erc.addend = handle2.read(type_byte);
	       break;
      case R_ARM_THM_JUMP11:
      case R_ARM_GOT_BREL12:
      case R_ARM_GOTOFF12:
      case R_ARM_TLS_LDO12:
      case R_ARM_TLS_LE12:
      case R_ARM_TLS_IE12GP:
      case R_ARM_ABS12:
      case R_ARM_THM_PC12:
	       erc.addend = bioRead12(handle2);
	       break;
      case R_ARM_ABS16:
	       erc.addend = handle2.read(type_word);
	       break;
      case R_ARM_THM_JUMP19:
	       erc.addend = bioRead19(handle2);
	       break;
      case R_ARM_THM_XPC22:
      case R_ARM_THM_RPC22:
	       erc.addend = bioRead22(handle2);
	       break;
	case R_ARM_SWI24:
      case R_ARM_JUMP24:
      case R_ARM_THM_JUMP24:
      case R_ARM_RPC24:
	       erc.addend = bioRead24(handle2);
	       break;
      case R_ARM_XPC25:
      case R_ARM_RXPC25:
	       erc.addend = bioRead25(handle2);
	       break;
      default:
	       erc.addend = handle2.read(type_dword);
	       break;
    }
}

void Elf_i386::read_reloc(binary_stream& handle2,Elf_Reloc& erc) const
{
    switch(reader().R_TYPE(erc.info)) {
      case R_386_GNU_8:
      case R_386_GNU_PC8:
	       erc.addend = handle2.read(type_byte);
	       break;
      case R_386_GNU_16:
      case R_386_GNU_PC16:
	       erc.addend = handle2.read(type_word);
	       break;
      default:
	       erc.addend = handle2.read(type_dword);
	       break;
    }
}

void Elf_x86_64::read_reloc(binary_stream& handle2,Elf_Reloc& erc) const {
    switch(reader().R_TYPE(erc.info)) {
      case R_X86_64_8:
      case R_X86_64_PC8:
	       erc.addend = handle2.read(type_byte);
	       break;
      case R_X86_64_16:
      case R_X86_64_PC16:
	       erc.addend = handle2.read(type_word);
	       break;
      case R_X86_64_32:
      case R_X86_64_32S:
      case R_X86_64_PC32:
      case R_X86_64_GOT32:
      case R_X86_64_PLT32:
      case R_X86_64_DTPOFF32:
      case R_X86_64_GOTTPOFF:
      case R_X86_64_TPOFF32:
      case R_X86_64_GOTPC32_TLSDESC:
	       erc.addend = handle2.read(type_dword);
	       break;
      default:
	       erc.addend = handle2.read(type_qword);
	       break;
    }
}

void Elf_Ppc::read_reloc(binary_stream& handle2,Elf_Reloc& erc) const
{
    switch(reader().R_TYPE(erc.info)) {
      case R_ARM_THM_JUMP6:
      case R_ARM_THM_PC8:
      case R_ARM_ABS8:
      case R_ARM_THM_JUMP8:
	       erc.addend = handle2.read(type_byte);
	       break;
      case R_ARM_THM_JUMP11:
      case R_ARM_GOT_BREL12:
      case R_ARM_GOTOFF12:
      case R_ARM_TLS_LDO12:
      case R_ARM_TLS_LE12:
      case R_ARM_TLS_IE12GP:
      case R_ARM_ABS12:
      case R_ARM_THM_PC12:
	       erc.addend = bioRead12(handle2);
	       break;
      default:
	       erc.addend = handle2.read(type_word);
	       break;
      case R_PPC_ADDR24:
      case R_PPC_REL24:
      case R_PPC_PLTREL24:
      case R_PPC_LOCAL24PC:
	       erc.addend = bioRead24(handle2);
	       break;
      case R_PPC_ADDR32:
      case R_PPC_GLOB_DAT:
      case R_PPC_JMP_SLOT:
      case R_PPC_RELATIVE:
      case R_PPC_UADDR32:
      case R_PPC_REL32:
      case R_PPC_PLT32:
      case R_PPC_PLTREL32:
      case R_PPC_ADDR30:
      case R_PPC_DTPMOD32:
      case R_PPC_TPREL32:
      case R_PPC_DTPREL32:
      case R_PPC_EMB_NADDR32:
      case R_PPC_RELAX32:
      case R_PPC_RELAX32PC:
      case R_PPC_RELAX32_PLT:
      case R_PPC_RELAX32PC_PLT:
      case R_PPC_GNU_VTINHERIT:
      case R_PPC_GNU_VTENTRY:
	       erc.addend = handle2.read(type_dword);
	       break;
      case R_PPC64_ADDR64:
      case R_PPC64_UADDR64:
      case R_PPC64_REL64:
      case R_PPC64_PLT64:
      case R_PPC64_PLTREL64:
      case R_PPC64_TOC:
	       erc.addend = handle2.read(type_qword);
	       break;
    }
}

bool Elf_Arch::build_refer_str(binary_stream& handle,std::string& str,
				const Elf_Reloc& erl,
				int flags,unsigned codelen,
				__filesize_t defval) const
{
    UNUSED(handle);
    UNUSED(str);
    UNUSED(erl);
    UNUSED(flags);
    UNUSED(codelen);
    UNUSED(defval);
    return false;
}

bool Elf_Arm::build_refer_str(binary_stream& handle,std::string& str,
				const Elf_Reloc&  erl,
				int flags,unsigned codelen,
				__filesize_t defval) const
{
  bool retval = true;
  uint32_t r_type;
  bool ret=false, use_addend = false;
  std::string buff;
  UNUSED(codelen);
  UNUSED(defval);
  r_type = reader().R_TYPE(erl.info);
  buff[0] = 0;
  switch(r_type)
  {
    default:
    case R_ARM_RELATIVE: /* BVA + addendum */
    case R_ARM_NONE: /* nothing to do */
    case R_ARM_COPY: /* nothing to do */
		   retval = false;
		   break;
    case R_ARM_THM_ABS5:
    case R_ARM_ABS8:
    case R_ARM_ABS12:
    case R_ARM_ABS16:
    case R_ARM_ABS32:  /* symbol + addendum */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_ARM_THM_PC8:
    case R_ARM_THM_PC12:
    case R_ARM_PC24: /* symbol + addendum - this */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_ARM_PLT32: /* PLT[offset] + addendum - this */
		   str+="PLT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_ARM_GLOB_DAT:  /* symbol */
    case R_ARM_JUMP_SLOT:  /* symbol */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret) str+=buff;
		   break;
    case R_ARM_GOTOFF32: /* symbol + addendum - GOT */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     str+="-GOT";
		     use_addend = true;
		   }
		   else retval = false;
		   break;
  }
  if(erl.addend && use_addend && ret &&
     !(flags & APREF_TRY_LABEL)) /* <- it for readability */
  {
    str+="+";
    str+=Get8Digit(erl.addend);
  }
  return retval;
}

bool Elf_i386::build_refer_str(binary_stream& handle,std::string& str,
				const Elf_Reloc& erl,
				int flags,unsigned codelen,
				__filesize_t defval) const
{
  bool retval = true;
  uint32_t r_type;
  bool ret=false, use_addend = false;
  std::string buff;
  UNUSED(codelen);
  UNUSED(defval);
  r_type = reader().R_TYPE(erl.info);
  buff[0] = 0;
  switch(r_type)
  {
    default:
    case R_386_RELATIVE: /* BVA + addend. Ignoring it is best choice */
    case R_386_NONE: /* nothing to do */
    case R_386_COPY: /* nothing to do */
		   retval = false;
		   break;
    case R_386_GNU_8:
    case R_386_GNU_16:
    case R_386_32:  /* symbol + addendum */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_386_GNU_PC8:
    case R_386_GNU_PC16:
    case R_386_PC32: /* symbol + addendum - this */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_386_GOT32: /* GOT[offset] + addendum - this */
		   str+="GOT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_386_PLT32: /* PLT[offset] + addendum - this */
		   str+="PLT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_386_GLOB_DAT:  /* symbol */
    case R_386_JMP_SLOT:  /* symbol */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret) str+=buff;
		   break;
    case R_386_GOTOFF: /* symbol + addendum - GOT */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     str+="-GOT";
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_386_GOTPC: /* GOT + addendum - this */
		   str+="GOT-.here";
		   use_addend = true;
		   break;
  }
  if(erl.addend && use_addend && ret &&
     !(flags & APREF_TRY_LABEL)) /* <- it for readability */
  {
    str+="+";
    str+=Get8Digit(erl.addend);
  }
  return retval;
}

bool Elf_x86_64::build_refer_str(binary_stream& handle,std::string& str,
				const Elf_Reloc& erl,
				int flags,unsigned codelen,
				__filesize_t defval) const
{
  bool retval = true;
  uint32_t r_type;
  bool ret=false, use_addend = false;
  std::string buff;
  UNUSED(codelen);
  UNUSED(defval);
  r_type = reader().R_TYPE(erl.info);
  buff[0] = 0;
  switch(r_type)
  {
    default:
    case R_X86_64_RELATIVE: /* BVA + addendum */
    case R_X86_64_NONE: /* nothing to do */
    case R_X86_64_COPY: /* nothing to do */
		   retval = false;
		   break;
    case R_X86_64_8:
    case R_X86_64_16:
    case R_X86_64_32:
    case R_X86_64_64:  /* symbol + addendum */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_X86_64_PC8:
    case R_X86_64_PC16:
    case R_X86_64_PC32:
    case R_X86_64_PC64: /* symbol + addendum - this */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_X86_64_GOT32:
		   str+="GOT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_X86_64_GOT64:
    case R_X86_64_GOTPC64:
    case R_X86_64_GOTPLT64: /* GOT[offset] + addendum - this */
		   str+="GOT-";
		   str+=Get16Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_X86_64_PLT32:
		   str+="PLT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_X86_64_PLTOFF64: /* PLT[offset] + addendum - this */
		   str+="PLT-";
		   str+=Get16Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_X86_64_GLOB_DAT:  /* symbol */
    case R_X86_64_JUMP_SLOT:  /* symbol */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret) str+=buff;
		   break;
    case R_X86_64_GOTOFF64: /* symbol + addendum - GOT */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     str+="-GOT";
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_X86_64_GOTPCREL64: /* GOT + addendum - this */
		   str+="GOT-.here";
		   use_addend = true;
		   break;
  }
  if(erl.addend && use_addend && ret &&
     !(flags & APREF_TRY_LABEL)) /* <- it for readability */
  {
    str+="+";
    str+=Get8Digit(erl.addend);
  }
  return retval;
}

bool Elf_Ppc::build_refer_str(binary_stream& handle,std::string& str,
				const Elf_Reloc& erl,
				int flags,unsigned codelen,
				__filesize_t defval) const
{
  bool retval = true;
  uint32_t r_type;
  bool ret=false, use_addend = false;
  std::string buff;
  UNUSED(codelen);
  UNUSED(defval);
  r_type = reader().R_TYPE(erl.info);
  buff[0] = 0;
  switch(r_type)
  {
    default:
    case R_PPC_RELATIVE: /* BVA + addendum */
    case R_PPC_NONE: /* nothing to do */
    case R_PPC_COPY: /* nothing to do */
		   retval = false;
		   break;
    case R_PPC_ADDR14:
    case R_PPC_ADDR14_BRTAKEN:
    case R_PPC_ADDR14_BRNTAKEN:
    case R_PPC_ADDR16:
    case R_PPC_ADDR16_LO:
    case R_PPC_ADDR16_HI:
    case R_PPC_ADDR16_HA:
    case R_PPC_ADDR24:
    case R_PPC_ADDR32:
    case R_PPC_UADDR32:
    case R_PPC64_ADDR64:
    case R_PPC64_UADDR64: /* symbol + addendum */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_PPC_REL14:
    case R_PPC_REL14_BRTAKEN:
    case R_PPC_REL14_BRNTAKEN:
    case R_PPC_REL16:
    case R_PPC_REL16_LO:
    case R_PPC_REL16_HI:
    case R_PPC_REL16_HA:
    case R_PPC_REL24:
    case R_PPC_REL32:
    case R_PPC64_REL64: /* symbol + addendum - this */
		   ret = read_reloc_name(handle,erl, buff);
		   if(!buff.empty() && ret)
		   {
		     str+=buff;
		     /* strcat(str,"-.here"); <- it's commented for readability */
		     use_addend = true;
		   }
		   else retval = false;
		   break;
    case R_PPC_GOT16_LO:
    case R_PPC_GOT16_HI:
    case R_PPC_GOT16_HA:
		   str+="GOT-";
		    str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_PPC_PLT16_LO:
    case R_PPC_PLT16_HI:
    case R_PPC_PLT16_HA:
    case R_PPC_PLT32:
		   str+="PLT-";
		   str+=Get8Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_PPC64_PLT64: /* PLT[offset] + addendum - this */
		   str+="PLT-";
		   str+=Get16Digit(erl.offset);
		   use_addend = true;
		   break;
    case R_PPC_GLOB_DAT:  /* symbol */
    case R_PPC_JMP_SLOT:  /* symbol */
		   ret = read_reloc_name(handle,erl, buff);
		   if(buff[0] && ret) str+=buff;
		   break;
  }
  if(erl.addend && use_addend && ret &&
     !(flags & APREF_TRY_LABEL)) /* <- it for readability */
  {
    str+="+";
    str+=Get8Digit(erl.addend);
  }
  return retval;
}

bool Elf_Arch::read_reloc_name(binary_stream& handle,const Elf_Reloc& erl, std::string& buff) const
{
    Elf_Shdr shdr;
    Elf_Sym sym;
    __filesize_t fp;
    bool ret = true;
    fp = handle.tell();
    if(parent().is_sections_present()) {/* Section headers are present */
	shdr=reader().read_shdr(handle,reader().ehdr().e_shoff+erl.sh_idx*reader().ehdr().e_shentsize);
	handle.seek(shdr.sh_offset,binary_stream::Seek_Set);
	/* Minor integrity test */
	ret = shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM;
    }
    else handle.seek(erl.sh_idx,binary_stream::Seek_Set);
    if(ret) {
	/* We assume that dynsym and symtab are equal */
	__filesize_t r_sym = reader().R_SYM(erl.info);
	unsigned old_active;
	old_active = parent().get_active_shtbl();
	if(parent().is_sections_present()) parent().set_active_shtbl(shdr.sh_link);
	else {
	    __filesize_t dynptr;
	    unsigned nitems;
	    dynptr = parent().findPHEntry(PT_DYNAMIC,nitems);
	    parent().set_active_shtbl(parent().va2pa(parent().findPHDynEntry(DT_STRTAB,dynptr,nitems)));
	}
	handle.seek(r_sym*parent().sym_ent_size(),binary_stream::Seek_Cur);
	sym=reader().read_sym(handle,handle.tell());
	buff=read_nametableex(handle,sym.st_name,parent().get_active_shtbl());
	parent().set_active_shtbl(old_active);
	if(buff.empty()) {
	    /* reading name failed - try read at least section name */
	    if(parent().is_sections_present()) {
		if(ELF_ST_TYPE(sym.st_info) == STT_SECTION &&
		    sym.st_shndx &&
		    parent().ELF_IS_SECTION_PHYSICAL(sym.st_shndx)) {
		    shdr=reader().read_shdr(handle,reader().ehdr().e_shoff+sym.st_shndx*reader().ehdr().e_shentsize);
		    if(!parent().FindPubName(buff, shdr.sh_offset+erl.addend)) buff=read_nametable(handle,shdr.sh_name);
		}
	    }
	    if(buff.empty()) buff="?noname";
	}
    }
    handle.seek(fp,binary_stream::Seek_Set);
    return ret;
}

std::string Elf_Arch::read_nametable(binary_stream& b_cache,__filesize_t off) const
{
    std::string rc;
  __filesize_t foff;
  Elf_Shdr sh;
  unsigned char ch;

  foff = reader().ehdr().e_shoff+reader().ehdr().e_shstrndx*reader().ehdr().e_shentsize;
  sh=reader().read_shdr(b_cache,foff);
  foff = sh.sh_offset + off;
  b_cache.seek(foff,binary_stream::Seek_Set);
  while(1)
  {
     ch = b_cache.read(type_byte);
     if(!ch || b_cache.eof()) break;
     rc+=ch;
  }
  return rc;
}

std::string Elf_Arch::read_nametableex(binary_stream& b_cache,__filesize_t off,__filesize_t a_shtbl) const
{
    std::string rc;
  __filesize_t foff;
  Elf_Shdr sh;
  unsigned char ch;
  if(reader().ehdr().e_shoff) {
    foff = reader().ehdr().e_shoff+a_shtbl*reader().ehdr().e_shentsize;
    sh=reader().read_shdr(b_cache,foff);
    foff = sh.sh_offset + off;
  }
  /* if section headers are lost then active_shtbl should directly point to
     required string table */
  else  foff = a_shtbl + off;
  b_cache.seek(foff,binary_stream::Seek_Set);
  while(1) {
     ch = b_cache.read(type_byte);
     if(!ch || b_cache.eof()) break;
     rc+=ch;
  }
  return rc;
}

} // namespace	usr
