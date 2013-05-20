/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/elf386.h
 * @brief       This file defines standard ELF types, structures, and macros.
 * @version     -
 * @remark      Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation,
 *              Inc. This file is part of the GNU C Library.
 *              The GNU C Library is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Library General Public License as
 *              published by the Free Software Foundation; either version 2 of the
 *              License, or (at your option) any later version.
 *              The GNU C Library is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *              Library General Public License for more details.
 *              You should have received a copy of the GNU Library General Public
 *              License along with the GNU C Library; see the file COPYING.LIB.  If not,
 *              write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *              Boston, MA 02111-1307, USA.
 * @remark	portions taken from FreeBSD (sys/elf*.h), and updates from
 *              current draft (http://www.caldera.com/developers/gabi/)
 * @note        Requires POSIX compatible development system
 *
 * @author      GNU FSF
 * @since       1995
**/
#ifndef __ELF_INC
#define __ELF_INC
#include "config.h"
#include <endian.h>
#include "libbeye/bswap.h"
#include "libbeye/bstream.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif
enum {
    EI_NIDENT	=16,		/**< Size of e_ident[] */
/** Fields in e_ident[] */

    EI_MAG0	=0,		/**< File identification byte 0 index */
    ELFMAG0	=0x7F,		/**< Magic number byte 0 */

    EI_MAG1	=1,		/**< File identification byte 1 index */
    ELFMAG1	='E',		/**< Magic number byte 1 */

    EI_MAG2	=2,		/**< File identification byte 2 index */
    ELFMAG2	='L',		/**< Magic number byte 2 */

    EI_MAG3	=3,		/**< File identification byte 3 index */
    ELFMAG3	='F',		/**< Magic number byte 3 */

    EI_CLASS	=4,		/**< File class */
    ELFCLASSNONE=0,		/**< Invalid class */
    ELFCLASS32	=1,		/**< 32-bit objects */
    ELFCLASS64	=2,		/**< 64-bit objects */

    EI_DATA	=5,		/**< Data encoding */
    ELFDATANONE	=0,		/**< Invalid data encoding */
    ELFDATA2LSB	=1,		/**< 2's complement, little endian */
    ELFDATA2MSB	=2,		/**< 2's complement, big endian */

    EI_VERSION	=6,		/**< File version */
    EI_OSABI	=7		/**< Operating system / ABI identification */
};
enum {
    ELFOSABI_SYSV	=0,	/**< UNIX System V ABI */
    ELFOSABI_NONE	=ELFOSABI_SYSV,	/**< symbol used in old spec */
    ELFOSABI_HPUX	=1,	/**< HP-UX operating system */
    ELFOSABI_NETBSD	=2,	/**< NetBSD */
    ELFOSABI_LINUX	=3,	/**< GNU/Linux */
    ELFOSABI_HURD	=4,	/**< GNU/Hurd */
    ELFOSABI_86OPEN	=5,	/**< 86Open common IA32 ABI */
    ELFOSABI_SOLARIS	=6,	/**< Solaris */
    ELFOSABI_MONTEREY	=7,	/**< Monterey */
    ELFOSABI_IRIX	=8,	/**< IRIX */
    ELFOSABI_FREEBSD	=9,	/**< FreeBSD */
    ELFOSABI_TRU64	=10,	/**< TRU64 UNIX */
    ELFOSABI_MODESTO	=11,	/**< Novell Modesto */
    ELFOSABI_OPENBSD	=12,	/**< OpenBSD */
    ELFOSABI_ARM	=97,	/**< ARM */
    ELFOSABI_STANDALONE	=255,	/**< Standalone (embedded) application */

    EI_ABIVERSION	=8,	/**< ABI version */
    OLD_EI_BRAND	=8,	/**< Start of architecture identification. */
    EI_PAD		=9	/**< Start of padding (per SVR4 ABI). */
};
/* e_ident */
#define IS_ELF(e_ident)	(e_ident[EI_MAG0] == ELFMAG0 && \
			 e_ident[EI_MAG1] == ELFMAG1 && \
			 e_ident[EI_MAG2] == ELFMAG2 && \
			 e_ident[EI_MAG3] == ELFMAG3)

/** Values for e_type, which identifies the object file type */
enum {
    ET_NONE	=0,		/**< No file type */
    ET_REL	=1,		/**< Relocatable file */
    ET_EXEC	=2,		/**< Executable file */
    ET_DYN	=3,		/**< Shared object file */
    ET_CORE	=4,		/**< Core file */
    ET_LOOS	=0xFE00U,		/**< OS-specific */
    ET_HIOS	=0xFEFFU,		/**< OS-specific */
    ET_LOPROC	=0xFF00U,		/**< Processor-specific */
    ET_HIPROC	=0xFFFFU		/**< Processor-specific */
};
/** Values for e_machine, which identifies the architecture */
enum {
    EM_NONE		=0,	/**< No machine */
    EM_M32		=1,	/**< AT&T WE 32100 */
    EM_SPARC		=2,	/**< Sun SPARC */
    EM_386		=3,	/**< Intel 80386 */
    EM_68K		=4,	/**< Motorola m68k */
    EM_88K		=5,	/**< Motorola m88k */
				/* 6 reserved, was EM_486 */
    EM_860		=7,	/**< Intel 80860 */
    EM_MIPS		=8,	/**< MIPS I */
    EM_S370		=9,	/**< IBM System/370 */
    EM_MIPS_RS3_LE	=10,	/**< MIPS R3000 little-endian */
				/**< 11-14 reserved */
    EM_PARISC		=15,	/**< HP PA-RISC */
				/**< 16 reserved */
    EM_VPP500		=17,	/**< Fujitsu VPP500 */
    EM_SPARC32PLUS	=18,	/**< Sun SPARC v8plus */
    EM_960		=19,	/**< Intel 80960 */
    EM_PPC		=20,	/**< PowerPC */
    EM_PPC64		=21,	/**< PowerPC 64-bit */
    EM_S390		=22,	/**< IBM System/390 */
				/**< 23-35 reserved */
    EM_ADSP		=29,	/**< Atmel ADSP */
    EM_V800		=36,	/**< NEC V800 */
    EM_FR20		=37,	/**< Fujitsu FR20 */
    EM_RH32		=38,	/**< TRW RH-32 */
    EM_RCE		=39,	/**< Motorola RCE */
    EM_ARM		=40,	/**< Advanced RISC Machines ARM */
    EM_ALPHA		=41,	/**< Digital Alpha */
    EM_SH		=42,	/**< Hitachi SH */
    EM_SPARCV9		=43,	/**< SPARC Version 9 64-bit */
    EM_TRICORE		=44,	/**< Siemens TriCore embedded processor */
    EM_ARC		=45,	/**< Argonaut RISC Core, Argonaut Technologies Inc. */
    EM_H8_300		=46,	/**< Hitachi H8/300 */
    EM_H8_300H		=47,	/**< Hitachi H8/300H */
    EM_H8S		=48,	/**< Hitachi H8S */
    EM_H8_500		=49,	/**< Hitachi H8/500 */
    EM_IA_64		=50,	/**< Intel IA-64 processor architecture */
    EM_MIPS_X		=51,	/**< Stanford MIPS-X */
    EM_COLDFIRE		=52,	/**< Motorola ColdFire */
    EM_68HC12		=53,	/**< Motorola M68HC12 */
    EM_MMA		=54,	/**< Fujitsu MMA Multimedia Accelerator */
    EM_PCP		=55,	/**< Siemens PCP */
    EM_NCPU		=56,	/**< Sony nCPU embedded RISC processor */
    EM_NDR1		=57,	/**< Denso NDR1 microprocessor */
    EM_STARCORE		=58,	/**< Motorola Star*Core processor */
    EM_ME16		=59,	/**< Toyota ME16 processor */
    EM_ST100		=60,	/**< STMicroelectronics ST100 processor */
    EM_TINYJ		=61,	/**< Advanced Logic Corp. TinyJ embedded processor family */
    EM_X86_64		=62,	/**< AMD x86-64 architecture */
    EM_PDSP		=63,	/**< Sony DSP Processor */
    EM_PDP10		=64,	/**< Digital Equipment Corp. PDP-10 */
    EM_PDP11		=65,	/**< Digital Equipment Corp. PDP-11 */
    EM_FX66		=66,	/**< Siemens FX66 microcontroller */
    EM_ST9PLUS		=67,	/**< STMicroelectronics ST9+ 8/16 bit microcontroller */
    EM_ST7		=68,	/**< STMicroelectronics ST7 8-bit microcontroller */
    EM_68HC16		=69,	/**< Motorola MC68HC16 Microcontroller */
    EM_68HC11		=70,	/**< Motorola MC68HC11 Microcontroller */
    EM_68HC08		=71,	/**< Motorola MC68HC08 Microcontroller */
    EM_68HC05		=72,	/**< Motorola MC68HC05 Microcontroller */
    EM_SVX		=73,	/**< Silicon Graphics SVx */
    EM_ST19		=74,	/**< STMicroelectronics ST19 8-bit microcontroller */
    EM_VAX		=75,	/**< Digital VAX */
    EM_CRIS		=76,	/**< Axis Communications 32-bit embedded processor */
    EM_JAVELIN		=77,	/**< Infineon Technologies 32-bit embedded processor */
    EM_FIREPATH		=78,	/**< Element 14 64-bit DSP Processor */
    EM_ZSP		=79,	/**< LSI Logic 16-bit DSP Processor */
    EM_MMIX		=80,	/**< Donald Knuth's educational 64-bit processor */
    EM_HUANY		=81,	/**< Harvard University machine-independent object files */
    EM_PRISM		=82,	/**< SiTera Prism */
    EM_AVR		=83,	/**< Atmel AVR 8-bit microcontroller */
    EM_FR30		=84,	/**< Fujitsu FR30 */
    EM_D10V		=85,	/**< Mitsubishi D10V */
    EM_D30V		=86,	/**< Mitsubishi D30V */
    EM_V850		=87,	/**< NEC v850 */
    EM_M32R		=88,	/**< Mitsubishi M32R */
    EM_MN10300		=89,	/**< Matsushita MN10300 */
    EM_MN10200		=90,	/**< Matsushita MN10200 */
    EM_PJ		=91,	/**< picoJava */
    EM_OPENRISC		=92,	/**< OpenRISC 32-bit embedded processor */
    EM_ARC_A5		=93,	/**< ARC Cores Tangent-A5 */
    EM_XTENSA		=94,	/**< Tensilica Xtensa Architecture */
    EM_VIDEOCORE	=95,	/**< Alphamosaic VideoCore processor */
    EM_TMM_GPP		=96,	/**< Thompson Multimedia General Purpose Processor */
    EM_NS32K		=97,	/**< National Semiconductor 32000 series */
    EM_TPC		=98,	/**< Tenor Network TPC processor */
    EM_SNP1K		=99,	/**< Trebia SNP 1000 processor */
    EM_IP2K		=101,	/**< Ubicom IP2022 micro controller */
    EM_CR		=103,	/**< National Semiconductor CompactRISC */
    EM_MSP430		=105,	/**< TI msp430 micro controller */
    EM_BLACKFIN		=106,	/**< ADI Blackfin */
    EM_ALTERA_NIOS2	=113,	/**< Altera Nios II soft-core processor */
    EM_CRX		=114,	/**< National Semiconductor CRX */

