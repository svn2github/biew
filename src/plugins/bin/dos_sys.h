/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mz.h
 * @brief       This file contains DOS driver executable file definitions.
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
#ifndef __DOS_SYS_INC
#define __DOS_SYS_INC

#include "config.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef struct tagDOSDRIVER
{
  uint16_t ddAttribute;
  uint16_t ddStrategyOff;
  uint16_t ddInterruptOff;
  uint8_t  ddName[8];
}DOSDRIVER;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
