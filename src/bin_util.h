/**
 * @namespace	usr
 * @file        bin_util.h
 * @brief       This file contains prototypes of common functions of
 *              plugins\bin of BEYE project.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __BIN_UTIL__H
#define __BIN_UTIL__H

#include "libbeye/bswap.h"
#include "libbeye/bstream.h"
#include "beyeutil.h"

namespace	usr {
#if __BYTE_ORDER == __BIG_ENDIAN
    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) { return !is_big ? bswap_16(cval) : cval; }
    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) { return !is_big ? bswap_32(cval) :cval; }
    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) { return !is_big ? bswap_64(cval) :cval; }
#else
    inline uint16_t FMT_WORD(uint16_t cval,bool is_big) { return is_big ? bswap_16(cval) : cval; }
    inline uint32_t FMT_DWORD(uint32_t cval,bool is_big) { return is_big ? bswap_32(cval) :cval; }
    inline uint64_t FMT_QWORD(uint64_t cval,bool is_big) { return is_big ? bswap_64(cval) :cval; }
#endif

    /** Reads user defined name at given offset!
    **/
    bool __FASTCALL__ udnFindName(__filesize_t pa,char *buff, unsigned cb_buff);
    bool __FASTCALL__ udnFindName(__filesize_t pa,std::string& buff);

    /** Display select box to select user defined name and returns its offset.
    **/
    bool __FASTCALL__ udnSelectName(__filesize_t *off);

    /** Shows menu with operations for user defined names!
    **/
    bool __FASTCALL__ udnUserNames();

    void __FASTCALL__ udnInit( Ini_Profile& ini );
    void __FASTCALL__ udnTerm( Ini_Profile& ini );
} // namespace	usr
#endif
