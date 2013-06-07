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

    char  __FASTCALL__ Gebool(bool _bool) __CONST_FUNC__; // returns character representation of bool

    bool               FileUtils();

    char * __FASTCALL__ Get2Digit(uint8_t);
    char * __FASTCALL__ Get2SignDig(int8_t);
    char * __FASTCALL__ Get4Digit(uint16_t);
    char * __FASTCALL__ Get4SignDig(int16_t);
    char * __FASTCALL__ Get8Digit(uint32_t);
    char * __FASTCALL__ Get8SignDig(int32_t);
    char * __FASTCALL__ Get16Digit(uint64_t);
    char * __FASTCALL__ Get16SignDig(int64_t);
    std::string __FASTCALL__ GetBinary(char val);

    void     __FASTCALL__ CompressHex(unsigned char * dest,const char * src,unsigned sizedest,bool usespace);
    int      __FASTCALL__ ExpandHex(char * dest,const unsigned char * src,int size,char hard);
    unsigned __FASTCALL__ Summ(unsigned char *array,unsigned size) __PURE_FUNC__;

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
} // namespace	usr
#endif
