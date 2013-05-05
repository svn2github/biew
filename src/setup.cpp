#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
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
#include "libbeye/mmfile.h"
#include "libbeye/twin.h"
#include "libbeye/kbd_code.h"

#ifdef __QNX4__
extern int photon,bit7;
#endif
namespace beye {

const char * beyeGetHelpName( void )
{
  if(!beye_context().help_name[0])
  {
    beye_context().help_name=std::string(__get_rc_dir("beye"))+"beye.hlp";
  }
  return beye_context().help_name.c_str();
}

static const char *  __FASTCALL__ beyeGetColorSetName( void )
{
  if(!beye_context().skin_name[0])
  {
    beye_context().skin_name=std::string(__get_rc_dir("beye"))+"skn/standard.skn"; /* [dBorca] in skn/ subdir */
  }
  return beye_context().skin_name.c_str();
}

static const char *  __FASTCALL__ beyeGetSyntaxName( void )
{
  if(!beye_context().syntax_name[0])
  {
    beye_context().syntax_name=std::string(__get_rc_dir("beye"))+"syntax/syntax.stx"; /* [dBorca] in syntax/ subdir */
  }
  return beye_context().syntax_name.c_str();
}


static const char * setuptxt[] =
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

static unsigned default_cp = 15;
static const char * cp_list[] =
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

static bool __FASTCALL__ select_codepage( void )
{
  unsigned nModes;
  int i;
  nModes = sizeof(cp_list)/sizeof(char *);
  i = SelBoxA(cp_list,nModes," Select single-byte codepage: ",default_cp);
  if(i != -1)
  {
    unsigned len;
    const char *p;
    default_cp = i;
    p = strchr(cp_list[i],' ');
    len = p-cp_list[i];
    beye_context().codepage=std::string(cp_list[i]).substr(0,len);
    return true;
  }
  return false;
}

static void drawSetupPrompt( void )
{
   __drawSinglePrompt(setuptxt);
}

static void  __FASTCALL__ setup_paint( TWindow *twin )
{
  TWindow *usd;
  usd = twFocusedWin();
  twFocusWin(twin);
  twSetColorAttr(twin,dialog_cset.group.active);
  twGotoXY(twin,2,9);
  twPrintF(twin," [%c] - Direct console access "
	   ,Gebool((beye_context().vioIniFlags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS) == __TVIO_FLG_DIRECT_CONSOLE_ACCESS));
  twGotoXY(twin,2,10);
  twPrintF(twin," [%c] - Mouse sensitivity     "
	   ,Gebool((beye_context().kbdFlags & KBD_NONSTOP_ON_MOUSE_PRESS) == KBD_NONSTOP_ON_MOUSE_PRESS));
  twGotoXY(twin,2,11);
  twPrintF(twin," [%c] - Force mono            "
	   ,Gebool((beye_context().twinIniFlags & TWIF_FORCEMONO) == TWIF_FORCEMONO));
  twGotoXY(twin,2,12);
#ifdef __QNX4__
  if(photon)
  {
    twSetColorAttr(twin,dialog_cset.group.disabled);
    twPrintF(twin," [%c] - Force 7-bit output    "
	    ,Gebool(bit7));
    twSetColorAttr(twin,dialog_cset.group.active);
  }
  else
#endif
  twPrintF(twin," [%c] - Force 7-bit output    "
	   ,Gebool((beye_context().vioIniFlags & __TVIO_FLG_USE_7BIT) == __TVIO_FLG_USE_7BIT));
  twGotoXY(twin,32,9);
  twPrintF(twin," [%c] - Apply plugin settings to all files     "
	   ,Gebool(beye_context().iniSettingsAnywhere));
  twGotoXY(twin,32,10);
  if(!MMFile::has_mmio) twSetColorAttr(twin,dialog_cset.group.disabled);
  twPrintF(twin," [%c] - Use MMF                                "
	   ,Gebool(beye_context().fioUseMMF));
  twSetColorAttr(twin,dialog_cset.group.active);
  twGotoXY(twin,32,11);
  twPrintF(twin," [%c] - Preserve timestamp                     "
	   ,Gebool(beye_context().iniPreserveTime));
  twGotoXY(twin,32,12);
  twPrintF(twin," [%c] - Enable usage of external programs      "
	   ,Gebool(beye_context().iniUseExtProgs));
  twSetColorAttr(twin,dialog_cset.main);
  twGotoXY(twin,50,7); twPutS(twin,beye_context().codepage.c_str());
  twFocusWin(usd);
}

void Setup(void)
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int ret;
  TWindow * wdlg,*ewnd[4];
  char estr[3][FILENAME_MAX+1];
  int active = 0;
  strcpy(estr[0],beyeGetHelpName());
  strcpy(estr[1],beyeGetColorSetName());
  strcpy(estr[2],beyeGetSyntaxName());
  wdlg = CrtDlgWndnls(" Setup ",78,13);
  twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;

