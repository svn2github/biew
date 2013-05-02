#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        refs.c
 * @brief       This file contains basic level routines for resolving references.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "beye.h"
#include "plugins/plugin.h"
#include "beyeutil.h"
#include "bconsole.h"

namespace beye {
bool __FASTCALL__ AppendAsmRef(const DisMode& parent,char *str,__filesize_t ulShift,int mode,char codelen,__filesize_t r_sh)
{
  return beye_context().bin_format().bind(parent,str,ulShift,mode,codelen,r_sh);
}
} // namespace beye