    EM_XGATE		=115,	/**< Motorola XGATE embedded processor */
    EM_C166		=116,	/**< Infineon C16x/XC16x processor */
    EM_M16C		=117,	/**< Renesas M16C series microprocessors */
    EM_DSPIC30F		=118,	/**< Microchip Technology dsPIC30F Digital Signal Controller */
    EM_CE		=119,	/**< Freescale Communication Engine RISC core */
    EM_M32C		=120,	/**< Renesas M32C series microprocessors */

    EM_TSK3000		=131,	/**< Altium TSK3000 core */
    EM_RS08		=132,	/**< Freescale RS08 embedded processor */

    EM_ECOG2		=134,	/**< Cyan Technology eCOG2 microprocessor */
    EM_SCORE		=135,	/**< Sunplus Score */
    EM_DSP24		=136,	/**< New Japan Radio (NJR) 24-bit DSP Processor */
    EM_VIDEOCORE3	=137,	/**< Broadcom VideoCore III processor */
    EM_LATTICEMICO32	=138,	/**< RISC processor for Lattice FPGA architecture */
    EM_SE_C17		=139,	/**< Seiko Epson C17 family */

    EM_MMDSP_PLUS	=160,	/**< STMicroelectronics 64bit VLIW Data Signal Processor */
    EM_CYPRESS_M8C	=161,	/**< Cypress M8C microprocessor */
    EM_R32C		=162,	/**< Renesas R32C series microprocessors */
    EM_TRIMEDIA		=163,	/**< NXP Semiconductors TriMedia architecture family */
    EM_QDSP6		=164,	/**< QUALCOMM DSP6 Processor */
    EM_8051		=165,	/**< Intel 8051 and variants */
    EM_STXP7X		=166,	/**< STMicroelectronics STxP7x family */
    EM_NDS32		=167,	/**< Andes Technology compact code size embedded RISC processor family */
    EM_ECOG1X		=168,	/**< Cyan Technology eCOG1X family */
    EM_MAXQ30		=169,	/**< Dallas Semiconductor MAXQ30 Core Micro-controllers */
    EM_XIMO16		=170,	/**< New Japan Radio (NJR) 16-bit DSP Processor */
    EM_MANIK		=171,	/**< M2000 Reconfigurable RISC Microprocessor */
    EM_CRAYNV2		=172,	/**< Cray Inc. NV2 vector architecture */
    EM_RX		=173,	/**< Renesas RX family */
    EM_METAG		=174,	/**< Imagination Technologies META processor architecture */
    EM_MCST_ELBRUS	=175,	/**< MCST Elbrus general purpose hardware architecture */
    EM_ECOG16		=176,	/**< Cyan Technology eCOG16 family */
    EM_CR16		=177,	/**< National Semiconductor CompactRISC 16-bit processor */

/** If it is necessary to assign new unofficial EM_* values, please pick large
   random numbers (0x8523, 0xa7f2, etc.) to minimize the chances of collision
   with official or non-GNU unofficial values.

   NOTE: Do not just increment the most recent number by one.
   Somebody else somewhere will do exactly the same thing, and you
   will have a collision.  Instead, pick a random number.  */

/** Cygnus PowerPC ELF backend.  Written in the absence of an ABI.  */
    EM_CYGNUS_POWERPC	=0x9025U,

/** Cygnus M32R ELF backend.  Written in the absence of an ABI.  */
    EM_CYGNUS_M32R	=0x9041U,

/** D10V backend magic number.  Written in the absence of an ABI.  */
    EM_CYGNUS_D10V	=0x7650U,

/** mn10200 and mn10300 backend magic numbers.
   Written in the absense of an ABI.  */
    EM_CYGNUS_MN10200	=0xdeadL,
    EM_CYGNUS_MN10300	=0xbeefL
};
/** See the above comment before you add a new EM_* value here.  */

/** Values for e_version */
enum {
    EV_NONE	=0,		/**< Invalid ELF version */
    EV_CURRENT	=1		/**< Current version */
};
/** Values for program header, p_type field */
enum {
    PT_NULL	=0,		/**< Program header table entry unused */
    PT_LOAD	=1,		/**< Loadable program segment */
    PT_DYNAMIC	=2,		/**< Dynamic linking information */
    PT_INTERP	=3,		/**< Program interpreter */
    PT_NOTE	=4,		/**< Auxiliary information */
    PT_SHLIB	=5,		/**< Reserved, unspecified semantics */
    PT_PHDR	=6,		/**< Entry for header table itself */
    PT_NUM	=7,		/**< Number of defined types.  */
    PT_LOPROC	=0x70000000UL,	/**< Processor-specific */
    PT_HIPROC	=0x7FFFFFFFUL	/**< Processor-specific */
};
/** Program segment permissions, in program header p_flags field */
enum {
    PF_X		=(1 << 0),	/**< Segment is executable */
    PF_W		=(1 << 1),	/**< Segment is writable */
    PF_R		=(1 << 2),	/**< Segment is readable */
    PF_MASKPROC		=0xF0000000UL	/**< Processor-specific reserved bits */
};
/** Values for section header, sh_type field */
enum {
    SHT_NULL	=0,		/**< Section header table entry unused */
    SHT_PROGBITS=1,		/**< Program specific (private) data */
    SHT_SYMTAB	=2,		/**< Link editing symbol table */
    SHT_STRTAB	=3,		/**< A string table */
    SHT_RELA	=4,		/**< Relocation entries with addends */
    SHT_HASH	=5,		/**< A symbol hash table */
    SHT_DYNAMIC	=6,		/**< Information for dynamic linking */
    SHT_NOTE	=7,		/**< Information that marks file */
    SHT_NOBITS	=8,		/**< Section occupies no space in file */
    SHT_REL	=9,		/**< Relocation entries, no addends */
    SHT_SHLIB	=10,		/**< Reserved, unspecified semantics */
    SHT_DYNSYM	=11,		/**< Dynamic linking symbol table */
    SHT_NUM	=12,		/**< Number of defined types.  */

    SHT_LOSUNW	=0x6ffffffdUL,	/**< Sun-specific low bound.  */

/** The next three section types are defined by Solaris, and are named
   SHT_SUNW*.  We use them in GNU code, so we also define SHT_GNU*
   versions.  */
    SHT_SUNW_verdef	=0x6ffffffdUL,	/**< Versions defined by file */
    SHT_SUNW_verneed	=0x6ffffffeUL,	/**< Versions needed by file */
    SHT_SUNW_versym	=0x6fffffffUL,	/**< Symbol versions */

    SHT_GNU_verdef	=SHT_SUNW_verdef,
    SHT_GNU_verneed	=SHT_SUNW_verneed,
    SHT_GNU_versym	=SHT_SUNW_versym,

    SHT_LOPROC	=0x70000000UL,	/**< Processor-specific semantics, lo */
    SHT_HIPROC	=0x7FFFFFFFUL,	/**< Processor-specific semantics, hi */
    SHT_LOUSER	=0x80000000UL,	/**< Application-specific semantics */
    SHT_HIUSER	=0x8FFFFFFFUL	/**< Application-specific semantics */
};
/** Values for section header, sh_flags field */
enum {
    SHF_WRITE		=(1 << 0),	/**< Writable data during execution */
    SHF_ALLOC		=(1 << 1),	/**< Occupies memory during execution */
    SHF_EXECINSTR	=(1 << 2),	/**< Executable machine instructions */
    SHF_MASKPROC	=0xF0000000UL	/**< Processor-specific semantics */
};
/** Values of note segment descriptor types for core files. */
enum {
    NT_PRSTATUS	=1,		/**< Contains copy of prstatus struct */
    NT_FPREGSET	=2,		/**< Contains copy of fpregset struct */
    NT_PRPSINFO	=3,		/**< Contains copy of prpsinfo struct */
/** Values of note segment descriptor types for object files.
   (Only for hppa right now.  Should this be moved elsewhere?)  */

    NT_VERSION	=1		/**< Contains a version string.  */
};

/** These three macros disassemble and assemble a symbol table st_info field,
    which contains the symbol binding and symbol type.  The STB_ and STT_
    defines identify the binding and type. */

inline uint8_t ELF_ST_BIND(uint8_t val)			{ return val>>4; }
inline uint8_t ELF_ST_TYPE(uint8_t val)			{ return val&0xF; }
inline uint8_t ELF_ST_INFO(uint8_t bind,uint8_t type)	{ return (bind<<4)+(type&0xF); }
enum {
    STN_UNDEF	=0		/**< undefined symbol index */
};
enum {
    STB_LOCAL	=0,		/**< Symbol not visible outside obj */
    STB_GLOBAL	=1,		/**< Symbol visible outside obj */
    STB_WEAK	=2,		/**< Like globals, lower precedence */
    STB_NUM	=3,		/**< Number of defined types.  */
    STB_LOPROC	=13,		/**< Application-specific semantics */
    STB_HIPROC	=15		/**< Application-specific semantics */
};

enum {
    STT_NOTYPE	=0,		/**< Symbol type is unspecified */
    STT_OBJECT	=1,		/**< Symbol is a data object */
    STT_FUNC	=2,		/**< Symbol is a code object */
    STT_SECTION	=3,		/**< Symbol associated with a section */
    STT_FILE	=4,		/**< Symbol gives a file name */
    STT_NUM	=5,		/**< Number of defined types.  */
    STT_LOPROC	=13,		/**< Application-specific semantics */
    STT_HIPROC	=15		/**< Application-specific semantics */
};
/** Special section indices, which may show up in st_shndx fields, among
    other places. */
