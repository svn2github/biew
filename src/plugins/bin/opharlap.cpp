#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_auto
 * @file        plugins/bin/opharlap.c
 * @brief       This file contains implementation of Old PharLap file format decoder.
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
#include <stdio.h>
#include <string.h>

#include "colorset.h"
#include "bin_util.h"
#include "bmfile.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "reg_form.h"
#include "plugins/bin/pharlap.h"
#include "plugins/disasm.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
static oldPharLap oph;

static bool __FASTCALL__ IsOldPharLap()
{
   char sign[2];
   bmReadBufferEx(sign,2,0,BFile::Seek_Set);
   if(sign[0] == 'M' && sign[1] == 'P') return true;
   return false;
}

static __filesize_t __FASTCALL__ ShowOPharLapHeader()
{
  __filesize_t fpos,entrypoint;
  TWindow * w;
  unsigned keycode;
  fpos = BMGetCurrFilePos();
  entrypoint = oph.plHeadSize*16 + oph.plEIP;
  w = CrtDlgWndnls(" Old PharLap executable ",54,11);
  w->goto_xy(1,1);
  w->printf(
	   "Image size reminder on last page   = %04XH\n"
	   "Image size in pages                = %04XH\n"
	   "Number of relocation items         = %04XH\n"
	   "Header size in paragraphs          = %04XH\n"
	   "Min. number of extra 4K pages      = %04XH\n"
	   "Max. number of extra 4K pages      = %04XH\n"
	   "Initial ESP                        = %08lXH\n"
	   "File checksum                      = %04XH\n"
	   "Initial EIP                        = %08lXH\n"
	   "Offset of first relocation item    = %04XH\n"
	   "Number of overlays                 = %04XH\n"
	   ,oph.plSizeRemaind
	   ,oph.plImageSize
	   ,oph.plNRelocs
	   ,oph.plHeadSize
	   ,oph.plMinExtraPages
	   ,oph.plMaxExtraPages
	   ,oph.plESP
	   ,oph.plCheckSum
	   ,oph.plEIP
	   ,oph.plFirstReloc
	   ,oph.plNOverlay);
  w->set_color(dialog_cset.entry);
  w->printf("Entry Point                        = %08lXH",entrypoint);
  w->clreol();
  while(1)
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER) { fpos = entrypoint; break; }
    else
      if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
  }
  delete w;
  return fpos;
}

static void __FASTCALL__ OPharLapInit(CodeGuider& code_guider)
{
    UNUSED(code_guider);
  bmReadBufferEx(&oph,sizeof(oph),0,BFile::Seek_Set);
}

static void __FASTCALL__ OPharLapDestroy()
{
}

static bool __FASTCALL__ OldPharLapAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  if(cfpos < sizeof(oldPharLap))
  {
    strcpy(addr,"oplhdr:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else bret = false;
  return bret;
}

static __filesize_t __FASTCALL__ HelpOPharLap()
{
  hlpDisplay(10008);
  return BMGetCurrFilePos();
}

static int __FASTCALL__ OldPharLapPlatform() { return DISASM_CPU_IX86; }

extern const REGISTRY_BIN OldPharLapTable =
{
  "Pharlap",
  { "PLHelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { HelpOPharLap, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  IsOldPharLap,
  OPharLapInit,
  OPharLapDestroy,
  ShowOPharLapHeader,
  NULL,
  OldPharLapPlatform,
  NULL,
  NULL,
  OldPharLapAddrResolv,
  NULL,
  NULL,
  NULL,
  NULL
};
} // namespace beye
