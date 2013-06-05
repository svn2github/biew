#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/rdoff2.c
 * @brief       This file contains implementation of decoder for RDOFF v2 file format.
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
#include <string.h>

#include "plugins/disasm.h"
#include "plugins/bin/rdoff2.h"
#include "udn.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class RDOff2_Parser : public Binary_Parser {
	public:
	    RDOff2_Parser(binary_stream&,CodeGuider&,udn&);
	    virtual ~RDOff2_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	private:
	    binary_stream&		main_handle;
	    udn&			_udn;
    };
static const char* txt[]={"RdHelp","","","","","","","","",""};
const char* RDOff2_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t RDOff2_Parser::show_header() const
{
  int endian;
  __filesize_t fpos;
  unsigned long hs_len,im_len;
  TWindow *w;
  fpos = beye_context().tell();
  main_handle.seek(5,binary_stream::Seek_Set);
  endian = main_handle.read(type_byte);
  im_len = main_handle.read(type_dword);
  hs_len = main_handle.read(type_dword);
  w = CrtDlgWndnls(endian == 0x02 ? " RDOFFv2 big endian " : " RDOFFv2 little endian ",54,5);
  w->goto_xy(1,1);
  w->printf(
	   "Image length                = %08lXH\n"
	   "Header length               = %08lXH\n"
	   ,im_len
	   ,hs_len);
  while(1)
  {
    int keycode;
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    switch(keycode)
    {
/*
      case KE_ENTER:      fpos = cs_start; goto exit;
      case KE_F(5):
      case KE_CTL_ENTER:  fpos = ds_start; goto exit;
*/
      case KE_F(10):
      case KE_ESCAPE:     goto exit;
      default:            break;
    };
  }
  exit:
  delete w;
  return fpos;
}

__filesize_t RDOff2_Parser::action_F1()
{
    Beye_Help bhelp;
    if(bhelp.open(true)) {
	bhelp.run(10012);
	bhelp.close();
    }
    return beye_context().tell();
}


RDOff2_Parser::RDOff2_Parser(binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(h,code_guider,u)
	    ,main_handle(h)
	    ,_udn(u)
{
    char rbuff[6];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(rbuff,sizeof(rbuff));
    if(!(memcmp(rbuff,"RDOFF2",sizeof(rbuff)) == 0 ||
	 memcmp(rbuff,"RDOFF\x2",sizeof(rbuff)) == 0)) throw bad_format_exception();
}
RDOff2_Parser::~RDOff2_Parser() {}

int RDOff2_Parser::query_platform() const { return DISASM_CPU_IX86; }

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) RDOff2_Parser(h,_parent,u); }
extern const Binary_Parser_Info rdoff2_info = {
    "RDOFF v2 (Relocatable Dynamic Object File Format)",	/**< plugin name */
    query_interface
};
} // namespace	usr