enum {
    SHN_UNDEF	=0,		/**< Undefined section reference */
    SHN_LORESERVE=0xFF00U,		/**< Begin range of reserved indices */
    SHN_LOPROC	=0xFF00U,		/**< Begin range of appl-specific */
    SHN_HIPROC	=0xFF1FU,		/**< End range of appl-specific */
    SHN_ABS	=0xFFF1U,		/**< Associated symbol is full */
    SHN_COMMON	=0xFFF2U,		/**< Associated symbol is in common */
    SHN_HIRESERVE=0xFFFFU		/**< End range of reserved indices */
};
/** relocation info handling macros */

/** Legal values for d_tag (dynamic entry type).  */
enum {
    DT_NULL	=0,		/**< Marks end of dynamic section */
    DT_NEEDED	=1,		/**< Name of needed library */
    DT_PLTRELSZ	=2,		/**< Size in bytes of PLT relocs */
    DT_PLTGOT	=3,		/**< Processor defined value */
    DT_HASH	=4,		/**< Address of symbol hash table */
    DT_STRTAB	=5,		/**< Address of string table */
    DT_SYMTAB	=6,		/**< Address of symbol table */
    DT_RELA	=7,		/**< Address of Rela relocs */
    DT_RELASZ	=8,		/**< Total size of Rela relocs */
    DT_RELAENT	=9,		/**< Size of one Rela reloc */
    DT_STRSZ	=10,		/**< Size of string table */
    DT_SYMENT	=11,		/**< Size of one symbol table entry */
    DT_INIT	=12,		/**< Address of init function */
    DT_FINI	=13,		/**< Address of termination function */
    DT_SONAME	=14,		/**< Name of shared object */
    DT_RPATH	=15,		/**< Library search path */
    DT_SYMBOLIC	=16,		/**< Start symbol search here */
    DT_REL	=17,		/**< Address of Rel relocs */
    DT_RELSZ	=18,		/**< Total size of Rel relocs */
    DT_RELENT	=19,		/**< Size of one Rel reloc */
    DT_PLTREL	=20,		/**< Type of reloc in PLT */
    DT_DEBUG	=21,		/**< For debugging; unspecified */
    DT_TEXTREL	=22,		/**< Reloc might modify .text */
    DT_JMPREL	=23,		/**< Address of PLT relocs */
    DT_NUM	=24,		/**< Number used */
    DT_LOPROC	=0x70000000UL,	/**< Start of processor-specific */
    DT_HIPROC	=0x7fffffffUL,	/**< End of processor-specific */
//    DT_PROCNUM	=DT_MIPS_NUM,	/**< Most used by any processor */
/** The versioning entry types.  The next are defined as part of the
   GNU extension.  */
    DT_VERSYM	=0x6ffffff0UL,

/** These were chosen by Sun.  */
    DT_VERDEF	=0x6ffffffcUL,	/**< Address of version definition
					   table */
    DT_VERDEFNUM=0x6ffffffdUL,	/**< Number of version definitions */
    DT_VERNEED	=0x6ffffffeUL,	/**< Address of table with needed
				   versions */
    DT_VERNEEDNUM=0x6fffffffUL,	/**< Number of needed versions */
    DT_VERSIONTAGNUM=16,

/** Sun added these machine-independent extensions in the "processor-specific"
   range.  Be compatible.  */
    DT_AUXILIARY    =0x7ffffffdUL,      /**< Shared object to load before self */
    DT_FILTER       =0x7fffffffUL,     /**< Shared object to get values from */
    DT_EXTRANUM	=3
};
#define DT_VERSIONTAGIDX(tag)	(DT_VERNEEDNUM - (tag))	/**< Reverse order! */
#define DT_EXTRATAGIDX(tag)	((tByte)-((Elf386_Sword) (tag) <<1>>1)-1)

/** These constants are used for the version number of a Elf386_Verdef
   structure.  */
enum {
    VER_DEF_NONE	=0,
    VER_DEF_CURRENT	=1,
    VER_DEF_NUM	        =2		/**< Given version number */
};
/** These constants appear in the vd_flags field of a Elf386_Verdef
   structure.  */
enum {
    VER_FLG_BASE	=0x1,
    VER_FLG_WEAK	=0x2
};
/** These special constants can be found in an Elf386_Versym field.  */
enum {
    VER_NDX_LOCAL	=0,
    VER_NDX_GLOBAL	=1
};
/** These constants are used for the version number of a Elf386_Verneed
   structure.  */
enum {
    VER_NEED_NONE	=0,
    VER_NEED_CURRENT	=1,
    VER_NEED_NUM	=2		/**< Given version number */
};
/** This flag appears in a Versym structure.  It means that the symbol
   is hidden, and is only visible with an explicit version number.
   This is a GNU extension.  */
enum {
    VERSYM_HIDDEN	=0x8000U,
/** This is the mask for the rest of the Versym information.  */
    VERSYM_VERSION	=0x7fffU
};
/** This is a special token which appears as part of a symbol name.  It
   indictes that the rest of the name is actually the name of a
   version node, and is not part of the actual name.  This is a GNU
   extension.  For example, the symbol name `stat@ver2' is taken to
   mean the symbol `stat' in version `ver2'.  */
enum {
    ELF_VER_CHR='@'
};

typedef struct {
  uint8_t	e_ident[16];		/**< ELF "magic number" */
  uint8_t	e_type[2];		/**< Identifies object file type */
  uint8_t	e_machine[2];		/**< Specifies required architecture */
  uint8_t	e_version[4];		/**< Identifies object file version */
  uint8_t	e_entry[4];		/**< Entry point virtual address */
  uint8_t	e_phoff[4];		/**< Program header table file offset */
  uint8_t	e_shoff[4];		/**< Section header table file offset */
  uint8_t	e_flags[4];		/**< Processor-specific flags */
  uint8_t	e_ehsize[2];		/**< ELF header size in bytes */
  uint8_t	e_phentsize[2];		/**< Program header table entry size */
  uint8_t	e_phnum[2];		/**< Program header table entry count */
  uint8_t	e_shentsize[2];		/**< Section header table entry size */
  uint8_t	e_shnum[2];		/**< Section header table entry count */
  uint8_t	e_shstrndx[2];		/**< Section header string table index */
} Elf386_External_Ehdr;

typedef struct {
  uint8_t	e_ident[16];		/**< ELF "magic number" */
  uint8_t	e_type[2];		/**< Identifies object file type */
  uint8_t	e_machine[2];		/**< Specifies required architecture */
  uint8_t	e_version[4];		/**< Identifies object file version */
  uint8_t	e_entry[8];		/**< Entry point virtual address */
  uint8_t	e_phoff[8];		/**< Program header table file offset */
  uint8_t	e_shoff[8];		/**< Section header table file offset */
  uint8_t	e_flags[4];		/**< Processor-specific flags */
  uint8_t	e_ehsize[2];		/**< ELF header size in bytes */
  uint8_t	e_phentsize[2];		/**< Program header table entry size */
  uint8_t	e_phnum[2];		/**< Program header table entry count */
  uint8_t	e_shentsize[2];		/**< Section header table entry size */
  uint8_t	e_shnum[2];		/**< Section header table entry count */
  uint8_t	e_shstrndx[2];		/**< Section header string table index */
} Elf64_External_Ehdr;

/** Program header */

typedef struct {
  uint8_t	p_type[4];		/**< Identifies program segment type */
  uint8_t	p_offset[4];		/**< Segment file offset */
  uint8_t	p_vaddr[4];		/**< Segment virtual address */
  uint8_t	p_paddr[4];		/**< Segment physical address (ignored on SystemV) */
  uint8_t	p_filesz[4];		/**< Segment size in file */
  uint8_t	p_memsz[4];		/**< Segment size in memory */
  uint8_t	p_flags[4];		/**< Segment flags */
  uint8_t	p_align[4];		/**< Segment alignment, file & memory */
} Elf386_External_Phdr;

typedef struct {
  uint8_t	p_type[4];		/**< Identifies program segment type */
  uint8_t	p_flags[4];		/**< Segment flags */
  uint8_t	p_offset[8];		/**< Segment file offset */
  uint8_t	p_vaddr[8];		/**< Segment virtual address */
  uint8_t	p_paddr[8];		/**< Segment physical address (ignored on SystemV)*/
  uint8_t	p_filesz[8];		/**< Segment size in file */
  uint8_t	p_memsz[8];		/**< Segment size in memory */
  uint8_t	p_align[8];		/**< Segment alignment, file & memory */
} Elf64_External_Phdr;

/** Section header */

typedef struct {
  uint8_t	sh_name[4];		/**< Section name, index in string tbl */
  uint8_t	sh_type[4];		/**< Type of section */
  uint8_t	sh_flags[4];		/**< Miscellaneous section attributes */
  uint8_t	sh_addr[4];		/**< Section virtual addr at execution */
  uint8_t	sh_offset[4];		/**< Section file offset */
  uint8_t	sh_size[4];		/**< Size of section in bytes */
  uint8_t	sh_link[4];		/**< Index of another section */
  uint8_t	sh_info[4];		/**< Additional section information */
  uint8_t	sh_addralign[4];	/**< Section alignment */
  uint8_t	sh_entsize[4];		/**< Entry size if section holds table */
} Elf386_External_Shdr;

typedef struct {
  uint8_t	sh_name[4];		/**< Section name, index in string tbl */
  uint8_t	sh_type[4];		/**< Type of section */
  uint8_t	sh_flags[8];		/**< Miscellaneous section attributes */
  uint8_t	sh_addr[8];		/**< Section virtual addr at execution */
  uint8_t	sh_offset[8];		/**< Section file offset */
  uint8_t	sh_size[8];		/**< Size of section in bytes */
  uint8_t	sh_link[4];		/**< Index of another section */
  uint8_t	sh_info[4];		/**< Additional section information */
  uint8_t	sh_addralign[8];	/**< Section alignment */
  uint8_t	sh_entsize[8];		/**< Entry size if section holds table */
} Elf64_External_Shdr;

/** Symbol table entry */

typedef struct {
  uint8_t	st_name[4];		/**< Symbol name, index in string tbl */
  uint8_t	st_value[4];		/**< Value of the symbol */
  uint8_t	st_size[4];		/**< Associated symbol size */
  uint8_t	st_info[1];		/**< Type and binding attributes */
  uint8_t	st_other[1];		/**< No defined meaning, 0 */
  uint8_t	st_shndx[2];		/**< Associated section index */
} Elf386_External_Sym;

