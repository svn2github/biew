/**
 * @namespace   libbeye
 * @file        libbeye/osdep/__os_dep.h
 * @brief       This file contains all operating system depended part of BEYE project.
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
 * @warning     Under some OSes program may be destroyed if more than one timer per program is created
 * @deprecated  OS File manipulating functions: I had problem with standard i/o
 *              function like: open, read, write in different C libraries such
 *              as emx and possible some one.  It cause to born this section.
 *
 * @author      Mauro Giachero
 * @since       11.2007
 * @note        Added __get_home_dir()
**/
#ifndef __OS_DEP_H
#define __OS_DEP_H 1

#include <limits>

#ifndef __SYS_DEP_H
#include "libbeye/_sys_dep.h"
#endif

/* OS specific functions */
		   /** Initializes system depended part of libbeye.
		     * @return                none
		     * @note                  You must call this function before calling any other function from libbeye library
		     * @see                   __term_sys
		    **/
extern void      __FASTCALL__ __init_sys();

		   /** Terminates system depended part of libbeye.
		     * @return                none
		     * @note                  You must call this function after last call any other function from libbeye library
		     * @see                   __init_sys
		    **/
extern void      __FASTCALL__ __term_sys();

		   /** Realizes time slice between waiting of input events
		     * @return                none
		     * @note                  This function provides mechanism of
		     *                        system speedup, when application is
		     *                        waiting for input events. For realizing
		     *                        time slice inside of computing loops
		     *                        application must call SLEEP or it
		     *                        analogs.
		    **/
extern void      __FASTCALL__ __OsYield();

		   /** Gets ctrl-break signal
		     * @return                true if occured; false - otherwise
		     * @note                  Function does not differ soft and
		     *                        hard c-break.
		     * @warning               After getting signal by program
					      semaphore is not being cleaned.
		    **/
extern bool     __FASTCALL__ __OsGetCBreak();

		   /** Sets control-break signal
		     * @return                none
		     * @param state           indicates new state of
					      control-break semaphore
		     * @note                  Function does not differ soft and
		     *                        hard c-break.
		    **/
extern void      __FASTCALL__ __OsSetCBreak( bool state );

		   /** Builds OS specific name of home directory
		     * @return                Slash terminated path to home directory
		     * @param progname        indicates name of program, that can be used to revert to a fallback in case the path gets too long
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_home_dir
		    **/
extern char *    __FASTCALL__ __get_home_dir( const char *progname );

		   /** Builds OS specific name of initializing file
		     * @return                fully qualified name of .ini file
		     * @param progname        indicates name of program, that will be used as part of .ini file name
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_rc_dir
		    **/
extern char *    __FASTCALL__ __get_ini_name( const char *progname );

		   /** Builds OS specific name of program resource directory
		     * @return                Slash terminated path to program resource directory
		     * @param progname        indicates name of program.
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_ini_name
		    **/
extern char *    __FASTCALL__ __get_rc_dir( const char *progname );

typedef void timer_callback(); /**< This is the code type used to represent user supplied function of timer callback */

		   /** Sets user defined function as timer callback with given time interval
		     * @return                Real call back interval in milliseconds
		     * @param ms              indicates desired call back interval in milliseconds
		     * @param func            indicates user supplied function to be used as timer callback
		     * @see                   __OsRestoreTimer
		    **/
extern unsigned  __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback *func);

		   /** Restores time callback function to original state
		     * @return                none
		     * @see                   __OsSetTimercallBack
		    **/
extern void      __FASTCALL__ __OsRestoreTimer();

/* National Language Support */

	   /** Checks whether the specified character is OEM pseudographical symbol */
inline bool NLS_IS_OEMPG(unsigned char ch) { return ch >= 0xB0 && ch <= 0xDF; }

		   /** Converts buffer from OEM codepage to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_CmdlineToOem __nls_CmdlineToFs
		    **/
extern void      __FASTCALL__ __nls_OemToOsdep(unsigned char *str,unsigned size);

		   /** Converts buffer from codepage of command line to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @depricated            This function only used in Win32
		     *                        where codepages of console and command
		     *                        line is differ.
		     * @see                   __nls_OemToOsdep __nls_CmdlineToFs
		    **/
extern void      __FASTCALL__ __nls_CmdlineToOem(unsigned char *str,unsigned size);

		   /** Converts buffer from OEM codepage to currently used by OS's file system.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_OemToOsdep __nls_CmdlineToOem
		    **/
extern void      __FASTCALL__ __nls_OemToFs(unsigned char *str,unsigned size);

#endif
