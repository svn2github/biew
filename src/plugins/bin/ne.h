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
	bool operator<(const NERefChain& rhs) const { return offset<rhs.offset; }
    };

    class LX_Parser;
    class LE_Parser;
    class PE_Parser;
    class NE_Parser : public MZ_Parser {
	public:
	    NE_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
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

	    virtual std::string		bind(const DisMode& _parent,__filesize_t shift,Bin_Format::bind_type flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual Bin_Format::bitness	query_bitness(__filesize_t) const;
	    virtual std::string		address_resolving(__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev);
	    virtual Object_Info		get_object_attribute(__filesize_t pa);
	protected:
	    friend class LX_Parser;
	    friend class LE_Parser;
	    friend class PE_Parser;
	    static unsigned		GetNamCountNE(binary_stream&handle,__filesize_t offset);
	    static std::vector<std::string> RNamesReadItems(binary_stream& handle,size_t nnames,__filesize_t offset);

	    static std::string		GetPMWinAPI(unsigned flag) __CONST_FUNC__;
	    static const char*		__nedata[];
	private:
	    std::string			ne_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    bool			BuildReferStrNE(const DisMode&parent,std::string& str,const RELOC_NE& rne,int flags,__filesize_t ulShift);
	    std::string			rdImpNameNELX(unsigned idx,bool useasoff,__filesize_t OffTable) const;
	    unsigned			__findSpecType(__filesize_t sstart,__filesize_t ssize,unsigned segnum,__filesize_t target,char codelen,char type,unsigned defval);
	    RELOC_NE			__found_RNE_spec(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen,int type);
	    RELOC_NE			__found_RNE(__filesize_t segoff,__filesize_t slength,unsigned segnum,unsigned keyoff,char codelen);
	    void			BuildNERefChain(__filesize_t segoff,__filesize_t slength);
	    unsigned int		GetResourceGroupCountNE(binary_stream&handle) const;
	    std::vector<std::string>	__ReadResourceGroupNE(binary_stream&handle,size_t nitems,long *addr) const;
	    std::string			GetResourceIDNE(binary_stream&handle,unsigned rid,__filesize_t BegResTab) const;
	    unsigned			GetEntryCountNE() const;
	    std::vector<ENTRY>		__ReadEntryTableNE(binary_stream& handle) const;
	    __filesize_t		CalcEntryNE(unsigned ord,bool dispmsg) const;
	    bool			ReadSegDefNE(SEGDEF *obj,unsigned segnum) const;
	    bool			ReadEntryNE(ENTRY *obj,unsigned entnum) const;
	    void			SkipEntryItemNE(binary_stream& handle,unsigned char etype) const;
	    void			ReadEntryItemNE(binary_stream& handle,ENTRY *obj,unsigned char etype) const;
	    unsigned			NENRNamesNumItems(binary_stream& handle) const;
	    unsigned			NERNamesNumItems(binary_stream& handle) const;
	    std::vector<SEGDEF>		__ReadSegTableNE(binary_stream& handle,size_t nnames) const;
	    std::vector<std::string>	NENRNamesReadItems(binary_stream& handle,size_t nnames) const;
	    std::vector<std::string>	NERNamesReadItems(binary_stream& handle,size_t nnames) const;
	    std::vector<std::string>	__ReadProcListNE(binary_stream& handle,int modno) const;
	    bool			isPresent(const std::vector<std::string>& objs,const std::string& _tmpl) const;
	    void			ShowProcListNE(int modno) const;
	    std::vector<std::string>	__ReadModRefNamesNE(binary_stream& handle) const;
	    void			EntPaintNE(TWindow& win,const std::vector<ENTRY>& names,unsigned start) const;
	    void			SegPaintNE(TWindow& win,const std::vector<SEGDEF>& names,unsigned start) const;
	    void			paintdummyentryNE(TWindow& w) const;
	    void			entpaintNE(TWindow& w,const ENTRY& nam,unsigned flags) const;
	    void			PaintNewHeaderNE(TWindow& win,const std::vector<std::string>& ptr,unsigned npage) const;
	    void			PaintNewHeaderNE_2(TWindow& w) const;
	    void			PaintNewHeaderNE_1(TWindow& w) const;
	    std::string			__getNEType(unsigned type) const __CONST_FUNC__;
	    std::string			rd_ImpName(unsigned idx,bool useasoff) const;
	    Symbol_Info			FindPubName(__filesize_t pa) const;
	    __filesize_t		CalcEntryPointNE(unsigned segnum,unsigned offset) const;
	    void			ne_ReadPubNameList(binary_stream& handle);
	    bool			ReadPubNames(binary_stream& handle,__filesize_t offset);
	    Object_Info			__get_object_attribute(__filesize_t pa) const;

	    NEHEADER ne;
	    std::set<symbolic_information> PubNames;

	    binary_stream* ne_cache;
	    binary_stream* ne_cache1;
	    binary_stream* ne_cache2;
	    binary_stream* ne_cache3;

	    unsigned long	CurrSegmentStart;
	    unsigned long	CurrSegmentLength;
	    unsigned		CurrChainSegment;
	    int			CurrSegmentHasReloc;
	    std::set<NERefChain> CurrNEChain;
	    char		__type;
	    __filesize_t	entryNE;

	    static void			(NE_Parser::*nephead[])(TWindow& w) const;
    };
} // namespace	usr
#endif
