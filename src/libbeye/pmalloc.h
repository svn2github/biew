/**
 * @namespace   libbeye
 * @file        libbeye/pmalloc.h
 * @brief       This file contains prototypes of preemptive memory allocation technology.
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
#ifndef __PMALLOC_H
#define __PMALLOC_H 1

#include <stddef.h>

#ifndef __BEYELIB_H
#include "libbeye/libbeye.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

                   /** Initializes memory manager.
                     * @param mode            upper limit of random generator
                     * @note                  Pseudo-randomizing memory objects
                     *                        makes memory exploits harder
                    **/
extern void __FASTCALL__ PMallocInit(unsigned rnd_limit);

                   /** Allocates memory object with given size.
                     * @return                Pointer to allocated object or NULL if error occured
                     * @param obj_size        specifies required size of object in bytes
                     * @note                  When memory is exhausted will call user specified
                     *                        functions for freeing unneeded memory.
                     * @see                   PRealloc PFree
                    **/
extern any_t*           __FASTCALL__ PMalloc(size_t obj_size);

                   /** Reallocates memory object with new size.
                     * @return                Pointer to reallocated object or NULL if error occured
                     * @param ptr             specifies object to be reallocated
                     * @param obj_size        specifies new size of object in bytes
                     * @note                  if new address will be assigned for
                     *                        realocated object, then contents will
                     *                        be preserved. When memory is exhausted
                     *                        will call user specified functions
                     *                        for freeing unneeded memory.
                     * @see                   PMalloc PFree
                    **/
extern any_t*           __FASTCALL__ PRealloc(any_t*ptr,size_t obj_size);

                   /** Frees memory object.
                     * @return                Pointer to reallocated object or NULL if error occured
                     * @param ptr             specifies object to be reallocated
                     * @param obj_size        specifies new size of object in bytes
                     * @note                  if new address will be assigned for
                     *                        realocated object, then contents will
                     *                        be preserved.
                     * @see                   PMalloc PRealloc
                    **/
extern void             __FASTCALL__ PFree(any_t*ptr);

                   /** Huge version of PMalloc.
                     * @note                  For 16-bit application it works with
                     *                        huge memory objects, for 32-bit apps.
                     *                        it work same as PMalloc
                     * @see                   PMalloc
                    **/
extern void __HUGE__ *  __FASTCALL__ PHMalloc(unsigned long obj_size);

                   /** Huge version of PRealloc.
                     * @note                  For 16-bit application it works with
                     *                        huge memory objects, for 32-bit apps.
                     *                        it work same as PRealloc
                     * @see                   PRealloc
                    **/
extern void __HUGE__ *  __FASTCALL__ PHRealloc(void __HUGE__ *ptr,unsigned long obj_size);

                   /** Huge version of PFree.
                     * @note                  For 16-bit application it works with
                     *                        huge memory objects, for 32-bit apps.
                     *                        it work same as PFree
                     * @see                   PFree
                    **/
extern void             __FASTCALL__ PHFree(void __HUGE__ * ptr);

/** Frees pointer and nullifies it */
#define PFREE(ptr)      { PFree(ptr); ptr = 0; }
/** Huge version of PFREE */
#define PHFREE(ptr)     { PHFree(ptr); ptr = 0; }

                   /** Pointer to a user supplied function that freed unneeded memory.
                     * @return                true if part of required memory is free.
                     * @param need_mem        specifies size of needed memory
                    **/
typedef bool   (__FASTCALL__ *LowMemCallBack)(unsigned long need_mem);

                   /** Registers user-defined callback.
                     * @return                true if foperation performed successfully
                     * @param func            specifies user-defined callback function
                     * @see                   PMUnregLowMemCallBack
                    **/
extern bool    __FASTCALL__ PMRegLowMemCallBack(LowMemCallBack func);

                   /** Unregisters user-defined callback.
                     * @return                true if foperation performed successfully
                     * @param func            specifies user-defined callback function
                     * @see                   PMRegLowMemCallBack
                    **/
extern bool    __FASTCALL__ PMUnregLowMemCallBack(LowMemCallBack func);

#ifdef __cplusplus
}
#endif

#endif

