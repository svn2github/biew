/**
 * @namespace   libbeye
 * @file        libbeye/sysdep/_sys_dep.h
 * @brief       This file contains all development system depended part of BEYE project.
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
#ifndef __SYS_DEP_H
#define __SYS_DEP_H 1

#if defined(__i386__)
#include "libbeye/sysdep/ia32/_inlines.h"
#elif defined(__x86_64__)
#include "libbeye/sysdep/x86_64/_inlines.h"
#else
#include "libbeye/sysdep/generic/_inlines.h"
#endif

		/** Translates byte via table lookup
		  * @return         byte readed from table \e t at offset \e i
		  * @param t        pointer to 256-byte memory block from which will be readed byte
		  * @param i        index of memory block where byte is to be readed
		**/
__inline static uint8_t __FASTCALL__ __CONSTFUNC__ __Xlat__(const uint8_t *_table, uint8_t _idx)
{
 return _table[_idx];
}
#define __Xlat__ __Xlat__

		/** Compares two long numbers.
		  * @return         -1 if v1 < v2; +1 if v1 > v2 and 0 if v1 == v2
		  * @param _val1    specified first number to be compared
		  * @param _val2    specified second number to be compared
		**/
#ifndef __CmpLong__
#define __CmpLong__(_val1,_val2)\
	((_val1) < (_val2) ? -1 : (_val1) > (_val2) ? 1 : 0)
#endif

		   /** Converts all alphabetic characters in buffer to upper case.
		     * @return                none
		     * @param buff            buffer to be converted
		     * @param cbBuff          size of buffer
		     * @see                   memlwr
		    **/
extern void __FASTCALL__ memupr(any_t*buffer,unsigned cb_buffer);

		   /** Converts all alphabetic characters in buffer to lower case.
		     * @return                none
		     * @param buff            buffer to be converted
		     * @param cbBuff          size of buffer
		     * @see                   memlwr
		    **/
extern void __FASTCALL__ memlwr(any_t*buffer,unsigned cb_buffer);

#if defined(__GLIBC__) || defined (__UNIX__)
#define strupr(s) memupr(s,strlen(s)) /**< C library of *nix systems lacks strupr function */
#define strlwr(s) memlwr(s,strlen(s)) /**< C library of *nix systems lacks strlwr function */
#define stricmp strcasecmp            /**< Alias of stricmp for *nix systems */
#endif
#if (defined( __GNUC__ ) && !defined(__WIN32__)) || defined(__GLIBC__)
		   /** Converts the long integer to a NULL terminated ASCII string.
		     * @return                pointer to received buffer
		     * @param buff            pointer to the buffer in which the string version of the converted number will be stored
		     * @param cbBuff          size of buffer
		     * @note                  C library of *nix systems lacks it function
		     * @see                   ultoa
		    **/
extern char *        ltoa(long _value, char *_s, int _radix);
#endif
#if defined( DJGPP ) || (defined( __GNUC__ ) && defined(__WIN32__)) || defined(__QNX4__)
#ifdef atoll
#undef atoll
#endif
#define atoll(s) strtoll(s, NULL, 10) /* temporal workaround */
#endif
#if defined( __GNUC__ ) && defined(__WIN32__)
extern long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
#endif
#ifdef __GNUC__
#define atoul atoi /**< Ugly alias. Must be removed in future. */

		   /** Converts the unsigned long integer to a NULL terminated ASCII string.
		     * @return                pointer to received buffer
		     * @param buff            pointer to the buffer in which the string version of the converted number will be stored
		     * @param cbBuff          size of buffer
		     * @note                  C library of *nix systems lacks it function
		     * @see                   ltoa
		    **/
extern char *        ultoa(unsigned long _value, char *_s, int _radix);
#endif
#if __WORDSIZE >= 32
extern char *        lltoa(long long int _value, char *_s, int _radix);
extern char *        ulltoa(unsigned long long int _value, char *_s, int _radix);
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define BYTE_SWAP_L(longval)      ByteSwapL(longval)   /**< Swaps long integer if current system has little endian */
    #define BYTE_SWAP_S(shortval)     ByteSwapS(shortval)  /**< Swaps short integer if current system has little endian */
    #define BIG_BYTE_SWAP_L(longval)  (longval)
    #define BIG_BYTE_SWAP_S(shortval) (shortval)
#else
    #define BYTE_SWAP_L(longval)      (longval)
    #define BYTE_SWAP_S(shortval)     (shortval)
    #define BIG_BYTE_SWAP_L(longval)  ByteSwapL(longval)   /**< Swaps long integer if current system has big endian */
    #define BIG_BYTE_SWAP_S(shortval) ByteSwapS(shortval)  /**< Swaps short integer if current system has big endian */
#endif

#endif
