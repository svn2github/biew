#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/osdep/unix/keyboard.c
 * @brief       slang/curses/vt100 keyboard library
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Konstantin Boldyshev
 * @since       1999
 * @note        Development, fixes and improvements
**/

/*
    Copyright (C) 1999-2001 Konstantin Boldyshev <konst@linuxassembly.org>

    $Id: keyboard.c,v 1.9 2009/09/20 13:43:37 nickols_k Exp $
*/
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "system.h"
#include "input_interface.h"

namespace	usr {
    class input_null : public input_interface {
	public:
	    input_null(System&,const std::string& user_cp);
	    virtual ~input_null();

	    virtual bool		get_cbreak() const;
	    virtual void		set_cbreak( bool state );

	    virtual int			get_key( unsigned long flg);
	    virtual int			test_key( unsigned long flg );
	    virtual int			get_shifts();
	    virtual int			raw_info(char *head, char *text);

	    virtual bool		ms_get_state() const;
	    virtual void		ms_set_state(bool is_visible);
	    virtual void		ms_get_pos(tAbsCoord& x, tAbsCoord& y) const;
	    virtual int			ms_get_btns();
	private:
	    System&			sys;
    };


bool input_null::get_cbreak() const
{
    throw std::logic_error("input_null::get_cbreak");
}

void input_null::set_cbreak(bool state)
{
    throw std::logic_error("input_null::set_cbreak");
}

int input_null::get_shifts()
{
    throw std::logic_error("input_null::get_shifts");
}

int input_null::test_key(unsigned long flg)
{
    UNUSED(flg);
    throw std::logic_error("input_null::test_key");
}

int input_null::get_key(unsigned long flg)
{
    UNUSED(flg);
    throw std::logic_error("input_null::get_key");
}

int input_null::raw_info(char *head, char *text)
{
    UNUSED(head);
    UNUSED(text);
    throw std::logic_error("input_null::raw_info");
}

bool input_null::ms_get_state() const
{
    throw std::logic_error("input_null::ms_get_state");
}

void input_null::ms_set_state(bool ms_visible)
{
    UNUSED(ms_visible);
    throw std::logic_error("input_null::ms_set_visible");
}

void input_null::ms_get_pos(tAbsCoord& x, tAbsCoord& y) const
{
    UNUSED(x);
    UNUSED(y);
    throw std::logic_error("input_null::ms_set_btns");
}

int input_null::ms_get_btns()
{
    throw std::logic_error("input_null::ms_get_btns");
}


input_null::input_null(System& s,const std::string& user_cp)
		:input_interface(s,user_cp)
		,sys(s)
{
    throw missing_device_exception();
}

input_null::~input_null()
{
}

static input_interface* query_interface(System& s,const std::string& user_cp) { return new(zeromem) input_null(s,user_cp); }

extern const input_interface_info input_null_info = {
    "null input",
    query_interface
};
} // namespace	usr