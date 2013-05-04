#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        info_win.c
 * @brief       This file contains information interface of BEYE project.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added ASSEMBle option to casmtext
**/
#include <stdio.h>
#include <string.h>

#include "beye.h"
#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "reg_form.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"
#include "plugins/plugin.h"

namespace beye {
static void  __FASTCALL__ ShowFunKey(TWindow* w,const char * key,const char * text)
{
 twSetColorAttr(w,prompt_cset.digit);
 twPutS(w,key);
 twSetColorAttr(w,prompt_cset.button);
 twPutS(w,text);
}

static const char * ftext[] = { "1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"," 9","10" };

static void  __FASTCALL__ drawControlKeys(TWindow* w,int flg)
{
  char ckey;
  if(flg & KS_SHIFT) ckey = 'S';
  else
    if(flg & KS_ALT) ckey = 'A';
    else
      if(flg & KS_CTRL) ckey = 'C';
      else              ckey = ' ';
  twGotoXY(w,1,1);
  twSetColorAttr(w,prompt_cset.control);
  twPutChar(w,ckey);
}

void __FASTCALL__ __drawMultiPrompt(const char * const norm[], const char *const shift[], const char * const alt[], const char * const ctrl[])
{
  TWindow *_using;
  int flg = __kbdGetShiftsKey();
  int i;
  const char * cptr;
  _using = twFocusedWin();
  twFocusWin(HelpWnd);
  twFreezeWin(HelpWnd);
  twGotoXY(HelpWnd,2,1);
  for(i = 0;i < 10;i++)
  {
    /* todo: it might be better to ensure that if
       text!=NULL then text[i]!=NULL, rather than
       checking it all the time
     */
    if (flg & KS_SHIFT)
	cptr = shift && shift[i] ? shift[i] : "      ";
    else if (flg & KS_ALT)
	cptr = alt && alt[i] ? alt[i] : "      ";
    else if (flg & KS_CTRL)
	cptr = ctrl && ctrl[i] ? ctrl[i] : "      ";
    else cptr = norm && norm[i] ? norm[i] : "      ";

    ShowFunKey(HelpWnd,ftext[i],cptr);
  }
  drawControlKeys(HelpWnd,flg);
  twRefreshWin(HelpWnd);
  twFocusWin(_using);
}

void __FASTCALL__ __drawSinglePrompt(const char *prmt[])
{
  __drawMultiPrompt(prmt, NULL, NULL, NULL);
}


static const char * ShiftFxText[] =
{
  "ModHlp",
  "      ",
  "      ",
  "      ",
  "Where ",
  "SysInf",
  "NextSr",
  "Tools ",
  "      ",
  "FilUtl"
};

static const char * FxText[] =
{
  "Intro ",
  "ViMode",
  "More  ",
  "      ",
  "Goto  ",
  "ReRead",
  "Search",
  "      ",
  "Setup ",
  "Quit  "
};

static void  fillFxText( void )
{
  FxText[3] = beye_context().active_mode().misckey_name();
  FxText[7] = "Header";
}

void drawPrompt( void )
{
    fillFxText();
    const char* prmt[10];
    const char* fprmt[10];
    size_t i;
    for(i=0;i<10;i++) prmt[i]=beye_context().active_mode().prompt(i);
    for(i=0;i<10;i++) fprmt[i]=beye_context().bin_format().prompt(i);
    __drawMultiPrompt(FxText, ShiftFxText, fprmt, prmt);
}

static const char * amenu_names[] =
{
   "~Base",
   "~Alternative",
   "~Format-depended",
   "~Mode-depended"
};

int MainActionFromMenu( void )
{
    const char* prmt[10];
    size_t j;
  unsigned nModes;
  int i;
  nModes = sizeof(amenu_names)/sizeof(char *);
  i = SelBoxA(amenu_names,nModes," Select action: ",0);
  if(i != -1)
  {
    switch(i)
    {
	default:
	case 0:
		fillFxText();
		i = SelBoxA(FxText,10," Select base action: ",0);
		if(i!=-1) return KE_F(i+1);
		break;
	case 1:
		i = SelBoxA(ShiftFxText,10," Select alternative action: ",0);
		if(i!=-1) return KE_SHIFT_F(i+1);
		break;
	case 2:
		for(j=0;j<10;j++) prmt[j]=beye_context().bin_format().prompt(i);
		i = SelBoxA(prmt,10," Select format-depended action: ",0);
		if(i!=-1) return KE_ALT_F(i+1);
		break;
	case 3:
		for(j=0;j<10;j++) prmt[j]=beye_context().active_mode().prompt(i);
		i = SelBoxA(prmt,10," Select mode-depended action: ",0);
		if(i!=-1) return KE_CTL_F(i+1);
		break;
    }
  }
  return 0;
}

static const char * fetext[] =
{
  "Help  ",
  "Update",
  "InitXX",
  "Not   ",
  "Or  XX",
  "And XX",
  "Xor XX",
  "Put XX",
  "Undo  ",
  "Escape"
};

static const char * casmtext[] =
{
  "AsmRef",
  "SysInf",
  "Tools ",
  "Assemb",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      "
};

static const char * empttext[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};
static const char * emptlsttext[] =
{
  "      ",
  "      ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      "
};

void drawEditPrompt( void )
{
  __drawSinglePrompt(fetext);
}

/*
int EditActionFromMenu( void )
{
  int i;
  i = SelBoxA(fetext,10," Select editor's action: ",0);
  if(i != -1) return KE_F(i+1);
  return 0;
}
*/

void drawEmptyPrompt( void )
{
  __drawSinglePrompt(empttext);
}

void drawEmptyListPrompt( void )
{
  __drawSinglePrompt(emptlsttext);
}

void drawAsmEdPrompt( void )
{
  __drawMultiPrompt(fetext, NULL, NULL, casmtext);
}

int EditAsmActionFromMenu( void )
{
  int i;
  i = SelBoxA(amenu_names,2," Select asm editor's action: ",0);
  if(i != -1)
  {
    switch(i)
    {
	default:
	case 0:
		fillFxText();
		i = SelBoxA(fetext,10," Select base action: ",0);
		if(i!=-1) return KE_F(i+1);
		break;
	case 1:
		i = SelBoxA(casmtext,10," Select alternative action: ",0);
		if(i!=-1) return KE_CTL_F(i+1);
		break;
    }
  }
  return 0;
}

static const char * ordlisttxt[] =
{
  "      ",
  "SrtNam",
  "SrtOrd",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * listtxt[] =
{
  "      ",
  "Sort  ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * searchlisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "SaveAs",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

static const char * shlisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "NextSr",
  "      ",
  "      ",
  "      "
};

static const char * helptxt[] =
{
  "Licenc",
  "KeyHlp",
  "Credit",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * helplisttxt[] =
{
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Search",
  "      ",
  "      ",
  "Escape"
};

void drawListPrompt( void )
{
  __drawMultiPrompt(listtxt, shlisttxt, NULL, NULL);
}

void drawOrdListPrompt( void )
{
  __drawMultiPrompt(ordlisttxt, shlisttxt, NULL, NULL);
}

void drawSearchListPrompt( void )
{
  __drawMultiPrompt(searchlisttxt, shlisttxt, NULL, NULL);
}

void drawHelpPrompt( void )
{
  __drawSinglePrompt(helptxt);
}

int HelpActionFromMenu( void )
{
  int i;
  i = SelBoxA(helptxt,10," Select help action: ",0);
  if(i != -1) return KE_F(i+1);
  return 0;
}

void drawHelpListPrompt( void )
{
  __drawMultiPrompt(helplisttxt, shlisttxt, NULL, NULL);
}

typedef struct tagvbyte
{
  char x;
  char y;
  char image;
}vbyte;

typedef struct tagcvbyte
{
  char x;
  char y;
  char image;
  Color color;
}cvbyte;

const vbyte stars[] = {
{ 51,8,'.' },
{ 48,6,'*' },
{ 50,11,'*' },
{ 49,9,'.' },
{ 69,6,'*' },
{ 72,11,'*' },
{ 68,10,'.' },
{ 71,8,'.' },
{ 70,12,'*' },
{ 48,12,'.' }
};

const cvbyte buttons[] = {
{ 65,12,0x07, LightRed },
{ 66,12,0x07, LightGreen },
{ 67,12,0x07, LightCyan },
{ 55,12,TWC_SH, Black },
{ 56,12,TWC_SH, Black }
};

void About( void )
{
 TWindow * hwnd;
 unsigned i,j,len;
 char str[2];
 const unsigned char core[8] = { TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, TWC_LT_SHADE, 0x00 };
 hwnd = WindowOpen(0,0,73,13,TWS_FRAMEABLE | TWS_NLSOEM);
 twCentredWin(hwnd,NULL);
 twSetColor(hwnd,LightCyan,Black);
 twClearWin(hwnd);
 twSetFrame(hwnd,TW_DOUBLE_FRAME,White,Black);
 twSetTitle(hwnd,BEYE_VER_MSG,TW_TMODE_CENTER,White,Black);
 twShowWin(hwnd);

 twFocusWin(hwnd);
 twFreezeWin(hwnd);
 twGotoXY(hwnd,1,1); twPutS(hwnd,msgAboutText);
 twTextColor(hwnd,White);
 for(i = 0;i < 13;i++)  { twGotoXY(hwnd,47,i + 1); twPutChar(hwnd,TWC_SV); }
 for(i = 0;i < 47;i++) { twGotoXY(hwnd,i + 1,6); twPutChar(hwnd,TWC_SH);  }
 twGotoXY(hwnd,47,6); twPutChar(hwnd,TWC_SV_Sl);
 for(i = 0;i < 5;i++)
 {
   len=strlen(BeyeLogo[i]);
   for(j=0;j<len;j++) {
    twTextColor(hwnd,BeyeLogo[i][j]=='0'?Green:LightGreen);
    twGotoXY(hwnd,49+j,i + 1);
    twPutChar(hwnd,BeyeLogo[i][j]);
   }
 }
 twTextColor(hwnd,LightGreen); twTextBkGnd(hwnd,Green);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(hwnd,1,i+7); twPutS(hwnd,MBoardPicture[i]);
 }
 twinDrawFrame(hwnd,3,8,13,12,TW_UP3D_FRAME,White,LightGray);
 twinDrawFrame(hwnd,4,9,12,11,TW_DN3D_FRAME,Black,LightGray);
 twGotoXY(hwnd,5,10);
 twPutS(hwnd,(const char*)core);
 twTextColor(hwnd,Brown); twTextBkGnd(hwnd,Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(hwnd,17,i+7); twPutS(hwnd,ConnectorPicture[i]);
 }
 twTextColor(hwnd,Gray); twTextBkGnd(hwnd,Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(hwnd,22,i+7); twPutS(hwnd,BitStreamPicture[i]);
 }
 twTextColor(hwnd,LightGray); twTextBkGnd(hwnd,Black);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(hwnd,31,i+7); twPutS(hwnd,BeyePicture[i]);
 }
 twTextColor(hwnd,LightCyan); twTextBkGnd(hwnd,Blue);
 for(i = 0;i < 5;i++)
 {
   twGotoXY(hwnd,32,i+8); twPutS(hwnd,BeyeScreenPicture[i]);
 }
 twTextBkGnd(hwnd,Black);   twTextColor(hwnd,LightCyan);
 for(i = 0;i < 10;i++) twDirectWrite(hwnd,stars[i].x,stars[i].y,&stars[i].image,1);
 twTextColor(hwnd,LightGray);
 for(i = 0;i < 7;i++)
 {
   twGotoXY(hwnd,52,i + 6);
   twPutS(hwnd,CompPicture[i]);
 }
 twTextBkGnd(hwnd,LightGray);
 for(i = 0;i < 5;i++)
 {
   twTextColor(hwnd,buttons[i].color);
   str[0] = buttons[i].image;
   str[1] = 0;
   twDirectWrite(hwnd,buttons[i].x,buttons[i].y,str,1);
 }
 twSetColor(hwnd,LightCyan,Black);
 for(i = 0;i < 4;i++)
 {
   twGotoXY(hwnd,54,i + 7);
   twPutS(hwnd,CompScreenPicture[i]);
 }
 twRefreshWin(hwnd);
 while(1)
 {
   int ch;
   ch = GetEvent(drawHelpPrompt,HelpActionFromMenu,hwnd);
   switch(ch)
   {
      case KE_ESCAPE:
      case KE_F(10):  goto bye_help;
      case KE_F(1):   hlpDisplay(1); break;
      case KE_F(2):   hlpDisplay(3); break;
      case KE_F(3):   hlpDisplay(4); break;
      default:        break;
   }
 }
 bye_help:
 CloseWnd(hwnd);
}

__filesize_t __FASTCALL__ WhereAMI(__filesize_t ctrl_pos)
{
  TWindow *hwnd,*wait_wnd;
  char vaddr[64],prev_func[61],next_func[61],oname[25];
  const char *btn;
  int obj_class,obj_bitness;
  unsigned obj_num,func_class;
  __filesize_t obj_start,obj_end;
  __filesize_t cfpos,ret_addr,va,prev_func_pa,next_func_pa;
  hwnd = CrtDlgWndnls(" Current position information ",78,5);
  twSetFooterAttr(hwnd,"[Enter] - Prev. entry [Ctrl-Enter | F5] - Next entry]",TW_TMODE_RIGHT,dialog_cset.selfooter);
  twGotoXY(hwnd,1,1);
  wait_wnd = PleaseWaitWnd();
  cfpos = BMGetCurrFilePos();
  va = beye_context().bin_format().pa2va(ctrl_pos);
  if(va==Bin_Format::Bad_Address) va = ctrl_pos;
  vaddr[0] = '\0';
  sprintf(&vaddr[strlen(vaddr)],"%016llXH",va);
  prev_func_pa = next_func_pa = 0;
  prev_func[0] = next_func[0] = '\0';
  prev_func_pa = beye_context().bin_format().get_public_symbol(prev_func,sizeof(prev_func),
					      &func_class,ctrl_pos,true);
  next_func_pa = beye_context().bin_format().get_public_symbol(next_func,sizeof(next_func),
					      &func_class,ctrl_pos,false);
  prev_func[sizeof(prev_func)-1] = next_func[sizeof(next_func)-1] = '\0';
  obj_num = beye_context().bin_format().get_object_attribute(ctrl_pos,oname,sizeof(oname),
					  &obj_start,&obj_end,&obj_class,
					  &obj_bitness);
  oname[sizeof(oname)-1] = 0;
  if(!obj_num)
  {
    obj_num = 0;
    oname[0] = 0;
    obj_start = 0;
    obj_end = BMGetFLength();
    obj_class = OC_CODE;
    obj_bitness = DAB_USE16;
  }
  CloseWnd(wait_wnd);
  switch(obj_bitness)
  {
    case DAB_USE16: btn = "USE16"; break;
    case DAB_USE32: btn = "USE32"; break;
    case DAB_USE64: btn = "USE64"; break;
    case DAB_USE128:btn = "USE128"; break;
    case DAB_USE256:btn = "USE256"; break;
    default: btn = "";
  }
  twPrintF(hwnd,
	   "File  offset : %016llXH\n"
	   "Virt. address: %s\n"
	   "%s entry  : %s\n"
	   "Next  entry  : %s\n"
	   "Curr. object : #%u %s %s %016llXH=%016llXH %s"
	   ,ctrl_pos
	   ,vaddr
	   ,prev_func_pa == ctrl_pos ? "Curr." : "Prev."
	   ,prev_func
	   ,next_func
	   ,obj_num
	   ,obj_class == OC_CODE ? "CODE" : obj_class == OC_DATA ? "DATA" : "no obj."
	   ,btn
	   ,obj_start
	   ,obj_end
	   ,oname
	   );
  ret_addr = ctrl_pos;
  while(1)
  {
    int ch;
    ch = GetEvent(drawEmptyPrompt,NULL,hwnd);
    switch(ch)
    {
      case KE_F(10):
      case KE_ESCAPE: goto exit;
      case KE_ENTER:
		    {
		      if(prev_func_pa)
		      {
			ret_addr = prev_func_pa;
		      }
		      else ErrMessageBox(NOT_ENTRY,NULL);
		    }
		    goto exit;
      case KE_F(5):
      case KE_CTL_ENTER:
		    {
		      if(next_func_pa)
		      {
			ret_addr = next_func_pa;
		      }
		      else ErrMessageBox(NOT_ENTRY,NULL);
		    }
		    goto exit;
      default: break;
    }
  }
  exit:
  BMSeek(cfpos,BFile::Seek_Set);
  CloseWnd(hwnd);
  return ret_addr;
}
} // namespace beye
