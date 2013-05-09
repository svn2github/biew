/**
 * @namespace	usr
 * @file        beyeutil.h
 * @brief       This file contains prototypes of BEYE utilities.
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
#ifndef __BEYEUTIL__H
#define __BEYEUTIL__H

#include <limits>
#include <limits.h>

#include "libbeye/libbeye.h"
#include "libbeye/twindow.h"
#include "libbeye/bstream.h"
#include "libbeye/file_ini.h"

namespace	usr {
    class DisMode;
    extern const char legalchars[];
    extern bool DumpMode;
    extern bool EditMode;

    extern __filesize_t lastbyte;
    extern char * ini_name;

    int   __FASTCALL__ Gebool(bool _bool);
    any_t**            FAllocPtrPtr(unsigned num);
    any_t*             FAllocPtr(unsigned size);
    void               FFreeArr(any_t**arr,unsigned n);
    void               CriticalExit(int code);

    bool               FileUtils();

    char * __FASTCALL__ Get2Digit(uint8_t);
    char * __FASTCALL__ Get2SignDig(int8_t);
    char * __FASTCALL__ Get4Digit(uint16_t);
    char * __FASTCALL__ Get4SignDig(int16_t);
    char * __FASTCALL__ Get8Digit(uint32_t);
    char * __FASTCALL__ Get8SignDig(int32_t);
    char * __FASTCALL__ Get16Digit(uint64_t);
    char * __FASTCALL__ Get16SignDig(int64_t);
    char * __FASTCALL__ GetBinary(char val);

    void     __FASTCALL__ CompressHex(unsigned char * dest,const char * src,unsigned sizedest,bool usespace);
    unsigned __FASTCALL__ Summ(unsigned char *array,unsigned size);

    void   ExtHelp();
    void   drawEditPrompt();
    void   drawEmptyPrompt();
    void   drawEmptyListPrompt();
    void   drawAsmEdPrompt();
    int    EditAsmActionFromMenu();
    void   drawListPrompt();
    void   drawOrdListPrompt();
    void   drawSearchListPrompt();
    void   drawHelpPrompt();
    int    HelpActionFromMenu();
    void   drawHelpListPrompt();
    void   drawPrompt();
    int    MainActionFromMenu();
    void   About();

    __filesize_t __FASTCALL__ WhereAMI(__filesize_t ctrl_pos);

    void  ShowSysInfo();

    int  __FASTCALL__ isHOnLine(__filesize_t cp,int width);
    enum {
	HLS_NORMAL               =0x0000,
	HLS_USE_DOUBLE_WIDTH     =0x0001,
	HLS_USE_BUFFER_AS_VIDEO  =0x0002
    };

typedef union tag_HLInfo
{
  const char     *text;
  tvioBuff        buff;
}HLInfo;

    void __FASTCALL__ HiLightSearch(TWindow *out,__filesize_t cfp,tRelCoord minx,
			  tRelCoord maxx,tRelCoord y,HLInfo *buff,unsigned flags);

/** Class memory array */

typedef struct tag_memArray
{
  any_t**  data;
  unsigned nItems;
  unsigned nSize;
}memArray;

    memArray *__FASTCALL__ ma_Build( int maxitems, bool interact );
    bool     __FASTCALL__ ma_AddString(memArray *obj,const std::string& data,bool interact);
    bool     __FASTCALL__ ma_AddData(memArray *obj,const any_t*data,unsigned size,bool interact);
    void      __FASTCALL__ ma_Destroy(memArray *obj);
    int       __FASTCALL__ ma_Display(memArray *obj,const std::string& title,int flg,unsigned defsel);

    int      __FASTCALL__ ExpandHex(char * dest,const unsigned char * src,int size,char hard);

    void		PaintTitle();
    __filesize_t       IsNewExe();
} // namespace	usr
#endif
