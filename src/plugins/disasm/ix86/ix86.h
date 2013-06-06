/**
 * @namespace	usr_plugins_II
 * @file        plugins/disasm/ix86/ix86.h
 * @brief       This file contains declaration of internal Intel x86 disassembler functions.
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
#ifndef ____DISASM_H
#define ____DISASM_H

#include "plugins/disasm.h"

#ifndef __BEYELIB_H
#include "libbeye/libbeye.h"
#endif

namespace	usr {
    class ix86_Disassembler;
enum {
    TAB_POS   =10,
    TILE_SAFE =4000,
    DUMMY_PTR =0, /**< "" */
    BYTE_PTR  =1, /**<" <b>"*/
    WORD_PTR  =2, /**<" <w>"*/
    DWORD_PTR =3, /**<" <d>"*/
    PWORD_PTR =4, /**<" <p>"*/
    QWORD_PTR =5, /**<" <q>"*/
    TWORD_PTR =6  /**<" <t>"*/
};

enum {
    PFX_SEGMASK		=0x00000007,
    PFX_SEG_CS		=0x00000000,
    PFX_SEG_DS		=0x00000001,
    PFX_SEG_ES		=0x00000002,
    PFX_SEG_SS		=0x00000003,
    PFX_SEG_FS		=0x00000004,
    PFX_SEG_GS		=0x00000005,
    PFX_SEG_US		=0x00000006,
    PFX_SEG_XS		=0x00000007,
    PFX_LOCK		=0x00000008,
    PFX_F2_REPNE	=0x00000010,
    PFX_F3_REP		=0x00000020,
    PFX_66		=0x00000040,
    PFX_67		=0x00000080,
    PFX_OF		=0x00000100,/* for VEX compatibility */
    PFX_REX		=0x01000000,
    PFX_VEX		=0x02000000,
    PFX_XOP		=0x04000000
};
enum {
    MOD_WIDE_DATA	=0x00000001,
    MOD_WIDE_ADDR	=0x00000002,
    MOD_MMX		=0x00000010,
    MOD_SSE		=0x00000020
};
/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
typedef struct tagix86Param
{
  unsigned long pro_clone; /**< processor family */
  __filesize_t  DisasmPrefAddr; /**< address of instruction with prefixes */
  __filesize_t  CodeAddress; /**< without prefixes */
  MBuffer       CodeBuffer; /**< buffer with source code */
  MBuffer       RealCmd; /**< buffer without prefixes */
  unsigned      flags; /**< refer to disasm.h header */
  unsigned char codelen;
  unsigned long insn_flags; /**< contains copy of flags32/flags64 field from INSN_TABLE */
  unsigned long pfx;
  unsigned long mode;
/*
  REX is 0x4? opcodes
  bits  meaning
  0     rex.b (extension to the Base)
  1     rex.x (extsnsion to the SIB indeX)
  2     rex.r (extension to the ModRM/REG)
  3     rex.w (extension to the operand Width)
  7-4   0100
  DEFAULT operand size:
    if rex.w then 64
    else 66_pref then 16
    else 32
  DEFAULT address size:
    if 67_pref then 32
    else 64
    (note: address displasement always has 8, 16 or 32-bit)
*/
  unsigned char REX;
/*
  VEX is C4, C5 opcodes
		   Byte 0         Byte 1             Byte 2
  (Bit Position) 7        0   7 6 5 4        0   7 6    3 2 1 0
		+----------+ +-----+----------+ +-+------+-+---+
  3-byte VEX C4 | 11000100 | |R X B|  m-mmmm  | |W| vvvv |L| pp|
		+----------+ +-----+----------+ +-+------+-+---+
		 7        0   7 6   3 2 1 0
		+----------+ +-+-----+-+---+
  2-byte VEX C5 | 11000101 | |R| vvvv|L| pp|
		+----------+ +-+-----+-+---+

   R: REX.R in 1's complement (inverted) form
      1: Same as REX.R=0 (must be 1 in 32-bit mode)
      0: Same as REX.R=1 (64-bit mode only)
   X: REX.X in 1's complement (inverted) form
      1: Same as REX.X=0 (must be 1 in 32-bit mode)
      0: Same as REX.X=1 (64-bit mode only)
   B: REX.B in 1's complement (inverted) form
      1: Same as REX.B=0 (Ignored in 32-bit mode).
      0: Same as REX.B=1 (64-bit mode only)
   W: opcode specific (use like REX.W, or used for memory operand
      select on 4-operand instructions, or ignored, depending on the opcode)
   m-mmmm:
     00000: Reserved for future use (will #UD)
     00001: implied 0F leading opcode byte
     00010: implied 0F 38 leading opcode bytes
     00011: implied 0F 3A leading opcode bytes
     00100-11111: Reserved for future use (will #UD)
   vvvv: a register specifier (in 1's complement form) or 1111 if unused.
   L: Vector Length
	0: scalar or 128-bit vector
	1: 256-bit vector
   pp: opcode extension providing equivalent functionality of a SIMD prefix
	00: None
	01: 66
	10: F3
	11: F2
*/
  unsigned char VEX_m;
  unsigned char VEX_vlp;
  unsigned char XOP_m;
}ix86Param;

