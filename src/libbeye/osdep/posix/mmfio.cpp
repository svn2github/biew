#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/posix/mmfio.c
 * @brief       This file contains generic implementation of memory mapped file i/o routines.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <errno.h>
#include <stdlib.h>

#ifndef ENOSYS
#define ENOSYS -1
#endif

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  UNUSED(fname);
  UNUSED(mode);
  errno = ENOSYS;
  return NULL;
}

bool              __FASTCALL__ __mmfFlush(mmfHandle mh)
{
  UNUSED(mh);
  errno = ENOSYS;
  return false;
}

mmfHandle     __FASTCALL__ __mmfSync(mmfHandle mh)
{
  UNUSED(mh);
  return NULL;
}

bool          __FASTCALL__ __mmfProtect(mmfHandle mh,int flags)
{
  UNUSED(mh);
  UNUSED(flags);
  errno = ENOSYS;
  return false;
}

bool              __FASTCALL__ __mmfResize(mmfHandle mh,long size)
{
  UNUSED(mh);
  UNUSED(size);
  errno = ENOSYS;
  return false;
}

void               __FASTCALL__ __mmfClose(mmfHandle mh)
{
  UNUSED(mh);
  errno = ENOSYS;
}

any_t*             __FASTCALL__ __mmfAddress(mmfHandle mh)
{
  UNUSED(mh);
  return (any_t*)-1;
}

long              __FASTCALL__ __mmfSize(mmfHandle mh)
{
  UNUSED(mh);
  return 0L;
}

bool             __FASTCALL__ __mmfIsWorkable( void ) { return false; }