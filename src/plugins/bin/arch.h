/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/arch.h
 * @brief       This file contains Archive file definitions.
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
#ifndef __ARCH_INC
#define __ARCH_INC

#include "config.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif


/** Note that the usual '\n' in magic strings may translate to different
   characters, as allowed by ANSI.  '\012' has a fixed value, and remains
   compatible with existing BSDish archives. */

#define ARMAG  "!<arch>\012"	/**< For COFF and a.out archives */
#define ARMAGB "!<bout>\012"	/**< For b.out archives */
#define SARMAG 8
#define ARFMAG "`\012"

/** The ar_date field of the armap (__.SYMDEF) member of an archive
   must be greater than the modified date of the entire file, or
   BSD-derived linkers complain.  We originally write the ar_date with
   this offset from the real file's mod-time.  After finishing the
   file, we rewrite ar_date if it's not still greater than the mod date.  */

#define ARMAP_TIME_OFFSET       60

struct ar_hdr {
    int8_t  ar_magic[8];	/**< !<arch>012 */
    int8_t  ar_name[16];	/**< name of this member */
    int8_t  ar_date[12];	/**< file mtime */
    uint8_t ar_uid[6];		/**< owner uid; printed as decimal */
    uint8_t ar_gid[6];		/**< owner gid; printed as decimal */
    uint8_t ar_mode[8];		/**< file mode, printed as octal   */
    uint8_t ar_size[10];	/**< file size, printed as decimal */
    uint8_t ar_fmag[2];		/**< should contain ARFMAG */
};

struct ar_sub_hdr {
    int8_t  ar_name[16];	/**< name of this member */
    int8_t  ar_date[12];	/**< file mtime */
    uint8_t ar_uid[6];		/**< owner uid; printed as decimal */
    uint8_t ar_gid[6];		/**< owner gid; printed as decimal */
    uint8_t ar_mode[8];		/**< file mode, printed as octal   */
    uint8_t ar_size[10];	/**< file size, printed as decimal */
    uint8_t ar_fmag[2];		/**< should contain ARFMAG */
};

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
