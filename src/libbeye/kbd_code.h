/**
 * @namespace   libbeye
 * @file        libbeye/kbd_code.h
 * @brief       This file includes OS depended keyboard codes.
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
#ifdef __beos__
#include "libbeye/osdep/beos/kbd_code.h"
#elif defined(__dos__)
#include "libbeye/osdep/dos/kbd_code.h"
#elif defined(__dos4gw__)
#include "libbeye/osdep/dos4gw/kbd_code.h"
#elif defined(__linux__)
#include "libbeye/osdep/linux/kbd_code.h"
#elif defined(__os2__)
#include "libbeye/osdep/os2/kbd_code.h"
#elif defined(__qnx__)
#include "libbeye/osdep/qnx/kbd_code.h"
#elif defined(__qnxnto__)
#include "libbeye/osdep/qnxnto/kbd_code.h"
#elif defined(__WIN32__)
#include "libbeye/osdep/win32/kbd_code.h"
#else
#include "libbeye/osdep/unix/kbd_code.h"
#endif