typedef struct {
  uint8_t	st_name[4];		/**< Symbol name, index in string tbl */
  uint8_t	st_info[1];		/**< Type and binding attributes */
  uint8_t	st_other[1];		/**< No defined meaning, 0 */
  uint8_t	st_shndx[2];		/**< Associated section index */
  uint8_t	st_value[8];		/**< Value of the symbol */
  uint8_t	st_size[8];		/**< Associated symbol size */
} Elf64_External_Sym;

/** Note segments */

typedef struct {
  uint8_t	namesz[4];		/**< Size of entry's owner string */
  uint8_t	descsz[4];		/**< Size of the note descriptor */
  uint8_t	type[4];		/**< Interpretation of the descriptor */
  int8_t	name[1];		/**< Start of the name+desc data */
} Elf_External_Note;

/** Relocation Entries */
typedef struct {
  uint8_t	r_offset[4];	/**< Location at which to apply the action */
  uint8_t	r_info[4];	/**< index and type of relocation */
} Elf386_External_Rel;

typedef struct {
  uint8_t	r_offset[4];	/**< Location at which to apply the action */
  uint8_t	r_info[4];	/**< index and type of relocation */
  uint8_t	r_addend[4];	/**< Constant addend used to compute value */
} Elf386_External_Rela;

typedef struct {
  uint8_t	r_offset[8];	/**< Location at which to apply the action */
  uint8_t	r_info[8];	/**< index and type of relocation */
} Elf64_External_Rel;

typedef struct {
  uint8_t	r_offset[8];	/**< Location at which to apply the action */
  uint8_t	r_info[8];	/**< index and type of relocation */
  uint8_t	r_addend[8];	/**< Constant addend used to compute value */
} Elf64_External_Rela;

/** dynamic section structure */

typedef struct {
  uint8_t	d_tag[4];		/**< entry tag value */
  union {
    uint8_t	d_val[4];
    uint8_t	d_ptr[4];
  } d_un;
} Elf386_External_Dyn;

typedef struct {
  uint8_t	d_tag[8];		/**< entry tag value */
  union {
    uint8_t	d_val[8];
    uint8_t	d_ptr[8];
  } d_un;
} Elf64_External_Dyn;

/** The version structures are currently size independent.  They are
   named without a 32 or 64.  If that ever changes, these structures
   will need to be renamed.  */

/** This structure appears in a SHT_GNU_verdef section.  */

typedef struct {
  uint8_t		vd_version[2];
  uint8_t		vd_flags[2];
  uint8_t		vd_ndx[2];
  uint8_t		vd_cnt[2];
  uint8_t		vd_hash[4];
  uint8_t		vd_aux[4];
  uint8_t		vd_next[4];
} Elf_External_Verdef;

/** This structure appears in a SHT_GNU_verdef section.  */

typedef struct {
  uint8_t		vda_name[4];
  uint8_t		vda_next[4];
} Elf_External_Verdaux;

/** This structure appears in a SHT_GNU_verneed section.  */

typedef struct {
  uint8_t		vn_version[2];
  uint8_t		vn_cnt[2];
  uint8_t		vn_file[4];
  uint8_t		vn_aux[4];
  uint8_t		vn_next[4];
} Elf_External_Verneed;

/** This structure appears in a SHT_GNU_verneed section.  */

typedef struct {
  uint8_t		vna_hash[4];
  uint8_t		vna_flags[2];
  uint8_t		vna_other[2];
  uint8_t		vna_name[4];
  uint8_t		vna_next[4];
} Elf_External_Vernaux;

/** This structure appears in a SHT_GNU_versym section.  This is not a
   standard ELF structure; ELF just uses Elf386_Half.  */

typedef struct {
  uint8_t		vs_vers[2];
} Elf_External_Versym;

/** Auxiliary vector.  */

/** This vector is normally only used by the program interpreter.  The
   usual definition in an ABI supplement uses the name auxv_t.  The
   vector is not usually defined in a standard <elf.h> file, but it
   can't hurt.  We rename it to avoid conflicts.  The sizes of these
   types are an arrangement between the exec server and the program
   interpreter, so we don't fully specify them here.  */

typedef struct
{
  int32_t a_type;		/**< Entry type */
  union
    {
      int32_t a_val;		/**< Integer value */
      any_t*a_ptr;		/**< Pointer value */
      void (*a_fcn) ();	/**< Function pointer value */
    } a_un;
} Elf386_auxv_t;

typedef struct
{
  int32_t a_type;		/**< Entry type */
  union
    {
      int32_t a_val;		/**< Integer value */
      any_t*a_ptr;		/**< Pointer value */
      void (*a_fcn) ();	/**< Function pointer value */
    } a_un;
} Elf64_auxv_t;

/** Legal values for a_type (entry type).  */
enum {
    AT_NULL	=0,		/**< End of vector */
    AT_IGNORE	=1,		/**< Entry should be ignored */
    AT_EXECFD	=2,		/**< File descriptor of program */
    AT_PHDR	=3,		/**< Program headers for program */
    AT_PHENT	=4,		/**< Size of program header entry */
    AT_PHNUM	=5,		/**< Number of program headers */
    AT_PAGESZ	=6,		/**< System page size */
    AT_BASE	=7,		/**< Base address of interpreter */
    AT_FLAGS	=8,		/**< Flags */
    AT_ENTRY	=9,		/**< Entry point of program */
    AT_NOTELF	=10,		/**< Program is not ELF */
    AT_UID	=11,		/**< Real uid */
    AT_EUID	=12,		/**< Effective uid */
    AT_GID	=13,		/**< Real gid */
    AT_EGID	=14		/**< Effective gid */
};
/** Motorola 68k specific definitions.  */

/** m68k relocs.  */
enum {
    R_68K_NONE	=0,		/**< No reloc */
    R_68K_32	=1,		/**< Direct 32 bit  */
    R_68K_16	=2,		/**< Direct 16 bit  */
    R_68K_8	=3,		/**< Direct 8 bit  */
    R_68K_PC32	=4,		/**< PC relative 32 bit */
    R_68K_PC16	=5,		/**< PC relative 16 bit */
    R_68K_PC8	=6,		/**< PC relative 8 bit */
    R_68K_GOT32	=7,		/**< 32 bit PC relative GOT entry */
    R_68K_GOT16	=8,		/**< 16 bit PC relative GOT entry */
    R_68K_GOT8	=9,		/**< 8 bit PC relative GOT entry */
    R_68K_GOT32O=10,		/**< 32 bit GOT offset */
    R_68K_GOT16O=11,		/**< 16 bit GOT offset */
    R_68K_GOT8O	=12,		/**< 8 bit GOT offset */
    R_68K_PLT32	=13,		/**< 32 bit PC relative PLT address */
    R_68K_PLT16	=14,		/**< 16 bit PC relative PLT address */
    R_68K_PLT8	=15,		/**< 8 bit PC relative PLT address */
    R_68K_PLT32O=16,		/**< 32 bit PLT offset */
    R_68K_PLT16O=17,		/**< 16 bit PLT offset */
    R_68K_PLT8O	=18,		/**< 8 bit PLT offset */
    R_68K_COPY	=19,		/**< Copy symbol at runtime */
    R_68K_GLOB_DAT=20,		/**< Create GOT entry */
    R_68K_JMP_SLOT=21,		/**< Create PLT entry */
    R_68K_RELATIVE=22,		/**< Adjust by program base */
    R_68K_NUM	=23
};

/* PowerPC related declarations */
enum {
    R_PPC64_ADDR64		=38,
    R_PPC64_ADDR16_HIGHER	=39,
    R_PPC64_ADDR16_HIGHERA	=40,
    R_PPC64_ADDR16_HIGHEST	=41,
    R_PPC64_ADDR16_HIGHESTA	=42,
    R_PPC64_UADDR64		=43,
    R_PPC64_REL64		=44,
    R_PPC64_PLT64		=45,
    R_PPC64_PLTREL64		=46,
    R_PPC64_TOC16		=47,
    R_PPC64_TOC16_LO		=48,
    R_PPC64_TOC16_HI		=49,
    R_PPC64_TOC16_HA		=50,
    R_PPC64_TOC			=51,
    R_PPC64_PLTGOT16		=52,
    R_PPC64_PLTGOT16_LO		=53,
    R_PPC64_PLTGOT16_HI		=54,
    R_PPC64_PLTGOT16_HA		=55,

