#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/pharlap.c
 * @brief       This file contains implementation of PharLap file format decoder.
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
#include <stdlib.h>
#include <stdio.h>

#include "colorset.h"
#include "bconsole.h"
#include "udn.h"
#include "beyehelp.h"
#include "tstrings.h"
#include "plugins/bin/pharlap.h"
#include "plugins/disasm.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    class PharLap_Parser : public Binary_Parser {
	public:
	    PharLap_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~PharLap_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F9();
	    virtual __filesize_t	action_F10();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	    virtual std::string		address_resolving(__filesize_t);
	private:
	    std::vector<PLRunTimeParms>	__PLReadRunTime(binary_stream& handle,size_t nnames) const;
	    void			PLRunTimePaint(TWindow& win,const std::vector<PLRunTimeParms>& names,unsigned start) const;
	    std::vector<PLSegInfo>	__PLReadSegInfo(binary_stream& handle,size_t nnames) const;
	    void			PLSegPaint(TWindow& win,const std::vector<PLSegInfo>& names,unsigned start) const;

	    BeyeContext&		bctx;
	    binary_stream&		main_handle;
	    udn&			_udn;
	    binary_stream*		pl_cache;
	    newPharLap			nph;
    };
static const char* txt[]={ "PLHelp", "", "", "", "", "", "", "", "RunTim", "SegInf" };
const char* PharLap_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t PharLap_Parser::show_header() const
{
  __filesize_t fpos;
  TWindow *w;
  unsigned keycode;
  char sign[3];
  fpos = bctx.tell();
  strncpy(sign,(char *)nph.plSignature,2);
  sign[2] = 0;
  w = CrtDlgWndnls(" New PharLap executable ",59,23);
  w->goto_xy(1,1);
  w->printf(
	   "Signature                        = %s\n"
	   "Level                            = %s\n"
	   "Header size                      = %04XH\n"
	   "File size                        = %08lXH\n"
	   "Check sum (32-bit checksum)      = %04XH (%08lXH)\n"
	   "Run-time parameters (offset/size)= %08lXH/%08lXH\n"
	   "Relocations (offset/size)        = %08lXH/%08lXH\n"
	   "Segment info table (offset/size) = %08lXH/%08lXH\n"
	   "Image (offset/size)              = %08lXH/%08lXH\n"
	   "Symbol table (offset/size)       = %08lXH/%08lXH\n"
	   "GDT (offset/size)                = %08lXH/%08lXH\n"
	   "LDT (offset/size)                = %08lXH/%08lXH\n"
	   "IDT (offset/size)                = %08lXH/%08lXH\n"
	   "TSS (offset/size)                = %08lXH/%08lXH\n"
	   "Min. number of extra 4K pages    = %08lXH\n"
	   "Max. number of extra 4K pages    = %08lXH\n"
	   "Image base (flat level only)     = %08lXH\n"
	   "Initial stack (SS:ESP)           = %04XH:%08lXH\n"
	   "Initail code  (CS:EIP)           = %04XH:%08lXH\n"
	   "Initial LDT/TSS                  = %04XH/%04XH\n"
	   "Flags                            = %04XH\n"
	   "Memory requirement for image     = %08lXH\n"
	   "Stack size                       = %08lXH"
	   ,sign
	   ,nph.plLevel == 0x01 ? "Flat" : nph.plLevel == 0x02 ? "Multisegmented" : "Unknown"
	   ,nph.plHeaderSize
	   ,nph.plFileSize
	   ,nph.plCheckSum,nph.plChecksum32
	   ,nph.plRunTimeParms,nph.plRunTimeSize
	   ,nph.plRelocOffset,nph.plRelocSize
	   ,nph.plSegInfoOffset,nph.plSegInfoSize
	   ,nph.plImageOffset,nph.plImageSize
	   ,nph.plSymTabOffset,nph.plSymTabSize
	   ,nph.plGDTOffset,nph.plGDTSize
	   ,nph.plLDTOffset,nph.plLDTSize
	   ,nph.plIDTOffset,nph.plIDTSize
	   ,nph.plTSSOffset,nph.plTSSSize
	   ,nph.plMinExtraPages
	   ,nph.plMaxExtraPages
	   ,nph.plBase
	   ,nph.plSS,nph.plESP
	   ,nph.plCS,nph.plEIP
	   ,nph.plLDT,nph.plTSS
	   ,nph.plFlags
	   ,nph.plMemReq
	   ,nph.plStackSize);
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { /* fpos = entrypoint*/; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  delete w;
  return fpos;
}

void PharLap_Parser::PLSegPaint(TWindow& win,const std::vector<PLSegInfo>& names,unsigned start) const
{
    char buffer[81];
    const PLSegInfo& nam = names[start];
    win.freeze();
    win.clear();
    sprintf(buffer," Segment Table [ %u / %u ] ",start + 1,names.size());
    win.set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    win.goto_xy(1,1);
    win.printf(
	  "Selector number            = %04hXH\n"
	  "Flags                      = %04hXH\n"
	  "Base offset of selector    = %08lXH\n"
	  "Min extra memory alloc     = %08lXH"
	  ,nam.siSelector
	  ,nam.siFlags
	  ,nam.siBaseOff
	  ,nam.siMinAlloc);
    win.refresh_full();
}

std::vector<PLSegInfo> PharLap_Parser::__PLReadSegInfo(binary_stream& handle,size_t nnames) const
{
    std::vector<PLSegInfo> rc;
    unsigned i;
    for(i = 0;i < nnames;i++) {
	PLSegInfo plsi;
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(&plsi,sizeof(PLSegInfo));
	rc.push_back(plsi);
    }
    return rc;
}

__filesize_t PharLap_Parser::action_F10()
{
    binary_stream& handle = *pl_cache;
    unsigned nnames;
    __filesize_t fpos;
    if(nph.plSegInfoOffset && nph.plSegInfoSize) nnames = (unsigned)(nph.plSegInfoSize / sizeof(PLSegInfo));
    else                                           nnames = 0;
    fpos = bctx.tell();
    if(!nnames) { bctx.NotifyBox(NOT_ENTRY," Segment Info table "); return fpos; }
    handle.seek(nph.plSegInfoOffset,binary_stream::Seek_Set);
    std::vector<PLSegInfo> objs = __PLReadSegInfo(handle,nnames);
    if(!objs.empty()) {
	int i = PageBox(50,4,objs,*this,&PharLap_Parser::PLSegPaint) + 1;
	if(i > 0) fpos = ((__filesize_t)objs[i - 1].siBaseOff)+nph.plImageOffset;
    }
    return fpos;
}

void PharLap_Parser::PLRunTimePaint(TWindow& win,const std::vector<PLRunTimeParms>& names,unsigned start) const
{
    char buffer[81];
    char sign[3];
    const PLRunTimeParms& nam = names[start];
    win.freeze();
    win.clear();
    sprintf(buffer," Run-time Parameters Table [ %u / %u ] ",start + 1,names.size());
    win.set_title(buffer,TWindow::TMode_Center,dialog_cset.title);
    win.set_footer(PAGEBOX_SUB,TWindow::TMode_Right,dialog_cset.selfooter);
    strncpy(sign,(const char *)nam.rtSignature,2);
    sign[2] = 0;
    win.goto_xy(1,1);
    win.printf(
	  "Signature                      = %s\n"
	  "Min. number of real-mode parms = %04hXH\n"
	  "Max. number of real-mode parms = %04hXH\n"
	  "Min. interrupt buffer size     = %04hXH\n"
	  "Max. interrupt buffer size     = %04hXH\n"
	  "Number of interrupt stacks     = %04hXH\n"
	  "Size of each interrupt stack   = %04hXH\n"
	  "Offset of end of real-mode data= %08lXH\n"
	  "Call buffer size               = %04hXH\n"
	  "Flags                          = %04hXH\n"
	  "Unpriviledge flags             = %04hXH"
	  ,sign
	  ,nam.rtMinRModeParms
	  ,nam.rtMaxRModeParms
	  ,nam.rtMinIBuffSize
	  ,nam.rtMaxIBuffSize
	  ,nam.rtNIStacks
	  ,nam.rtIStackSize
	  ,nam.rtEndRModeOffset
	  ,nam.rtCallBuffSize
	  ,nam.rtFlags
	  ,nam.rtUnprivFlags);
    win.refresh_full();
}

std::vector<PLRunTimeParms> PharLap_Parser::__PLReadRunTime(binary_stream& handle,size_t nnames) const
{
    std::vector<PLRunTimeParms> rc;
    unsigned i;
    for(i = 0;i < nnames;i++) {
	PLRunTimeParms plrtp;
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(&plrtp,sizeof(PLRunTimeParms));
	rc.push_back(plrtp);
    }
    return rc;
}

__filesize_t PharLap_Parser::action_F9()
{
    binary_stream& handle = *pl_cache;
    unsigned nnames;
    __filesize_t fpos;
    if(nph.plRunTimeParms && nph.plRunTimeSize) nnames = (unsigned)(nph.plRunTimeSize / sizeof(PLRunTimeParms));
    else                                          nnames = 0;
    fpos = bctx.tell();
    if(!nnames) { bctx.NotifyBox(NOT_ENTRY," Run-time parameters "); return fpos; }
    handle.seek(nph.plRunTimeParms,binary_stream::Seek_Set);
    std::vector<PLRunTimeParms> objs = __PLReadRunTime(handle,nnames);
    if(!objs.empty()) {
	int i = PageBox(50,11,objs,*this,&PharLap_Parser::PLRunTimePaint) + 1;
	if(i > 0) fpos = nph.plRunTimeParms+i*sizeof(PLRunTimeParms);
    }
    return fpos;
}

PharLap_Parser::PharLap_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
	    ,pl_cache(&h)
{
    char sign[2];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(sign,2);
    if(!(sign[0] == 'P' && (sign[1] == '2' || sign[1] == '3'))) throw bad_format_exception();

    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&nph,sizeof(nph));
    pl_cache = main_handle.dup();
}

PharLap_Parser::~PharLap_Parser()
{
    if(pl_cache != &main_handle) delete pl_cache;
}

std::string PharLap_Parser::address_resolving(__filesize_t cfpos)
{
    std::string addr;
    /* Since this function is used in references resolving of disassembler
	it must be seriously optimized for speed. */
    if(cfpos < sizeof(newPharLap)) {
	addr="nplhdr:";
	addr+=Get2Digit(cfpos);
    }
    return addr;
}

__filesize_t PharLap_Parser::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(10010);
	bhelp.close();
    }
    return bctx.tell();
}

int PharLap_Parser::query_platform() const { return DISASM_CPU_IX86; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) PharLap_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info pharlap_info = {
    "PharLap",	/**< plugin name */
    query_interface
};
} // namespace	usr
