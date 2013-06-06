/**
 * @namespace	usr
 * @file        setup.h
 * @brief       This file included BEYE setup functions description.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#ifndef __SETUP__H
#define __SETUP__H

namespace	usr {
    class Setup : public Opaque {
	public:
	    Setup(BeyeContext& bc);
	    virtual ~Setup();

	    virtual void	run();
	private:
	    bool		select_codepage();
	    void		paint(TWindow& twin);

	    static void		draw_prompt();

	    BeyeContext&	bctx;
	    unsigned		default_cp;
	    static const char*	setuptxt[];
	    static const char*	cp_list[];
    };
    std::string		beyeGetHelpName();
} // namespace	usr
#endif