  /* The following relocs were added in the 64-bit PowerPC ELF ABI
     revision 1.2. */
    R_PPC64_ADDR16_DS	=56,
    R_PPC64_ADDR16_LO_DS=57,
    R_PPC64_GOT16_DS	=58,
    R_PPC64_GOT16_LO_DS	=59,
    R_PPC64_PLT16_LO_DS	=60,
    R_PPC64_SECTOFF_DS	=61,
    R_PPC64_SECTOFF_LO_DS=62,
    R_PPC64_TOC16_DS	=63,
    R_PPC64_TOC16_LO_DS	=64,
    R_PPC64_PLTGOT16_DS	=65,
    R_PPC64_PLTGOT16_LO_DS=66
};
enum {
    R_PPC_NONE		  =0,
    R_PPC_ADDR32	  =1,
    R_PPC_ADDR24	  =2,
    R_PPC_ADDR16	  =3,
    R_PPC_ADDR16_LO	  =4,
    R_PPC_ADDR16_HI	  =5,
    R_PPC_ADDR16_HA	  =6,
    R_PPC_ADDR14	  =7,
    R_PPC_ADDR14_BRTAKEN  =8,
    R_PPC_ADDR14_BRNTAKEN =9,
    R_PPC_REL24		 =10,
    R_PPC_REL14		 =11,
    R_PPC_REL14_BRTAKEN	 =12,
    R_PPC_REL14_BRNTAKEN =13,
    R_PPC_GOT16		 =14,
    R_PPC_GOT16_LO	 =15,
    R_PPC_GOT16_HI	 =16,
    R_PPC_GOT16_HA	 =17,
    R_PPC_PLTREL24	 =18,
    R_PPC_COPY		 =19,
    R_PPC_GLOB_DAT	 =20,
    R_PPC_JMP_SLOT	 =21,
    R_PPC_RELATIVE	 =22,
    R_PPC_LOCAL24PC	 =23,
    R_PPC_UADDR32	 =24,
    R_PPC_UADDR16	 =25,
    R_PPC_REL32		 =26,
    R_PPC_PLT32		 =27,
    R_PPC_PLTREL32	 =28,
    R_PPC_PLT16_LO	 =29,
    R_PPC_PLT16_HI	 =30,
    R_PPC_PLT16_HA	 =31,
    R_PPC_SDAREL16	 =32,
    R_PPC_SECTOFF	 =33,
    R_PPC_SECTOFF_LO	 =34,
    R_PPC_SECTOFF_HI	 =35,
    R_PPC_SECTOFF_HA	 =36,
    R_PPC_ADDR30	 =37,
  /* Relocs added to support TLS.  */
    R_PPC_TLS		 =67,
    R_PPC_DTPMOD32	 =68,
    R_PPC_TPREL16	 =69,
    R_PPC_TPREL16_LO	 =70,
    R_PPC_TPREL16_HI	 =71,
    R_PPC_TPREL16_HA	 =72,
    R_PPC_TPREL32	 =73,
    R_PPC_DTPREL16	 =74,
    R_PPC_DTPREL16_LO	 =75,
    R_PPC_DTPREL16_HI	 =76,
    R_PPC_DTPREL16_HA	 =77,
    R_PPC_DTPREL32	 =78,
    R_PPC_GOT_TLSGD16	 =79,
    R_PPC_GOT_TLSGD16_LO =80,
    R_PPC_GOT_TLSGD16_HI =81,
    R_PPC_GOT_TLSGD16_HA =82,
    R_PPC_GOT_TLSLD16	 =83,
    R_PPC_GOT_TLSLD16_LO =84,
    R_PPC_GOT_TLSLD16_HI =85,
    R_PPC_GOT_TLSLD16_HA =86,
    R_PPC_GOT_TPREL16	 =87,
    R_PPC_GOT_TPREL16_LO =88,
    R_PPC_GOT_TPREL16_HI =89,
    R_PPC_GOT_TPREL16_HA =90,
    R_PPC_GOT_DTPREL16	 =91,
    R_PPC_GOT_DTPREL16_LO =92,
    R_PPC_GOT_DTPREL16_HI =93,
    R_PPC_GOT_DTPREL16_HA =94,

/* The remaining relocs are from the Embedded ELF ABI and are not
   in the SVR4 ELF ABI.  */
    R_PPC_EMB_NADDR32	=101,
    R_PPC_EMB_NADDR16	=102,
    R_PPC_EMB_NADDR16_LO=103,
    R_PPC_EMB_NADDR16_HI=104,
    R_PPC_EMB_NADDR16_HA=105,
    R_PPC_EMB_SDAI16	=106,
    R_PPC_EMB_SDA2I16	=107,
    R_PPC_EMB_SDA2REL	=108,
    R_PPC_EMB_SDA21	=109,
    R_PPC_EMB_MRKREF	=110,
    R_PPC_EMB_RELSEC16	=111,
    R_PPC_EMB_RELST_LO	=112,
    R_PPC_EMB_RELST_HI	=113,
    R_PPC_EMB_RELST_HA	=114,
    R_PPC_EMB_BIT_FLD	=115,
    R_PPC_EMB_RELSDA	=116,

/* Fake relocations for branch stubs only used internally by ld.  */
    R_PPC_RELAX32	=245,
    R_PPC_RELAX32PC	=246,
    R_PPC_RELAX32_PLT	=247,
    R_PPC_RELAX32PC_PLT	=248,

/* These are GNU extensions used in PIC code sequences.  */
    R_PPC_REL16		=249,
    R_PPC_REL16_LO	=250,
    R_PPC_REL16_HI	=251,
    R_PPC_REL16_HA	=252,

/* These are GNU extensions to enable C++ vtable garbage collection.  */
    R_PPC_GNU_VTINHERIT	=253,
    R_PPC_GNU_VTENTRY	=254,

/* This is a phony reloc to handle any old fashioned TOC16 references
   that may still be in object files.  */
    R_PPC_TOC16		=255
};



/** Intel 80386 specific definitions.  */
/** 386 relocs.  */
enum {
    R_386_NONE	=0,		/**< No reloc */
    R_386_32	=1,		/**< Direct 32 bit  */
    R_386_PC32	=2,		/**< PC relative 32 bit */
    R_386_GOT32	=3,		/**< 32 bit GOT entry */
    R_386_PLT32	=4,		/**< 32 bit PLT address */
    R_386_COPY	=5,		/**< Copy symbol at runtime */
    R_386_GLOB_DAT=6,		/**< Create GOT entry */
    R_386_JMP_SLOT=7,		/**< Create PLT entry */
    R_386_RELATIVE=8,		/**< Adjust by program base */
    R_386_GOTOFF=9,		/**< 32 bit offset to GOT */
    R_386_GOTPC	=10,		/**< 32 bit PC relative offset to GOT */
    R_386_NUM	=11,
    R_386_GNU_16=20,		/**< Direct 16 bit  */
    R_386_GNU_PC16=21,		/**< PC relative 16 bit */
    R_386_GNU_8	=22,	/**< Direct 8 bit  */
    R_386_GNU_PC8=23,		/**< PC relative 8 bit */
    R_386_GNU_max=24
};
/** AMD64 specific definitions */
enum {
    R_X86_64_NONE	=0,	/* No reloc */
    R_X86_64_64		=1,	/* Direct 64 bit  */
    R_X86_64_PC32	=2,	/* PC relative 32 bit signed */
    R_X86_64_GOT32	=3,	/* 32 bit GOT entry */
    R_X86_64_PLT32	=4,	/* 32 bit PLT address */
    R_X86_64_COPY	=5,	/* Copy symbol at runtime */
    R_X86_64_GLOB_DAT	=6,	/* Create GOT entry */
    R_X86_64_JUMP_SLOT	=7,	/* Create PLT entry */
    R_X86_64_RELATIVE	=8,	/* Adjust by program base */
    R_X86_64_GOTPCREL	=9,	/* 32 bit signed pc relative offset to GOT entry */
    R_X86_64_32		=10,	/* Direct 32 bit zero extended */
    R_X86_64_32S	=11,	/* Direct 32 bit sign extended */
    R_X86_64_16		=12,	/* Direct 16 bit zero extended */
    R_X86_64_PC16	=13,	/* 16 bit sign extended pc relative*/
    R_X86_64_8		=14,	/* Direct 8 bit sign extended */
    R_X86_64_PC8	=15,	/* 8 bit sign extended pc relative*/
    R_X86_64_DTPMOD64	=16,	/* ID of module containing symbol */
    R_X86_64_DTPOFF64	=17,	/* Offset in TLS block */
    R_X86_64_TPOFF64	=18,	/* Offset in initial TLS block */
    R_X86_64_TLSGD	=19,	/* PC relative offset to GD GOT block */
    R_X86_64_TLSLD	=20,	/* PC relative offset to LD GOT block */
    R_X86_64_DTPOFF32	=21,	/* Offset in TLS block */
    R_X86_64_GOTTPOFF	=22,	/* PC relative offset to IE GOT entry */
    R_X86_64_TPOFF32	=23,	/* Offset in initial TLS block */
    R_X86_64_PC64	=24,	/* PC relative 64 bit */
    R_X86_64_GOTOFF64	=25,	/* 64 bit offset to GOT */
    R_X86_64_GOTPC32	=26,	/* 32 bit signed pc relative offset to GOT */
    R_X86_64_GOT64	=27,	/* 64 bit GOT entry offset */
    R_X86_64_GOTPCREL64	=28,	/* 64 bit signed pc relative offset to GOT entry */
    R_X86_64_GOTPC64	=29,	/* 64 bit signed pc relative offset to GOT */
    R_X86_64_GOTPLT64	=30,	/* like GOT64, but indicates that PLT entry is needed */
    R_X86_64_PLTOFF64	=31,	/* 64 bit GOT relative offset to PLT entry */
     /* 32 .. 33 */
    R_X86_64_GOTPC32_TLSDESC=34, /* 32 bit signed pc relative offset to TLS descriptor in the GOT.  */
    R_X86_64_TLSDESC_CALL=35, /* Relaxable call through TLS descriptor.  */
    R_X86_64_TLSDESC=36,	/* 2x64-bit TLS descriptor.  */
    R_X86_64_GNU_VTINHERIT=250, /* GNU C++ hack  */
    R_X86_64_GNU_VTENTRY=251 /* GNU C++ hack  */
};

/** SUN SPARC specific definitions.  */

/** SPARC relocs.  */
enum {
    R_SPARC_NONE	=0,		/**< No reloc */
    R_SPARC_8		=1,		/**< Direct 8 bit */
    R_SPARC_16		=2,		/**< Direct 16 bit */
    R_SPARC_32		=3,		/**< Direct 32 bit */
    R_SPARC_DISP8	=4,		/**< PC relative 8 bit */
    R_SPARC_DISP16	=5,		/**< PC relative 16 bit */
    R_SPARC_DISP32	=6,		/**< PC relative 32 bit */
    R_SPARC_WDISP30	=7,		/**< PC relative 30 bit shifted */
    R_SPARC_WDISP22	=8,		/**< PC relative 22 bit shifted */
    R_SPARC_HI22	=9,		/**< High 22 bit */
    R_SPARC_22		=10,		/**< Direct 22 bit */
    R_SPARC_13		=11,		/**< Direct 13 bit */
    R_SPARC_LO10	=12,		/**< Truncated 10 bit */
    R_SPARC_GOT10	=13,		/**< Truncated 10 bit GOT entry */
    R_SPARC_GOT13	=14,		/**< 13 bit GOT entry */
    R_SPARC_GOT22	=15,		/**< 22 bit GOT entry shifted */
    R_SPARC_PC10	=16,		/**< PC relative 10 bit truncated */
    R_SPARC_PC22	=17,		/**< PC relative 22 bit shifted */
    R_SPARC_WPLT30	=18,		/**< 30 bit PC relative PLT address */
    R_SPARC_COPY	=19,		/**< Copy symbol at runtime */
    R_SPARC_GLOB_DAT	=20,		/**< Create GOT entry */
    R_SPARC_JMP_SLOT	=21,		/**< Create PLT entry */
    R_SPARC_RELATIVE	=22,		/**< Adjust by program base */
    R_SPARC_UA32	=23,		/**< Direct 32 bit unaligned */
    R_SPARC_NUM		=24
};
/** MIPS R3000 specific definitions.  */

