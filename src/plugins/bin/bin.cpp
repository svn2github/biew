#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/bin.c
 * @brief       This file contains implementation of decoder for any not handled
 *              binary file format.
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
#include <stddef.h>

#include "reg_form.h"
#include "plugins/disasm.h"

namespace	usr {
static bool  __FASTCALL__ bin_check_fmt() { return true; }
static void __FASTCALL__ bin_init_fmt(CodeGuider& code_guider) { UNUSED(code_guider); }
static void __FASTCALL__ bin_destroy_fmt() {}
static int  __FASTCALL__ bin_platform() { return DISASM_DEFAULT; }

extern const REGISTRY_BIN binTable =
{
  "Binary file",
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  bin_check_fmt,
  bin_init_fmt,
  bin_destroy_fmt,
  NULL,
  NULL,
  bin_platform,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
} // namespace	usr
