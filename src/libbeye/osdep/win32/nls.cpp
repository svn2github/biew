#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/win32/nls.c
 * @brief       This file contains implementation of OEM codepages support
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
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>

bool win32_use_ansi;

void __FASTCALL__ __nls_OemToOsdep(unsigned char *buff,unsigned len)
{
 if(win32_use_ansi)
 {
   OemToCharBuff((LPCSTR)buff,(LPSTR)buff,len);
 }
}

void __FASTCALL__ __nls_OemToFs(unsigned char *buff,unsigned len)
{
}

void __FASTCALL__ __nls_CmdlineToOem(unsigned char *buff,unsigned len)
{
   CharToOemBuff((LPCSTR)buff,(LPSTR)buff,len);
}
