/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/lx_le.h
 * @brief       This file contains LX and LE executable file definitions.
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
#ifndef __LX_LE_INC
#define __LX_LE_INC

#ifndef __BEYEUTIL__H
#include "beyeutil.h"
#endif

#ifndef __BBIO_H
#include "bbio.h"
#endif

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/** Linear eXecutable header */
typedef struct tagLXHEADER
{
  uint8_t    lxSignature[2];  /**< 'LX' */
  uint8_t    lxByteOrdering;
  uint8_t    lxWordOrdering;
  uint32_t   lxFormatLevel;
  uint16_t   lxCPUType;
  uint16_t   lxOSType;
  uint32_t   lxModuleVersion;
  uint32_t   lxModuleFlags;
  uint32_t   lxPageCount;
  uint32_t   lxEIPObjectNumbers;
  uint32_t   lxEIP;
  uint32_t   lxESPObjectNumbers;
  uint32_t   lxESP;
  uint32_t   lxPageSize;
  /* ------ specific LX part --------- */
  uint32_t   lxPageOffsetShift;
  uint32_t   lxFixupSectionSize;
  uint32_t   lxFixupSectionChecksum;      /**< different LE/LX part */
  uint32_t   lxLoaderSectionSize;
  uint32_t   lxLoaderSectionChecksum;
  uint32_t   lxObjectTableOffset;
  uint32_t   lxObjectCount;
  uint32_t   lxObjectPageTableOffset;
  uint32_t   lxObjectIterPageOffset;
  uint32_t   lxResourceTableOffset;
  uint32_t   lxNumberResourceTableEntries;
  uint32_t   lxResidentNameTableOffset;
  uint32_t   lxEntryTableOffset;
  uint32_t   lxModuleDirectivesOffset;
  uint32_t   lxNumberModuleDirectives;
  uint32_t   lxFixupPageTableOffset;
  uint32_t   lxFixupRecordTableOffset;
  uint32_t   lxImportModuleTableOffset;
  uint32_t   lxImportModuleTableEntries;
  uint32_t   lxImportProcedureTableOffset;
  uint32_t   lxPerPageChecksumOffset;
  uint32_t   lxDataPagesOffset;
  uint32_t   lxNumberPreloadPages;
  uint32_t   lxNonResidentNameTableOffset;
  uint32_t   lxNonResidentNameTableLength;
  uint32_t   lxNonResidentNameTableChecksum;
  uint32_t   lxAutoDSObjectNumber;     /**< not present in LE */
  uint32_t   lxDebugInfoOffset;
  uint32_t   lxDebugInfoLength;
  uint32_t   lxNumberInstancePreload;  /**< not present in LE */
  uint32_t   lxNumberInstanceDemand;   /**< not present in LE */
  uint32_t   lxHeapSize;               /**< not present in LE */
  uint32_t   lxStackSize;              /**< not present in LE */
}LXHEADER;

