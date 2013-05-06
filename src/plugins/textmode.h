/**
 * @namespace   beye_plugins_I
 * @file        plugins/textmode.h
 * @brief       This file contains prototypes for textmode plugins.
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
#ifndef __TEXTMODE_H
#define __TEXTMODE_H

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "libbeye/file_ini.h"
#endif

namespace beye {
typedef struct tag_REGISTRY_NLS
{
  const char *   set_name;
  unsigned       (__FASTCALL__ *convert_buffer)(char *buffer,unsigned len,bool use_fs_nls);
  unsigned       (__FASTCALL__ *get_symbol_size)( void );
  bool          (__FASTCALL__ *select_table)( void );
  void           (__FASTCALL__ *read_ini)(Ini_Profile& );
  void           (__FASTCALL__ *save_ini)(Ini_Profile& );
  void           (__FASTCALL__ *init)( void );
  void           (__FASTCALL__ *term)( void );
}REGISTRY_NLS;

/** Below describes function prototypes for small table conversations */
    void __FASTCALL__ txt_cvt_full(char * str,int size,const unsigned char *tmpl);
    void __FASTCALL__ txt_cvt_hi80(char * str,unsigned size,const unsigned char *tmpl);
    void __FASTCALL__ txt_cvt_lo80(char * str,unsigned size,const unsigned char *tmpl);
} // namespace beye
#endif


