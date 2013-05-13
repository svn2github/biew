#ifndef ARMV5TE_H
#define ARMV5TE_H 1

namespace	usr {
    enum {
	TAB_POS		=10,
	ARM_V1		=0x00000001UL,
	ARM_V2		=0x00000002UL,
	ARM_V3		=0x00000003UL,
	ARM_V4		=0x00000004UL,
	ARM_V5		=0x00000005UL,
	ARM_VX		=0x000000FFUL,
	ARM_INTEGER	=0x00000000UL,
	ARM_FPU		=0x00000100UL,
	ARM_DSP		=0x00000200UL,
	ARM_XSCALE	=0x00000400UL
    };

    struct arm_opcode32 {
	const char*	name;
	const char*	mask;
	const long	flags;
	unsigned	bmsk;
	unsigned	bits;
    };

    struct arm_opcode16 {
	const char*	name;
	const char*	mask;
	const long	flags;
	unsigned	bmsk;
	unsigned	bits;
    };

    class ARM_Disassembler : public Disassembler {
	public:
	    ARM_Disassembler(Bin_Format& b,binary_stream& h,DisMode& parent);
	    virtual ~ARM_Disassembler();
	
	    virtual const char*	prompt(unsigned idx) const;
	    virtual bool	action_F1();
	    virtual bool	action_F3();
	    virtual bool	action_F4();

	    virtual DisasmRet	disassembler(__filesize_t shift,MBuffer insn_buff,unsigned flags);

	    virtual void	show_short_help() const;
	    virtual int		max_insn_len();
	    virtual ColorAttr	get_insn_color(unsigned long clone);
	    virtual ColorAttr	get_opcode_color(unsigned long clone);

	    virtual int		get_bitness();
	    virtual char	clone_short_name(unsigned long clone);
	    virtual void	read_ini(Ini_Profile&);
	    virtual void	save_ini(Ini_Profile&);
	private:
	    void		arm16EncodeTail(DisasmRet *dret,uint16_t opcode,__filesize_t ulShift,const char *msk,long flags);
	    void		arm16Init();
	    void		arm16Term();
	    void		arm16Disassembler(DisasmRet *dret,__filesize_t ulShift,
						uint16_t opcode, unsigned flags);

	    void		arm32EncodeTail(DisasmRet *dret,__filesize_t ulShift,uint32_t opcode, unsigned flags,unsigned _index);
	    void		arm32Init();
	    void		arm32Term();
	    void		arm32Disassembler(DisasmRet *dret,__filesize_t ulShift,
						uint32_t opcode, unsigned flags);

	    DisMode&		parent;
	    binary_stream&	main_handle;
	    Bin_Format&		bin_format;
	    char*		outstr;
	    int			armBitness;
	    int			armBigEndian;

	    static arm_opcode16	opcode_table[];
	    static const char*	arm_reg_name[];
	    static const char*	armCCnames[16];

	    static arm_opcode32	opcode32_table[];
	    static const char *arm_sysfreg_name[16];
	    static const char *arm_freg_name[32];
	    static const char * arm_wreg_name[];
    };
} // namespace	usr
#endif
