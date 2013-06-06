/**
 * @namespace	usr
 * @file        codeguid.h
 * @brief       This file contains prototypes code navigator.
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
#ifndef __CODEGUID__H
#define __CODEGUID__H
#include <vector>

namespace	usr {
    class DisMode;
    class BeyeContext;
    class CodeGuider : public Opaque {
	public:
	    CodeGuider(BeyeContext&);
	    virtual ~CodeGuider();

	    void		reset_go_address( int keycode );
	    void		add_go_address(const DisMode& parent,std::string& str,__filesize_t addr);
	    void		add_back_address();
	    __filesize_t	get_go_address(unsigned keycode);
	    std::string		encode_address(__filesize_t cfpos,bool aresolv);
	    const char*		image() const { return codeguid_image; }
	private:
	    char*		gidBuildKeyStr();
	    int			gidGetKeyIndex(char key) const;
	    char		gidGetAddressKey( unsigned _index ) const;

	    char		codeguid_image[6];
	    std::vector<__filesize_t> BackAddr;
	    std::vector<std::pair<__filesize_t,unsigned> > GoAddr;
	    unsigned char	Alarm;
	    BeyeContext&	bctx;
	    std::string		addr;
    };
} // namespace	usr
#endif
