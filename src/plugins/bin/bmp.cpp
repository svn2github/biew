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
#include "beyeutil.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
    class BMP_Parser : public Binary_Parser {
	public:
	    BMP_Parser(CodeGuider&);
	    virtual ~BMP_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* BMP_Parser::prompt(unsigned idx) const { return txt[idx]; }

BMP_Parser::BMP_Parser(CodeGuider& code_guider):Binary_Parser(code_guider) {}
BMP_Parser::~BMP_Parser() {}
int BMP_Parser::query_platform() const { return DISASM_DEFAULT; }

__filesize_t BMP_Parser::show_header()
{
 unsigned keycode;
 TWindow * hwnd;
 BITMAPINFOHEADER bmph;
 __filesize_t fpos,fpos2;
 fpos = beye_context().bm_file().tell();
 beye_context().sc_bm_file().seek(2,binary_stream::Seek_Set);
 /*filesize = */beye_context().sc_bm_file().read(type_dword);
 beye_context().sc_bm_file().seek(4,binary_stream::Seek_Cur);
 fpos2=beye_context().sc_bm_file().read(type_word); /* data offset */
 beye_context().sc_bm_file().seek(2,binary_stream::Seek_Cur);
 beye_context().sc_bm_file().read(&bmph,sizeof(BITMAPINFOHEADER));
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
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
   else
     if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 delete hwnd;
 return fpos;
}

static bool probe() {
    beye_context().sc_bm_file().seek(0,binary_stream::Seek_Set);
    if(	beye_context().sc_bm_file().read(type_byte) == 'B' &&
	beye_context().sc_bm_file().read(type_byte) == 'M') return true;
    return false;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) BMP_Parser(_parent); }
extern const Binary_Parser_Info bmp_info = {
    "BitMaP file format",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
