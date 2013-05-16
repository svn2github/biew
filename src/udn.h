#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        udn.h
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
#ifndef __UDN_HPP_INCLUDED
#define __UDN_HPP_INCLUDED 1

namespace	usr {
    class Ini_Profile;
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
