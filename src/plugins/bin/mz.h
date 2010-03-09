/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/mz.h
 * @brief       This file contains MZ executable file definitions.
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
#ifndef __MZ_INC
#define __MZ_INC

#ifndef __BEYEUTIL__H
#include "beyeutil.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef struct tagMZHEADER
{
 uint16_t mzPartLastPage;
 uint16_t mzPageCount;
 uint16_t mzRelocationCount;
 uint16_t mzHeaderSize;
 uint16_t mzMinMem;
 uint16_t mzMaxMem;
 uint16_t mzRelocationSS;
 uint16_t mzExeSP;
 uint16_t mzCheckSumm;
 uint16_t mzExeIP;
 uint16_t mzRelocationCS;
 uint16_t mzTableOffset;
 uint16_t mzOverlayNumber;
}MZHEADER;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
