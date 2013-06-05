/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mz.h
 * @brief       This file contains MZ executable file definitions.
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
#ifndef __MZ_INC
#define __MZ_INC
#include <set>

#include "config.h"
#include "plugins/binary_parser.h"

namespace	usr {
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack(1)
#endif
    struct MZHEADER {
	uint16_t mzPartLastPage;
	uint16_t mzPageCount;
	uint16_t mzRelocationCount;
	uint16_t mzHeaderSize;
	uint16_t mzMinMem;
	uint16_t mzMaxMem;
	uint16_t mzRelocationSS;
	uint16_t mzExeSP;
	uint16_t mzCheckSumm;
	uint16_t mzExeIP;
	uint16_t mzRelocationCS;
	uint16_t mzTableOffset;
	uint16_t mzOverlayNumber;
    };
#ifdef __HAVE_PRAGMA_PACK__
#pragma pack()
#endif
    struct MZ_Reloc {
	MZ_Reloc(const char& c):__codelen(c) {}

	__filesize_t	laddr;

	bool operator<(const MZ_Reloc& rhs) const { return laddr<rhs.laddr; }
	bool operator==(const MZ_Reloc& rhs) const { return laddr >= laddr && rhs.laddr < rhs.laddr + __codelen; }
	private:
	    const char&	__codelen;
    };

    class MZ_Parser : public Binary_Parser {
	public:
	    MZ_Parser(binary_stream& h,CodeGuider&,udn&);
	    virtual ~MZ_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,Bin_Format::bind_type flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;

	    static __filesize_t		is_new_exe(binary_stream& main_handle);
	protected:
	    CodeGuider&			code_guider() const __PURE_FUNC__ { return _code_guider; }
	    binary_stream&		main_handle() const __PURE_FUNC__ { return _main_handle; }
	    virtual __filesize_t	headshift() const __PURE_FUNC__ { return _headshift; }
	    udn&			_udn() const __PURE_FUNC__ { return __udn; }
	    char			__codelen;
	private:
	    std::string			QueryAddInfo( unsigned char *memmap ) const;
	    std::string			QueryAddInfo() const;
	    void			BuildMZChain();
	    bool			isMZReferenced(__filesize_t shift,char len);

	    MZHEADER		mz;
	    unsigned long	HeadSize;
	    std::set<MZ_Reloc> CurrMZChain;
	    __filesize_t	_headshift;
	    binary_stream&	_main_handle;
	    CodeGuider&		_code_guider;
	    udn&		__udn;
    };
} // namespace	usr
#endif
