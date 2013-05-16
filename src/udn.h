#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        udn.h
 * @brief       This file contains prototypes for user defined names
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
**/
#ifndef __UDN_HPP_INCLUDED
#define __UDN_HPP_INCLUDED 1
#include <set>

#include "beyeutil.h"

namespace	usr {
    class Ini_Profile;

    struct udn_record {
	char		name[256];
	__filesize_t	offset;

	bool operator<(const udn_record& rhs) const { return offset<rhs.offset; }
    };

    class udn;
    typedef bool (udn::*udnFunc)();

    class udn : public Opaque {
	public:
	    udn();
	    virtual ~udn();

	    /** Reads user defined name at given offset!
	    **/
	    virtual bool		find(__filesize_t pa,std::string& buff) const;

	    /** Display select box to select user defined name and returns its offset.
	    **/
	    virtual bool		select(__filesize_t& off);

	    /** Shows menu with operations for user defined names!
	    **/
	    virtual bool		names();

	    virtual void		read_ini(Ini_Profile& ini);
	    virtual void		save_ini(Ini_Profile& ini);

	private:
	    bool		add_item();
	    bool		delete_item();
	    bool		read_items(binary_stream& handle,memArray* names,unsigned nnames);
	    bool		__load_list();
	    bool		load_list();
	    bool		save_list();
	    bool		__save_list();

	    std::set<udn_record>	udn_list;
	    bool			udn_modified;
	    std::string			udn_fname;

	    static udnFunc	funcs[];
    };
} // namespace	usr
#endif
