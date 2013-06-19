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

		   /** Initializes keyboard handler.
		     * @return                none
		     * @note                  You must call this function before calling any other keyboard related functions
		     * @see                   __term_keyboard
		    **/
extern void      __FASTCALL__ __init_keyboard( const char *user_cp );

		   /** Terminates keyboard handler.
		     * @return                none
		     * @note                  You must call this function after last call any other keyboard related functions
		     * @see                   __init_keyboard
		    **/
extern void      __FASTCALL__ __term_keyboard();

		   /** Receives next keyboard event or note about mouse event.
		     * @return                event. For detail see KE_* flag definitions in keycode.h file.
		     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
		     * @note                  This function only sends note about
		     *                        occured mouse event. For detailizing
		     *                        event you must call one of__Ms*
		     *                        function family.
		     * @see                   __kbdTestKey __kbdGetShiftKey
		    **/
extern int       __FASTCALL__ __kbdGetKey( unsigned long flg );

		   /** Checks the keyboard and mouse to determine whether there is available event.
		     * @return                event if available, 0 - otherwise. For detail see KE_* flag definitions in keycode.h file.
		     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
		     * @note                  This function only sends note about
		     *                        occured mouse event. For detailizing
		     *                        event you must call one of__Ms*
		     *                        function family.
		     * @see                   __kbdGetKey __kbdGetShiftKey
		    **/
extern int       __FASTCALL__ __kbdTestKey( unsigned long flg );

		   /** Determines the current SHIFT key status.
		     * @return                Current SHIFT key state. For detail see KS_* flag definitions in keycode.h file.
		     * @see                   __kbdTestKey __kbdGetKey
		    **/
extern int       __FASTCALL__ __kbdGetShiftsKey();

		   /** Describes info about input events.
		     * @param head            Pointer to header to be filled
		     * @param text            Pointer to formatted text to be filled
		     * @return                0 if ESCAPE's functionality key pressed;
					      1 if regulare event occured
					      -1 if functionality is not implemented(supported).
		    **/
extern int       __FASTCALL__ __inputRawInfo(char *head, char *text);

		   /** Initializes mouse handler.
		     * @return                none
		     * @note                  You must call this function before calling any other mouse related functions
		     * @see                   __term_mouse
		    **/
extern int       __FASTCALL__ __init_mouse();

		   /** Terminates mouse handler.
		     * @return                none
		     * @note                  You must call this function after last call any other mouse related functions
		     * @see                   __init_mouse
		    **/
extern void      __FASTCALL__ __term_mouse();

		   /** Returns mouse cursor visibility.
		     * @return                true if mouse cursor is in visible state
		     * @see                   __MsGetPos __MsGetBtns __MsSetState
		    **/
extern bool     __FASTCALL__ __MsGetState();

		   /** Sets mouse cursor visibility.
		     * @return                none
		     * @param is_visible      Indicates visibility of mouse cursor
		     * @see                   __MsGetState
		    **/
extern void      __FASTCALL__ __MsSetState( bool is_visible );

		   /** Returns mouse position in screen coordinates.
		     * @return                none
		     * @param x,y             Pointers to memory where will be stored current mouse coordinates.
		     * @note                  Coordinates is measured in text cells of screen
		     * @see                   __MsGetBtns __MsGetState
		    **/
extern void      __FASTCALL__ __MsGetPos( tAbsCoord *x, tAbsCoord *y );

		   /** Returns mouse buttons state.
		     * @return                State of mouse button. For detail see MS_ flag definitions.
		     * @see                   __MsGetState __MsGetPos
		    **/
extern int       __FASTCALL__ __MsGetBtns();

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

		   /** Prepares tvioBuff buffer from OEM codepage to currently used by OS.
		     * @return                none
		     * @param it              buffer to be converted
		     * @param size            size of buffer elemets in bytes
		     * @see                   __nls_OemToOsdep __nls_CmdlineToOem NLS_IS_OEMPG
		    **/
extern void      __FASTCALL__ __nls_PrepareOEMForTVio(tvioBuff *it,unsigned size);

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
