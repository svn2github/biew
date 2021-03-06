#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/bmp.c
 * @brief       This file contains implementation of decoder for BMP
 *              file format.
 * @version     -
 * @remark      this source file is part of bmpary vIEW project (BEYE).
 *              The bmpary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
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
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class BMP_Parser : public Binary_Parser {
	public:
	    BMP_Parser(BeyeContext& b,binary_stream& h,CodeGuider&,udn&);
	    virtual ~BMP_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	private:
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* BMP_Parser::prompt(unsigned idx) const { return txt[idx]; }

BMP_Parser::BMP_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    main_handle.seek(0,binary_stream::Seek_Set);
    if(!(main_handle.read(type_byte) == 'B' &&
	main_handle.read(type_byte) == 'M')) throw bad_format_exception();
}
BMP_Parser::~BMP_Parser() {}
int BMP_Parser::query_platform() const { return DISASM_DEFAULT; }

__filesize_t BMP_Parser::show_header() const
{
    unsigned keycode;
    TWindow * hwnd;
    BITMAPINFOHEADER bmph;
    __filesize_t fpos,fpos2;
    fpos = bctx.tell();
    main_handle.seek(2,binary_stream::Seek_Set);
    /*filesize = */main_handle.read(type_dword);
    main_handle.seek(4,binary_stream::Seek_Cur);
    fpos2=main_handle.read(type_word); /* data offset */
    main_handle.seek(2,binary_stream::Seek_Cur);
    binary_packet bp=main_handle.read(sizeof(BITMAPINFOHEADER)); memcpy(&bmph,bp.data(),bp.size());
    hwnd = CrtDlgWndnls(" BMP File Header ",43,6);
    hwnd->goto_xy(1,1);
    hwnd->printf(
	  "WxH                  = %lux%lu\n"
	  "PlanesxBitCount      = %ux%u\n"
	  "Compression          = %c%c%c%c\n"
	  "ImageSize            = %lu\n"
	  "XxYPelsPerMeter      = %lux%lu\n"
	  "ColorUsedxImportant  = %lux%lu\n"
	  ,bmph.biWidth,bmph.biHeight
	  ,bmph.biPlanes,bmph.biBitCount
	  INT_2_CHAR_ARG(bmph.biCompression)
	  ,bmph.biSizeImage
	  ,bmph.biXPelsPerMeter,bmph.biYPelsPerMeter
	  ,bmph.biClrUsed,bmph.biClrImportant);
    while(1) {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
	else if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    delete hwnd;
    return fpos;
}

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) BMP_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info bmp_info = {
    "BitMaP file format",	/**< plugin name */
    query_interface
};
} // namespace	usr
