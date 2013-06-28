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
    class system_posix : public system_interface {
	public:
	    system_posix();
	    virtual ~system_posix();

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
	private:
	    static void			cleanup(int sig);
	    static void			my_alarm_handler( int signo );

	    char			_ini_name[FILENAME_MAX + 1];
	    char			_rc_dir_name[FILENAME_MAX + 1];
	    char			_home_dir_name[FILENAME_MAX + 1];

	    timer_callback*		user_func;
	    struct itimerval		otimer;
	    void			(*old_alrm)(int);

	    static const unsigned char	alt2koi[];
	    static const unsigned char	alt2win[];
	    static const unsigned char	alt2iso[];
    };
/*
The home directory is a good place for configuration
and temporary files.
At least (strlen(progname) + 9) characters should be
available before the buffer end.
The trailing '/' is included in the returned string.
*/
std::string system_posix::get_home_dir(const std::string& progname)
{
    const char *p;

    if (_home_dir_name[0]) return _home_dir_name; //Already computed

    p = getenv("HOME");
    if (p == NULL || strlen(p) < 2) {
	struct passwd* psw = getpwuid(getuid());
	if (psw != NULL) p = psw->pw_dir;
    }

    if (p == NULL || strlen(p) > FILENAME_MAX - (progname.length() + 10))
	p = "/tmp";

    strcpy(_home_dir_name, p);
    strcat(_home_dir_name, "/");

    return _home_dir_name;
}

std::string system_posix::get_ini_name(const std::string& progname)
{
    const char *p;

    if (_ini_name[0]) return _ini_name; //Already computed

    p = get_home_dir(progname).c_str();
    strcpy(_ini_name, p);
    strcat(_ini_name, ".");
    strcat(_ini_name, progname.c_str());
    return strcat(_ini_name, "rc");
}

std::string system_posix::get_rc_dir(const std::string& progname)
{
    UNUSED(progname);
    if (_rc_dir_name[0]) return _rc_dir_name; //Already computed

    strcpy(_rc_dir_name, DATADIR);
    /*strcat(_rc_dir_name, progname);*/
    return strcat(_rc_dir_name, "/");
}


void system_posix::yield_timeslice() const
{
#ifdef	__BEOS__
    /* usleep(10000); */
#else
    struct timespec t = { 0, 100000 };
    nanosleep(&t, NULL);
#endif
}

void system_posix::cleanup(int sig)
{
    std::ostringstream os;
    os<<sig;
    throw std::runtime_error(std::string("Terminated by signal ")+os.str());
}

static system_posix* handle;
void system_posix::my_alarm_handler(int signo)
{
    if(handle->user_func) (*handle->user_func)();
    UNUSED(signo);
}

unsigned system_posix::set_timer_callback(unsigned ms,timer_callback func)
{
    unsigned ret;
    struct itimerval itimer;
    user_func = func;
    ::getitimer(ITIMER_REAL,&otimer);
    old_alrm = ::signal(SIGALRM,my_alarm_handler);
    ::signal(SIGALRM,my_alarm_handler);
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = ms*1000;
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = ms*1000;
    ::setitimer(ITIMER_REAL,&itimer,NULL);
    ::getitimer(ITIMER_REAL,&itimer);
    ret = itimer.it_interval.tv_sec*1000 + itimer.it_interval.tv_usec/1000;
    if(!ret) restore_timer();
    return ret;
}

			     /* Restore time callback function to original
				state */
void system_posix::restore_timer() const
{
    ::signal(SIGALRM,old_alrm);
    ::setitimer(ITIMER_REAL,&otimer,NULL);
}

const unsigned char system_posix::alt2koi[] =
			 { 0xe1, 0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa,
			   0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
			   0xf2, 0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe,
			   0xfb, 0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1,
			   0xc1, 0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xda,
			   0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
			   0x90, 0x91, 0x92, 0x81, 0x87, 0xb2, 0xb4, 0xa7,
			   0xa6, 0xb5, 0xa1, 0xa8, 0xae, 0xad, 0xac, 0x83,
			   0x84, 0x89, 0x88, 0x86, 0x80, 0x8a, 0xaf, 0xb0,
			   0xab, 0xa5, 0xbb, 0xb8, 0xb1, 0xa0, 0xbe, 0xb9,
			   0xba, 0xb6, 0xb7, 0xaa, 0xa9, 0xa2, 0xa4, 0xbd,
			   0xbc, 0x85, 0x82, 0x8d, 0x8c, 0x8e, 0x8f, 0x8b,
			   0xd2, 0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde,
			   0xdb, 0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1,
			   0xb3, 0xa3, 0x99, 0x98, 0x93, 0x9b, 0x9f, 0x97,
			   0x9c, 0x95, 0x9e, 0x96, 0xbf, 0x9d, 0x94, 0x9a
			 };
const unsigned char system_posix::alt2win[] =
			 { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
			   0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
			   0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
			   0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
			   0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
			   0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
			   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
			   0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
			   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
			   0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
			   0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
			   0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
			   0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
			   0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
			   0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
			   0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf
			 };