/** Legal values for e_flags field of Elf386_Ehdr.  */
enum {
    EF_MIPS_NOREORDER	=1,		/**< A .noreorder directive was used */
    EF_MIPS_PIC		=2,		/**< Contains PIC code */
    EF_MIPS_CPIC	=4,		/**< Uses PIC calling sequence */
    EF_MIPS_ARCH	=0xf0000000U	/**< MIPS architecture level */
};
/** Legal values for MIPS architecture level.  */
enum {
    E_MIPS_ARCH_1	=0x00000000UL,	/**< -mips1 code.  */
    E_MIPS_ARCH_2	=0x10000000UL,	/**< -mips2 code.  */
    E_MIPS_ARCH_3	=0x20000000UL	/**< -mips3 code.  */
};
/** Special section indices.  */
enum {
    SHN_MIPS_ACOMMON	=0xff00,		/**< Allocated common symbols */
    SHN_MIPS_TEXT	=0xff01,		/**< Allocated test symbols.  */
    SHN_MIPS_DATA	=0xff02,		/**< Allocated data symbols.  */
    SHN_MIPS_SCOMMON	=0xff03,		/**< Small common symbols */
    SHN_MIPS_SUNDEFINED	=0xff04	/**< Small undefined symbols */
};
/** Legal values for sh_type field of Elf386_Shdr.  */
enum {
    SHT_MIPS_LIBLIST  =0x70000000UL,	/**< Shared objects used in link */
    SHT_MIPS_CONFLICT =0x70000002UL,	/**< Conflicting symbols */
    SHT_MIPS_GPTAB    =0x70000003UL,	/**< Global data area sizes */
    SHT_MIPS_UCODE    =0x70000004UL,	/**< Reserved for SGI/MIPS compilers */
    SHT_MIPS_DEBUG    =0x70000005UL,	/**< MIPS ECOFF debugging information */
    SHT_MIPS_REGINFO  =0x70000006UL,	/**< Register usage information */
    SHT_MIPS_OPTIONS  =0x7000000dUL,	/**< Miscellaneous options.  */
    SHT_MIPS_DWARF    =0x7000001eUL,	/**< DWARF debugging information.  */
    SHT_MIPS_EVENTS   =0x70000021UL	/**< Event section.  */
};
/** Legal values for sh_flags field of Elf386_Shdr.  */
enum {
    SHF_MIPS_GPREL	=0x10000000UL	/**< Must be part of global data area */
};
/** Entries found in sections of type SHT_MIPS_GPTAB.  */

typedef union
{
  struct
    {
      uint8_t gt_current_g_value[4];	/**< -G value used for compilation */
      uint8_t gt_unused[4];		/**< Not used */
    } gt_header;        	   	/**< First entry in section */
  struct
    {
      uint8_t gt_g_value[4];		/**< If this value were used for -G */
      uint8_t gt_bytes[4];		/**< This many bytes would be used */
    } gt_entry;				/**< Subsequent entries in section */
} Elf386_gptab;

/** Entry found in sections of type SHT_MIPS_REGINFO.  */

typedef struct
{
  uint8_t	ri_gprmask[4];		/**< General registers used */
  uint8_t	ri_cprmask[4][4];	/**< Coprocessor registers used */
  uint8_t	ri_gp_value[4];		/**< $gp register value */
} Elf386_RegInfo;

/** MIPS relocs.  */
enum {
    R_MIPS_NONE	=0,		/**< No reloc */
    R_MIPS_16	=1,		/**< Direct 16 bit */
    R_MIPS_32	=2,		/**< Direct 32 bit */
    R_MIPS_REL32=3,		/**< PC relative 32 bit */
    R_MIPS_26	=4,		/**< Direct 26 bit shifted */
    R_MIPS_HI16	=5,		/**< High 16 bit */
    R_MIPS_LO16	=6,		/**< Low 16 bit */
    R_MIPS_GPREL16=7,		/**< GP relative 16 bit */
    R_MIPS_LITERAL=8,		/**< 16 bit literal entry */
    R_MIPS_GOT16=9,		/**< 16 bit GOT entry */
    R_MIPS_PC16=10,		/**< PC relative 16 bit */
    R_MIPS_CALL16=11,		/**< 16 bit GOT entry for function */
    R_MIPS_GPREL32=12,		/**< GP relative 32 bit */
    R_MIPS_NUM	=13
};
/** Legal values for p_type field of Elf386_Phdr.  */
enum {
    PT_MIPS_REGINFO	=0x70000000	/**< Register usage information */
};
/** Legal values for d_tag field of Elf386_Dyn.  */
enum {
    DT_MIPS_RLD_VERSION  =0x70000001UL, /**< Runtime linker interface version */
    DT_MIPS_TIME_STAMP   =0x70000002UL, /**< Timestamp */
    DT_MIPS_ICHECKSUM    =0x70000003UL, /**< Checksum */
    DT_MIPS_IVERSION     =0x70000004UL, /**< Version string (string tbl index) */
    DT_MIPS_FLAGS	 =0x70000005UL, /**< Flags */
    DT_MIPS_BASE_ADDRESS =0x70000006UL, /**< Base address */
    DT_MIPS_CONFLICT     =0x70000008UL, /**< Address of CONFLICT section */
    DT_MIPS_LIBLIST	 =0x70000009UL, /**< Address of LIBLIST section */
    DT_MIPS_LOCAL_GOTNO  =0x7000000aUL, /**< Number of local GOT entries */
    DT_MIPS_CONFLICTNO   =0x7000000bUL, /**< Number of CONFLICT entries */
    DT_MIPS_LIBLISTNO    =0x70000010UL, /**< Number of LIBLIST entries */
    DT_MIPS_SYMTABNO     =0x70000011UL, /**< Number of DYNSYM entries */
    DT_MIPS_UNREFEXTNO   =0x70000012UL, /**< First external DYNSYM */
    DT_MIPS_GOTSYM	 =0x70000013UL, /**< First GOT entry in DYNSYM */
    DT_MIPS_HIPAGENO     =0x70000014UL, /**< Number of GOT page table entries */
    DT_MIPS_RLD_MAP	 =0x70000016UL, /**< Address of run time loader map.  */
    DT_MIPS_NUM	         =0x17
};
/** Legal values for DT_MIPS_FLAG Elf386_Dyn entry.  */
enum {
    RHF_NONE		   =0,		/**< No flags */
    RHF_QUICKSTART	   =(1 << 0),	/**< Use quickstart */
    RHF_NOTPOT		   =(1 << 1),	/**< Hash size not power of 2 */
    RHF_NO_LIBRARY_REPLACEMENT=(1 << 2)	/**< Ignore LD_LIBRARY_PATH */
};
/** Entries found in sections of type SHT_MIPS_LIBLIST.  */

typedef struct
{
  uint8_t	l_name[4];		/**< Name (string table index) */
  uint8_t	l_time_stamp[4];	/**< Timestamp */
  uint8_t	l_checksum[4];		/**< Checksum */
  uint8_t	l_version[4];		/**< Interface version */
  uint8_t	l_flags[4];		/**< Flags */
} Elf386_Lib;

/** Legal values for l_flags.  */
enum {
    LL_EXACT_MATCH   =(1 << 0),	/**< Require exact match */
    LL_IGNORE_INT_VER=(1 << 1)	/**< Ignore interface version */
};
/** HPPA specific definitions.  */

/** Legal values for sh_type field of Elf386_Shdr.  */
enum {
    SHT_PARISC_GOT	=0x70000000UL, /**< GOT for external data.  */
    SHT_PARISC_ARCH	=0x70000001UL, /**< Architecture extensions.  */
    SHT_PARISC_GLOBAL	=0x70000002UL, /**< Definition of $global$.  */
    SHT_PARISC_MILLI	=0x70000003UL, /**< Millicode routines.  */
    SHT_PARISC_UNWIND	=0x70000004UL, /**< Unwind information.  */
    SHT_PARISC_PLT	=0x70000005UL, /**< Procedure linkage table.  */
    SHT_PARISC_SDATA	=0x70000006UL, /**< Short initialized data.  */
    SHT_PARISC_SBSS	=0x70000007UL, /**< Short uninitialized data.  */
    SHT_PARISC_SYMEXTN	=0x70000008UL, /**< Argument/relocation info.  */
    SHT_PARISC_STUBS	=0x70000009UL  /**< Linker stubs.  */
};
/** Legal values for sh_flags field of Elf386_Shdr.  */
enum {
    SHF_PARISC_SHORT	=0x20000000UL /**< Section with short addressing. */
};
/** Legal values for ST_TYPE subfield of st_info (symbol type).  */
enum {
    STT_PARISC_MILLICODE=13	/**< Millicode function entry point.  */
};

/** Alpha specific declarations.  */

