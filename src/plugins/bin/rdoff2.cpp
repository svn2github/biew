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
#include "reg_form.h"
#include "bin_util.h"
#include "bmfile.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "libbeye/kbd_code.h"

namespace	usr {
    class RDOff2_Parser : public Binary_Parser {
	public:
	    RDOff2_Parser(CodeGuider&);
	    virtual ~RDOff2_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
    };
static const char* txt[]={"RdHelp","","","","","","","","",""};
const char* RDOff2_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t RDOff2_Parser::show_header()
{
  int endian;
  __filesize_t fpos;
  unsigned long hs_len,im_len;
  TWindow *w;
  fpos = BMGetCurrFilePos();
  endian = bmReadByteEx(5,binary_stream::Seek_Set);
  im_len = bmReadDWord();
  hs_len = bmReadDWord();
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
  hlpDisplay(10012);
  return BMGetCurrFilePos();
}


RDOff2_Parser::RDOff2_Parser(CodeGuider& code_guider):Binary_Parser(code_guider) {}
RDOff2_Parser::~RDOff2_Parser() {}

int RDOff2_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe() {
  char rbuff[6];
  bmReadBufferEx(rbuff,sizeof(rbuff),0L,binary_stream::Seek_Set);
  return memcmp(rbuff,"RDOFF2",sizeof(rbuff)) == 0 ||
	 memcmp(rbuff,"RDOFF\x2",sizeof(rbuff)) == 0;
}

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) RDOff2_Parser(_parent); }
extern const Binary_Parser_Info rdoff2_info = {
    "RDOFF v2 (Relocatable Dynamic Object File Format)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
