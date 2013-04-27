#ifndef ARMV5TE_H
#define ARMV5TE_H 1

namespace beye {
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
    void __FASTCALL__ arm16Init(DisMode*);
    void __FASTCALL__ arm16Term(void);
    void __FASTCALL__ arm16Disassembler(DisasmRet *dret,__filesize_t ulShift,
						uint16_t opcode, unsigned flags);
    void __FASTCALL__ arm32Init(DisMode*);
    void __FASTCALL__ arm32Term(void);
    void __FASTCALL__ arm32Disassembler(DisasmRet *dret,__filesize_t ulShift,
						uint32_t opcode, unsigned flags);
} // namespace beye
#endif
