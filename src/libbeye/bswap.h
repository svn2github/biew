#ifndef __BSWAP_H__
#define __BSWAP_H__
#include "config.h"

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
#include <inttypes.h> /* for __WORDSIZE */


namespace	usr {

#if __BYTE_ORDER == __BIG_ENDIAN
#define FOURCC_TAG(ch0,ch1,ch2,ch3) (((uint32_t)(ch3)|((uint32_t)ch2<<8)|(uint32_t)ch1<<16)|((uint32_t)ch0<<24))
#define TWOCC_TAG(ch0,ch1) ((uint16_t)ch0|((uint16_t)ch1)<<8)
#else
#define FOURCC_TAG(ch0,ch1,ch2,ch3) (((uint32_t)(ch0)|((uint32_t)ch1<<8)|(uint32_t)ch2<<16)|((uint32_t)ch3<<24))
#define TWOCC_TAG(ch0,ch1) ((uint16_t)ch1|((uint16_t)ch0)<<8)
#endif
    inline uint32_t MAKE_FOURCC(uint8_t ch0,uint8_t ch1,uint8_t ch2,uint8_t ch3 ) {
	return  (uint32_t)ch3|
		((uint32_t)ch2)<<8|
		((uint32_t)ch1)<<16|
		((uint32_t)ch0)<<24;
    }

    inline uint16_t MAKE_TWOCC(uint8_t ch0,uint8_t ch1) {
	return  (uint16_t)ch0|((uint16_t)ch1)<<8;
    }

#ifndef HAVE_BYTESWAP_H
#if defined(__i386__)
    inline uint16_t bswap_16(uint16_t x) {
	__asm("xchgb %b0,%h0"	:
		"=q" (x)	:
		"0" (x));
	return x;
    }

