#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/win32/os_dep.c
 * @brief       This file contains implementation of OS depended part for Win32s.
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
 *
 * @author      Mauro Giachero
 * @since       11.2007
 * @note        Added __get_home_dir() and some optimizations
**/
#include <algorithm>
/* for cygwin - remove unnecessary includes */
#define _OLE_H
#define _OLE2_H
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

OSVERSIONINFO win32_verinfo;

static char rbuff[FILENAME_MAX+1];
static char rbuff2[FILENAME_MAX+1];
static char _home_dir_name[FILENAME_MAX + 1];

static bool __c__break = false;

#if defined( _MSC_VER ) || __GNUC_MINOR__ >= 95
static BOOL WINAPI MyHandler( DWORD type )
#else
static BOOL __attribute((stdcall)) MyHandler( DWORD type )
#endif
{
  switch(type)
  {
     case CTRL_C_EVENT:
     case CTRL_BREAK_EVENT:
			 if(__c__break)  exit(EXIT_FAILURE);
			 else __c__break = true;
			 return true;
     default:
			 return FALSE;
  }
}

bool __FASTCALL__ __OsGetCBreak()
{
  return __c__break;
}

void __FASTCALL__ __OsSetCBreak( bool state )
{
  __c__break = state;
}

extern HANDLE hIn;
extern bool hInputTrigger;

void __FASTCALL__ __init_sys()
{
  win32_verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&win32_verinfo);
  SetConsoleCtrlHandler(MyHandler,TRUE);

  rbuff[0] = '\0';
  rbuff2[0] = '\0';
  _home_dir_name[0] = '\0';
}

void __FASTCALL__ __term_sys()
{
  SetConsoleCtrlHandler(MyHandler,FALSE);
}

void __FASTCALL__ __OsYield()
{
       WaitForSingleObject(hIn,INFINITE);
       hInputTrigger++;
}

static void __FASTCALL__ getStartupFolder(char *to,unsigned cblen)
{
   GetModuleFileName(0,to,cblen);
}

char * __FASTCALL__ __get_ini_name( const char *progname )
{
   int len;

   if (rbuff[0]) return rbuff; //Already computed

   UNUSED(progname);
   getStartupFolder(rbuff,sizeof(rbuff));
   len = strlen(rbuff);
   if(stricmp(&rbuff[len-4],".exe") == 0) strcpy(&rbuff[len-4],".ini");
   else                                   strcat(rbuff,".ini");
   return rbuff;
}

char * __FASTCALL__ __get_rc_dir( const char *progname )
{
   char *p1,*p2,last;

   if (rbuff2[0]) return rbuff2; //Already computed

   UNUSED(progname);
   getStartupFolder(rbuff2,sizeof(rbuff2));
   p1 = strrchr(rbuff2,'\\');
   p2 = strrchr(rbuff2,'/');
   p1 = std::max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(rbuff2,"\\");
   return rbuff2;
}

/*
The home directory is a good place for configuration
and temporary files.
The trailing '\\' is included in the returned string.
*/
char * __FASTCALL__ __get_home_dir(const char *progname)
{
   char *p1,*p2,last;

   if (_home_dir_name[0]) return _home_dir_name; //Already computed

   UNUSED(progname);
   getStartupFolder(_home_dir_name,sizeof(_home_dir_name));
   p1 = strrchr(_home_dir_name,'\\');
   p2 = strrchr(_home_dir_name,'/');
   p1 = std::max(p1,p2);
   if(p1) p1[1] = '\0';
   last = p1[strlen(p1)-1];
   if(!(last == '\\' || last == '/')) strcat(_home_dir_name,"\\");
   return _home_dir_name;
}

#if defined(__GNUC__) && !defined(_MMSYSTEM_H) && __MACHINE__!=x86_64
/****************************************************************\
* Cygnus GNU C/C++ v0.20b does not have 'mmsystem.h' header file *
\****************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/** timer device capabilities data structure */
typedef struct timecaps_tag {
    UINT    wPeriodMin;     /**< minimum period supported  */
    UINT    wPeriodMax;     /**< maximum period supported  */
    } TIMECAPS;
typedef TIMECAPS       *PTIMECAPS;
typedef TIMECAPS       *LPTIMECAPS;

UINT WINAPI timeGetDevCaps(TIMECAPS FAR* lpTimeCaps, UINT uSize);

/** timer data types */
typedef void (CALLBACK TIMECALLBACK) (UINT uTimerID,
				      UINT uMessage,
				      DWORD dwUser,
				      DWORD dw1,
				      DWORD dw2);

typedef TIMECALLBACK FAR *LPTIMECALLBACK;

/* flags for wFlags parameter of timeSetEvent() function */
#define TIME_ONESHOT    0   /**< program timer for single event */
#define TIME_PERIODIC   1   /**< program for continuous periodic event */

extern UINT WINAPI timeSetEvent( UINT uDelay,
				 UINT uResolution,
				 LPTIMECALLBACK lpTimeProc,
				 DWORD dwUser,
				 UINT fuEvent );

extern UINT WINAPI timeKillEvent( UINT uTimerID );
extern UINT WINAPI timeBeginPeriod(UINT uPeriod);
extern UINT WINAPI timeEndPeriod(UINT uPeriod);

#ifdef __cplusplus
}
#endif
#endif

static UINT uTimerID = 0;
static UINT uPeriod = 0;
static timer_callback *user_callback = NULL;

static VOID CALLBACK my_callback( UINT _uTimerID, UINT uMessage, DWORD dwUser, DWORD dw1, DWORD dw2 )
{
  UNUSED(_uTimerID);
  UNUSED(uMessage);
  UNUSED(dwUser);
  UNUSED(dw1);
  UNUSED(dw2);
  if(user_callback) (*user_callback)();
}

unsigned   __FASTCALL__ __OsSetTimerCallBack(unsigned ms,timer_callback *func)
{
  UINT cur_period;
  TIMECAPS tc;
   if(!uTimerID)
   {
     user_callback = func;
     if(timeGetDevCaps(&tc,sizeof(TIMECAPS)) == 0)
     {
       cur_period = tc.wPeriodMin;
       uPeriod = ms / cur_period;
       if(uPeriod < ms) uPeriod += cur_period-1;
       if(uPeriod > tc.wPeriodMax) uPeriod = tc.wPeriodMax-1;
       if(timeBeginPeriod(uPeriod) == 0)
       {
	 uTimerID = timeSetEvent(uPeriod,0,my_callback,0L,TIME_PERIODIC);
	 return uPeriod;
       }
     }
   }
   return 0;
}
			     /* Restore time callback function to original
				state */
void   __FASTCALL__ __OsRestoreTimer()
{
  if(uTimerID)
  {
    timeEndPeriod(uPeriod);
    timeKillEvent(uTimerID);
    uTimerID = 0;
  }
}

bool win32_use_ansi;

void __FASTCALL__ __nls_OemToOsdep(unsigned char *buff,unsigned len)
{
 if(win32_use_ansi)
 {
   OemToCharBuff((LPCSTR)buff,(LPSTR)buff,len);
 }
}

void __FASTCALL__ __nls_OemToFs(unsigned char *buff,unsigned len)
{
}

void __FASTCALL__ __nls_CmdlineToOem(unsigned char *buff,unsigned len)
{
   CharToOemBuff((LPCSTR)buff,(LPSTR)buff,len);
}
