/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/pharlap.h
 * @brief       This file contains PharLap executable file definitions.
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
#ifndef __PHARLAP_INC
#define __PHARLAP_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** Format of .old Phar Lap .EXP file header */
typedef struct tagoldPharLap
{
  uint8_t            plSignature[2];   /**< "MP" 0x4D 0x50 */
  uint16_t           plSizeRemaind;    /**< remainder of image size / page size (page size = 512h) */
  uint16_t           plImageSize;      /**< size of image in pages */
  uint16_t           plNRelocs;        /**< number of relocation items */
  uint16_t           plHeadSize;       /**< header size in paragraphs */
  uint16_t           plMinExtraPages;  /**< minimum number of extra 4K pages to be allocated at the end of program, when it is loaded */
  uint16_t           plMaxExtraPages;  /**< maximum number of extra 4K pages to be allocated at the end of program, when it is loaded */
  uint32_t           plESP;            /**< initial ESP */
  uint16_t           plCheckSum;       /**< check sum of file */
  uint32_t           plEIP;            /**< initial EIP */
  uint16_t           plFirstReloc;     /**< offset of first relocation item */
  uint16_t           plNOverlay;       /**< overlay number */
  uint16_t           plReserved;       /**< (???) wants to be 1 */
}oldPharLap;

/** Format of new Phar Lap .EXP file header */
typedef struct tagnewPharLap
{
  uint8_t            plSignature[2];   /**< "P2" for 286 .EXP and "P3" for 386 .EXP */
  uint16_t           plLevel;          /**< 0x01 - flat model 0x02 - multisegmented file */
  uint16_t           plHeaderSize;
  uint32_t           plFileSize;       /**< Size of file in bytes */
  uint16_t           plCheckSum;
  uint32_t           plRunTimeParms;   /**< offset of run-time parameters within file */
  uint32_t           plRunTimeSize;    /**< size of run-time parameters in bytes */
  uint32_t           plRelocOffset;    /**< offset of relocation table within file */
  uint32_t           plRelocSize;      /**< size of relocation table in bytes */
  uint32_t           plSegInfoOffset;  /**< offset of segment information table within file */
  uint32_t           plSegInfoSize;    /**< size of segment information table in bytes */
  uint16_t           plSegEntrySize;   /**< size of segment information table entry in bytes */
  uint32_t           plImageOffset;    /**< offset of load image within file */
  uint32_t           plImageSize;      /**< size of load image on disk */
  uint32_t           plSymTabOffset;   /**< offset of symbol table within file */
  uint32_t           plSymTabSize;     /**< size of symbol table in bytes */
  uint32_t           plGDTOffset;      /**< offset of GDT within load image */
  uint32_t           plGDTSize;        /**< size of GDT in bytes */
  uint32_t           plLDTOffset;      /**< offset of LDT within load image */
  uint32_t           plLDTSize;        /**< size of LDT in bytes */
  uint32_t           plIDTOffset;      /**< offset of IDT within load image */
  uint32_t           plIDTSize;        /**< size of IDT in bytes */
  uint32_t           plTSSOffset;      /**< offset of TSS within load image */
  uint32_t           plTSSSize;        /**< size of TSS in bytes */
  uint32_t           plMinExtraPages;  /**< minimum number of extra 4K pages to be allocated at the end of program, level 1 only */
  uint32_t           plMaxExtraPages;  /**< maximum number of extra 4K pages to be allocated at the end of program, level 1 only */
  uint32_t           plBase;           /**< base load offset (level 1 executables only) */
  uint32_t           plESP;            /**< initial ESP */
  uint16_t           plSS;             /**< initial SS */
  uint32_t           plEIP;            /**< initial EIP */
  uint16_t           plCS;             /**< initial CS */
  uint16_t           plLDT;            /**< initial LDT */
  uint16_t           plTSS;            /**< initial TSS */
  uint16_t           plFlags;          /**< bit 0: load image is packed */
				     /**< bit 1: 32-bit checksum is present */
				     /**< bits 4-2: type of relocation table */
  uint32_t           plMemReq;         /**< memory requirements for load image */
  uint32_t           plChecksum32;     /**< 32-bit checksum (optional) */
  uint32_t           plStackSize;      /**< size of stack segment in bytes */
  uint8_t            plReserv[256];
}newPharLap;

typedef struct tagPLSegInfo
{
  uint16_t           siSelector;       /**< selector number */
  uint16_t           siFlags;
  uint32_t           siBaseOff;        /**< base offset of selector */
  uint32_t           siMinAlloc;       /**< minimum number of extra bytes to be allocated to the segment */
}PLSegInfo;

typedef struct tagPLRunTimeParms
{
  uint8_t            rtSignature[2];   /**< "DX" 44h 58h */
  uint16_t           rtMinRModeParms;  /**< minimum number of real-mode params to leave free at run time */
  uint16_t           rtMaxRModeParms;  /**< maximum number of real-mode params to leave free at run time */
  uint16_t           rtMinIBuffSize;   /**< minimum interrupt buffer size in KB */
  uint16_t           rtMaxIBuffSize;   /**< maximum interrupt buffer size in KB */
  uint16_t           rtNIStacks;       /**< number of interrupt stacks */
  uint16_t           rtIStackSize;     /**< size in KB of each interrupt stack */
  uint32_t           rtEndRModeOffset; /**< offset of byte past end of real-mode code and data */
  uint16_t           rtCallBuffSize;   /**< size in KB of call buffers */
  uint16_t           rtFlags;
				     /**< bit 0: file is virtual memory manager */
				     /**< bit 1: file is a debugger */
  uint16_t           rtUnprivFlags;    /**< unprivileged flag (if nonzero, executes at ring 1, 2, or 3) */
  uint8_t            rtReserv[104];
}PLRunTimeParms;

typedef struct tagPLRepeatBlock
{
  uint16_t           rbCount;          /**< byte count */
  uint8_t            rbString[1];      /**< repeat string length */
}PLRepeatBlock;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
