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

		/** Performs interleaving of two buffers into destinition one.
		  * @return         none
		  * @param limit    specified size of evenbuffer and oddbuffer
		  * @param destbuffer specified pointer to the destinition buffer
				    where result will be placed.
		  * @param evenbuffer specified source buffer with even bytes.
		  * @param offbuffer specified source buffer with odd bytes.
		 **/
inline void __INTERLEAVE_BUFFERS(uint32_t limit,
				any_t*destbuffer,
				const any_t*evenbuffer,
				const any_t*oddbuffer) {
    uint32_t freq;
    for(freq=0;freq<(size_t)limit;freq++) {
	((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];
	((char *)destbuffer)[freq+freq+1] = ((char *)oddbuffer)[freq];
    }
}

		/** Compares two long numbers.
		  * @return         -1 if v1 < v2; +1 if v1 > v2 and 0 if v1 == v2
		  * @param _val1    specified first number to be compared
		  * @param _val2    specified second number to be compared
		**/
inline int __CmpLong__(long _val1,long _val2) { return ((_val1) < (_val2) ? -1 : (_val1) > (_val2) ? 1 : 0); }
#endif
