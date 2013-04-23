/**
 * @namespace   libbeye
 * @file        libbeye/sysdep/ia32/_inlines.h
 * @brief       This file includes 32-bit Intel architecture little inline functions.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
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

#define __NEAR__           /**< Obsolete for ia32 platform modifier of near call and data */
#define __FAR__            /**< Obsolete for ia32 platform modifier of far call and data */
#define __HUGE__           /**< Obsolete for ia32 platform modifier of huge pointer */
#define __INTERRUPT__      /**< Impossible for definition with gcc modifier of interrupt call */
#define halloc malloc      /**< For ia32 platform is alias of huge malloc */
#define hrealloc realloc   /**< For ia32 platform is alias of huge realloc */
#define hfree free         /**< For ia32 platform is alias of huge free */
#define HMemCpy memcpy     /**< For ia32 platform is alias of huge memcpy */

#ifndef __FASTCALL__
#define __FASTCALL__       /**< defined in config.h */
#endif
#define __NORETURN__ __attribute__ (( __noreturn__ ))                 /**< Noreturn modifier for ia32 */
#define __CONSTFUNC__ __attribute__ (( __const__ ))                   /**< Modifier of contant function for ia32 */
#ifdef __clpusplus
extern "C" {
#endif

		/** Changes byte order in 16-bit number */
__inline static uint16_t __FASTCALL__ __CONSTFUNC__ ByteSwapS(uint16_t _val)
{
  __asm("xchgb %b0,%h0"	:
	"=q" (_val)	:
	"0" (_val));
    return _val;
}
#define ByteSwapS ByteSwapS

		/** Changes byte order in 32-bit number */
__inline static uint32_t __FASTCALL__ __CONSTFUNC__ ByteSwapL(uint32_t _val)
{
#if __CPU__ > 386
 __asm("bswap	%0"	:
      "=r" (_val)       :
#else
 __asm("xchgb	%b0,%h0\n"
      "	rorl	$16,%0\n"
      "	xchgb	%b0,%h0":
      "=q" (_val)	:
#endif
      "0" (_val));
  return _val;
}
#define ByteSwapL ByteSwapL

		/** Changes byte order in 64-bit number */
__inline static uint64_t __FASTCALL__ __CONSTFUNC__ ByteSwapLL(uint64_t x)
{
  register union { __extension__ unsigned long long int __ll;
	  unsigned long int __l[2]; } __x;
  asm("xchgl	%0,%1":
      "=r"(__x.__l[0]),"=r"(__x.__l[1]):
      "0"(ByteSwapL((uint32_t)x)),"1"(ByteSwapL((uint32_t)(x>>32))));
  return __x.__ll;
}
#define ByteSwapLL ByteSwapLL

		/** Exchanges two bytes in memory.
		  * @return         none
		  * @param _val1    specified pointer to the first byte to be exchanged
		  * @param _val2    specified pointer to the second byte to be exchanged
		  * @note           Main difference from ByteSwap function family -
				    it is work with different number, rather than
				    changing byte order within given number.
		 **/
__inline static void __FASTCALL__ __XchgB__(uint8_t *_val1, uint8_t *_val2)
{
 register char _tmp;
 __asm("xchgb	%b1,(%2)":
      "=q"(_tmp):
      "0"(*_val2),
      "r"(_val1));
  *_val2 = _tmp;
}
#define __XchgB__ __XchgB__

extern void (__FASTCALL__ *InterleaveBuffers_ptr)(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer);
#ifdef InterleaveBuffers
#undef InterleaveBuffers
#endif
#define InterleaveBuffers(a,b,c,d) (*InterleaveBuffers_ptr)(a,b,c,d)
#define __INTERLEAVE_BUFFERS InterleaveBuffers


extern void (__FASTCALL__ *CharsToShorts_ptr)(uint32_t limit,
					     any_t*destbuffer,
					     const any_t*evenbuffer);
#ifdef CharsToShorts
#undef CharsToShorts
#endif
#define CharsToShorts(a,b,c) (*CharsToShorts_ptr)(a,b,c)
#define __CHARS_TO_SHORTS CharsToShorts

extern void (__FASTCALL__ *ShortsToChars_ptr)(uint32_t limit,
				     any_t* destbuffer, const any_t* srcbuffer);

#ifdef ShortsToChars
#undef ShortsToChars
#endif
#define ShortsToChars(a,b,c) (*ShortsToChars_ptr)(a,b,c)
#define __SHORTS_TO_CHARS ShortsToChars

#define COREDUMP() { __asm __volatile(".short 0xffff":::"memory"); }

#endif
#endif
#undef ___INLINES_H
#include "libbeye/sysdep/generic/_inlines.h"
