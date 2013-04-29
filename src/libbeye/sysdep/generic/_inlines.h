/**
 * @namespace   libbeye
 * @file        libbeye/sysdep/generic/_inlines.h
 * @brief       This file includes generic architecture little inline functions.
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
#ifndef ___INLINES_H
#define ___INLINES_H 1

#ifdef __cplusplus
extern "C" {
#endif

		/** Performs interleaving of two buffers into destinition one.
		  * @return         none
		  * @param limit    specified size of evenbuffer and oddbuffer
		  * @param destbuffer specified pointer to the destinition buffer
				    where result will be placed.
		  * @param evenbuffer specified source buffer with even bytes.
		  * @param offbuffer specified source buffer with odd bytes.
		 **/
#ifndef __INTERLEAVE_BUFFERS
#define __INTERLEAVE_BUFFERS(limit, destbuffer, evenbuffer, oddbuffer)\
{\
  register size_t freq;\
  for(freq=0;freq<(size_t)limit;freq++)\
  {\
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];\
    ((char *)destbuffer)[freq+freq+1] = ((char *)oddbuffer)[freq];\
  }\
}
#endif

#ifndef COREDUMP
#define COREDUMP()
#endif

#ifdef __cplusplus
}
#endif

#endif
