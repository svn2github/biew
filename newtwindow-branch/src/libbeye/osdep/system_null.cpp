#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/unix/os_dep.c
 * @brief       This file contains implementation of unix compatible OS dependent part.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
 *
 * @author      Mauro Giachero
 * @since       11.2007
 * @note        Added __get_home_dir() and some optimizations
**/

/*
    Copyright (C) 1999-2002 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: os_dep.c,v 1.10 2009/09/03 16:57:40 nickols_k Exp $
*/
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef lint
static const char rcs_id[] = "$Id: os_dep.c,v 1.10 2009/09/03 16:57:40 nickols_k Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "system_interface.h"

#ifndef	PREFIX
#define	PREFIX	"/usr/local"
#endif

#ifndef	DATADIR
#define DATADIR	PREFIX"/share/beye"
#endif


namespace	usr {
    class system_null : public system_interface {
	public:
	    system_null();
	    virtual ~system_null();

	    virtual void		yield_timeslice() const;
	    virtual std::string		get_home_dir(const std::string& progname);
	    virtual std::string		get_ini_name(const std::string& progname);
	    virtual std::string		get_rc_dir(const std::string& progname);

	    virtual unsigned		set_timer_callback(unsigned ms,timer_callback *func);
	    virtual void		restore_timer() const;
/* National Language Support */
	    virtual void		nls_oem2osdep(unsigned char *str,unsigned size) const;
	    virtual void		nls_cmdline2oem(unsigned char *str,unsigned size) const;
	    virtual void		nls_oem2fs(unsigned char *str,unsigned size) const;
    };

std::string system_null::get_home_dir(const std::string& progname)
{
    UNUSED(progname);
    throw std::logic_error("system_null::get_home_dir");
}

std::string system_null::get_ini_name(const std::string& progname)
{
    UNUSED(progname);
    throw std::logic_error("system_null::get_ini_name");
}

std::string system_null::get_rc_dir(const std::string& progname)
{
    UNUSED(progname);
    throw std::logic_error("system_null::get_rc_dir");
}

void system_null::yield_timeslice() const
{
    throw std::logic_error("system_null::yield_timeslice");
}

unsigned system_null::set_timer_callback(unsigned ms,timer_callback func)
{
    UNUSED(ms); UNUSED(func);
    throw std::logic_error("system_null::set_timer_callback");
}

			     /* Restore time callback function to original
				state */
void system_null::restore_timer() const
{
    throw std::logic_error("system_null::restore_timer");
}

void system_null::nls_oem2osdep(unsigned char *buff, unsigned int len) const
{
    UNUSED(buff), UNUSED(len);
    throw std::logic_error("system_null::nls_oem2osdep");
}

void system_null::nls_oem2fs(unsigned char *buff, unsigned int len) const
{
    UNUSED(buff), UNUSED(len);
    throw std::logic_error("system_null::nls_oem2fs");
}

void system_null::nls_cmdline2oem(unsigned char *buff, unsigned int len) const
{
    UNUSED(buff), UNUSED(len);
    throw std::logic_error("system_null::nls_cmdline2oem");
}

/* static struct sigaction sa; */
system_null::system_null()
{
    throw missing_device_exception();
}

system_null::~system_null() {}

static system_interface* query_interface() { return new(zeromem) system_null; }

extern const system_interface_info system_null_info = {
    "null system",
    query_interface
};
} // namespace	usr
