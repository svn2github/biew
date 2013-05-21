#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/bbio.c
 * @brief       This file contains implementation of Buffering binary input/ouput routines.
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
 * @todo        Support for preemptive memory allocation and multiple buffers for
 *              one opened stream
**/
#include <algorithm>
#include <iostream>

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "libbeye/bbio.h"

namespace	usr {
BBio_File::BBio_File(unsigned bSize,unsigned optimization)
	:buffsize(bSize)
	,buffer(new char[bSize])
	,optimize(optimization)
{
}

BBio_File::~BBio_File() { close(); delete buffer; }

bool BBio_File::open(const std::string& _fname,unsigned _openmode)
{
    if(!binary_stream::open(_fname,_openmode)) return false;
    handle().rdbuf()->pubsetbuf(buffer,buffsize);
    return true;
}

unsigned BBio_File::set_optimization(unsigned flags)
{
    unsigned ret;
    ret = optimize;
    optimize = flags;
    return ret;
}

unsigned BBio_File::get_optimization() const
{
    return optimize;
}

bool BBio_File::dup(BBio_File& it) const
{
    bool rc = binary_stream::dup(it);
    it.handle().rdbuf()->pubsetbuf(it.buffer,it.buffsize);
    return rc;
}

binary_stream* BBio_File::dup()
{
    BBio_File* ret = new(zeromem) BBio_File(buffsize,Opt_Db);
    if(!dup(*ret)) return this;
    return ret;
}
} // namespace	usr
