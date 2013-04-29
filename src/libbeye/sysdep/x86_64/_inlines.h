/**
 * @namespace   libbeye
 * @file        libbeye/sysdep/x86_64/_inlines.h
 * @brief       This file includes 64-bit AMD architecture little inline functions.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2009
 * @note        Development, fixes and improvements
**/
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define inline __inline
#endif

#if !defined(CAN_COMPILE_X86_GAS)
#include "libbeye/sysdep/generic/_inlines.h"
#else
#ifndef ___INLINES_H
#define ___INLINES_H 1

#ifdef __clpusplus
extern "C" {
#endif

extern void (__FASTCALL__ *InterleaveBuffers_ptr)(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer);
#ifdef InterleaveBuffers
#undef InterleaveBuffers
#endif
#define InterleaveBuffers(a,b,c,d) (*InterleaveBuffers_ptr)(a,b,c,d)
#define __INTERLEAVE_BUFFERS InterleaveBuffers

#define COREDUMP() { __asm__ __volatile__(".short 0xffff":::"memory"); }

#endif
#endif
#undef ___INLINES_H
#include "libbeye/sysdep/generic/_inlines.h"