#define K86_REX (DisP->REX)
#define REX_W(rex) (((rex)&0x08)>>3)
#define REX_R(rex) (((rex)&0x04)>>2)
#define REX_X(rex) (((rex)&0x02)>>1)
#define REX_B(rex) ((rex)&0x01)
#define REX_w(rex) ((rex)&0x08)
#define REX_r(rex) ((rex)&0x04)
#define REX_x(rex) ((rex)&0x02)
#define REX_b(rex) ((rex)&0x01)

extern char * SJump[];
typedef void (ix86_Disassembler::*ix86_method)(char *encode_str,ix86Param&) const;

typedef char* ( ix86_Disassembler::*FPUroutine)(char *,const char *,ix86Param&) const;
struct FPUcall {
    FPUroutine	f;
    const char*	c;
};

enum {
    IX86_CPU086		=0x00000000UL,
    IX86_CPU186		=0x00000001UL,
    IX86_CPU286		=0x00000002UL,
    IX86_CPU386		=0x00000003UL,
    IX86_CPU486		=0x00000004UL,
    IX86_CPU586		=0x00000005UL,
    IX86_CPU686		=0x00000006UL,
    IX86_CPU786		=0x00000007UL,
    IX86_CPU886		=0x00000008UL,
    IX86_CPU986		=0x00000009UL,
    IX86_CPU1086	=0x0000000AUL,
    IX86_CPU1186	=0x0000000BUL,
    IX86_CPU1286	=0x0000000CUL,
    IX86_CPU1386	=0x0000000CUL,
    IX86_CPUMASK	=0x000000FFUL,

    IX86_P2		=IX86_CPU686,
    IX86_P3		=IX86_CPU786,
    IX86_P4		=IX86_CPU886,
/* Prescott processor (SSE3) */
    IX86_P5		=IX86_CPU986,
/* Xeon5100 processor (SSSE3)*/
    IX86_P6		=IX86_CPU1086,
/* Xeon5200 processor (SSE4)*/
    IX86_P7		=IX86_CPU1186,
    IX86_P8		=IX86_CPU1286,
    IX86_P9		=IX86_CPU1386,

    K64_ATHLON		=0x00000000UL,
    K64_GEODE		=0x00000000UL,
    K64_FAM9		=0x00000001UL,
    K64_FAM10		=0x00000002UL,
    K64_FAM11		=0x00000003UL,
    K64_CPUMASK		=0x000000FFUL,

    IX86_CLONEMASK	=0x00000700UL,
    IX86_INTEL		=0x00000000UL,
    IX86_AMD		=0x00000100UL,
    IX86_CYRIX		=0x00000200UL,
    IX86_VIA		=0x00000300UL,

    INSN_SYSTEMMASK	=0x00000800UL,
    INSN_CPL0		=0x00000800UL,

    INSN_REGGROUP	=0x0000F000UL,
    INSN_GPR		=0x00000000UL, /* insn works with general purpose registers */
    INSN_FPU		=0x00001000UL, /* insn works with fpu registers */
    INSN_MMX		=0x00002000UL, /* insn works with mmx registers */
    INSN_SSE		=0x00004000UL, /* insn works with sse registers */
    INSN_AVX		=0x00008000UL, /* insn works with avx registers */

