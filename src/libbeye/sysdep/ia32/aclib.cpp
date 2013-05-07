#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   libbeye
 * @file        libbeye/sysdep/ia32/aclib.c
 * @brief       This file contains functions which are expand are improve
 *              standard C library ones.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 * @remark      I used such form of this file because of build-in assembler
 *              allow write calling convention independed code. In addition,
 *              GNU C compiler is ported under multiple OS's. If somebody will
 *              port it under ABC-xyz platform, then more easy find compiler
 *              with build-in assembler, instead rewriting of makefile with .s
 *              (or .asm) extensions for choosen development system.
 *
 * @author      Nickols_K
 * @since       2001
 * @note        Development, fixes and improvements
**/
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "libbeye/sysdep/ia32/_inlines.h"

#if defined( CAN_COMPILE_X86_GAS ) && defined( USE_FASTMEMCPY )
#define CAN_COMPILE_X86_ASM
#endif

//Note: we have MMX, MMX2, 3DNOW version there is no 3DNOW+MMX2 one
//Plain C versions
//#if !defined (HAVE_MMX) || defined (RUNTIME_CPUDETECT)
//#define COMPILE_C
//#endif
#define USE_MMX		0x00000001UL
#define USE_MMX2	0x00000002UL
#define USE_3DNOW	0x00000004UL
#define USE_3DNOW2	0x00000008UL
#define USE_SSE		0x00000010UL
#define USE_SSE2	0x00000020UL

#ifdef CAN_COMPILE_X86_ASM
static int _mmx_inited=0;
static unsigned __mmx_caps=0;
static inline void do_cpuid(unsigned int ax, unsigned int *p)
{
	int a, c;

	__asm__ __volatile__ (
			  /* See if CPUID instruction is supported ... */
			  /* ... Get copies of EFLAGS into eax and ecx */
			  "pushf\n\t"
			  "popl %0\n\t"
			  "movl %0, %1\n\t"

			  /* ... Toggle the ID bit in one copy and store */
			  /*     to the EFLAGS reg */
			  "xorl $0x200000, %0\n\t"
			  "push %0\n\t"
			  "popf\n\t"

			  /* ... Get the (hopefully modified) EFLAGS */
			  "pushf\n\t"
			  "popl %0\n\t"
			  : "=a" (a), "=c" (c)
			  :
			  : "cc"
			  );
    if(a!=c)
    __asm __volatile
	("movl %%ebx, %%esi\n\t"
	 "cpuid\n\t"
	 "xchgl %%ebx, %%esi"
	 : "=a" (p[0]), "=S" (p[1]),
	   "=c" (p[2]), "=d" (p[3])
	 : "0" (ax));
    else p[0]=p[1]=p[2]=p[3]=ax=0;
}

static void GetCpuCaps()
{
	unsigned int regs[4];
	unsigned int regs2[4];
	_mmx_inited=1;
	__mmx_caps = 0;
	do_cpuid(0x00000000, regs); /* get _max_ cpuid level and vendor name*/
	if (regs[0]>=0x00000001)
	{
		do_cpuid(0x00000001, regs2);
		/* general feature flags: */
		if((regs2[3] & (1 << 23 )) >> 23) __mmx_caps |= USE_MMX;
		if((regs2[3] & (1 << 25 )) >> 25) __mmx_caps |= USE_SSE;
		if((regs2[3] & (1 << 26 )) >> 26) __mmx_caps |= USE_SSE2;
		if(__mmx_caps & USE_SSE) __mmx_caps |= USE_MMX2; /* SSE cpus supports mmxext too */
	}
	do_cpuid(0x80000000, regs);
	if (regs[0]>=0x80000001) {
		do_cpuid(0x80000001, regs2);
		if((regs2[3] & (1 << 23 )) >> 23) __mmx_caps |= USE_MMX;
		if((regs2[3] & (1 << 22 )) >> 22) __mmx_caps |= USE_MMX2;
		if((regs2[3] & (1 << 31 )) >> 31) __mmx_caps |= USE_3DNOW;
		if((regs2[3] & (1 << 30 )) >> 30) __mmx_caps |= USE_3DNOW2;
	}
}
#else
static int _mmx_inited=1;
static unsigned __mmx_caps=0;
static void GetCpuCaps() {}
#endif

#undef HAVE_MMX
#undef HAVE_MMX2
#undef HAVE_3DNOW
#undef HAVE_SSE
#define RENAME(a) a ## _C
#ifdef __MSDOS__
#include "libbeye/sysdep/ia32/aclib_~1.cpp"
#else
#include "libbeye/sysdep/ia32/aclib_template.cpp"
#endif

#ifdef CAN_COMPILE_X86_ASM

//MMX versions
#undef RENAME
#define HAVE_MMX
#define HAVE_MMX1
#undef HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX
#ifdef __MSDOS__
#include "libbeye/sysdep/ia32/aclib_~1.cpp"
#else
#include "libbeye/sysdep/ia32/aclib_template.cpp"
#endif

//MMX2 versions
#undef RENAME
#define HAVE_MMX
#define HAVE_MMX2
#undef HAVE_3DNOW
#define RENAME(a) a ## _MMX2
#ifdef __MSDOS__
#include "libbeye/sysdep/ia32/aclib_~1.cpp"
#else
#include "libbeye/sysdep/ia32/aclib_template.cpp"
#endif

#endif

static void __FASTCALL__ init_InterleaveBuffers(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer)
{
#ifdef CAN_COMPILE_X86_ASM
	/* ordered per speed fastest first */
	if(!_mmx_inited) GetCpuCaps();
	if(__mmx_caps & USE_MMX2)	InterleaveBuffers_ptr = InterleaveBuffers_MMX2;
//	else if(__mmx_caps & USE_3DNOW)	InterleaveBuffers_ptr = InterleaveBuffers_3DNow;
	else if(__mmx_caps & USE_MMX)	InterleaveBuffers_ptr = InterleaveBuffers_MMX;
	else
#endif
	InterleaveBuffers_ptr = InterleaveBuffers_C;
	(*InterleaveBuffers_ptr)(limit,destbuffer,evenbuffer,oddbuffer);
}

void (__FASTCALL__ *InterleaveBuffers_ptr)(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer) = init_InterleaveBuffers;
