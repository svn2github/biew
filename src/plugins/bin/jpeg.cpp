#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/jpeg.c
 * @brief       This file contains implementation of decoder for jpeg
 *              file format.
 * @version     -
 * @remark      this source file is part of jpegary vIEW project (BEYE).
 *              The jpegary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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
#include <string.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
    class Jpeg_Parser : public Binary_Parser {
	public:
	    Jpeg_Parser(CodeGuider&);
	    virtual ~Jpeg_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* Jpeg_Parser::prompt(unsigned idx) const { return txt[idx]; }

Jpeg_Parser::Jpeg_Parser(CodeGuider& code_guider):Binary_Parser(code_guider) { UNUSED(code_guider); }
Jpeg_Parser::~Jpeg_Parser() {}
int Jpeg_Parser::query_platform() const { return DISASM_DEFAULT; }

__filesize_t Jpeg_Parser::show_header()
{
    beye_context().ErrMessageBox("Not implemented yet!","JPEG format");
    return beye_context().bm_file().tell();
}

static bool probe() {
    unsigned long val;
    unsigned char id[4];
    beye_context().sc_bm_file().seek(0,binary_stream::Seek_Set);
    val = beye_context().sc_bm_file().read(type_dword);
    beye_context().sc_bm_file().seek(6,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(id,4);
    if(val==0xE0FFD8FF && memcmp(id,"JFIF",4)==0) return true;
    return false;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) Jpeg_Parser(_parent); }
extern const Binary_Parser_Info jpeg_info = {
    "JPEG file format",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