/** Linear EXE header */
typedef struct tagLEHEADER
{
  /* --------- common LE/LX part ------------- */
  uint8_t    leSignature[2];  /**< 'LE' */
  uint8_t    leByteOrdering;
  uint8_t    leWordOrdering;
  uint32_t   leFormatLevel;
  uint16_t   leCPUType;
  uint16_t   leOSType;
  uint32_t   leModuleVersion;
  uint32_t   leModuleFlags;
  uint32_t   lePageCount;
  uint32_t   leEIPObjectNumbers;
  uint32_t   leEIP;
  uint32_t   leESPObjectNumbers;
  uint32_t   leESP;
  uint32_t   lePageSize;
  /* ---------- specific LE part ----------------- */
  uint32_t   lePageOffsetShift; /**< possible not used */
  uint32_t   leFixupSize;
  uint32_t   lePageChecksum;            /**< different LE/LX part */
  uint32_t   leLoaderSectionSize;
  uint32_t   leLoaderSectionChecksum;
  uint32_t   leObjectTableOffset;
  uint32_t   leObjectCount;
  uint32_t   leObjectPageMapTableOffset;
  uint32_t   leObjectIterDataMapOffset;
  uint32_t   leResourceTableOffset;
  uint32_t   leResourceCount;
  uint32_t   leResidentNameTableOffset;
  uint32_t   leEntryTableOffset;
  uint32_t   leModuleDirectivesOffset;
  uint32_t   leModuleDirectivesCount;
  uint32_t   leFixupPageTableOffset;
  uint32_t   leFixupRecordTableOffset;
  uint32_t   leImportModuleTableOffset;
  uint32_t   leImportModuleEntryCount;
  uint32_t   leImportProcedureNamesTableOffset;
  uint32_t   lePerPageChecksumTableOffset;
  uint32_t   leDataPagesOffset;
  uint32_t   lePreloadPageCount;
  uint32_t   leNonResidentNameTableOffset;
  uint32_t   leNonResidentNameTableLength;
  uint32_t   leNonResidentNameTableChecksum;
  uint32_t   leDebugInfoOffset;
  uint32_t   leDebugInfoLength;
}LEHEADER;

typedef struct tag_VxD_Desc_Block
{
   uint32_t       DDB_Next                ; /**< VMM RESERVED FIELD */
   uint16_t       DDB_SDK_Version         ; /**< VMM RESERVED FIELD */
   uint16_t       DDB_Req_Device_Number   ; /**< Required device number */
   uint8_t        DDB_Dev_Major_Version   ; /**< Major device number */
   uint8_t        DDB_Dev_Minor_Version   ; /**< Minor device number */
   uint16_t       DDB_Flags               ; /**< Flags for init calls complete */
   int8_t         DDB_Name[8]             ; /**< Device name */
   uint32_t       DDB_Init_Order          ; /**< Initialization Order */
   uint32_t       DDB_Control_Proc        ; /**< Offset of control procedure */
   uint32_t       DDB_V86_API_Proc        ; /**< Offset of API procedure (or 0) */
   uint32_t       DDB_PM_API_Proc         ; /**< Offset of API procedure (or 0) */
   uint32_t       DDB_V86_API_CSIP        ; /**< CS:IP of API entry point */
   uint32_t       DDB_PM_API_CSIP         ; /**< CS:IP of API entry point */
   uint32_t       DDB_Reference_Data      ; /**< Reference data from real mode */
   uint32_t       DDB_Service_Table_Ptr   ; /**< Pointer to service table */
   uint32_t       DDB_Service_Table_Size  ; /**< Number of services */
}VxD_Desc_Block;


extern union LX_LE
{
  LEHEADER le;
  LXHEADER lx;
}lxe;

/** Flat .EXE object table entry */
typedef struct o32_obj
{
    uint32_t       o32_size;       /**< Object virtual size */
    uint32_t       o32_base;       /**< Object base virtual address */
    uint32_t       o32_flags;      /**< Attribute flags */
    uint32_t       o32_pagemap;    /**< Object page map index */
    uint32_t       o32_mapsize;    /**< Number of entries in object page map */
    uint32_t       o32_reserved;   /**< Reserved */
}LX_OBJECT;

#define PAGE_VALID       0x0000    /**< Valid Physical Page in .EXE */
#define PAGE_ITERDATA    0x0001    /**< Iterated Data Page */
#define PAGE_INVALID     0x0002    /**< Invalid Page */
#define PAGE_ZEROED      0x0003    /**< Zero Filled Page */
#define PAGE_RANGE       0x0004    /**< Range of pages */
#define PAGE_ITERDATA2   0x0005    /**< Iterated Data Page Type II */

/* Object Page Table entry */
typedef struct o32_map
{
    uint32_t  o32_pagedataoffset;     /**< file offset of page */
    uint16_t  o32_pagesize;           /**< # bytes of page data */
    uint16_t  o32_pageflags;          /**< Per-Page attributes */
}LX_MAP_TABLE;

