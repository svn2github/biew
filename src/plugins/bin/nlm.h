/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/nlm386.h
 * @brief       This file contains NLM file format definitions.
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
#ifndef __NLM_INC
#define __NLM_INC

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif

typedef uint32_t file_ptr;
typedef uint32_t bfd_size_type;
typedef uint32_t bfd_vma;
typedef uint32_t PTR;
#define NLM_SIGNATURE "NetWare Loadable Module\x1a"
enum {
    NLM_SIGNATURE_SIZE=24,
    NLM_MODULE_NAME_SIZE=14,
    NLM_MAX_DESCRIPTION_LENGTH=41,
    NLM_OLD_THREAD_NAME_LENGTH=5,
    NLM_MAX_SCREEN_NAME_LENGTH=4,
    NLM_MAX_THREAD_NAME_LENGTH=7,
    NLM_MAX_COPYRIGHT_MESSAGE_LENGTH=1
};

typedef struct nlm_internal_fixed_header
{
  int8_t        nlm_signature[NLM_SIGNATURE_SIZE];
  int32_t       nlm_version;
  int8_t        nlm_moduleName[NLM_MODULE_NAME_SIZE];
  file_ptr      nlm_codeImageOffset;
  bfd_size_type nlm_codeImageSize;
  file_ptr      nlm_dataImageOffset;
  bfd_size_type nlm_dataImageSize;
  bfd_size_type nlm_uninitializedDataSize;
  file_ptr      nlm_customDataOffset;
  bfd_size_type nlm_customDataSize;
  file_ptr      nlm_moduleDependencyOffset;
  int32_t       nlm_numberOfModuleDependencies;
  file_ptr      nlm_relocationFixupOffset;
  int32_t       nlm_numberOfRelocationFixups;
  file_ptr      nlm_externalReferencesOffset;
  int32_t       nlm_numberOfExternalReferences;
  file_ptr      nlm_publicsOffset;
  int32_t       nlm_numberOfPublics;
  file_ptr      nlm_debugInfoOffset;
  int32_t       nlm_numberOfDebugRecords;
  file_ptr      nlm_codeStartOffset;
  file_ptr      nlm_exitProcedureOffset;
  file_ptr      nlm_checkUnloadProcedureOffset;
  int32_t       nlm_moduleType;
  int32_t       nlm_flags;
} Nlm_Internal_Fixed_Header;

typedef struct nlm_internal_variable_header
{
  uint8_t        descriptionLength;
  int8_t         descriptionText[NLM_MAX_DESCRIPTION_LENGTH + 1];
  int32_t        stackSize;
  int32_t        reserved; /**< should contain zero */
  int8_t         oldThreadName[NLM_OLD_THREAD_NAME_LENGTH]; /**< " LONG" */
  uint8_t        screenNameLength;
  int8_t         screenName[NLM_MAX_SCREEN_NAME_LENGTH + 1];
  uint8_t        threadNameLength;
  int8_t         threadName[NLM_MAX_THREAD_NAME_LENGTH + 1];
} Nlm_Internal_Variable_Header;

/** The header is recognized by "VeRsIoN#" in the stamp field. */
typedef struct nlm_internal_version_header
{
  int8_t          stamp[8];
  int32_t         majorVersion;
  int32_t         minorVersion;
  int32_t         revision;
  int32_t         year;
  int32_t         month;
  int32_t         day;
} Nlm_Internal_Version_Header;

/** The header is recognized by "CoPyRiGhT=" in the stamp field. */
typedef struct nlm_internal_copyright_header
{
  int8_t         stamp[10];
  uint8_t        copyrightMessageLength;
  int8_t         copyrightMessage[NLM_MAX_COPYRIGHT_MESSAGE_LENGTH];
} Nlm_Internal_Copyright_Header;

/** The header is recognized by "MeSsAgEs" in the stamp field. */
typedef struct nlm_internal_extended_header
{
  int8_t        stamp[8];
  int32_t       languageID;
  file_ptr      messageFileOffset;
  bfd_size_type messageFileLength;
  int32_t       messageCount;
  file_ptr      helpFileOffset;
  bfd_size_type helpFileLength;
  file_ptr      RPCDataOffset;
  bfd_size_type RPCDataLength;
  file_ptr      sharedCodeOffset;
  bfd_size_type sharedCodeLength;
  file_ptr      sharedDataOffset;
  bfd_size_type sharedDataLength;
  file_ptr      sharedRelocationFixupOffset;
  int32_t       sharedRelocationFixupCount;
  file_ptr      sharedExternalReferenceOffset;
  int32_t       sharedExternalReferenceCount;
  file_ptr      sharedPublicsOffset;
  int32_t       sharedPublicsCount;
  file_ptr      sharedDebugRecordOffset;
  int32_t       sharedDebugRecordCount;
  bfd_vma       SharedInitializationOffset;
  bfd_vma       SharedExitProcedureOffset;
  int32_t       productID;
  int32_t       reserved[6];
} Nlm_Internal_Extended_Header;

/** The format of a custom header as stored internally is different
   from the external format.  This is how we store a custom header
   which we do not recognize.  */
/** The header is recognized by "CuStHeAd" in the stamp field. */
typedef struct nlm_internal_custom_header
{
  int8_t        stamp[8];
  bfd_size_type hdrLength;
  file_ptr      dataOffset;
  bfd_size_type dataLength;
  int8_t        dataStamp[8];
  PTR           hdr;
} Nlm_Internal_Custom_Header;

/** The internal Cygnus header is written out externally as a custom
    header.  We don't try to replicate that structure here.  */

/** The header is recognized by "CyGnUsEx" in the stamp field. */
typedef struct nlm_internal_cygnus_ext_header
{
  int8_t        stamp[8];
  file_ptr      offset;  /**< File location of debugging information.  */
  bfd_size_type length;   /**< Length of debugging information.  */
} Nlm_Internal_Cygnus_Ext_Header;

/**
   Public names table:
   ===================
   +0 ( 1 byte )         - name length
   +1 ( length bytes )   - name
   +length+1 ( 4 bytes ) - physical offset from begin of code section
*/

/**
   External name table:
   ====================
   +0 ( 1 byte )         - name length
   +1 ( length bytes )   - name
   +length+1 ( 4 bytes ) - number of fixups
   +length+5 ( 4 bytes array) - physical offsets from begin of code section
*/


typedef uint32_t	Nlm32_Addr;	/**< Unsigned program address */
typedef uint32_t	Nlm32_Off;	/**< Unsigned file offset */
typedef int32_t		Nlm32_Sword;	/**< Signed large integer */
typedef uint32_t	Nlm32_Word;	/**< Unsigned large integer */
typedef uint16_t	Nlm32_Half;	/**< Unsigned medium integer */
typedef uint8_t		Nlm32_Char;	/**< Unsigned tiny integer */

typedef int32_t		Nlm64_Sword;
typedef uint32_t	Nlm64_Word;
typedef uint16_t	Nlm64_Half;

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
} // namespace	usr
#endif
