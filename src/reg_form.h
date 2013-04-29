/**
 * @namespace   beye
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

#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#include "libbeye/file_ini.h"
#endif

#ifndef __TWIN_H
#include "libbeye/twin.h"
#endif

namespace beye {
typedef __filesize_t  (__FASTCALL__ *BinFunc)( void );
typedef bool         (__FASTCALL__ *ModFunc)( void );

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
   * @return             one of RAPREF_* constants (see beyeutil.h file for detail)
*/
typedef unsigned long (__FASTCALL__ *AppRefs)(const DisMode& parent,char *str,__filesize_t shift,int flags,int codelen,__filesize_t r_shift);

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

class CodeGuider;
struct REGISTRY_BIN
{
  const char * name;                            /**< name of binary format */
  const char * prompt[10];                      /**< on ALT-Fx selection */
  const BinFunc  action[10];                         /**< action on ALT-Fx selection */
  bool   (__FASTCALL__ *check_format)( void ); /**< Checks format */
  void    (__FASTCALL__ *init)(CodeGuider&);    /**< Inits plugin (if check o'k) (constructor) */
  void    (__FASTCALL__ *destroy)( void );      /**< Destroys plugin (destructor) */
  BinFunc   showHdr;                            /**< if not an MZ style format */
  AppRefs   bind;                               /**< for show references */

			 /** Returns CPU platform, that required by format.
			   * @note           Full list of platform please see in
			   *                 plugins/disasm.h file. If this
			   *                 function return -1 then platform is
			   *                 undefined.
			  **/
  int     (__FASTCALL__ *query_platform)( void );

			 /** Returns DAB_XXX. Quick version for disassembler */
  int     (__FASTCALL__ *query_bitness)(__filesize_t);

			 /** Returns DAE_XXX. */
  int     (__FASTCALL__ *query_endian)(__filesize_t);

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
  bool   (__FASTCALL__ *AddressResolving)(char *,__filesize_t);

			 /** Converts virtual address to physical (means file offset).
			   * @param va       indicates virtual address to be converted
			   * @return         0 if operation meaningless
			  **/
 __filesize_t (__FASTCALL__ *va2pa)(__filesize_t va);

			 /** Converts physical address to virtual.
			   * @param pa       indicates physical address to be converted
			   * @note           seg pointer can be NULL
			  **/
  __filesize_t (__FASTCALL__ *pa2va)(__filesize_t pa);


/*-- Below placed functions for 'put structures' method of save as dialog --*/

			 /** Fills the string with public symbol
			   * @param str       pointer to the string to be filled
			   * @param cb_str    indicates maximal length of string
			   * @param _class    pointer to the memory where can be stored class of symbol (See SC_* conatnts)
			   * @param pa        indicates physical offset within file
			   * @param as_prev   indicates direction of symbol searching from given physical offset
			   * @return          0 - if no symbol name available
			   *                  in given direction (as_prev)
			   *                  physical address of public symbol
			   *                  which is found in given direction
			  **/
  __filesize_t (__FASTCALL__ *GetPubSym)(char *str,unsigned cb_str,unsigned *_class,
			     __filesize_t pa,bool as_prev);

			 /** Determines attributes of object at given physical file address.
			   * @param pa        indicates physical file offset of object
			   * @param name      pointer to the string which is to be filled with object name
			   * @param cb_name   indicates maximal length of string
			   * @param start     pointer to the memory where must be stored start of given object, as file offset.
			   * @param end       pointer to the memory where must be stored end of given object, as file offset.
			   * @param _class    pointer to the memory where must be stored _class of object (See OC_* constants).
			   * @param bitness   pointer to the memory where must be stored bitness of object (See DAB_* constants).
			   * @return          logical number of object or 0 if at given offset is no object.
			   * @note            all arguments exclude name of object
			   *                  must be filled.
			   * @remark          For example: if exe-format - new
			   *                  exe i.e. contains MZ and NEW
			   *                  header and given file offset
			   *                  points to old exe stub then start
			   *                  = 0, end = begin of first data or
			   *                  code object).
			  **/
  unsigned    (__FASTCALL__ *GetObjAttr)(__filesize_t pa,char *name,unsigned cb_name,
			      __filesize_t *start,__filesize_t *end,int *_class,int *bitness);
};

typedef struct tag_REGISTRY_TOOL
{
  const char *  name;                /**< Tool name */
  void          (*tool)( void );     /**< Tool body */
  void          (*read_ini)( void ); /**< read beye.ini if need */
  void          (*save_ini)( void ); /**< write to beye.ini if need */
}REGISTRY_TOOL;

typedef struct tag_REGISTRY_SYSINFO
{
  const char *  name;                /**< System depended information name */
  void          (*sysinfo)( void );  /**< System depended information body */
  void          (*read_ini)( void ); /**< reads beye.ini if need */
  void          (*save_ini)( void ); /**< writes to beye.ini if need */
}REGISTRY_SYSINFO;
} // namespace beye
#endif