/** Alpha relocs.  */
enum {
    R_ALPHA_NONE		=0,	/**< No reloc */
    R_ALPHA_REFLONG		=1,	/**< Direct 32 bit */
    R_ALPHA_REFQUAD		=2,	/**< Direct 64 bit */
    R_ALPHA_GPREL32		=3,	/**< GP relative 32 bit */
    R_ALPHA_LITERAL		=4,	/**< GP relative 16 bit w/optimization */
    R_ALPHA_LITUSE		=5,	/**< Optimization hint for LITERAL */
    R_ALPHA_GPDISP		=6,	/**< Add displacement to GP */
    R_ALPHA_BRADDR		=7,	/**< PC+4 relative 23 bit shifted */
    R_ALPHA_HINT		=8,	/**< PC+4 relative 16 bit shifted */
    R_ALPHA_SREL16		=9,	/**< PC relative 16 bit */
    R_ALPHA_SREL32		=10,	/**< PC relative 32 bit */
    R_ALPHA_SREL64		=11,	/**< PC relative 64 bit */
    R_ALPHA_OP_PUSH		=12,	/**< OP stack push */
    R_ALPHA_OP_STORE		=13,	/**< OP stack pop and store */
    R_ALPHA_OP_PSUB		=14,	/**< OP stack subtract */
    R_ALPHA_OP_PRSHIFT		=15,	/**< OP stack right shift */
    R_ALPHA_GPVALUE		=16,
    R_ALPHA_GPRELHIGH		=17,
    R_ALPHA_GPRELLOW		=18,
    R_ALPHA_IMMED_GP_16		=19,
    R_ALPHA_IMMED_GP_HI32	=20,
    R_ALPHA_IMMED_SCN_HI32	=21,
    R_ALPHA_IMMED_BR_HI32	=22,
    R_ALPHA_IMMED_LO32		=23,
    R_ALPHA_COPY		=24,	/**< Copy symbol at runtime */
    R_ALPHA_GLOB_DAT		=25,	/**< Create GOT entry */
    R_ALPHA_JMP_SLOT		=26,	/**< Create PLT entry */
    R_ALPHA_RELATIVE		=27,	/**< Adjust by program base */
    R_ALPHA_NUM			=28
};
/* ARM soecific declaratoins */
enum {
    R_ARM_NONE		  =0,
    R_ARM_PC24		  =1,  /* deprecated */
    R_ARM_ABS32		  =2,
    R_ARM_REL32		  =3,
    R_ARM_LDR_PC_G0	  =4,
    R_ARM_ABS16		  =5,
    R_ARM_ABS12		  =6,
    R_ARM_THM_ABS5	  =7,
    R_ARM_ABS8		  =8,
    R_ARM_SBREL32	  =9,
    R_ARM_THM_CALL	 =10,
    R_ARM_THM_PC8	 =11,
    R_ARM_BREL_ADJ	 =12,
    R_ARM_SWI24		 =13,  /* obsolete */
    R_ARM_THM_SWI8	 =14,  /* obsolete */
    R_ARM_XPC25		 =15,  /* obsolete */
    R_ARM_THM_XPC22	 =16,  /* obsolete */
    R_ARM_TLS_DTPMOD32	 =17,
    R_ARM_TLS_DTPOFF32	 =18,
    R_ARM_TLS_TPOFF32	 =19,
    R_ARM_COPY		 =20,   /* Copy symbol at runtime.  */
    R_ARM_GLOB_DAT	 =21,   /* Create GOT entry.  */
    R_ARM_JUMP_SLOT	 =22,   /* Create PLT entry.  */
    R_ARM_RELATIVE	 =23,   /* Adjust by program base.  */
    R_ARM_GOTOFF32	 =24,   /* 32 bit offset to GOT.  */
    R_ARM_BASE_PREL	 =25,   /* 32 bit PC relative offset to GOT.  */
    R_ARM_GOT_BREL	 =26,   /* 32 bit GOT entry.  */
    R_ARM_PLT32		 =27,   /* deprecated - 32 bit PLT address.  */
    R_ARM_CALL		 =28,
    R_ARM_JUMP24	 =29,
    R_ARM_THM_JUMP24	 =30,
    R_ARM_BASE_ABS	 =31,
    R_ARM_ALU_PCREL7_0	 =32,  /* obsolete */
    R_ARM_ALU_PCREL15_8	 =33,  /* obsolete */
    R_ARM_ALU_PCREL23_15 =34,  /* obsolete */
    R_ARM_LDR_SBREL_11_0 =35,  /* deprecated should have _NC suffix */
    R_ARM_ALU_SBREL_19_12=36,   /* deprecated should have _NC suffix */
    R_ARM_ALU_SBREL_27_20=37,   /* deprecated should have _CK suffix */
    R_ARM_TARGET1	 =38,
    R_ARM_SBREL31	 =39,   /* deprecated */
    R_ARM_V4BX		 =40,
    R_ARM_TARGET2	 =41,
    R_ARM_PREL31	 =42,
    R_ARM_MOVW_ABS_NC	 =43,
    R_ARM_MOVT_ABS	 =44,
    R_ARM_MOVW_PREL_NC	 =45,
    R_ARM_MOVT_PREL	 =46,
    R_ARM_THM_MOVW_ABS_NC=47,
    R_ARM_THM_MOVT_ABS	 =48,
    R_ARM_THM_MOVW_PREL_NC=49,
    R_ARM_THM_MOVT_PREL	 =50,
    R_ARM_THM_JUMP19	 =51,
    R_ARM_THM_JUMP6	 =52,
    R_ARM_THM_ALU_PREL_11_0=53,
    R_ARM_THM_PC12	 =54,
    R_ARM_ABS32_NOI	 =55,
    R_ARM_REL32_NOI	 =56,
    R_ARM_ALU_PC_G0_NC	 =57,
    R_ARM_ALU_PC_G0	 =58,
    R_ARM_ALU_PC_G1_NC	 =59,
    R_ARM_ALU_PC_G1	 =60,
    R_ARM_ALU_PC_G2	 =61,
    R_ARM_LDR_PC_G1	 =62,
    R_ARM_LDR_PC_G2	 =63,
    R_ARM_LDRS_PC_G0	 =64,
    R_ARM_LDRS_PC_G1	 =65,
    R_ARM_LDRS_PC_G2	 =66,
    R_ARM_LDC_PC_G0	 =67,
    R_ARM_LDC_PC_G1	 =68,
    R_ARM_LDC_PC_G2	 =69,
    R_ARM_ALU_SB_G0_NC	 =70,
    R_ARM_ALU_SB_G0	 =71,
    R_ARM_ALU_SB_G1_NC	 =72,
    R_ARM_ALU_SB_G1	 =73,
    R_ARM_ALU_SB_G2	 =74,
    R_ARM_LDR_SB_G0	 =75,
    R_ARM_LDR_SB_G1	 =76,
    R_ARM_LDR_SB_G2	 =77,
    R_ARM_LDRS_SB_G0	 =78,
    R_ARM_LDRS_SB_G1	 =79,
    R_ARM_LDRS_SB_G2	 =80,
    R_ARM_LDC_SB_G0	 =81,
    R_ARM_LDC_SB_G1	 =82,
    R_ARM_LDC_SB_G2	 =83,
    R_ARM_MOVW_BREL_NC	 =84,
    R_ARM_MOVT_BREL	 =85,
    R_ARM_MOVW_BREL	 =86,
    R_ARM_THM_MOVW_BREL_NC=87,
    R_ARM_THM_MOVT_BREL	 =88,
    R_ARM_THM_MOVW_BREL	 =89,
  /* 90-93 unallocated */
    R_ARM_PLT32_ABS	 =94,
    R_ARM_GOT_ABS	 =95,
    R_ARM_GOT_PREL	 =96,
    R_ARM_GOT_BREL12	 =97,
    R_ARM_GOTOFF12	 =98,
    R_ARM_GOTRELAX	 =99,
    R_ARM_GNU_VTENTRY	=100,   /* deprecated - old C++ abi */
    R_ARM_GNU_VTINHERIT	=101,   /* deprecated - old C++ abi */
    R_ARM_THM_JUMP11	=102,
    R_ARM_THM_JUMP8	=103,
    R_ARM_TLS_GD32	=104,
    R_ARM_TLS_LDM32	=105,
    R_ARM_TLS_LDO32	=106,
    R_ARM_TLS_IE32	=107,
    R_ARM_TLS_LE32	=108,
    R_ARM_TLS_LDO12	=109,
    R_ARM_TLS_LE12	=110,
    R_ARM_TLS_IE12GP	=111,
  /* 112 - 127 private range */
    R_ARM_ME_TOO	=128,   /* obsolete */

  /* Extensions?  R=read-only?  */
    R_ARM_RXPC25	=249,
    R_ARM_RSBREL32	=250,
    R_ARM_THM_RPC22	=251,
    R_ARM_RREL32	=252,
    R_ARM_RABS32	=253,
    R_ARM_RPC24		=254,
    R_ARM_RBASE		=255
};
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

    struct Elf_Ehdr {
	uint8_t		e_ident[16];	/**< ELF "magic number" */
	uint16_t	e_type;		/**< Identifies object file type */
	uint16_t	e_machine;	/**< Specifies required architecture */
	uint32_t	e_version;	/**< Identifies object file version */
	uint64_t	e_entry;	/**< Entry point virtual address */
	uint64_t	e_phoff;	/**< Program header table file offset */
	uint64_t	e_shoff;	/**< Section header table file offset */
	uint32_t	e_flags;	/**< Processor-specific flags */
	uint16_t	e_ehsize;	/**< ELF header size in bytes */
	uint16_t	e_phentsize;	/**< Program header table entry size */
	uint16_t	e_phnum;	/**< Program header table entry count */
	uint16_t	e_shentsize;	/**< Section header table entry size */
	uint16_t	e_shnum;	/**< Section header table entry count */
	uint16_t	e_shstrndx;	/**< Section header string table index */
    };

    struct Elf_Shdr {
	uint32_t	sh_name;	/**< Section name, index in string tbl */
	uint32_t	sh_type;	/**< Type of section */
	uint64_t	sh_flags;	/**< Miscellaneous section attributes */
	uint64_t	sh_addr;	/**< Section virtual addr at execution */
	uint64_t	sh_offset;	/**< Section file offset */
	uint64_t	sh_size;	/**< Size of section in bytes */
	uint32_t	sh_link;	/**< Index of another section */
	uint32_t	sh_info;	/**< Additional section information */
	uint64_t	sh_addralign;	/**< Section alignment */
	uint64_t	sh_entsize;	/**< Entry size if section holds table */
    };

    struct Elf_Phdr {
	uint32_t	p_type;		/**< Identifies program segment type */
	uint32_t	p_flags;	/**< Segment flags */
	uint64_t	p_offset;	/**< Segment file offset */
	uint64_t	p_vaddr;	/**< Segment virtual address */
	uint64_t	p_paddr;	/**< Segment physical address (ignored on SystemV)*/
	uint64_t	p_filesz;	/**< Segment size in file */
	uint64_t	p_memsz;	/**< Segment size in memory */
	uint64_t	p_align;	/**< Segment alignment, file & memory */
    };

    struct Elf_Dyn {
	uint64_t	d_tag;		/**< entry tag value */
	union {
	    uint64_t	d_val;
	    uint64_t	d_ptr;
	} d_un;
    };

    struct Elf_Rel {
	uint64_t	r_offset;	/**< Location at which to apply the action */
	uint64_t	r_info;		/**< index and type of relocation */
    };

    struct Elf_Rela {
	uint64_t	r_offset;	/**< Location at which to apply the action */
	uint64_t	r_info;		/**< index and type of relocation */
	uint64_t	r_addend;	/**< Constant addend used to compute value */
    };

    struct Elf_Sym {
	uint32_t	st_name;	/**< Symbol name, index in string tbl */
	uint8_t		st_info;	/**< Type and binding attributes */
	uint8_t		st_other;	/**< No defined meaning, 0 */
	uint16_t	st_shndx;	/**< Associated section index */
	uint64_t	st_value;	/**< Value of the symbol */
	uint64_t	st_size;	/**< Associated symbol size */
    };

#if __BYTE_ORDER == __BIG_ENDIAN
    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) { return !is_big ? bswap_16(cval) : cval; }
    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) { return !is_big ? bswap_32(cval) :cval; }
    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) { return !is_big ? bswap_64(cval) :cval; }
#else
    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) { return is_big ? bswap_16(cval) : cval; }
    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) { return is_big ? bswap_32(cval) :cval; }
    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) { return is_big ? bswap_64(cval) :cval; }
