/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/ne.h
 * @brief       This file contains NE executable file definitions.
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
#ifndef __NE_INC
#define __NE_INC

#include "beyeutil.h"
#include "libbeye/bstream.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

enum {
    NE_WIN2X_ON_3X_PROTMODE=2,
    NE_WIN2X_PROPORTFONT   =4,
    NE_FASTLOADAREA        =8
};
/** New EXE header */
typedef struct tagNEHEADER
{
 uint8_t   neSignature[2];               /**< 'NE' */
 uint8_t   neLinkerVersion;
 uint8_t   neLinkerRevision;
 uint16_t  neOffsetEntryTable;
 uint16_t  neLengthEntryTable;
 uint32_t  neChecksum;
 uint16_t  neContestEXE;
 uint16_t  neAutoDataSegmentCount;
 uint16_t  neHeapSize;
 uint16_t  neStackSize;
 uint32_t  neCSIPvalue;
 uint32_t  neSSSPvalue;
 uint16_t  neSegmentTableCount;
 uint16_t  neModuleReferenceTableCount;
 uint16_t  neLengthNonResidentNameTable;
 uint16_t  neOffsetSegmentTable;
 uint16_t  neOffsetResourceTable;
 uint16_t  neOffsetResidentNameTable;
 uint16_t  neOffsetModuleReferenceTable;
 uint16_t  neOffsetImportTable;
 uint32_t  neOffsetNonResidentNameTable;
 uint16_t  neMoveableEntryPointCount;
 uint16_t  neLogicalSectorShiftCount;
 uint16_t  neResourceSegmentCount;
 uint8_t   neOperatingSystem;
 uint8_t   neFlagsOther;
 /* os depended 64 bytes struct */
 uint16_t  neOffsetFastLoadArea;
 uint16_t  neLengthFastLoadArea;
 uint16_t  neReserved;
 uint16_t  neWindowsVersion;

}NEHEADER;

typedef struct tagSEGDEF
{
  uint16_t sdOffset;
  uint16_t sdLength;
  uint16_t sdFlags;
  uint16_t sdMinMemory;
}SEGDEF;

extern int ReadSegDef(SEGDEF *,uint16_t numseg);

typedef struct tagENTRY
{
 uint8_t  eFlags;
 uint8_t  eFixed; /**< 1 - fixed 0 - moveable */
 /* uint16_t eInt3F; */
 uint8_t  eSegNum;
 uint16_t eSegOff;
}ENTRY;

typedef struct tagNAMEINFO
{
  uint16_t rnOffset;
  uint16_t rnLength;
  uint16_t rnFlags;
  uint16_t rnID;
  uint16_t rnHandle;
  uint16_t rnUsage;
} NAMEINFO;

extern int ReadEntry(ENTRY *,uint16_t entnum);

typedef struct tagRELOC_NE
{
  uint8_t  AddrType;
  uint8_t  Type;
  uint16_t RefOff;
  uint16_t idx;
  uint16_t ordinal;
}RELOC_NE;

extern const char * __FASTCALL__ GetPMWinAPI(unsigned flag);
extern const char * __nedata[];
extern unsigned __FASTCALL__ GetNamCountNE(binary_stream&,__filesize_t);
extern bool __FASTCALL__ RNamesReadItems(binary_stream&,memArray *,unsigned,__filesize_t);


#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
