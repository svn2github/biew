/**
 * @namespace	usr_plugins_I
 * @file        plugins/disasm.h
 * @brief       This file contains function prototypes for disassembler interface.
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
#ifndef __DISASM_H
#define __DISASM_H

#include <vector>

#include "plugin.h"
#include "colorset.h"
#include "bconsole.h"
#include "libbeye/twin.h"

struct hIniProfile;
namespace	usr {
/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
typedef struct tagDisasmRet
{
  unsigned long pro_clone; /**< contains processor clone when normal disassembling; instruction type on __DISF_GETTYPE */
  char         *str;       /**< contains disassembler output */
  unsigned long field;     /**< on __DISF_GETTYPE contains offset to field for binding from begin of insn, if present. */
  unsigned char codelen;   /**< contains length of instruction when normal disassembling; length of field for binding on __DISF_GETTYPE */
}DisasmRet;
typedef unsigned char * MBuffer;
    struct Disassembler_Info;
    class Disassembler;
    class DisMode : public Plugin {
	public:
	    /* New features: commentaries */
	    enum {
		Comm_Size=256       /**< Size of disassembler commentaries */
	    };
	    enum e_severity {
		CommSev_None    =0, /**< means no comments were appended */
		CommSev_String  =1, /**< means comment is representation of multibyte immediate operand */
		CommSev_InsnRef =2, /**< means comment is instruction reference (like ; RETURN) */
		CommSev_StrPtr  =3, /**< means comment is pointer to the string */
		CommSev_Func    =4  /**< means comment is function name */
	    };
	    enum e_disaddr {
		Short   =0x00,
		Near16  =0x01,
		Near32  =0x02,
		Near64  =0x04,
		UseSeg  =0x10,
		Far16   =(UseSeg | Near16),
		Far32   =(UseSeg | Near32)
	    };
	    enum e_disarg {
		Arg_LLong    =0x0080U, /**< signed 8-byte value */
		Arg_Long     =0x0040U, /**< signed 4-byte value */
		Arg_Short    =0x0020U, /**< signed 2-byte value */
		Arg_Char     =0x0010U, /**< signed 1-byte value */
		Arg_Byte     =0x0001U, /**< unsigned 1-byte value */
		Arg_Word     =0x0002U, /**< unsigned 2-byte value */
		Arg_DWord    =0x0004U, /**< unsigned 4-byte value */
		Arg_QWord    =0x0008U, /**< unsigned 8-byte value */
/* Contsants for references predictions */
		Arg_Imm      =0x1000U, /**< Immediate argument */
		Arg_Disp     =0x2000U, /**< Argument is displacement only */
		Arg_IdxDisp  =0x4000U, /**< Argument is displacement which is combined with registers */
		Arg_Rip      =0x8000U  /**< Argument is displacement relatively current Instruction Pointer */
	    };
/* references resolution */
	    enum e_ref {
		Ref_Predict =3,    /**< resolves all instructions and uses prediction mechanism */
		Ref_All     =2,    /**< resolves all instructions that has binding */
		Ref_Jmp     =1,    /**< resolves only jump and call instructions */
		Ref_None    =0     /**< do not resolve references */
	    };

	    enum e_panel {
		Panel_Full   =2,       /**< full mode of panel: address + instruction bytes + instruction */
		Panel_Medium =1,       /**< medium mode of panel: address + instruction */
		Panel_Wide   =0        /**< full mode of panel: instruction only */
	    };

	    DisMode(CodeGuider& code_guider);
	    virtual ~DisMode();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual bool		action_F1();
	    virtual bool		action_F2();
	    virtual bool		action_F3();
	    virtual bool		action_F4();
	    virtual bool		action_F5();
	    virtual bool		action_F6();
	    virtual bool		action_F7();
	    virtual bool		action_F8();
	    virtual bool		action_F9();
	    virtual bool		action_F10();

	    virtual bool		detect();
	    virtual e_flag		flags() const;
	    virtual unsigned		paint(unsigned keycode,unsigned textshift);

	    virtual unsigned		get_symbol_size() const;
	    virtual unsigned		get_max_symbol_size() const;
	    virtual unsigned		get_max_line_length() const;
	    virtual const char*		misckey_name() const;
	    virtual void		misckey_action();
	    virtual unsigned long	prev_page_size() const;
	    virtual unsigned long	curr_page_size() const;
	    virtual unsigned long	prev_line_width() const;
	    virtual unsigned long	curr_line_width() const;
	    virtual void		help() const;
	    virtual void		read_ini(Ini_Profile& );
	    virtual void		save_ini(Ini_Profile& );
	    virtual __filesize_t	search_engine(TWindow *pwnd, __filesize_t start, __filesize_t *slen, unsigned flg, bool is_continue, bool *is_found);
	    virtual DisasmRet		disassembler(__filesize_t ulShift,MBuffer buffer,unsigned flags);
/** Appends symbolic information to address field of jump instructions
    @param str       string to be appended
    @param ulShift   indicates offset to field for binding
    @param codelen   contains length of field for binding
    @param distin    contains original value of field (like defval)
    @param r_sh      real shift to begin of insn (for pass to CodeGuider as return addr)
    @param type      see above (DISARG_SHORT - DISARG_FAR32 family)
    @param seg       contains segment value (optional)
    @return          true if appended
    @remark
    Examples:
    *** 1 ***
    00001002: 9A 78 56 34 12       callf 1234:5678
    after references resolving it may:
				   callf KERNEL.@90
    i.e. i must call this function as:
    strcpy(outstr,"callf ");
    disAppendFAddr(outstr, 0x1003, 0x5678, 0x1002, DISADR_FAR16, 0x1234, 4);
    *** 2 ***
    00001002: 66 E9 78 56 34 12       calln32 12345678
    after references resolving it may:
				      calln32 KERNEL32.TerminateApp
    i.e. i must call this function as:
    strcpy(outstr,"calln32 ");
    disAppendFAddr(outstr, 0x1004, 0x12345678, 0x1002, DISADR_NEAR32, 0, 4);
**/
	    virtual bool		append_faddr(char * str,__fileoff_t ulShift,__fileoff_t distin,__filesize_t r_sh,e_disaddr type,unsigned seg,char codelen);
/** Appends symbolic information instead digits to instruction string
    @param str       string to be appended
    @param flags     same as described in reg_form.h (APREF_* family)
    @param ulShift   indicates offset to field for binding
    @param codelen   contains length of field for binding
    @param defval    contains default value if not binding
    @param type      see above (DISARG_LONG - DISARG_DWORD family)
    @return          true if appended
    @remark
    Examples:
    *** 1 ***
    00005678: B8 34 12       mov   ax, 1234
    after references resolving it can will:
			     mov   ax, off16 KERNEL.@4
    i.e. i must call this function as:
    strcpy(outstr,"mov ax,");
    disAppendDigits(outstr, 0x5679, 1, 2, 0x1234, DISARG_WORD);
    *** 2 ***
    00005678: 8B 80 34 12       mov   ax, [bx+si+1234]
    after references resolving it can will:
				mov   ax, [bx+si+off16 KERNEL.@4]
    i.e. i must call this function as:
    strcpy(outstr,"mov ax,[bx+si+");
    disAppendDigits(outstr, 0x5680, 1, 2, 0x1234, DISARG_WORD);
    strcat(outstr,"]");
**/
	    virtual bool		append_digits(char *str,__filesize_t ulShift,int flags,char codelen,any_t*defval,e_disarg type);

	    virtual e_panel		panel_mode() const { return disPanelMode; }
	    virtual bool		prepare_mode() const { return DisasmPrepareMode; }
	    virtual int			get_curr_line_num() const { return DisasmCurrLine; }

	    e_ref			disNeedRef;
	    char*			dis_comments;
	    e_severity			dis_severity;
	private:
	    void			accept_actions();
	    void			fill_prev_asm_page(__filesize_t bound,unsigned predist);
	    void			prepare_asm_lines(int keycode,__filesize_t cfpos);
	    bool			def_asm_action(int _lastbyte,int start);
	    void			disasm_screen(TWindow* ewnd,__filesize_t cp,__filesize_t flen,int st,int stop,int start);
	    int				full_asm_edit(TWindow * ewnd);

	    unsigned			DefDisasmSel;
	    Disassembler*		activeDisasm;

	    unsigned long		PrevPageSize,CurrPageSize,PrevStrLen,CurrStrLen;
	    int				DisasmCurrLine;
	    e_panel			disPanelMode;
	    int				HiLight;
	    unsigned char*		CurrStrLenBuff;
	    unsigned long*		PrevStrLenAddr;
	    char			LastPrevLen;
	    char			PrevStrCount;
	    char*			disCodeBuffer;
	    char*			disCodeBufPredict;
	    int				disMaxCodeLen;

	    CodeGuider&			code_guider;
	    bool			DisasmPrepareMode;
	    std::vector<const Disassembler_Info*> list;
    };
    inline DisMode::e_disarg operator~(DisMode::e_disarg a) { return static_cast<DisMode::e_disarg>(~static_cast<unsigned>(a)); }
    inline DisMode::e_disarg operator|(DisMode::e_disarg a, DisMode::e_disarg b) { return static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline DisMode::e_disarg operator&(DisMode::e_disarg a, DisMode::e_disarg b) { return static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline DisMode::e_disarg operator^(DisMode::e_disarg a, DisMode::e_disarg b) { return static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline DisMode::e_disarg operator|=(DisMode::e_disarg& a, DisMode::e_disarg b) { return (a=static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline DisMode::e_disarg operator&=(DisMode::e_disarg& a, DisMode::e_disarg b) { return (a=static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline DisMode::e_disarg operator^=(DisMode::e_disarg& a, DisMode::e_disarg b) { return (a=static_cast<DisMode::e_disarg>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }

    inline DisMode::e_disaddr operator~(DisMode::e_disaddr a) { return static_cast<DisMode::e_disaddr>(~static_cast<unsigned>(a)); }
    inline DisMode::e_disaddr operator|(DisMode::e_disaddr a, DisMode::e_disaddr b) { return static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline DisMode::e_disaddr operator&(DisMode::e_disaddr a, DisMode::e_disaddr b) { return static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline DisMode::e_disaddr operator^(DisMode::e_disaddr a, DisMode::e_disaddr b) { return static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline DisMode::e_disaddr operator|=(DisMode::e_disaddr& a, DisMode::e_disaddr b) { return (a=static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline DisMode::e_disaddr operator&=(DisMode::e_disaddr& a, DisMode::e_disaddr b) { return (a=static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline DisMode::e_disaddr operator^=(DisMode::e_disaddr& a, DisMode::e_disaddr b) { return (a=static_cast<DisMode::e_disaddr>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }

/** List of CPU platform. */
    enum {
	DISASM_DATA	=0,  /**< indicates data disassembler */
	DISASM_CPU_IX86	=1,  /**< indicates Intel-x86 disassembler */
	DISASM_CPU_AVR	=2,  /**< indicates Atmel-AVR disassembler */
	DISASM_JAVA	=3,  /**< indicates Java disassembler */
	DISASM_CPU_ARM	=4,  /**< indicates ARM disassembler */
	DISASM_CPU_PPC	=5,  /**< indicates PowerPC disassembler */
			    /* ... here may placed other constants!!! ... */
	DISASM_CPU_IA64	=6,  /**< indicates Itanium disassembler */
	DISASM_CPU_ALPHA=7,  /**< indicates DEC Alpha disassembler */
	DISASM_CPU_MIPS	=8,  /**< indicates MIPS disassembler */
	DISASM_CPU_SPARC=9,  /**< indicates SUN Sparc disassembler */
	DISASM_CPU_SH	=10, /**< indicates Hitachi SH disassembler */
	DISASM_CPU_CRAY	=11, /**< indicates Cray disassembler */
			    /* ... here may placed other constants!!! ... */
	DISASM_DEFAULT	=0  /**< indicates unspecified disassembler: format default */
    };

    enum {
	ASM_NOERR  =0,
	ASM_SYNTAX =1
    };
/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
    struct AsmRet {
	MBuffer		insn;
	int		err_code;
	unsigned char	insn_len;
    };

    enum {
	PREDICT_DEPTH =15 /**< means depth of prediction is 15 insns which have max_insn_len */
    };
    /** Flags of disassembler */
    enum {
	__DISF_NORMAL	=0x0000, /**< Performs normal disassembling */
	__DISF_GETTYPE	=0x0001, /**< Tells to disassembler that field pro_clone must to contain type of instruction */
	__DISF_SIZEONLY	=0x8000 /**< Performs computing size of insns */
    };
    /** Types of instruction (for __DISF_GETTYPE) in future must grow */
    enum {
	__INSNT_ORDINAL	=0x00000000L, /**< Any unspecified instruction */
	__INSNT_RET	=0x00000001L, /**< Instruction of return class */
	__INSNT_LEAVE	=0x00000002L, /**< Instruction of leave class: Example: pop reg1; pop reg2; mov esp, ebp; pop ebp; retx or similar */
	__INSNT_JMPVVT	=0x00000003L, /**< Jump via virtual table */
	__INSNT_JMPPIC	=0x00000004L, /**< Jump via PIC. Like: .i386: jmp name@GOT(ebx) */
	__INSNT_JMPRIP	=0x00000005L  /**< Jump via RIP. Like: .i386: jmp [rip+name@GOT(rip)] */
    };
    class Disassembler : public Opaque {
	public:
	    Disassembler(DisMode& parent) { UNUSED(parent); }
	    virtual ~Disassembler() {}
	
	    virtual const char*	prompt(unsigned idx) const = 0;	/**< prompt on Ctrl-(F1,F3-F5) */
	    virtual bool	action_F1() { return false; }	/**< actions on Ctrl-(F1,F3-F5) */
	    virtual bool	action_F3() { return false; }	/**< actions on Ctrl-(F1,F3-F5) */
	    virtual bool	action_F4() { return false; }	/**< actions on Ctrl-(F1,F3-F5) */
	    virtual bool	action_F5() { return false; }	/**< actions on Ctrl-(F1,F3-F5) */

	    virtual DisasmRet	disassembler(__filesize_t shift,MBuffer insn_buff,unsigned flags) = 0; /**< main function of disasm */
	    virtual AsmRet	assembler(const char *str) { AsmRet ret = {NULL, ASM_SYNTAX, 0 }; UNUSED(str); ErrMessageBox("Sorry, no assembler available",""); return ret; }

	    virtual void	show_short_help() const = 0; /**< displays short help */
	    virtual int		max_insn_len() = 0; /**< Max length of 1 disasm instruction */
	    virtual ColorAttr	get_insn_color(unsigned long clone) { UNUSED(clone); return browser_cset.main; } /**< returns color of instruction */
	    virtual ColorAttr	get_opcode_color(unsigned long clone) { UNUSED(clone); return disasm_cset.opcodes; } /**< returns color of instruction */
	    virtual ColorAttr	get_alt_insn_color(unsigned long clone) { return get_insn_color(clone); } /**< returns color of instruction */
	    virtual ColorAttr	get_alt_opcode_color(unsigned long clone) { return get_opcode_color(clone); } /**< returns color of instruction */

	    virtual int		get_bitness() = 0;  /**< returns currently used bitness */
	    virtual char	clone_short_name(unsigned long clone) = 0; /**< returns short clone name of instruction */
	    virtual void	read_ini(Ini_Profile&) {}  /**< reads settings of plugin from .ini file */
	    virtual void	save_ini(Ini_Profile&) {}  /**< stores settings of plugin into .ini file */
    };

    struct Disassembler_Info {
	unsigned	type;	/**< DISASM_XXX constant */
	const char*	name;	/**< disassembler name */
	Disassembler* (*query_interface)(DisMode& parent);
    };

/** Common disassembler utility */
    char * __FASTCALL__ TabSpace(char * str,unsigned nSpace);
    void   __FASTCALL__ disSetModifier(char *str,const char *modf);
} // namespace	usr
#endif
