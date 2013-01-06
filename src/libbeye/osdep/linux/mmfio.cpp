/**
 * @namespace   libbeye
 * @file        libbeye/osdep/linux/mmfio.c
 * @brief       This file contains implementation of memory mapped file i/o routines for POSIX compatible OS.
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
#include <algorithm>
#ifdef __DISABLE_MMF
#include "libbeye/osdep/posix/mmfio.cpp"
#else
#include <sys/mman.h>
#include "libbeye/pmalloc.h"
#include "libbeye/libbeye.h"

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

/* The lack of this function declaration on some systems and may cause segfault */
extern any_t*mremap (any_t*__addr, size_t __old_len, size_t __new_len,
		     int __flags, ...);

struct mmfRecord
{
  any_t*    addr;
  long      length;
  bhandle_t fhandle;
  int       mode;
};

static int __FASTCALL__ mk_prot(int mode)
{
  int pflg;
  pflg = PROT_READ;
  if(mode & FO_WRITEONLY) pflg = PROT_WRITE;
  else
   if(mode & FO_READWRITE) pflg |= PROT_WRITE;
  return pflg;
}

static int __FASTCALL__ mk_flags(int mode)
{
  int pflg;
  pflg = 0;
#ifdef MAP_SHARED
  if((mode & SO_DENYREAD) ||
     (mode & SO_DENYWRITE) ||
     (mode & SO_DENYNONE))          pflg |= MAP_SHARED;
#endif
#ifdef MAP_PRIVATE
  if(mode & SO_PRIVATE) pflg |= MAP_PRIVATE;
#endif
  return pflg;
}

mmfHandle          __FASTCALL__ __mmfOpen(const char *fname,int mode)
{
  struct mmfRecord *mret;
  bhandle_t fhandle;
  fhandle = __OsOpen(fname,mode);
  if(((long)fhandle) != -1)
  {
    mret = new mmfRecord;
    if(mret)
    {
      __fileoff_t length;
      any_t*addr;
      length = __FileLength(fhandle);
      if(length <= std::numeric_limits<long>::max())
      {
	addr = mmap(NULL,length,mk_prot(mode),
		  mk_flags(mode),reinterpret_cast<long>(fhandle),0L);
	if(addr != (any_t*)-1)
	{
	    mret->fhandle = fhandle;
	    mret->addr    = addr;
	    mret->length  = length;
	    mret->mode    = mode;
	    return mret;
	}
     }
     delete mret;
    }
    __OsClose(fhandle);
  }
  return NULL;
}

bool              __FASTCALL__ __mmfFlush(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  return msync(mrec->addr,mrec->length,MS_SYNC) ? false : true;
}

mmfHandle       __FASTCALL__ __mmfSync(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  long length;
  any_t* new_addr;
  length = __FileLength(mrec->fhandle);
  msync(mrec->addr,std::min(length,mrec->length),MS_SYNC);
  if(length!=mrec->length) {
    if((new_addr = mremap(mrec->addr,mrec->length,length,0)) != (any_t*)-1)
    {
	mrec->length = length;
	mrec->addr = new_addr;
    }
    else
    {
	__OsClose(mrec->fhandle);
	delete mrec;
	return NULL;
    }
  }
  return mrec;
}

bool              __FASTCALL__ __mmfProtect(mmfHandle mh,int flags)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  mrec->mode = flags;
  return mprotect(mrec->addr,mrec->length,mk_prot(flags)) ? false : true;
}

bool              __FASTCALL__ __mmfResize(mmfHandle mh,long size)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  any_t*new_addr;
  bool can_continue = false;
  if(mrec->length > size) /* truncate */
  {
    if((new_addr = mremap(mrec->addr,mrec->length,size,MREMAP_MAYMOVE)) != (any_t*)-1) can_continue = true;
    if(can_continue)
      can_continue = __OsChSize(mrec->fhandle,size) != -1 ? true : false;
  }
  else /* expand */
  {
    if(__OsChSize(mrec->fhandle,size) != -1) can_continue = true;
    if(can_continue)
      can_continue = (new_addr = mremap(mrec->addr,mrec->length,size,MREMAP_MAYMOVE)) != (any_t*)-1 ? true : false;
  }
  if(can_continue)
  {
      mrec->addr = new_addr;
      mrec->length = size;
      return true;
  }
  else /* Attempt to unroll transaction back */
    __OsChSize(mrec->fhandle,mrec->length);
  return false;
}

void               __FASTCALL__ __mmfClose(mmfHandle mh)
{
  struct mmfRecord *mrec = (struct mmfRecord *)mh;
  munmap(mrec->addr,mrec->length);
  __OsClose(mrec->fhandle);
  delete mrec;
}

any_t*             __FASTCALL__ __mmfAddress(mmfHandle mh)
{
  return ((struct mmfRecord *)mh)->addr;
}

long              __FASTCALL__ __mmfSize(mmfHandle mh)
{
  return ((struct mmfRecord *)mh)->length;
}

bool             __FASTCALL__ __mmfIsWorkable( void ) { return true; }

#endif
