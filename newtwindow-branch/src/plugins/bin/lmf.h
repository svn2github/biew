/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/lmf.c
 * @brief       This file contains lmf file structures and constants.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *              LMF file format header file based on Watcom C/QNX4 <sys/lmf.h>
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
 * @todo        wc 10.6 debug information support!!! (need to use lmf.tgz)
**/
#ifndef __LMF_INC
#define __LMF_INC

#include "config.h"

namespace	usr {
#if __WATCOMC__ > 1000
#pragma pack(__push,1);
#else
#pragma pack(1)
#endif

/*	LMF structure defenitions
 */

typedef struct tag_lmf_header	/* This preceeds each record defined below */
{
	int8_t rec_type,
		zero1;
	uint16_t data_nbytes,
		spare;
} lmf_header;

typedef struct tag_lmf_definition	/* Must be first record in load file */
{
	uint16_t version_no,
		cflags,
		cpu,
		fpu,
		code_index,
		stack_index,
		heap_index,
		argv_index,
		zero1[4];
	uint32_t code_offset,
		stack_nbytes,
		heap_nbytes,
		flat_offset,	/* Must be zero if not set _PCF_FLAT in cflags	(AG) */
		unmapped_size,	/* I never seen this field nonzero		(AG) */
		zero2;
	/* Variable length field of n longs starts here */
	/* Sizes of segments.      ^^^ n is segments number.		(AG) */
} lmf_definition;

typedef struct tag_lmf_data	/* Code or data record to load into memory */
{
	uint16_t index;
	uint32_t offset;
	/* Variable length field of n bytes starts here */
	/* Data to load in         ^^^ n is a length of loading data
	   segment numbered        n = lmf_header.data_nbytes of this record
	   by index.                   - sizeof(lmf_data).			(AG) */
} lmf_data;

typedef struct tag_lmf_resource
{
	uint16_t resource_type;   /* 0 - usage messages */
	uint16_t zero[3];
} lmf_resource;

/*	Record types
 */
enum {
    _LMF_DEFINITION_REC		=0,
    _LMF_COMMENT_REC		=1,
			    /* ^^^ Never seen this record.		(AG)  */
    _LMF_DATA_REC		=2,
    _LMF_FIXUP_SEG_REC		=3,
    _LMF_FIXUP_80X87_REC	=4,
    _LMF_EOF_REC		=5,
    _LMF_RESOURCE_REC		=6,
    _LMF_ENDDATA_REC		=7,
    _LMF_FIXUP_LINEAR_REC	=8,
			    /* ^^^ Never seen this record.		(AG)  */
    _LMF_PHRESOURCE		=9,	/* A widget resource for photon apps */
			    /* ^^^ Never seen this record.		(AG)  */
/*	The top 4 bits of the segment sizes
 */

    _LMF_CODE			=0x2
};
/*	Bit defitions for lh_code_flags
 */
enum {
    _PCF_LONG_LIVED     =0x0001,
    _PCF_32BIT          =0x0002,
    _PCF_PRIVMASK       =0x000c,  /* Two bits */
    _PCF_FLAT           =0x0010,
    _PCF_NOSHARE        =0x0020
};

#if __WATCOMC__ > 1000
#pragma pack(__pop);
#else
#pragma pack()
#endif
} // namespace	usr
#endif/*__LMF_INC*/
