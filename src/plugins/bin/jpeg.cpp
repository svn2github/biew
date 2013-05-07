#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/jpeg.c
 * @brief       This file contains implementation of decoder for jpeg
 *              file format.
 * @version     -
 * @remark      this source file is part of jpegary vIEW project (BEYE).
 *              The jpegary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <stddef.h>
#include <string.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

namespace beye {
static bool  __FASTCALL__ jpeg_check_fmt()
{
    unsigned long val;
    unsigned char id[4];
    val=bmReadDWordEx(0,BFile::Seek_Set);
    bmReadBufferEx(id,4,6,BFile::Seek_Set);
    if(val==0xE0FFD8FF && memcmp(id,"JFIF",4)==0) return true;
    return false;
}

static void __FASTCALL__ jpeg_init_fmt(CodeGuider& code_guider) { UNUSED(code_guider); }
static void __FASTCALL__ jpeg_destroy_fmt() {}
static int  __FASTCALL__ jpeg_platform() { return DISASM_DEFAULT; }

static __filesize_t __FASTCALL__ Show_JPEG_Header()
{
    ErrMessageBox("Not implemented yet!","JPEG format");
    return BMGetCurrFilePos();
}

extern const REGISTRY_BIN jpegTable =
{
  "JPEG file format",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  jpeg_check_fmt,
  jpeg_init_fmt,
  jpeg_destroy_fmt,
  Show_JPEG_Header,
  NULL,
  jpeg_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
} // namespace beye