#endif
    inline uint16_t ELF_WORD(const uint16_t* cval,bool is_msbf) { return FMT_WORD(*cval,is_msbf); }
    inline uint32_t ELF_DWORD(const uint32_t* cval,bool is_msbf) { return FMT_DWORD(*cval,is_msbf); }
    inline uint64_t ELF_QWORD(const uint64_t* cval,bool is_msbf) { return FMT_QWORD(*cval,is_msbf); }

    template<typename foff_t>
    class Elf_xx {
	public:
	    Elf_xx(binary_stream& _fs)
		:fs(_fs) {
		uint8_t buf[16];
		fs.seek(0,binary_stream::Seek_Set);
		fs.read(buf,16);
		is_msbf = (buf[EI_DATA] == ELFDATA2MSB);
		is_64bit = (buf[EI_CLASS] == ELFCLASS64);
	    }
	    virtual ~Elf_xx() {}

	    Elf_Ehdr		read_ehdr() const {
		Elf_Ehdr rc;
		uint16_t tmp16;
		uint32_t tmp32;
		foff_t   tmp;
		fs.seek(0,binary_stream::Seek_Set);
		fs.read(&rc.e_ident,16);
		fs.read(&tmp16,2); rc.e_type=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_machine=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp32,4); rc.e_version=ELF_DWORD(&tmp32,is_msbf);
		fs.read(&tmp,sizeof(foff_t)); rc.e_entry=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		fs.read(&tmp,sizeof(foff_t)); rc.e_phoff=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		fs.read(&tmp,sizeof(foff_t)); rc.e_shoff=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		fs.read(&tmp32,4); rc.e_flags=ELF_DWORD(&tmp32,is_msbf);
		fs.read(&tmp16,2); rc.e_ehsize=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_phentsize=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_phnum=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_shentsize=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_shnum=ELF_WORD(&tmp16,is_msbf);
		fs.read(&tmp16,2); rc.e_shstrndx=ELF_WORD(&tmp16,is_msbf);
		return rc;
	    }
	    Elf_Shdr		read_shdr(binary_stream& _fs,__filesize_t off) const {
		Elf_Shdr rc;
		uint32_t tmp32;
		foff_t   tmp;
		_fs.seek(off,binary_stream::Seek_Set);
		_fs.read(&tmp32,4); rc.sh_name=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp32,4); rc.sh_type=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_flags=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_addr=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_offset=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_size=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp32,4); rc.sh_link=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp32,4); rc.sh_info=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_addralign=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.sh_entsize=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		return rc;
	    }
	    Elf_Phdr		read_phdr(binary_stream& _fs,__filesize_t off) const {
		Elf_Phdr rc;
		uint32_t tmp32;
		foff_t   tmp;
		_fs.seek(off,binary_stream::Seek_Set);
		_fs.read(&tmp32,4); rc.p_type=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp32,4); rc.p_flags=ELF_DWORD(&tmp32,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_offset=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_vaddr=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_paddr=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_filesz=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_memsz=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.p_align=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		return rc;
	    }
	    Elf_Dyn		read_dyn(binary_stream& _fs,__filesize_t off) const {
		Elf_Dyn rc;
		foff_t   tmp;
		_fs.seek(off,binary_stream::Seek_Set);
		_fs.read(&tmp,sizeof(foff_t)); rc.d_tag=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.d_un.d_val=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		return rc;
	    }
	    Elf_Rel		read_rel(binary_stream& _fs,__filesize_t off) const {
		Elf_Rel rc;
		foff_t   tmp;
		_fs.seek(off,binary_stream::Seek_Set);
		_fs.read(&tmp,sizeof(foff_t)); rc.r_offset=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.r_info=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		return rc;
	    }
	    Elf_Rela		read_rela(binary_stream& _fs,__filesize_t off) const {
		Elf_Rela rc;
		foff_t   tmp;
		_fs.seek(off,binary_stream::Seek_Set);
		_fs.read(&tmp,sizeof(foff_t)); rc.r_offset=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.r_info=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		_fs.read(&tmp,sizeof(foff_t)); rc.r_addend=is_64bit?ELF_QWORD((uint64_t*)&tmp,is_msbf):ELF_DWORD((uint32_t*)&tmp,is_msbf);
		return rc;
	    }
	    bool		is_msbf,is_64bit;
	private:
	    binary_stream&	fs;
    };

    class Elf_Reader {
	public:
	    Elf_Reader() {}
	    virtual ~Elf_Reader() {}

	    virtual size_t R_SYM(size_t i) const = 0;
	    virtual size_t R_TYPE(size_t i) const = 0;

	    virtual const Elf_Ehdr&		ehdr() const = 0;
	    virtual size_t			ehdr_size() const = 0;
	    virtual Elf_Shdr			read_shdr(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			shdr_size() const = 0;
	    virtual Elf_Phdr			read_phdr(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			phdr_size() const = 0;
	    virtual Elf_Dyn			read_dyn(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			dyn_size() const = 0;
	    virtual Elf_Rel			read_rel(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			rel_size() const = 0;
	    virtual Elf_Rela			read_rela(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			rela_size() const = 0;
	    virtual Elf_Sym			read_sym(binary_stream& fs,__filesize_t off) const = 0;
	    virtual size_t			sym_size() const = 0;
    };
    class Elf32_Reader : public Elf_Reader {
	public:
	    Elf32_Reader(binary_stream& fs):elf(fs) {
		_ehdr = elf.read_ehdr();
	    }
	    virtual ~Elf32_Reader() {}

	    virtual size_t R_SYM(size_t i) const { return i>>8; }
	    virtual size_t R_TYPE(size_t i) const { return i&0xff; }

	    virtual const Elf_Ehdr&		ehdr() const { return _ehdr; }
	    virtual size_t			ehdr_size() const { return sizeof(Elf386_External_Ehdr); }
	    virtual Elf_Shdr			read_shdr(binary_stream& fs,__filesize_t off) const { return elf.read_shdr(fs,off); }
	    virtual size_t			shdr_size() const { return sizeof(Elf386_External_Shdr); }
	    virtual Elf_Phdr			read_phdr(binary_stream& fs,__filesize_t off) const { return elf.read_phdr(fs,off); }
	    virtual size_t			phdr_size() const { return sizeof(Elf386_External_Phdr); }
	    virtual Elf_Dyn			read_dyn(binary_stream& fs,__filesize_t off) const { return elf.read_dyn(fs,off); }
	    virtual size_t			dyn_size() const { return sizeof(Elf386_External_Dyn); }
	    virtual Elf_Rel			read_rel(binary_stream& fs,__filesize_t off) const { return elf.read_rel(fs,off); }
	    virtual size_t			rel_size() const { return sizeof(Elf386_External_Rel); }
	    virtual Elf_Rela			read_rela(binary_stream& fs,__filesize_t off) const { return elf.read_rela(fs,off); }
	    virtual size_t			rela_size() const { return sizeof(Elf386_External_Rela); }
	    virtual Elf_Sym			read_sym(binary_stream& fs,__filesize_t off) const {
		Elf_Sym rc;
		uint32_t tmp32;
		uint16_t tmp16;
		fs.seek(off,binary_stream::Seek_Set);
		tmp32=fs.read(type_dword); rc.st_name=ELF_DWORD(&tmp32,elf.is_msbf);
		tmp32=fs.read(type_dword); rc.st_value=ELF_DWORD(&tmp32,elf.is_msbf);
		tmp32=fs.read(type_dword); rc.st_size=ELF_DWORD(&tmp32,elf.is_msbf);
		rc.st_info=fs.read(type_byte);
		rc.st_other=fs.read(type_byte);
		tmp16=fs.read(type_word); rc.st_shndx=ELF_WORD(&tmp16,elf.is_msbf);
		return rc;
	    }
	    virtual size_t			sym_size() const { return sizeof(Elf386_External_Sym); }
	private:
	    Elf_xx<uint32_t> elf;
	    Elf_Ehdr _ehdr;
    };
    class Elf64_Reader : public Elf_Reader {
	public:
	    Elf64_Reader(binary_stream& fs):elf(fs) {
		_ehdr = elf.read_ehdr();
	    }
	    virtual ~Elf64_Reader() {}

	    virtual size_t R_SYM(size_t i) const { return i>>32; }
	    virtual size_t R_TYPE(size_t i) const { return i&0xffffffff; }

	    virtual const Elf_Ehdr&		ehdr() const { return _ehdr; }
	    virtual size_t			ehdr_size() const { return sizeof(Elf64_External_Ehdr); }
	    virtual Elf_Shdr			read_shdr(binary_stream& fs,__filesize_t off) const { return elf.read_shdr(fs,off); }
	    virtual size_t			shdr_size() const { return sizeof(Elf64_External_Shdr); }
	    virtual Elf_Phdr			read_phdr(binary_stream& fs,__filesize_t off) const { return elf.read_phdr(fs,off); }
	    virtual size_t			phdr_size() const { return sizeof(Elf64_External_Phdr); }
	    virtual Elf_Dyn			read_dyn(binary_stream& fs,__filesize_t off) const { return elf.read_dyn(fs,off); }
	    virtual size_t			dyn_size() const { return sizeof(Elf64_External_Dyn); }
	    virtual Elf_Rel			read_rel(binary_stream& fs,__filesize_t off) const { return elf.read_rel(fs,off); }
	    virtual size_t			rel_size() const { return sizeof(Elf64_External_Rel); }
	    virtual Elf_Rela			read_rela(binary_stream& fs,__filesize_t off) const { return elf.read_rela(fs,off); }
	    virtual size_t			rela_size() const { return sizeof(Elf64_External_Rela); }
	    virtual Elf_Sym			read_sym(binary_stream& fs,__filesize_t off) const {
		Elf_Sym rc;
		uint32_t tmp32;
		uint16_t tmp16;
		uint64_t tmp64;
		fs.seek(off,binary_stream::Seek_Set);
		tmp32=fs.read(type_dword); rc.st_name=ELF_DWORD(&tmp32,elf.is_msbf);
		rc.st_info=fs.read(type_byte);
		rc.st_other=fs.read(type_byte);
		tmp16=fs.read(type_word); rc.st_shndx=ELF_WORD(&tmp16,elf.is_msbf);
		tmp64=fs.read(type_qword); rc.st_value=ELF_QWORD(&tmp64,elf.is_msbf);
		tmp64=fs.read(type_qword); rc.st_size=ELF_QWORD(&tmp64,elf.is_msbf);
		return rc;
	    }
	    virtual size_t			sym_size() const { return sizeof(Elf64_External_Sym); }
	private:
	    Elf_xx<uint64_t> elf;
	    Elf_Ehdr _ehdr;
    };
} // namespace	usr
#endif
