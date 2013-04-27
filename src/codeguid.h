/**
 * @namespace   beye
 * @file        codeguid.h
 * @brief       This file contains prototypes code navigator.
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
#ifndef __CODEGUID__H
#define __CODEGUID__H

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

namespace beye {
    class DisMode;
    extern char codeguid_image[];

    void              __FASTCALL__ GidResetGoAddress( int keycode );
    void              __FASTCALL__ GidAddGoAddress(char *str,__filesize_t addr);
    void              __FASTCALL__ GidAddBackAddress( void );
    __filesize_t      __FASTCALL__ GidGetGoAddress(unsigned keycode);
    char *            __FASTCALL__ GidEncodeAddress(__filesize_t cfpos,bool aresolv);

    bool             __FASTCALL__ initCodeGuider(const DisMode& parent);
    void              __FASTCALL__ termCodeGuider( void );
} // namespace beye
#endif
