#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        setup.c
 * @brief       This file contains BEYE setup.
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
#include <stdio.h>

#include "beye.h"
#include "beyehelp.h"
#include "colorset.h"
#include "setup.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "listbox.h"
#include "libbeye/mmfile.h"
#include "libbeye/twindow.h"
#include "libbeye/kbd_code.h"
#include "libbeye/osdep/system.h"

#ifdef __QNX4__
extern int photon,bit7;
#endif
namespace	usr {

std::string beyeGetHelpName()
{
    BeyeContext& bctx = beye_context();
    if(!bctx.help_name[0]) {
	bctx.help_name=std::string(bctx.system().get_rc_dir("beye"))+"beye.hlp";
    }
    return bctx.help_name;
}

static std::string __FASTCALL__ beyeGetColorSetName()
{
    BeyeContext& bctx = beye_context();
    if(!bctx.skin_name[0]) {
	bctx.skin_name=std::string(bctx.system().get_rc_dir("beye"))+"skn/standard.skn"; /* [dBorca] in skn/ subdir */
    }
    return bctx.skin_name;
}

static std::string  __FASTCALL__ beyeGetSyntaxName()
{
    BeyeContext& bctx = beye_context();
    if(!bctx.syntax_name[0]) {
	bctx.syntax_name=std::string(bctx.system().get_rc_dir("beye"))+"syntax/syntax.stx"; /* [dBorca] in syntax/ subdir */
    }
    return bctx.syntax_name;
}

const char* Setup::setuptxt[] =
{
  "Help  ",
  "Consol",
  "Color ",
  "Mouse ",
  "Bit   ",
  "Plugin",
  "MMF   ",
  "Time  ",
  "ExtPrg",
  "Escape"
};

const char* Setup::cp_list[] =
{
  "CP437 - original IBM PC Latin US",
  "CP708 - Arabic language",
  "CP737 - Greek language",
  "CP775 - Baltic languages",
  "CP850 - IBM PC MSDOS Latin-1",
  "CP851 - IBM PC Greek-1",
  "CP852 - Central European languages (Latin-2)",
  "CP855 - Serbian language",
  "CP857 - Turkish language",
  "CP858 - Western European languages",
  "CP860 - Portuguese language",
  "CP861 - Icelandic language",
  "CP862 - Hebrew language",
  "CP863 - French language",
  "CP865 - Nordic languages",
  "CP866 - Cyrillic languages",
  "CP869 - IBM PC Greek-2",
};

bool Setup::select_codepage()
{
    unsigned nModes;
    int i;
    nModes = sizeof(cp_list)/sizeof(char *);
    ListBox lb(bctx);
    i = lb.run(cp_list,nModes," Select single-byte codepage: ",ListBox::Selective|ListBox::UseAcc,default_cp);
    if(i != -1) {
	unsigned len;
	const char *p;
	default_cp = i;
	p = strchr(cp_list[i],' ');
	len = p-cp_list[i];
	bctx.codepage=std::string(cp_list[i]).substr(0,len);
	return true;
    }
    return false;
}

void Setup::draw_prompt()
{
   __drawSinglePrompt(setuptxt);
}

void Setup::paint(TWindow& twin)
{
  twin.set_color(dialog_cset.group.active);
  twin.goto_xy(2,9);
  twin.printf(" [%c] - Direct console access "
	   ,Gebool((bctx.vioIniFlags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) == __TVIO_FLG_DIRECT_CONSOLE_ACCESS));
  twin.goto_xy(2,10);
  twin.printf(" [%c] - Mouse sensitivity     "
	   ,Gebool((bctx.kbdFlags & KBD_NONSTOP_ON_MOUSE_PRESS) == KBD_NONSTOP_ON_MOUSE_PRESS));
  twin.goto_xy(2,11);
  twin.printf(" [%c] - Force mono            "
	   ,Gebool((bctx.twinIniFlags & TWIF_FORCEMONO) == TWIF_FORCEMONO));
  twin.goto_xy(2,12);
#ifdef __QNX4__
  if(photon)
  {
    twin.set_color(dialog_cset.group.disabled);
    twin.printf(" [%c] - Force 7-bit output    "
	    ,Gebool(bit7));
    twin.set_color(dialog_cset.group.active);
  }
  else
#endif
  twin.printf(" [%c] - Force 7-bit output    "
	   ,Gebool((bctx.vioIniFlags & __TVIO_FLG_USE_7BIT) == __TVIO_FLG_USE_7BIT));
  twin.goto_xy(32,9);
  twin.printf(" [%c] - Apply plugin settings to all files     "
	   ,Gebool(bctx.iniSettingsAnywhere));
  twin.goto_xy(32,10);
  if(!MMFile::has_mmio) twin.set_color(dialog_cset.group.disabled);
  twin.printf(" [%c] - Use MMF                                "
	   ,Gebool(bctx.fioUseMMF));
  twin.set_color(dialog_cset.group.active);
  twin.goto_xy(32,11);
  twin.printf(" [%c] - Preserve timestamp                     "
	   ,Gebool(bctx.iniPreserveTime));
  twin.goto_xy(32,12);
  twin.printf(" [%c] - Enable usage of external programs      "
	   ,Gebool(bctx.iniUseExtProgs));
  twin.set_color(dialog_cset.main);
  twin.goto_xy(50,7); twin.puts(bctx.codepage);
}

void Setup::run()
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int ret;
  TWindow * wdlg,*ewnd[4];
  char estr[3][FILENAME_MAX+1];
  int active = 0;
  strcpy(estr[0],beyeGetHelpName().c_str());
  strcpy(estr[1],beyeGetColorSetName().c_str());
  strcpy(estr[2],beyeGetSyntaxName().c_str());
  wdlg = CrtDlgWndnls(" Setup ",78,13);
  wdlg->get_pos(x1,y1,x2,y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;

  X1 += 2;
  X2 -= 1;
  Y1 += 2;
  Y2 = Y1;
  ewnd[0] = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
  ewnd[0]->show();
  ewnd[0]->set_focus();
  PostEvent(KE_ENTER);
  xeditstring(ewnd[0],estr[0],NULL,sizeof(estr[0]), NULL);
  Y1 += 2;
  Y2 = Y1;
  ewnd[1] = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
  ewnd[1]->show();
  ewnd[1]->set_focus();
  PostEvent(KE_ENTER);
  xeditstring(ewnd[1],estr[1],NULL,sizeof(estr[1]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[2] = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
  ewnd[2]->show();
  ewnd[2]->set_focus();
  PostEvent(KE_ENTER);
  xeditstring(ewnd[2],estr[2],NULL,sizeof(estr[2]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[3] = new(zeromem) TWindow(60,Y1,2,Y2-Y1+1,TWindow::Flag_NLS);

  wdlg->goto_xy(2,1); wdlg->puts("Enter help file name (including full path):");
  wdlg->goto_xy(2,3); wdlg->puts("Enter color skin name (including full path):");
  wdlg->goto_xy(2,5); wdlg->puts("Enter syntax name (including full path):");
  wdlg->goto_xy(2,7); wdlg->puts("Enter OEM codepage (for utf-based terminals):");
  wdlg->set_footer(" [Enter] - Accept changes ",TWindow::TMode_Center,dialog_cset.footer);
  wdlg->draw_frame(1,8,78,13,TWindow::UP3D_FRAME,dialog_cset.main);

  paint(*wdlg);
  active = 0;
  ewnd[active]->set_focus();
  while(1)
  {
   if(active==3) {
	if(select_codepage() == true) paint(*wdlg);
	ret = KE_TAB;
   }
   else ret = xeditstring(ewnd[active],estr[active],NULL,sizeof(estr[active]),&Setup::draw_prompt);
   switch(ret)
   {
     case KE_F(10):
     case KE_ESCAPE: ret = 0; goto exit;
     case KE_ENTER: ret = 1; goto exit;
     case KE_SHIFT_TAB:
     case KE_TAB:   active++;
		    if(active>3) active=0;
		    ewnd[active]->set_focus();
		    continue;
     case KE_F(1):  {
			Beye_Help bhelp(bctx);
			if(bhelp.open(true)) {
			    bhelp.run(5);
			    bhelp.close();
			}
		    }
		    break;
     case KE_F(2):  bctx.vioIniFlags ^= __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
		    break;
     case KE_F(3):  bctx.twinIniFlags ^= TWIF_FORCEMONO ;
		    break;
     case KE_F(4):  bctx.kbdFlags ^= KBD_NONSTOP_ON_MOUSE_PRESS;
		    break;
     case KE_F(5):  bctx.vioIniFlags ^= __TVIO_FLG_USE_7BIT;
		    break;
     case KE_F(6):  bctx.iniSettingsAnywhere = bctx.iniSettingsAnywhere ? false : true;
		    break;
     case KE_F(7):  if(MMFile::has_mmio) bctx.fioUseMMF = bctx.fioUseMMF ? false : true;
		    break;
     case KE_F(8):  bctx.iniPreserveTime = bctx.iniPreserveTime ? false : true;
		    break;
     case KE_F(9):  bctx.iniUseExtProgs = bctx.iniUseExtProgs ? false : true;
		    break;
     default: continue;
   }
   paint(*wdlg);
  }
  exit:
  if(ret)
  {
    bctx.help_name=estr[0];
    bctx.skin_name=estr[1];
    bctx.syntax_name=estr[2];
  }
  delete ewnd[0];
  delete ewnd[1];
  delete ewnd[2];
  delete ewnd[3];
  delete wdlg;
}

Setup::Setup(BeyeContext& bc):bctx(bc),default_cp(15) {}
Setup::~Setup() {}
} // namespace	usr
