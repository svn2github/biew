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

#include "config.h"
#include "beyeutil.h"
#include "libbeye/bstream.h"
#include "mz.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif
    enum {
	NE_WIN2X_ON_3X_PROTMOD	=2,
	NE_WIN2X_PROPORTFONT	=4,
	NE_FASTLOADAREA		=8
    };
/** New EXE header */
    struct NEHEADER {
	uint8_t		neSignature[2];               /**< 'NE' */
	uint8_t		neLinkerVersion;
	uint8_t		neLinkerRevision;
	uint16_t	neOffsetEntryTable;
	uint16_t	neLengthEntryTable;
	uint32_t	neChecksum;
	uint16_t	neContestEXE;
	uint16_t	neAutoDataSegmentCount;
	uint16_t	neHeapSize;
	uint16_t	neStackSize;
	uint32_t	neCSIPvalue;
	uint32_t	neSSSPvalue;
	uint16_t	neSegmentTableCount;
	uint16_t	neModuleReferenceTableCount;
	uint16_t	neLengthNonResidentNameTable;
	uint16_t	neOffsetSegmentTable;
	uint16_t	neOffsetResourceTable;
	uint16_t	neOffsetResidentNameTable;
	uint16_t	neOffsetModuleReferenceTable;
	uint16_t	neOffsetImportTable;
	uint32_t	neOffsetNonResidentNameTable;
	uint16_t	neMoveableEntryPointCount;
	uint16_t	neLogicalSectorShiftCount;
	uint16_t	neResourceSegmentCount;
	uint8_t		neOperatingSystem;
	uint8_t		neFlagsOther;
/* os depended 64 bytes struct */
	uint16_t	neOffsetFastLoadArea;
	uint16_t	neLengthFastLoadArea;
	uint16_t	neReserved;
	uint16_t	neWindowsVersion;
    };

    struct SEGDEF {
	uint16_t	sdOffset;
	uint16_t	sdLength;
	uint16_t	sdFlags;
	uint16_t	sdMinMemory;
    };

    struct ENTRY {
	uint8_t		eFlags;
	uint8_t		eFixed; /**< 1 - fixed 0 - moveable */
 /* uint16_t eInt3F; */
	uint8_t		eSegNum;
	uint16_t	eSegOff;
    };

    struct NAMEINFO {
	uint16_t rnOffset;
	uint16_t rnLength;
	uint16_t rnFlags;
	uint16_t rnID;
	uint16_t rnHandle;
	uint16_t rnUsage;
    };

    struct RELOC_NE {
	uint8_t		AddrType;
	uint8_t		Type;
	uint16_t	RefOff;
	uint16_t	idx;
	uint16_t	ordinal;
    };

#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
    struct NERefChain {
	unsigned offset;
	unsigned number;
    };

    class LX_Parser;
    class LE_Parser;
    class PE_Parser;
    class NE_Parser : public MZ_Parser {
	public:
	    NE_Parser(binary_stream&,CodeGuider&);
	    virtual ~NE_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F4();
	    virtual __filesize_t	action_F6();
	    virtual __filesize_t	action_F7();
	    virtual __filesize_t	action_F8();
	    virtual __filesize_t	action_F10();

	    virtual bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(char *,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	    virtual __filesize_t	get_public_symbol(char *str,unsigned cb_str,unsigned *_class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
							__filesize_t *start,__filesize_t *end,int *_class,int *bitness);
	protected:
	    friend class LX_Parser;
	    friend class LE_Parser;
	    friend class PE_Parser;
	    static unsigned		GetNamCountNE(binary_stream&handle,__filesize_t offset);
	    static bool			RNamesReadItems(binary_stream&handle,memArray *obj,unsigned nnames,__filesize_t offset);

	    static const char*		GetPMWinAPI(unsigned flag);
	    static const char*		__nedata[];
	private:
	    void			ne_ReadPubName(binary_stream&b_cache,const struct PubName *it,char *buff,unsigned cb_buff);
	    bool			BuildReferStrNE(const DisMode&parent,char *str,RELOC_NE *rne,int flags,__filesize_t ulShift);
	    void			rdImpNameNELX(char *buff,int blen,unsigned idx,bool useasoff,__filesize_t OffTable);
	    unsigned			__findSpecType(__filesize_t sstart,__filesize_t ssize,unsigned segnum,__filesize_t target,char codelen,char type,unsigned defval);
	    RELOC_NE*			__found_RNE_spec(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen,int type);
	    RELOC_NE*			__found_RNE(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen);
	    static tCompare		compare_ne(const any_t *e1,const any_t *e2);
	    static tCompare		compare_ne_spec(const any_t *e1,const any_t *e2);
	    static tCompare		compare_chains(const any_t *v1,const any_t *v2);
	    void			BuildNERefChain(__filesize_t segoff,__filesize_t slength);
	    unsigned int		GetResourceGroupCountNE(binary_stream&handle);
	    bool			__ReadResourceGroupNE(binary_stream&handle,memArray *obj,unsigned nitems,long *addr);
	    char*			GetResourceIDNE(binary_stream&handle,unsigned rid,__filesize_t BegResTab);
	    unsigned			GetEntryCountNE();
	    bool			__ReadEntryTableNE(binary_stream&handle,memArray *obj);
	    __filesize_t		CalcEntryNE(unsigned ord,bool dispmsg);
	    bool			ReadSegDefNE(SEGDEF *obj,unsigned segnum) const;
	    bool			ReadEntryNE(ENTRY *obj,unsigned entnum);
	    void			SkipEntryItemNE(binary_stream&handle,unsigned char etype);
	    void			ReadEntryItemNE(binary_stream&handle,ENTRY *obj,unsigned char etype);
	    unsigned			NENRNamesNumItems(binary_stream&handle);
	    unsigned			NERNamesNumItems(binary_stream&handle);
	    bool			__ReadSegTableNE(binary_stream&handle,memArray *obj,unsigned nnames);
	    bool			NENRNamesReadItems(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			NERNamesReadItems(binary_stream&handle,memArray *names,unsigned nnames);
	    bool			__ReadProcListNE(binary_stream&handle,memArray *obj,int modno);
	    bool			isPresent(memArray *arr,unsigned nentry,char *_tmpl);
	    void			ShowProcListNE(int modno);
	    bool			__ReadModRefNamesNE(binary_stream&handle,memArray *obj);
	    static void			EntPaintNE(TWindow *win,const any_t **names,unsigned start,unsigned nlist);
	    static void			SegPaintNE(TWindow *win,const any_t **names,unsigned start,unsigned nlist);
	    static void			paintdummyentryNE(TWindow *w);
	    static void			entpaintNE(TWindow *w,const ENTRY *nam,unsigned flags);
	    static void			PaintNewHeaderNE(TWindow *win,const any_t **ptr,unsigned npage,unsigned tpage);
	    static void	__FASTCALL__	PaintNewHeaderNE_2(TWindow *w);
	    static void	__FASTCALL__	PaintNewHeaderNE_1(TWindow *w);
	    static const char*		__getNEType(unsigned type);
	    void			rd_ImpName(char *buff,int blen,unsigned idx,bool useasoff);
	    bool			FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
	    __filesize_t		CalcEntryPointNE(unsigned segnum,unsigned offset);
	    void			ne_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
	    bool			ReadPubNames(binary_stream& handle,__filesize_t offset,void (__FASTCALL__ *mem_out)(const std::string&));
	    unsigned			__get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,__filesize_t *start,__filesize_t *end,int *_class,int *bitness) const;

	    static void			(__FASTCALL__ * nephead[])(TWindow* w);
    };
} // namespace	usr
#endif