    inline uint32_t bswap_32(uint32_t x) {
#if __CPU__ > 386
	__asm("bswap	%0":
		"=r" (x)     :
#else
	__asm("xchgb	%b0,%h0\n"
	    "	rorl	$16,%0\n"
	    "	xchgb	%b0,%h0":
	    "=q" (x)		:
#endif
	    "0" (x));
	return x;
    }

    inline uint64_t bswap_64(uint64_t x) {
	union { __extension__ unsigned long long int __ll;
		unsigned long int __l[2]; } __x;
	asm("xchgl	%0,%1":
	    "=r"(__x.__l[0]),"=r"(__x.__l[1]):
	    "0"(bswap_32((unsigned long)x)),"1"(bswap_32((unsigned long)(x>>32))));
	return __x.__ll;
    }

#elif defined(__x86_64__)

    inline uint16_t bswap_16(uint16_t x) {
	__asm("rorw $8, %w0"	:
		"=r" (x)	:
		"0" (x)		:
		"cc");
	return x;
    }

    inline uint32_t bswap_32(uint32_t x) {
	__asm("bswapl	%0":
		"=r" (x)     :
		"0" (x));
	return x;
    }

    inline uint64_t bswap_64(uint64_t x) {
	__asm("bswapq	%0":
		"=r" (x)     :
		"0" (x));
	return x;
    }
#else // not __i386__ and not __x86_64__

    inline uint16_t bswap_16(uint16_t) { return ((x)&0x00ff)<<8|((x)&0xff00)>>8; }

// code from bits/byteswap.h (C) 1997, 1998 Free Software Foundation, Inc.
    inline uint32_t bswap_32(uint32_t x) {
	return (((x)&0xff000000)>>24)|(((x)&0x00ff0000)>>8)|
		(((x)&0x0000ff00)<<8)|(((x)&0x000000ff)<<24);
    }
#if __WORDSIZE >= 64
    inline uint64_t bswap_64(uint64_t x) {
	return (((x) &0xff00000000000000ull)>>56)|
		(((x)&0x00ff000000000000ull)>>40)|
		(((x)&0x0000ff0000000000ull)>>24)|
		(((x)&0x000000ff00000000ull)>>8)|
		(((x)&0x00000000ff000000ull)<<8)|
		(((x)&0x0000000000ff0000ull)<<24)|
		(((x)&0x000000000000ff00ull)<<40)|
		(((x)&0x00000000000000ffull)<<56);
    }
#else
    inline uint64_t bswap_64(uint64_t x) {
	return __extension__ {
	    union {
		__extension__ unsigned long long int __ll;
		     unsigned long int __l[2];
	    } __w, __r;
	    __w.__ll = (x);
	    __r.__l[0] = bswap_32 (__w.__l[1]);
	    __r.__l[1] = bswap_32 (__w.__l[0]);
	    __r.__ll;
	};
    }
#endif
#endif	/* !ARCH_X86 */

#endif	/* !HAVE_BYTESWAP_H */

    inline float bswap_flt(float x) {
	union {uint32_t i; float f;} u;
	u.f = x;
	u.i = bswap_32(u.i);
	return u.f;
    }

    inline double bswap_dbl(double x) {
	union {uint64_t i; double d;} u;
	u.d = x;
	u.i = bswap_64(u.i);
	return u.d;
    }

    inline long double bswap_ldbl(long double x) {
	union {char d[10]; long double ld;} uin;
	union {char d[10]; long double ld;} uout;
	uin.ld = x;
	uout.d[0] = uin.d[9];
	uout.d[1] = uin.d[8];
	uout.d[2] = uin.d[7];
	uout.d[3] = uin.d[6];
	uout.d[4] = uin.d[5];
	uout.d[5] = uin.d[4];
	uout.d[6] = uin.d[3];
	uout.d[7] = uin.d[2];
	uout.d[8] = uin.d[1];
	uout.d[9] = uin.d[0];
	return uout.ld;
    }

// be2me ... BigEndian to MachineEndian
// le2me ... LittleEndian to MachineEndian

#if __BYTE_ORDER == __BIG_ENDIAN
    inline uint16_t be2me_16(uint16_t x) { return x; }
    inline uint32_t be2me_32(uint32_t x) { return x; }
    inline uint64_t be2me_64(uint64_t x) { return x; }
    inline uint16_t me2be_16(uint16_t x) { return x; }
    inline uint32_t me2be_32(uint32_t x) { return x; }
    inline uint64_t me2be_64(uint64_t x) { return x; }
    inline uint16_t le2me_16(uint16_t x) { return bswap_16(x); }
    inline uint32_t le2me_32(uint32_t x) { return bswap_32(x); }
    inline uint64_t le2me_64(uint64_t x) { return bswap_64(x); }
    inline uint16_t me2le_16(uint16_t x) { return bswap_16(x); }
    inline uint32_t me2le_32(uint32_t x) { return bswap_32(x); }
    inline uint64_t me2le_64(uint64_t x) { return bswap_64(x); }
    inline float    be2me_flt(float x) { return x; }
    inline double   be2me_dbl(double x) { return x; }
    inline long double be2me_ldbl(long double x) { return x; }
    inline float    le2me_flt(float x) { return bswap_flt(x); }
    inline double   le2me_dbl(double x) { return bswap_dbl(x); }
    inline long double le2me_ldbl(long double x) { return bswap_ldbl(x); }
#else
    inline uint16_t be2me_16(uint16_t x) { return bswap_16(x); }
    inline uint32_t be2me_32(uint32_t x) { return bswap_32(x); }
    inline uint64_t be2me_64(uint64_t x) { return bswap_64(x); }
    inline uint16_t me2be_16(uint16_t x) { return bswap_16(x); }
    inline uint32_t me2be_32(uint32_t x) { return bswap_32(x); }
    inline uint64_t me2be_64(uint64_t x) { return bswap_64(x); }
    inline uint16_t le2me_16(uint16_t x) { return x; }
    inline uint32_t le2me_32(uint32_t x) { return x; }
    inline uint64_t le2me_64(uint64_t x) { return x; }
    inline uint16_t me2le_16(uint16_t x) { return x; }
    inline uint32_t me2le_32(uint32_t x) { return x; }
    inline uint64_t me2le_64(uint64_t x) { return x; }
    inline float    be2me_flt(float x) { return bswap_flt(x); }
    inline double   be2me_dbl(double x) { return bswap_dbl(x); }
    inline long double be2me_ldbl(long double x) { return bswap_ldbl(x); }
    inline float    le2me_flt(float x) { return x; }
    inline double   le2me_dbl(double x) { return x; }
    inline long double le2me_ldbl(long double x) { return x; }
#endif
} // namespace	usr
#endif
