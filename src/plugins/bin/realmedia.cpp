#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/realmdeia.c
 * @brief       This file contains implementation of decoder for jpeg
 *              file format.
 * @version     -
 * @remark      this source file is part of rmary vIEW project (BEYE).
 *              The rmary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "bmfile.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"

namespace	usr {
#define MKTAG(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))
    class RM_Parser : public Binary_Parser {
	public:
	    RM_Parser(CodeGuider&);
	    virtual ~RM_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* RM_Parser::prompt(unsigned idx) const { return txt[idx]; }


__filesize_t RM_Parser::show_header()
{
    beye_context().ErrMessageBox("Not implemented yet!","RM format");
    return BMGetCurrFilePos();
}

RM_Parser::RM_Parser(CodeGuider& code_guider):Binary_Parser(code_guider) {}
RM_Parser::~RM_Parser() {}
int  RM_Parser::query_platform() const { return DISASM_DEFAULT; }

static bool probe() {
    if(bmReadDWordEx(0,binary_stream::Seek_Set)==MKTAG('.', 'R', 'M', 'F')) return true;
    return false;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) RM_Parser(_parent); }
extern const Binary_Parser_Info rm_info = {
    "Real Media file format",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
