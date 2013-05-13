/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/pe.h
 * @brief       This file contains PE executable file definitions.
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
#ifndef __PE_INC
#define __PE_INC

#include "config.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

/* Portable EXE header */
typedef struct tagPEHEADER
{
  uint8_t    peSignature[4];        /**< 'PE\00\00' */
  uint16_t   peCPUType;
  uint16_t   peObjects;
  uint32_t   peTimeDataStamp;
  uint32_t   peCOFFSymTabOffset;
  uint32_t   peCOFFNumOfSyms;
  uint16_t   peNTHdrSize;
  uint16_t   peFlags;
  uint16_t   peMagic; /* 0x10B - normal exe; 0x20b - PE32+; 0x107 - rom */
  uint8_t    peLMajor;
  uint8_t    peLMinor;
  uint32_t   peSizeOfText;
  uint32_t   peSizeOfData;
  uint32_t   peSizeOfBSS;
  uint32_t   peEntryPointRVA;
  uint32_t   peBaseOfCode;
}PEHEADER;

typedef struct tagPE32HEADER {
  uint32_t   peBaseOfData; /* missing in PE32+ */
  uint32_t   peImageBase;
  uint32_t   peObjectAlign;
  uint32_t   peFileAlign;
  uint16_t   peOSMajor;
  uint16_t   peOSMinor;
  uint16_t   peUserMajor;
  uint16_t   peUserMinor;
  uint16_t   peSubSystMajor;
  uint16_t   peSubSystMinor;
  uint32_t   peReserv9;
  uint32_t   peImageSize;
  uint32_t   peHeaderSize;
  uint32_t   peFileChecksum;
  uint16_t   peSubSystem;
  uint16_t   peDLLFlags;
  uint32_t   peStackReserveSize;
  uint32_t   peStackCommitSize;
  uint32_t   peHeapReserveSize;
  uint32_t   peHeapCommitSize;
  uint32_t   peReserv10;
  uint32_t   peDirSize;
}PE32HEADER;

typedef struct tagPE32P_HEADER {
  uint64_t   peImageBase;
  uint32_t   peObjectAlign;
  uint32_t   peFileAlign;
  uint16_t   peOSMajor;
  uint16_t   peOSMinor;
  uint16_t   peUserMajor;
  uint16_t   peUserMinor;
  uint16_t   peSubSystMajor;
  uint16_t   peSubSystMinor;
  uint32_t   peReserv9;
  uint32_t   peImageSize;
  uint32_t   peHeaderSize;
  uint32_t   peFileChecksum;
  uint16_t   peSubSystem;
  uint16_t   peDLLFlags;
  uint64_t   peStackReserveSize;
  uint64_t   peStackCommitSize;
  uint64_t   peHeapReserveSize;
  uint64_t   peHeapCommitSize;
  uint32_t   peReserv10;
  uint32_t   peDirSize;
}PE32P_HEADER;

typedef struct tagPERVA
{
  uint32_t rva;
  uint32_t size;
} PERVA;

enum {
    PE_EXPORT           =0,
    PE_IMPORT           =1,
    PE_RESOURCE         =2,
    PE_EXCEPT           =3,
    PE_SECURITY         =4,
    PE_FIXUP            =5,
    PE_DEBUG            =6,
    PE_IMAGE_DESC       =7,
    PE_MACHINE          =8,
    PE_TLS              =9,
    PE_LOAD_CONFIG      =10,
    PE_BOUND_IMPORT     =11,
    PE_IAT              =12,
    PE_DELAY_IMPORT     =13,
    PE_COM              =14,
    PE_RESERVED         =15
};

typedef struct tagPE_ADDR
{
  uint32_t rva;
  uint32_t phys;
} PE_ADDR;

typedef struct tagExportTablePE
{
  uint32_t etFlags;
  uint32_t etDateTime;
  uint16_t etMajVer;
  uint16_t etMinVer;
  uint32_t etNameRVA;
  uint32_t etOrdinalBase;
  uint32_t etNumEATEntries;
  uint32_t etNumNamePtrs;
  uint32_t etAddressTableRVA;
  uint32_t etNamePtrTableRVA;
  uint32_t etOrdinalTableRVA;
}ExportTablePE;

typedef struct tagImportDirPE
{
  uint32_t idFlags;
  uint32_t idDateTime;
  uint16_t idMajVer;
  uint16_t idMinVer;
  uint32_t idNameRVA;
  uint32_t idLookupTableRVA;
}ImportDirPE;

typedef struct tagPE_OBJECT
{
  int8_t   oName[8];
  uint32_t oVirtualSize;
  uint32_t oRVA;
  uint32_t oPhysicalSize;
  uint32_t oPhysicalOffset;
  uint32_t oRelocPtr;
  uint32_t oLineNumbPtr;
  uint16_t oNReloc;
  uint16_t oNLineNumb;
  uint32_t oFlags;
}PE_OBJECT;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