typedef struct LX_Iter
{
    uint16_t LX_nIter;            /**< number of iterations */
    uint16_t LX_nBytes;           /**< number of bytes */
    uint8_t  LX_Iterdata;         /**< iterated data byte(s) */
}LX_ITER;

typedef struct b32_bundle
{
    uint8_t       b32_cnt;        /**< Number of entries in this bundle */
    uint8_t       b32_type;       /**< Bundle type */
    uint16_t      b32_obj;        /**< Object number */
}LX_BUNGLE;                       /* Follows entry types */

/** 16-bit or 32-bit offset */
typedef union _offset
{
    uint16_t offset16;
    uint32_t offset32;
}offset;

typedef struct e32_entry
{
    uint8_t       e32_flags;      /**< Entry point flags */
    union entrykind
    {
	offset          e32_offset;     /**< 16-bit/32-bit offset entry */
	struct callgate
	{
	    uint16_t offset;      /**< Offset in segment */
	    uint16_t callgate;    /**< Callgate selector */
	}e32_callgate;   /**< 286 (16-bit) call gate */
	struct fwd
	{
	    uint16_t  modord;     /**< Module ordinal number */
	    uint32_t  value;      /**< Proc name offset or ordinal */
	}e32_fwd;        /**< Forwarder */
    }e32_variant;    /**< Entry variant */
}e32_ENTRY;

/*
 *  In 32-bit .EXE file run-time relocations are written as varying size
 *  records, so we need many size definitions.
 */

#define RINTSIZE16      8
#define RINTSIZE32      10
#define RORDSIZE        8
#define RNAMSIZE16      8
#define RNAMSIZE32      10
#define RADDSIZE16      10
#define RADDSIZE32      12

/*
 *  BUNDLE TYPES
 */

#define EMPTY        0x00               /* Empty bundle */
#define ENTRY16      0x01               /* 16-bit offset entry point */
#define GATE16       0x02               /* 286 call gate (16-bit IOPL) */
#define ENTRY32      0x03               /* 32-bit offset entry point */
#define ENTRYFWD     0x04               /* Forwarder entry point */
#define TYPEINFO     0x80               /* Typing information present flag */

typedef struct lxEntry
{
  int8_t b32_type;
  int8_t b32_obj;
  e32_ENTRY entry;
}LX_ENTRY;

typedef struct tagLE_PAGE
{
  uint16_t flags;
  uint16_t number;
}LE_PAGE;

typedef struct tagLXResource
{
   uint16_t typeID;
   uint16_t nameID;
   uint32_t resourceSize;
   uint16_t object;
   uint32_t offset;
}LXResource;


extern void          __FASTCALL__ ShowFwdModOrdLX(const LX_ENTRY *_lxe);
extern __filesize_t  __FASTCALL__ ShowNewHeaderLX( void );
extern __filesize_t  __FASTCALL__ ShowObjectsLX( void );
extern unsigned      __FASTCALL__ LXRNamesNumItems(BFile*);
extern bool         __FASTCALL__ LXRNamesReadItems(BFile*,memArray *,unsigned);
extern __filesize_t  __FASTCALL__ ShowModRefLX( void );
extern unsigned      __FASTCALL__ LXNRNamesNumItems(BFile*);
extern bool         __FASTCALL__ LXNRNamesReadItems(BFile*,memArray *,unsigned);
extern __filesize_t  __FASTCALL__ ShowImpProcLXLE( void );
extern __filesize_t  __FASTCALL__ ShowEntriesLX( void );
extern const char *  __FASTCALL__ lxeGetMapAttr(unsigned long attr);
extern __filesize_t  __FASTCALL__ CalcEntryPointLE(unsigned long objnum,__filesize_t _offset);
extern __filesize_t  __FASTCALL__ CalcPageEntryLE(unsigned long idx);
extern __filesize_t  __FASTCALL__ CalcEntryLE(const LX_ENTRY *);

#define FILE_LX 1
#define FILE_LE 2
extern int LXType;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif

#endif