    INSN_VEXMASK	=0x000F0000UL,
    INSN_VEX_V		=0x00010000UL, /* means insns use VVVV register extension from VEX prefix*/
    INSN_VEXW_AS_SWAP	=0x00020000UL, /* means use VEX.W register to swap sources */
    INSN_VEXW_AS_SIZE	=0x00040000UL, /* means use VEX.W register as double size */
    INSN_VEX_VSIB	=0x00080000UL, /* means use V-SIB memory access */

    INSN_FLAGS_MASK	=0xFFF00000UL,
    INSN_LOAD		=0x00000000UL, /* means direction: OPCODE reg,[mem] */
    INSN_STORE		=0x00100000UL, /* means direction: OPCODE [mem],reg */
    INSN_OP_BYTE	=0x00200000UL, /* means operand size is 1 byte */
    INSN_OP_WORD	=0x00000000UL, /* means operand size is word (16,32 or 64) depends on mode */
    K64_NOCOMPAT	=0x01000000UL, /* means insns has no 16 or 32 bit forms */
    K64_DEF32		=0x02000000UL, /* means insns size depends on default data size but not address size */
    INSN_USERBIT	=0x40000000UL, /* overloaded for special purposes */

    BRIDGE_MMX_SSE	=INSN_USERBIT,
    BRIDGE_SSE_MMX	=0x00000000UL,
    BRIDGE_CPU_SSE	=INSN_USERBIT,
    BRIDGE_SSE_CPU	=0x00000000UL,
    IMM_BYTE		=INSN_USERBIT,
    IMM_WORD		=0x00000000UL,
    K64_FORCE64		=INSN_USERBIT,

/* Special features flags */
    TABDESC_MASK	=0x80000000UL,
    TAB_NAME_IS_TABLE	=0x80000000UL,

/* Furter processors */
    IX86_UNKCPU		=IX86_CPU1286,
    IX86_UNKFPU		=(IX86_UNKCPU|INSN_FPU),
    IX86_UNKMMX		=(IX86_UNKCPU|INSN_MMX),
    IX86_UNKSSE		=(IX86_UNKCPU|INSN_SSE),
    IX86_UNKAVX		=(IX86_UNKCPU|INSN_AVX),

    IX86_K6		=(IX86_AMD|IX86_CPU586),
    IX86_3DNOW		=(IX86_AMD|IX86_CPU686|INSN_MMX),
    IX86_ATHLON		=(IX86_AMD|IX86_CPU786|INSN_MMX),
    IX86_GEODE		=IX86_ATHLON,
    IX86_UNKAMD		=(IX86_AMD|IX86_CPU886|INSN_MMX),

