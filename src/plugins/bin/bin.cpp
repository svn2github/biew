#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/bin.c
 * @brief       This file contains implementation of decoder for any not handled
 *              binary file format.
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
#include <stddef.h>

#include "plugins/disasm.h"
#include "plugins/binary_parser.h"

namespace	usr {
    class Bin_Parser : public Binary_Parser {
	public:
	    Bin_Parser(binary_stream& h,CodeGuider&,udn&);
	    virtual ~Bin_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual int			query_platform() const;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* Bin_Parser::prompt(unsigned idx) const { return txt[idx]; }

Bin_Parser::Bin_Parser(binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(h,code_guider,u) {}
Bin_Parser::~Bin_Parser() {}
int  Bin_Parser::query_platform() const { return DISASM_DEFAULT; }

static bool probe(binary_stream&) { return true; }
static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) Bin_Parser(h,_parent,u); }
extern const Binary_Parser_Info bin_info = {
    "Binary file",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
