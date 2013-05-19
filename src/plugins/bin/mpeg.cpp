#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mpeg.c
 * @brief       This file contains implementation of decoder for MPEG-PES
 *              file format.
 * @version     -
 * @remark      this source file is part of mpegary vIEW project (BEYE).
 *              The mpegary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "plugins/binary_parser.h"

namespace	usr {
/*
 headers:
 0x00000100		- picture_start_code
 0x00000101-0x000001AF	- slice_start_code
 0x000001B0-0x000001B1	- reserved
 0x000001B2		- user_data_start_code
 0x000001B3		- sequence_header_code
 0x000001B4		- sequence_error_code
 0x000001B5		- extension_start_code
 0x000001B6		- reserved
 0x000001B7		- sequence_end_code
 0x000001B8		- group_start_code
 0x000001B9-0x000001FF	- system_start_code
  0x000001B9		- ISO_111172_end_code
  0x000001BA		- pack_start_code
  0x000001BB		- system_header_start_code
  0x000001BC		- program_stream_map
  0x000001BD		- private_stream_1
  0x000001BE		- padding_stream
  0x000001BF		- private_stream_2
  0x000001C0-0x000001DF - audio_stream_prefixes
  0x000001E0-0x000001EF - video_stream_prefixes
  0x000001F0		- ECM_stream
  0x000001F1		- EMM_stream
  0x000001F2		- DSM_CC_stream
  0x000001F3		- ISO_13522_stream
  0x000001FF		- prog_stream_dir
*/
    class Mpeg_Parser : public Binary_Parser {
	public:
	    Mpeg_Parser(binary_stream&,CodeGuider&,udn& u);
	    virtual ~Mpeg_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual int			query_platform() const;
	private:
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* Mpeg_Parser::prompt(unsigned idx) const { return txt[idx]; }

Mpeg_Parser::Mpeg_Parser(binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(h,code_guider,u)
	    ,main_handle(h)
	    ,_udn(u)
{}
Mpeg_Parser::~Mpeg_Parser() {}
int Mpeg_Parser::query_platform() const { return DISASM_DEFAULT; }

static bool probe(binary_stream& main_handle) {
    UNUSED(main_handle);
    return false;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) Mpeg_Parser(h,_parent,u); }
extern const Binary_Parser_Info mpeg_info = {
    "MPEG-PES file format",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
