/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/rdoff.h
 * @brief       This file contains RDOFF v1 file definitions.
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
**/
#ifndef __RDOFF_INC
#define __RDOFF_INC

#include "config.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** RDOFF v1 fixed header */
typedef struct tag_rdoff_Header
{
  uint8_t  id[6]; /**< RDOFF1 or RDOFF\01 */
  uint32_t header_len;
}rdoff_Header;

struct RDOFF_RELOC {
  uint8_t  reflen; /**< length of references */
  uint8_t  is_rel; /**< is relative fixup */
  uint16_t segto;  /**< logical # of segment or ext. reference
			    i.e. max external refers = 65536-2 */
  uint32_t offset; /**< offset from start of file */

    bool operator<(const RDOFF_RELOC& rhs) const { return offset<rhs.offset; }
};

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