    IX86_CYRIX486	=(IX86_CYRIX|IX86_CPU486),
    IX86_CYRIX686	=(IX86_CYRIX|IX86_CPU586),
    IX86_CYRIX686MMX	=(IX86_CYRIX|IX86_CPU586|INSN_MMX),
    IX86_UNKCYRIX	=(IX86_CYRIX|IX86_CPU686)
};

    struct ix86_Opcodes {
	const char*	name16;
	const char*	name32;
	const char*	name64;
	ix86_method	method;
	unsigned long	pro_clone;
	ix86_method	method64;
	unsigned long	flags64;
    };

    struct ix86_ExOpcodes {
	const char*	name;
	const char*	name64;
	ix86_method	method;
	ix86_method	method64;
	unsigned long	flags64;
	unsigned long	pro_clone;
    };

    struct assembler_t {
	const char *run_command;
	const char *detect_command;
    };

    struct ix86_3dNowopcodes {
	const char*	name;
	unsigned long	pro_clone;
    };

    struct DualStr {
	const char*	c1;
	const char*	c2;
    };

    extern unsigned x86_Bitness;

    class DisMode;
    class ix86_Disassembler : public Disassembler {
	public:
	    ix86_Disassembler(BeyeContext& bc,const Bin_Format& b,binary_stream&,DisMode&);
	    virtual ~ix86_Disassembler();

	    virtual const char*	prompt(unsigned idx) const;
	    virtual bool	action_F1();
	    virtual bool	action_F3();

	    virtual DisasmRet	disassembler(__filesize_t shift,MBuffer insn_buff,unsigned flags);
	    virtual AsmRet	assembler(const char *str);

	    virtual void	show_short_help() const;
	    virtual int		max_insn_len() const;
	    virtual ColorAttr	get_insn_color(unsigned long clone);
	    virtual ColorAttr	get_opcode_color(unsigned long clone);
	    virtual ColorAttr	get_alt_insn_color(unsigned long clone);
	    virtual ColorAttr	get_alt_opcode_color(unsigned long clone);

	    virtual Bin_Format::bitness	get_bitness() const;
	    virtual char	clone_short_name(unsigned long clone);
	    virtual void	read_ini(Ini_Profile&);
	    virtual void	save_ini(Ini_Profile&);
	private:
	/* ix86.cpp */
	    const ix86_ExOpcodes*	ix86_prepare_flags(const ix86_ExOpcodes *extable,ix86Param& DisP,unsigned char *code,unsigned char *codelen) const;
	    DisasmRet			ix86Disassembler(__filesize_t ulShift,MBuffer buffer,unsigned flags) const;
	    void			parse_XOP_8F(ix86Param& DisP) const;
	    void			parse_VEX_C5(ix86Param& DisP) const;
	    void			parse_VEX_C4(ix86Param& DisP) const;
	    void			parse_VEX_pp(ix86Param& DisP) const;
	    unsigned char		parse_REX(unsigned char code,ix86Param& DisP) const;
	    void			ix86_gettype(DisasmRet *dret,ix86Param& _DisP) const;
	    MBuffer			parse_REX_type(MBuffer insn,char *up,ix86Param& DisP) const;
	    bool			is_listed(unsigned char insn,const unsigned char *list,size_t listsize) const __PURE_FUNC__;
	    void			ix86_InOut(char *str,ix86Param& DisP) const;
	/* ix86_fpu.cpp */
	    void 			ix86_FPUCmd(char *str,ix86Param& DisP) const;
	    char*			FPUstist0_2(char *str,const char *name1,const char *name2,char code) const;
	    char*			FPUst0sti_2(char *str,const char *name1,const char *name2,char code) const;
	    char*			FPUcmdsti_2(char *str,const char *name1,const char *name2,char code) const;
	    char*			FPUcmdst0(char *str,const char *name) const;
	    char*			FPUcmdsti(char *str,const char *name,char code) const;
	    char*			FPUsttword(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUldtword(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUstist0(char *str,const char *cmd,char code1) const;
	    char*			FPUst0sti(char *str,const char *cmd,char code1) const;
	    char*			FPUstisti(char *str,const char *cmd,char code1,char code2) const;
	    char*			FPUld(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUstint32(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUint64st(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUint64(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUint16int32st(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUint16int32(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUmem64mem32st(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUmem64mem32(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			FPUmem(char *str,const char *cmd,ix86Param& DisP) const;
	    char*			__MemFPUfunc(char *str,const char *cmd,char opsize,ix86Param& DisP) const;
	    char*			__UniFPUfunc(char *str,const char *cmd,char opsize,char direct,ix86Param& DisP) const;
	    char*			SetNameTabD(char *str,const char *name,unsigned char size,ix86Param& DisP) const;
	    char*			SC(const char *name1,const char *name2) const;
	    char*			SetNameTab(char *str,const char *name) const;
	/* ix86_func.cpp */
	    void			ix86_3DNowPrefetchGrp(char *str,ix86Param& DisP) const;
	    void			ix86_3DNowOpCodes(char *str,ix86Param& DisP) const;
	    void			arg_fma(char *str,ix86Param& DisP) const;
	    void			arg_fma4_imm8(char *str,ix86Param& DisP) const;
	    void			arg_fma4(char *str,ix86Param& DisP) const;
	    void			arg_simd_xmm0(char *str,ix86Param& DisP) const;
	    void			arg_simd_clmul(char *str,ix86Param& DisP) const;
	    void			arg_xop_cmp(char *str,ix86Param& DisP) const;
	    void			arg_simd_cmp(char *str,ix86Param& DisP) const;
	    void			ix86_ArgXMMCmp(char *str,ix86Param& DisP,const char **sfx,unsigned nsfx,unsigned namlen,unsigned precopy) const;
	    void			arg_simd_rm_imm8_imm8(char *str,ix86Param& DisP) const;
	    void			arg_simd_regrm_imm8_imm8(char *str,ix86Param& DisP) const;
	    void			arg_simd_regrm(char *str,ix86Param& DisP) const;
	    void			arg_simd_imm8(char *str,ix86Param& DisP) const;
	    void			ix86_ArgFsGsBaseGrp(char *str,ix86Param& DisP) const;
	    void			ix86_ArgBm1Grp(char *str,ix86Param& DisP) const;
	    void			ix86_ArgKatmaiGrp2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgKatmaiGrp1(char *str,ix86Param& DisP) const;
	    void			ix86_BitGrp(char *str,ix86Param& DisP) const;
	    void			ix86_ArgMovYX(char *str,ix86Param& DisP) const;
	    void			ix86_ArgXMMXGr3(char *str,ix86Param& DisP) const;
	    void			ix86_ArgXMMXGr2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgXMMXGr1(char *str,ix86Param& DisP) const;
	    void			ix86_ArgMMXGr3(char *str,ix86Param& DisP) const;
	    void			ix86_ArgMMXGr2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgMMXGr1(char *str,ix86Param& DisP) const;
	    void			ix86_ArgxMMXGroup(char *str,const char *name,ix86Param& DisP,bool as_xmmx) const;
	    void			arg_simd(char *str,ix86Param& DisP) const;
	    void			arg_emms(char *str,ix86Param& DisP) const;
	    void			bridge_sse_mmx(char *str,ix86Param& DisP) const;
	    void			ix86_bridge_sse_mmx(char *str,ix86Param& DisP,bool xmmx_first) const;
	    void			bridge_simd_cpu_imm8(char *str,ix86Param& DisP) const;
	    void			bridge_simd_cpu(char *str,ix86Param& DisP) const;
	    void			ix86_bridge_cpu_simd(char *str,ix86Param& DisP,bool direct,bool as_xmmx) const;
	    void			ix86_ArgMovXRY(char *str,ix86Param& DisP) const;
	    void			ix86_ArgExGr1(char *str,ix86Param& DisP) const;
	    void			ix86_660FVMX(char *str,ix86Param& DisP) const;
	    void			ix86_0FVMX(char *str,ix86Param& DisP) const;
	    void			ix86_VMX(char *str,ix86Param& DisP) const;
	    void			ix86_ArgExGr0(char *str,ix86Param& DisP) const;
	    void			ix86_ExOpCodes(char *str,ix86Param& DisP) const;
	    void			ix86_ArgGrp2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgGrp1(char *str,ix86Param& DisP) const;
	    void			ix86_ShOpCL(char *str,ix86Param& DisP) const;
	    void			ix86_ShOp1(char *str,ix86Param& DisP) const;
	    void			ix86_DblShift(char *str,ix86Param& DisP) const;
	    void			ix86_ShOp2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgOp2(char *str,ix86Param& DisP) const;
	    void			ix86_ArgOp1(char *str,ix86Param& DisP) const;
	    void			ix86_ArgRmDigit(char *str,ix86Param& DisP,char w,char s) const;
	    void			arg_imm16_imm8(char *str,ix86Param& DisP) const;
	    void			arg_imm16(char *str,ix86Param& DisP) const;
	    void			arg_imm8(char *str,ix86Param& DisP) const;
	    void			arg_imm(char *str,ix86Param& DisP) const;
	    void			arg_insnreg_imm(char *str,ix86Param& DisP) const;
	    void			arg_insnreg(char *str,ix86Param& DisP) const;
	    void			arg_r0mem(char *str,ix86Param& DisP) const;
	    void			arg_r0rm(char *str,ix86Param& DisP) const;
	    void			arg_r0_imm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_modsegrm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_modregrm_imm8(char *str,ix86Param& DisP) const;
	    void			arg_cpu_modregrm_imm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_mod_rm_imm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_mod_rm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_modREGrm(char *str,ix86Param& DisP) const;
	    void			arg_cpu_modregrm(char *str,ix86Param& DisP) const;
	    char*			__buildModRegRmReg(ix86Param& DisP,bool d,unsigned char wrex) const;
	    char*			__buildModRegRm(ix86Param& DisP,bool w,bool d) const;
	    char*			ix86_CStile(ix86Param& DisP,char *str,const char *arg2) const;
	    char*			ix86_getModRM(bool w,unsigned char mod,unsigned char rm,ix86Param& DisP) const;
	    char*			ix86_getModRM64(bool w,unsigned char mod,unsigned char rm,ix86Param& DisP) const;
	    char*			ix86_getModRM32(bool w,unsigned char mod,unsigned char rm,ix86Param& DisP) const;
	    char*			ix86_getModRM16(bool w,unsigned char mod,unsigned char rm,ix86Param& DisP) const;
	    char*			ConstrSibMod(ix86Param& DisP,char *store,char *scale,char *_index,char *base,char code,char *mod) const;
	    void			getSIBRegs(ix86Param& DisP,char *base,char *scale,char *_index,char *mod,char code) const;
	    void			arg_offset(char *str,ix86Param& DisP) const;
	    void			arg_segoff(char *str,ix86Param& DisP) const;
	    char*			ix86_GetDigitTile(ix86Param& DisP,char wrd,char sgn,unsigned char loc_off) const;
	    void			ix86_ArgGS(char *str,ix86Param& param) const;
	    void			ix86_ArgFS(char *str,ix86Param& param) const;
	    void			ix86_ArgCS(char *str,ix86Param& param) const;
	    void			ix86_ArgSS(char *str,ix86Param& param) const;
	    void			ix86_ArgDS(char *str,ix86Param& param) const;
	    void			ix86_ArgES(char *str,ix86Param& param) const;
	    char*			GetDigitsApp(unsigned char loc_off,ix86Param& DisP,char codelen,DisMode::e_disarg type) const;
	    char*			Get16SquareDig(unsigned char loc_off,ix86Param& DisP,bool as_sign,bool is_disponly) const;
	    char*			Get8SquareDig(unsigned char loc_off,ix86Param& DisP,bool as_sign,bool is_disponly,bool as_rip) const;
	    char*			Get4SquareDig(unsigned char loc_off,ix86Param& DisP,bool as_sign,bool is_disponly) const;
	    char*			Get2SquareDig(unsigned char loc_off,ix86Param& DisP,bool as_sign) const;
	    inline char*		Get16SignDigApp(unsigned char loc_off,ix86Param& DisP){ return GetDigitsApp(loc_off,DisP,8,DisMode::Arg_LLong); }
	    inline char*		Get16DigitApp(unsigned char loc_off,ix86Param& DisP){ return GetDigitsApp(loc_off,DisP,8,DisMode::Arg_QWord); }
	    inline char*		Get8SignDigApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,4,DisMode::Arg_Long); }
	    inline char*		Get8DigitApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,4,DisMode::Arg_DWord); }
	    inline char*		Get4SignDigApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,2,DisMode::Arg_Short); }
	    inline char*		Get4DigitApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,2,DisMode::Arg_Word); }
	    inline char*		Get2SignDigApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,1,DisMode::Arg_Char); }
	    inline char*		Get2DigitApp(unsigned char loc_off,ix86Param& DisP) const { return GetDigitsApp(loc_off,DisP,1,DisMode::Arg_Byte); }
	    const char*			get_VEX_reg(ix86Param& DisP) const __PURE_FUNC__;
	    const char*			k64_getREG(ix86Param& DisP,unsigned char reg,bool w,bool rex,bool use_qregs) const __PURE_FUNC__;
	    unsigned			ix86_calcModifier(ix86Param& DisP,unsigned w) const __PURE_FUNC__;
	    const char*			getSREG(unsigned sreg) const __CONST_FUNC__ { return ix86_SegRegs[sreg]; }
	    void			ix86_Null(char *str,ix86Param& param) const __CONST_FUNC__;
	/* data */
	    BeyeContext&		bctx;
	    DisMode&			parent;
	    binary_stream&		main_handle;
	    const Bin_Format&		bin_format;

	    Bin_Format::bitness		x86_Bitness;
	    char*			ix86_voidstr;
	    char*			ix86_da_out;
	    char*			ix86_Katmai_buff;
	    char*			ix86_appstr;
	    char*			ix86_dtile;
	    char*			ix86_appbuffer;
	    char*			ix86_apistr;
	    char*			ix86_modrm_ret;
	    int				active_assembler;

	/* static data */
	    static const unsigned	MAX_IX86_INSN_LEN;
	    static const unsigned	MAX_DISASM_OUTPUT;

	    static const assembler_t	assemblers[];
	    static const unsigned	CODEBUFFER_LEN;

	    static const char		ix86CloneSNames[4];
	    static const char*		ix86_sizes[];
	    static const char*		ix86_A16[];

	    static const char*		i8086_ByteRegs[];
	    static const char*		k64_ByteRegs[];
	    static const char*		ix86_MMXRegs[];
	    static const char*		ix86_SegRegs[];

	    static const char*		k64_WordRegs[];
	    static const char*		k64_DWordRegs[];
	    static const char*		k64_QWordRegs[];
	    static const char*		k64_XMMXRegs[];
	    static const char*		k64_YMMXRegs[];
	    static const char*		k64_CrxRegs[];
	    static const char*		k64_DrxRegs[];
	    static const char*		k64_TrxRegs[];
	    static const char*		k64_XrxRegs[];

	    static const char*		ix86_Op1Names[];
	    static const char*		ix86_ShNames[];
	    static const char*		ix86_Gr1Names[];
	    static const char*		ix86_Gr2Names[];
	    static const char*		ix86_ExGrp0[];
	    static const char*		ix86_BitGrpNames[];
	    static const char*		ix86_MMXGr1[];
	    static const char*		ix86_MMXGr2[];
	    static const char*		ix86_MMXGr3[];
	    static const char*		ix86_XMMXGr1[];
	    static const char*		ix86_XMMXGr2[];
	    static const char*		ix86_XMMXGr3[];
	    static const char*		ix86_3dPrefetchGrp[];
	    static const char*		ix86_KatmaiGr2Names[];

	    static Bin_Format::bitness	BITNESS;
	    static char			ix86_segpref[4];
	    static const unsigned char	leave_insns[];

	    static const ix86_Opcodes	ix86_table[256];
	    static const ix86_ExOpcodes	ix86_F30F_PentiumTable[256];
	    static const ix86_ExOpcodes	ix86_F20F_PentiumTable[256];
	    static const ix86_ExOpcodes	ix86_F30F3A_Table[256];
	    static const ix86_ExOpcodes	ix86_F30F38_Table[256];
	    static const ix86_ExOpcodes	ix86_F20F3A_Table[256];
	    static const ix86_ExOpcodes	ix86_F20F38_Table[256];
	    static const ix86_ExOpcodes	ix86_660F_PentiumTable[256];
	    static const ix86_ExOpcodes	ix86_660F3A_Table[256];
	    static const ix86_ExOpcodes ix86_660F38_Table[256];
	    static const ix86_ExOpcodes	ix86_660F01_Table[256];
	    static const ix86_ExOpcodes	K64_XOP_Table[256];
	    static const ix86_3dNowopcodes ix86_3DNowtable[256];
	    static const ix86_ExOpcodes	ix86_extable[256];
	    static const ix86_ExOpcodes	ix86_0FA7_Table[256];
	    static const ix86_ExOpcodes	ix86_0FA6_Table[256];
	    static const ix86_ExOpcodes	ix86_0F3A_Table[256];
	    static const ix86_ExOpcodes	ix86_0F38_Table[256];

	    static const char*		mem64mem32[];
	    static const char*		int16int32[];
	    static const char*		DBEx[];
	    static const char*		D9Ex[];
	    static const char*		D9Fx[];

	    static const FPUcall	DFrm[8];
	    static const FPUcall	DDrm[8];
	    static const FPUcall	DBrm[8];
	    static const FPUcall	D9rm[8];
	    static const DualStr	D8str[4];
	    static const DualStr	DEstr[4];
	    static const char*		FCMOVc[];
	    static const char*		FCMOVnc[];
	    static const char*		FxCOMIP[];

	    static const char*		ix86_Bm1GrpNames[];
	    static const char*		ix86_FsGsBaseNames[];
	    static const char*		ix86_KatmaiCmpSuffixes[];
	    static const char*		ix86_KatmaiGr1Names[];
	    static const char*		ix86_KatmaiGr1Names11[];
	    static const char*		vex_cmp_sfx[];
	    static const char*		xop_cmp_sfx[];
	    static const char*		ix86_clmul_sfx[];

	    static const char**		k64_xry[];
	    static const char*		ix86_vmxname[];
	    static const char*		ix86_0Fvmxname[];
	    static const char*		ix86_660Fvmxname[];
	    static const char*		ix86_ExGrp1[];
    };
} // namespace	usr
#endif
