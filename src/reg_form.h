/**
 * @namespace	usr
 * @file        reg_form.h
 * @brief       This file contains structure prototypes for embededding new
 *              file formats, disassemblers, translation modes e.t.c. in BEYE.
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
#ifndef __REG_FORM__H
#define __REG_FORM__H

namespace	usr {
typedef __filesize_t  (__FASTCALL__ *BinFunc)();
typedef bool         (__FASTCALL__ *ModFunc)();

enum {
    __MAX_SYMBOL_SIZE=4
};

enum {
    APREF_NORMAL      =0x0000, /**< Append references in short form if it really present in binary */
    APREF_USE_TYPE    =0x0001, /**< Append references in detail form if it really present in binary */
    APREF_TRY_LABEL   =0x0002, /**< Append references in short form even if it not present in binary (smart method) */
    APREF_SAVE_VIRT   =0x0004, /**< Notifies plugin about preserving of virtual address, if binding is local */
    APREF_TRY_PIC     =0x0008  /**< Append references in short form assuming that shift is offset in .GOT table where references are binded */
};
class DisMode;
/**
   Appends disassembler reference to string.
   * @param str          string buffer for append to
   * @param shift        physical address of field, that required of binding
   * @param flags        see above
   * @param codelen      length of field, that required binding
   * @param r_shift      used only if APPREF_TRY_LABEL mode is set, contains real value of field, that required binding
   * @return             true if reference was appended
*/
typedef bool (__FASTCALL__ *AppRefs)(const DisMode& parent,char *str,__filesize_t shift,int flags,int codelen,__filesize_t r_shift);

/***************************************************************\
*  This form registry binary file formats                       *
\***************************************************************/

/** List of DisAssembler Bitness */
enum {
    DAB_USE16   =0,
    DAB_USE32   =1,
    DAB_USE64   =2,
    DAB_USE128  =3,
    DAB_USE256  =4,
    DAB_AUTO    =0xFFFFU /**< never must return! Only for selection */
};
/** List of DisAssembler Endian */
enum {
    DAE_LITTLE	=0,
    DAE_BIG	=1
};
/** Public symbols classes */
enum {
    SC_LOCAL    =0, /**< means: present as entry but not exported */
    SC_GLOBAL   =1  /**< means: exported entry point */
};
/** object classes */
enum {
    OC_CODE      =0, /**< for code objects */
    OC_DATA      =1, /**< for any data objects */
    OC_NOOBJECT =-1 /**< for non objects (means: relocs, resources, tables ...) */
};
} // namespace	usr
#endif
