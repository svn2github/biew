#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mov.c
 * @brief       This file contains implementation of decoder for MOV
 *              file format.
 * @version     -
 * @remark      this source file is part of movary vIEW project (BEYE).
 *              The movary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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
#include "libbeye/bswap.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
#define MOV_FOURCC(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|(d))
    class MOV_Parser : public Binary_Parser {
	public:
	    MOV_Parser(CodeGuider&);
	    virtual ~MOV_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;

	    static __filesize_t		mov_find_chunk(__filesize_t off,unsigned long id);
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* MOV_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t MOV_Parser::mov_find_chunk(__filesize_t off,unsigned long id)
{
    unsigned long ids,size;
    beye_context().sc_bm_file().seek(off,binary_stream::Seek_Set);
    while(!beye_context().sc_bm_file().eof())
    {
	size=be2me_32(beye_context().sc_bm_file().read(type_dword));
	if(size < 8) return -1;
	ids=be2me_32(beye_context().sc_bm_file().read(type_dword));
	if(ids==id) return beye_context().sc_bm_file().tell()-8;
	beye_context().sc_bm_file().seek(size-8,binary_stream::Seek_Cur);
    }
    return -1;
}


MOV_Parser::MOV_Parser(CodeGuider& code_guider):Binary_Parser(code_guider) {}
MOV_Parser::~MOV_Parser() {}
int MOV_Parser::query_platform() const { return DISASM_DEFAULT; }

__filesize_t MOV_Parser::show_header()
{
    beye_context().ErrMessageBox("Not implemented yet!","MOV format");
    return beye_context().bm_file().tell();
}

static bool probe() {
    __filesize_t moov,mdat;
    moov=MOV_Parser::mov_find_chunk(0,MOV_FOURCC('m','o','o','v'));
    mdat=MOV_Parser::mov_find_chunk(0,MOV_FOURCC('m','d','a','t'));
    if(moov != -1 && mdat != -1) return true;
    return false;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) MOV_Parser(_parent); }
extern const Binary_Parser_Info mov_info = {
    "MOV file format",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
