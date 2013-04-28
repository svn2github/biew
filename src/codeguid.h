/**
 * @namespace   beye
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

#ifndef __SYS_DEP_H
#include "_sys_dep.h"
#endif

namespace beye {
    class DisMode;
    class CodeGuider : public Opaque {
	public:
	    CodeGuider();
	    virtual ~CodeGuider();

	    void		reset_go_address( int keycode );
	    void		add_go_address(const DisMode& parent,char *str,__filesize_t addr);
	    void		add_back_address();
	    __filesize_t	get_go_address(unsigned keycode);
	    char*		encode_address(__filesize_t cfpos,bool aresolv);
	    const char*		image() const { return codeguid_image; }
	private:
	    char*		gidBuildKeyStr();
	    int			gidGetKeyIndex(char key);
	    char		gidGetAddressKey( unsigned _index );

	    char		codeguid_image[6];
	    __filesize_t*	BackAddr;
	    int			BackAddrPtr;
	    __filesize_t*	GoAddr;
	    unsigned int*	GoLineNums;
	    int			GoAddrPtr;
	    unsigned char	Alarm;
    };
} // namespace beye
#endif
