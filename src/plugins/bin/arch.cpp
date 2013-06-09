#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/arch.c
 * @brief       This file contains implementation of Archive file format decoder.
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
#include <algorithm>
#include <sstream>
#include <iomanip>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "libbeye/bswap.h"
#include "udn.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "listbox.h"
#include "tstrings.h"
#include "plugins/bin/arch.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class Arch_Parser : public Binary_Parser {
	public:
	    Arch_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~Arch_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F3();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	    virtual std::string		address_resolving(__filesize_t);
	private:
	    std::vector<std::string>	archReadModList(size_t nnames,__filesize_t* addr);

	    ar_hdr		arch;
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "ArcHlp", "", "ModLst", "", "", "", "", "", "", "" };
const char* Arch_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t Arch_Parser::show_header() const
{
  __filesize_t fpos;
  unsigned evt;
  TWindow * w;
  struct tm * tm;
  time_t ldat;
  char sout[50];
  fpos = bctx.tell();
  w = CrtDlgWndnls(" This is COFF or a.out archive ",54,6);
  w->goto_xy(1,1);
  strncpy(sout,(char *)arch.ar_name,16);
  sout[16] = 0;
  w->printf("Name           = %s\n",sout);
  strncpy(sout,(char *)arch.ar_date,12);
  sout[12] = 0;
  ldat = atol(sout);
  tm = localtime(&ldat);
  strftime(sout,sizeof(sout),"%X %x",tm);
  w->printf("Date           = %s\n",sout);
  strncpy(sout,(char *)arch.ar_uid,6);
  sout[6] = 0;
  w->printf("Owner UID      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_gid,6);
  sout[6] = 0;
  w->printf("Owner GID      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_mode,8);
  sout[8] = 0;
  w->printf("File mode      = %s\n",sout);
  strncpy(sout,(char *)arch.ar_size,10);
  sout[10] = 0;
  w->printf("File size      = %s bytes",sout);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,w);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  delete w;
  return fpos;
}

std::vector<std::string> Arch_Parser::archReadModList(size_t nnames,__filesize_t *addr)
{
    std::vector<std::string> rc;
    __filesize_t foff,flen;
    unsigned i;
    char stmp[80];
    flen = main_handle.flength();
    for(i = 0;i < nnames;i++) {
	bool is_eof;
/**
    Some archives sometimes have big and sometimes little endian.
    Here is a horrible attempt to determine it.
*/
	foff = addr[i];
	if(foff > flen)  foff = be2me_32(foff);
	if(IsKbdTerminate()) break;
	main_handle.seek(foff,binary_stream::Seek_Set);
	binary_packet bp=main_handle.read(sizeof(ar_sub_hdr)); memcpy(stmp,bp.data(),bp.size());
	is_eof = main_handle.eof();
	stmp[sizeof(ar_sub_hdr)-2] = 0;
	rc.push_back(is_eof ? CORRUPT_BIN_MSG : stmp);
	if(is_eof) break;
    }
    return rc;
}

__filesize_t Arch_Parser::action_F3()
{
    __filesize_t *addr;
    unsigned long rnames,bnames;
    unsigned nnames;
    __filesize_t fpos,flen;
    fpos = bctx.tell();
    flen = main_handle.flength();
    main_handle.seek(sizeof(ar_hdr),binary_stream::Seek_Set);
    rnames = main_handle.read(type_dword);
    bnames = be2me_32(rnames);
   /**
      Some archives sometimes have big and sometimes little endian.
      Here is a horrible attempt to determine it.
   */
    if(!(nnames = (unsigned)std::min(rnames,bnames))) { bctx.NotifyBox(NOT_ENTRY,"Archive modules list"); return fpos; }
   /**
      Some archives sometimes have length and sometimes number of entries
      Here is a horrible attempt to determine it.
   */
    if(!(nnames%4)) nnames/=sizeof(unsigned long);
    if(!(addr = new __filesize_t [nnames])) return fpos;
    main_handle.seek(sizeof(ar_hdr)+sizeof(unsigned long),binary_stream::Seek_Set);
    binary_packet bp=main_handle.read(sizeof(unsigned long)*nnames); memcpy(addr,bp.data(),bp.size());
    std::vector<std::string> objs = archReadModList(nnames,addr);
    if(!objs.empty()) {
	int ret;
	ListBox lb(bctx);
	ret = lb.run(objs," Archive modules list ",ListBox::Selective,-1);
	if(ret != -1) {
	/**
	    Some archives sometimes have big and sometimes little endian.
	    Here is a horrible attempt to determine it.
	*/
	    fpos = addr[ret];
	    if(fpos > flen) fpos = be2me_32(fpos);
	    fpos += sizeof(ar_sub_hdr);
	}
    }
    delete addr;
    return fpos;
}

Arch_Parser::Arch_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    main_handle.seek(0,binary_stream::Seek_Set);
    binary_packet bp=main_handle.read(sizeof(arch)); memcpy(&arch,bp.data(),bp.size());
    if(strncmp((const char*)arch.ar_magic,"!<arch>\012",8) != 0) throw bad_format_exception();
}
Arch_Parser::~Arch_Parser(){}

std::string Arch_Parser::address_resolving(__filesize_t cfpos)
{
    std::ostringstream oss;
    /* Since this function is used in references resolving of disassembler
	it must be seriously optimized for speed. */
    if(cfpos < sizeof(ar_hdr)) oss<<"arch.h:"<<std::hex<<std::setfill('0')<<std::setw(2)<<cfpos;
    return oss.str();
}

__filesize_t Arch_Parser::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(10001);
	bhelp.close();
    }
    return bctx.tell();
}

int Arch_Parser::query_platform() const { return DISASM_DEFAULT; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& main_handle,CodeGuider& _parent,udn& u) { return new(zeromem) Arch_Parser(b,main_handle,_parent,u); }
extern const Binary_Parser_Info arch_info = {
    "arch (Archive)",	/**< plugin name */
    query_interface
};
} // namespace	usr