const unsigned char system_posix::alt2iso[] =
			 { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
			   0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
			   0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
			   0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
			   0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
			   0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
			   0x82, 0x83, 0x84, 0x85, 0x86, 0x00, 0x00, 0x00,
			   0x00, 0x87, 0x88, 0x89, 0x8a, 0x00, 0x00, 0x8b,
			   0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x00, 0x00,
			   0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x9a, 0x9b, 0x9c, 0x9d, 0x00, 0x00, 0x9E,
			   0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
			   0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
			   0xa1, 0xf1, 0xa4, 0xf4, 0xa7, 0xf7, 0xae, 0xfe,
			   0x00, 0x00, 0x00, 0x00, 0xf0, 0x99, 0x9f, 0xa0
			 };

void system_posix::nls_oem2osdep(unsigned char *buff, unsigned int len) const
{
    nls_oem2fs(buff, len);
}

void system_posix::nls_oem2fs(unsigned char *buff, unsigned int len) const
{
    unsigned int i;
    const char* loc = NULL;
    const unsigned char* code = NULL;

    if ((loc == NULL) && ((loc = getenv("LANG")) != NULL))
    { if ((strcmp(loc, "ru_RU.KOI8-R") == 0) ||
	  (strcmp(loc, "ru_UA.KOI8-U") == 0))
	code = alt2koi;
      else if ((strcmp(loc, "ru_RU.CP1251") == 0) ||
	       (strcmp(loc, "ru_UA.CP1251") == 0))
	code = alt2win;
      else if ((strcmp(loc, "ru_RU.ISO88595") == 0) ||
	       (strcmp(loc, "ru_UA.ISO88595") == 0) ||
	       (strcmp(loc, "ru_RU") == 0) ||
	       (strcmp(loc, "ru_UA") == 0))
	code = alt2iso;
    }

    if (code != NULL)
    {
	for (i = 0; i < len; i++)
	    if (buff[i] >= 0x80) buff[i] = code[buff[i] - 0x80];
    }
}

void system_posix::nls_cmdline2oem(unsigned char *buff, unsigned int len) const
{
    UNUSED(buff), UNUSED(len);
}

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

static const char * langs[] = { "LANG", "LANGUAGE", "LINGUAS" };

char *nls_get_screen_cp()
{
    unsigned i;
    char *nls;
    static char to_cp[256];
    strcpy(to_cp,"UTF-8");
    for(i=0;i<sizeof(langs)/sizeof(char *);i++)
    {
	if((nls=getenv(langs[i]))!=NULL)
	{
		nls=strchr(nls,'.');
		if(nls) strcpy(to_cp,nls+1);
		break;
	}
    }
    return to_cp;
}

any_t* nls_init(const char *to_cp,const char *src_cp) {
#ifdef HAVE_ICONV
    iconv_t ic;
    errno=0;
    ic=iconv_open(to_cp,src_cp);
    if(errno) {
	std::cerr<<"ICONV("<<to_cp<<","<<src_cp<<"): Open with error: "<<strerror(errno)<<std::endl;
    }
    return ic;
#else
    return NULL;
#endif
}

void nls_term(any_t* ic) {
#ifdef HAVE_ICONV
 iconv_close(ic);
#endif
}

char *nls_recode2screen_cp(any_t* ic,const char *srcb,unsigned* len)
{
    char *obuff;
#ifdef HAVE_ICONV
    if(ic)
    {
	static int warned=0;
	const char *ibuff,*ib;
	char *ob;
	size_t inb,outb;
	errno=0;
	inb=*len;
	outb=((*len)+1)*4;
	obuff=new char[outb];
	ibuff=srcb;
	ob=obuff;
	ib=ibuff;
	if(iconv(ic,(char **)&ib,&inb,&ob,&outb) != (size_t)(-1))
	{
	    *ob='\0';
	    *len = ((*len)+1)*4 - outb;
	}
	else
	{
	    delete obuff;
	    if(warned<2) {
		std::cerr<<"ICONV: Can't recode: "<<strerror(errno)<<std::endl;
		warned++;
	    }
	    goto do_def;
	}
    }
    else
    {
	do_def:
	obuff=strdup(srcb);
    }
#else
    obuff=strdup(srcb);
#endif
    return obuff;
}

int nls_test(any_t* ic,const char *srcb,unsigned* len)
{
#ifdef HAVE_ICONV
    if(ic)
    {
	const char *ibuff,*ib;
	char *obuff;
	char *ob;
	size_t inb,outb,rval;
	errno=0;
	inb=*len;
	outb=((*len)+1)*4;
	obuff=new char[outb];
	ibuff=srcb;
	ob=obuff;
	ib=ibuff;
	rval=iconv(ic,(char **)&ib,&inb,&ob,&outb);
	delete obuff;
	if(iconv(ic,(char **)&ib,&inb,&ob,&outb) != (size_t)(-1))
	    return 0;
	else return errno;
    }
#endif
    return 0;
}

/* static struct sigaction sa; */
system_posix::system_posix()
	    :old_alrm(SIG_DFL)
{
    _ini_name[0] = '\0';
    _rc_dir_name[0] = '\0';
    _home_dir_name[0] = '\0';

    umask(0077);
    signal(SIGTERM, &system_posix::cleanup);
    signal(SIGINT,  &system_posix::cleanup);
    signal(SIGQUIT, &system_posix::cleanup);
    signal(SIGILL, &system_posix::cleanup);
/*
    sa.sa_handler = cleanup;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
*/
}

system_posix::~system_posix() {}

static system_interface* query_interface() { return new(zeromem) system_posix; }

extern const system_interface_info system_posix_info = {
    "posix system",
    query_interface
};
} // namespace	usr
