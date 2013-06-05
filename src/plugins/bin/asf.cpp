#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/asf.c
 * @brief       This file contains implementation of decoder for ASF v1
 *              file format.
 * @version     -
 * @remark      this source file is part of asfary vIEW project (BEYE).
 *              The asfary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class ASF_Parser : public Binary_Parser {
	public:
	    ASF_Parser(binary_stream& h,CodeGuider&,udn&);
	    virtual ~ASF_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	private:
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* ASF_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t ASF_Parser::show_header() const
{
    beye_context().ErrMessageBox("Not implemented yet!","ASF format");
    return beye_context().tell();
}

ASF_Parser::ASF_Parser(binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(h,code_guider,u)
	    ,main_handle(h)
	    ,_udn(u)
{
    const unsigned char asfhdrguid[16]= {0x30,0x26,0xB2,0x75,0x8E,0x66,0xCF,0x11,0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C};
/*    const unsigned char asf2hdrguid[16]={0xD1,0x29,0xE2,0xD6,0xDA,0x35,0xD1,0x11,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE}; */
    unsigned char buff[16];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(buff,16);
    if(memcmp(buff,asfhdrguid,16)!=0) throw bad_format_exception();
}
ASF_Parser::~ASF_Parser() {}

int ASF_Parser::query_platform() const { return DISASM_DEFAULT; }

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) ASF_Parser(h,_parent,u); }
extern const Binary_Parser_Info asf_info = {
    "Advanced stream file format v1",	/**< plugin name */
    query_interface
};
} // namespace	usr
