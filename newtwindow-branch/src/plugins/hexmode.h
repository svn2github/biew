/**
 * @namespace	usr_plugins_I
 * @file        plugins/hexmode.h
 * @brief       This file contains function prototypes for hexadecinal mode.
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
#ifndef __HEXMODE_H
#define __HEXMODE_H

struct hIniProfile;
namespace	usr {
    extern unsigned		hexAddressResolv;
    bool __FASTCALL__		hexAddressResolution(unsigned& hexAddressResolv);
    unsigned  __FASTCALL__	ReadIniAResolv(BeyeContext& bc, Ini_Profile& ini );
    void  __FASTCALL__		WriteIniAResolv(BeyeContext& bc, Ini_Profile& ini,unsigned hexAddressResolv, unsigned virt_width_corr);
} // namespace	usr
#endif
