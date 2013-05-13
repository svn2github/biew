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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "libbeye/bswap.h"
#include "bin_util.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "tstrings.h"
#include "plugins/bin/arch.h"
#include "plugins/disasm.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
    class Arch_Parser : public Binary_Parser {
	public:
	    Arch_Parser(CodeGuider&);
	    virtual ~Arch_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F3();

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
	    virtual bool		address_resolving(char *,__filesize_t);
	private:
	    bool			archReadModList(memArray *obj,unsigned nnames,__filesize_t *addr);

	    ar_hdr arch;
    };
static const char* txt[]={ "ArcHlp", "", "ModLst", "", "", "", "", "", "", "" };
const char* Arch_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t Arch_Parser::show_header()
{
  __filesize_t fpos;
  unsigned evt;
  TWindow * w;
  struct tm * tm;
  time_t ldat;
  char sout[50];
  fpos = beye_context().bm_file().tell();
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

bool Arch_Parser::archReadModList(memArray *obj,unsigned nnames,__filesize_t *addr)
{
  __filesize_t foff,flen;
  unsigned i;
  char stmp[80];
  flen = beye_context().sc_bm_file().flength();
  for(i = 0;i < nnames;i++)
  {
    bool is_eof;
    /**
       Some archives sometimes have big and sometimes little endian.
       Here is a horrible attempt to determine it.
    */
    foff = addr[i];
    if(foff > flen)  foff = be2me_32(foff);
    if(IsKbdTerminate()) break;
    beye_context().sc_bm_file().seek(foff,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(stmp,sizeof(ar_sub_hdr));
    is_eof = beye_context().sc_bm_file().eof();
    stmp[sizeof(ar_sub_hdr)-2] = 0;
    if(!ma_AddString(obj,is_eof ? CORRUPT_BIN_MSG : stmp,true)) break;
    if(is_eof) break;
  }
  return true;
}

__filesize_t Arch_Parser::action_F3()
{
   memArray *obj;
   __filesize_t *addr;
   unsigned long rnames,bnames;
   unsigned nnames;
   __filesize_t fpos,flen;
   fpos = beye_context().bm_file().tell();
   flen = beye_context().sc_bm_file().flength();
    beye_context().sc_bm_file().seek(sizeof(ar_hdr),binary_stream::Seek_Set);
    rnames = beye_context().sc_bm_file().read(type_dword);
   bnames = be2me_32(rnames);
   /**
      Some archives sometimes have big and sometimes little endian.
      Here is a horrible attempt to determine it.
   */
   if(!(nnames = (unsigned)std::min(rnames,bnames))) { beye_context().NotifyBox(NOT_ENTRY,"Archive modules list"); return fpos; }
   /**
      Some archives sometimes have length and sometimes number of entries
      Here is a horrible attempt to determine it.
   */
   if(!(nnames%4)) nnames/=sizeof(unsigned long);
   if(!(obj = ma_Build(nnames,true))) return fpos;
   if(!(addr = new __filesize_t [nnames])) goto exit;
    beye_context().sc_bm_file().seek(sizeof(ar_hdr)+sizeof(unsigned long),binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(addr,sizeof(unsigned long)*nnames);
   if(archReadModList(obj,nnames,addr))
   {
     int ret;
     ret = ma_Display(obj," Archive modules list ",LB_SELECTIVE,-1);
     if(ret != -1)
     {
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
   exit:
   ma_Destroy(obj);
   return fpos;
}

Arch_Parser::Arch_Parser(CodeGuider& code_guider)
	    :Binary_Parser(code_guider)
{
    beye_context().sc_bm_file().seek(0,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(&arch,sizeof(arch));
}
Arch_Parser::~Arch_Parser(){}

bool Arch_Parser::address_resolving(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  if(cfpos < sizeof(ar_hdr))
  {
    strcpy(addr,"arch.h:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else bret = false;
  return bret;
}

__filesize_t Arch_Parser::action_F1()
{
  hlpDisplay(10001);
  return beye_context().bm_file().tell();
}

static bool probe() {
  char str[16];
    beye_context().sc_bm_file().seek(0,binary_stream::Seek_Set);
    beye_context().sc_bm_file().read(str,sizeof(str));
  return strncmp(str,"!<arch>\012",8) == 0;
}

int Arch_Parser::query_platform() const { return DISASM_DEFAULT; }

static Binary_Parser* query_interface(CodeGuider& _parent) { return new(zeromem) Arch_Parser(_parent); }
extern const Binary_Parser_Info arch_info = {
    "arch (Archive)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