  X1 += 2;
  X2 -= 1;
  Y1 += 2;
  Y2 = Y1;
  ewnd[0] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[0]);
  twFocusWin(ewnd[0]);
  PostEvent(KE_ENTER);
  xeditstring(ewnd[0],estr[0],NULL,sizeof(estr[0]), NULL);
  Y1 += 2;
  Y2 = Y1;
  ewnd[1] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[1]);
  twFocusWin(ewnd[1]);
  PostEvent(KE_ENTER);
  xeditstring(ewnd[1],estr[1],NULL,sizeof(estr[1]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[2] = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twShowWin(ewnd[2]);
  twFocusWin(ewnd[2]);
  PostEvent(KE_ENTER);
  xeditstring(ewnd[2],estr[2],NULL,sizeof(estr[2]), NULL);

  Y1 += 2;
  Y2 = Y1;
  ewnd[3] = WindowOpen(60,Y1,61,Y2,TWS_NLSOEM);

  twFocusWin(wdlg);
  twGotoXY(wdlg,2,1); twPutS(wdlg,"Enter help file name (including full path):");
  twGotoXY(wdlg,2,3); twPutS(wdlg,"Enter color skin name (including full path):");
  twGotoXY(wdlg,2,5); twPutS(wdlg,"Enter syntax name (including full path):");
  twGotoXY(wdlg,2,7); twPutS(wdlg,"Enter OEM codepage (for utf-based terminals):");
  twSetFooterAttr(wdlg," [Enter] - Accept changes ",TW_TMODE_CENTER,dialog_cset.footer);
  twinDrawFrameAttr(wdlg,1,8,78,13,TW_UP3D_FRAME,dialog_cset.main);

  setup_paint(wdlg);
  active = 0;
  twFocusWin(ewnd[active]);
  while(1)
  {
   if(active==3) {
	if(select_codepage() == true) setup_paint(wdlg);
	ret = KE_TAB;
   }
   else ret = xeditstring(ewnd[active],estr[active],NULL,sizeof(estr[active]),drawSetupPrompt);
   switch(ret)
   {
     case KE_F(10):
     case KE_ESCAPE: ret = 0; goto exit;
     case KE_ENTER: ret = 1; goto exit;
     case KE_SHIFT_TAB:
     case KE_TAB:   active++;
		    if(active>3) active=0;
		    twFocusWin(ewnd[active]);
		    continue;
     case KE_F(1):  hlpDisplay(5);
		    break;
     case KE_F(2):  beye_context().vioIniFlags ^= __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
		    break;
     case KE_F(3):  beye_context().twinIniFlags ^= TWIF_FORCEMONO ;
		    break;
     case KE_F(4):  beye_context().kbdFlags ^= KBD_NONSTOP_ON_MOUSE_PRESS;
		    break;
     case KE_F(5):  beye_context().vioIniFlags ^= __TVIO_FLG_USE_7BIT;
		    break;
     case KE_F(6):  beye_context().iniSettingsAnywhere = beye_context().iniSettingsAnywhere ? false : true;
		    break;
     case KE_F(7):  if(MMFile::has_mmio) beye_context().fioUseMMF = beye_context().fioUseMMF ? false : true;
		    break;
     case KE_F(8):  beye_context().iniPreserveTime = beye_context().iniPreserveTime ? false : true;
		    break;
     case KE_F(9):  beye_context().iniUseExtProgs = beye_context().iniUseExtProgs ? false : true;
		    break;
     default: continue;
   }
   setup_paint(wdlg);
  }
  exit:
  if(ret)
  {
    beye_context().help_name=estr[0];
    beye_context().skin_name=estr[1];
    beye_context().syntax_name=estr[2];
  }
  CloseWnd(ewnd[0]);
  CloseWnd(ewnd[1]);
  CloseWnd(ewnd[2]);
  CloseWnd(wdlg);
}
} // namespace beye
