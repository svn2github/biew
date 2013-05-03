#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        dialogs.c
 * @brief       This file contains common dialogs of BEYE project.
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
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "colorset.h"
#include "bmfile.h"
#include "tstrings.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "bin_util.h"
#include "libbeye/kbd_code.h"
#include "libbeye/twin.h"

namespace beye {
bool __FASTCALL__ Get2DigitDlg(const char *title,const char * text,unsigned char *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 TWindow * hwnd,*ewnd,*_using;
 bool ret;
 int retval;
 char str[3] = "";
 _using = twFocusedWin();
 hwnd = CrtDlgWndnls(title,24,1);
 twFocusWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(hwnd,1,1); twPutS(hwnd,text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 X1 += 22;
 Y1 += 1;
 X2 = X1 + 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(ewnd);
 if(*xx) sprintf(str,"%X",(unsigned int)*xx);
 while(1)
 {
   retval = xeditstring(ewnd,str,&legalchars[2],2,NULL);
   if(retval == KE_ESCAPE || retval == KE_F(10)) { ret = false; break; }
   else
     if(retval == KE_ENTER) { ret = true; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = (unsigned char)strtoul(str,NULL,16);
 twFocusWin(_using);
 return ret;
}

enum {
    HEX     =0x00,
    UNSIGN  =0x00,
    SIGN    =0x01,
    DECIMAL =0x02
};

bool __FASTCALL__ Get8DigitDlg(const char *title,const char *text,char attr,unsigned long *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*_using;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 10 : 8;
 bool ret;
 char decleg[13];
 char str[12] = "";
 const char* legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 _using = twFocusedWin();
 hwnd = CrtDlgWndnls(title,34,1);
 twFocusWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(hwnd,1,1); twPutS(hwnd,text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 33;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(ewnd);
 if(attr & DECIMAL) legals = attr & SIGN ? decleg : &decleg[2];
 else               legals = attr & SIGN ? legalchars : &legalchars[2];
 if(*xx)
 {
   if(attr & SIGN) ltoa(*xx,str,base);
   else            ultoa(*xx,str,base);
 }
 while(1)
 {
   key = xeditstring(ewnd,str,legals,len,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = false; break; }
   else
     if(key == KE_ENTER) { ret = true; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = attr & SIGN ? (unsigned long)strtol(str,NULL,base) : strtoul(str,NULL,base);
 twFocusWin(_using);
 return ret;
}

bool        __FASTCALL__ Get16DigitDlg(const char *title,const char *text,char attr,
					unsigned long long int *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*_using;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 20 : 16;
 bool ret;
 char decleg[13];
 char str[20] = "";
 const char* legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 _using = twFocusedWin();
 hwnd = CrtDlgWndnls(title,44,1);
 twFocusWin(hwnd);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(hwnd,1,1); twPutS(hwnd,text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 43;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(ewnd);
 if(attr & DECIMAL) legals = attr & SIGN ? decleg : &decleg[2];
 else               legals = attr & SIGN ? legalchars : &legalchars[2];
 if(*xx)
 {
   if(attr & SIGN) lltoa(*xx,str,base);
   else            ulltoa(*xx,str,base);
 }
 while(1)
 {
   key = xeditstring(ewnd,str,legals,len,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = false; break; }
   else
     if(key == KE_ENTER) { ret = true; break; }
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret) *xx = attr & SIGN ? (unsigned long long int)strtoll(str,NULL,base) : strtoull(str,NULL,base);
 twFocusWin(_using);
 return ret;
}

static void  __FASTCALL__ paintJumpDlg(TWindow *wdlg,unsigned long flags)
{
  TWindow *usd;
  usd = twFocusedWin();
  twFocusWin(wdlg);
  twSetColorAttr(wdlg,dialog_cset.group.active);
  twGotoXY(wdlg,4,2); twPutChar(wdlg,flags == GJDLG_FILE_TOP ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(wdlg,4,3); twPutChar(wdlg,flags == GJDLG_RELATIVE ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(wdlg,4,4); twPutChar(wdlg,flags == GJDLG_REL_EOF  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(wdlg,4,5); twPutChar(wdlg,flags == GJDLG_VIRTUAL  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twGotoXY(wdlg,4,6); twPutChar(wdlg,flags == GJDLG_PERCENTS ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  twFocusWin(usd);
}

static const char * jmptxt[] =
{
  "      ",
  "Mode  ",
  "      ",
  "      ",
  "UsrNam",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static void drawJumpPrompt( void )
{
   __drawSinglePrompt(jmptxt);
}


bool __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd,*_using;
 unsigned len = HA_LEN()-1,stx = 0;
 bool ret,update;
 static char str[21] = "";
 const char * legals;
 char declegals[13];
 unsigned attr;
 _using = twFocusedWin();
 hwnd = CrtDlgWndnls(" Jump within file ",is_BMUse64()?34:26,6);
 twFocusWin(hwnd);
 memcpy(declegals,legalchars,12);
 twGetWinPos(hwnd,&x1,&y1,&x2,&y2);
 twGotoXY(hwnd,2,1); twPutS(hwnd,"Enter offset :");
 twSetColorAttr(hwnd,dialog_cset.group.active);
 twGotoXY(hwnd,2,2); twPutS(hwnd," ( ) - From top of file ");
 twGotoXY(hwnd,2,3); twPutS(hwnd," ( ) - From current pos ");
 twGotoXY(hwnd,2,4); twPutS(hwnd," ( ) - Relatively EOF   ");
 twGotoXY(hwnd,2,5); twPutS(hwnd," ( ) - Virtual          ");
 twGotoXY(hwnd,2,6); twPutS(hwnd," ( ) - Percents         ");
 twSetColorAttr(hwnd,dialog_cset.main);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X1 += 17;
 X2 = X1 + len - 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(ewnd);
 legals = *flags == GJDLG_RELATIVE ? legalchars : &legalchars[2];
 paintJumpDlg(hwnd,*flags);
 update = true;
 while(1)
 {
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!update) attr |= __ESS_NOREDRAW;
   key = eeditstring(ewnd,str,legals,&len,1,&stx,
		     attr,NULL,drawJumpPrompt);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = false; break; }
   else
     if(key == KE_ENTER) { ret = true; break; }
   update = true;
   switch(key)
   {
      case KE_F(1):  hlpDisplay(6);
		     update = false;
		     break;
      case KE_F(5):  if(udnSelectName(addr)) {
			if(is_BMUse64()) sprintf(str,"%016llX",*addr);
			else		 sprintf(str,"%08lX",(unsigned long)*addr);
		     }
		     break;
      case KE_F(2):  if(((*flags)&0xFF) < GJDLG_PERCENTS) (*flags)++;
		     else                                 (*flags) = 0;
		     legals = (*flags) == GJDLG_RELATIVE ||
			      (*flags) == GJDLG_REL_EOF ? legalchars :
			      (*flags) == GJDLG_PERCENTS ? declegals : &legalchars[2];
		     update = false;
		     break;
      case KE_LEFTARROW:
      case KE_RIGHTARROW:
		     update = false;
		     break;
      default:       break;
   }
   paintJumpDlg(hwnd,*flags);
 }
 CloseWnd(ewnd);
 CloseWnd(hwnd);
 if(ret)
 {
 if(is_BMUse64())
    *addr = (*flags) == GJDLG_RELATIVE ||
	    (*flags) == GJDLG_REL_EOF ? (unsigned long long int)strtoll(str,NULL,(*flags)==GJDLG_PERCENTS?10:16):
					strtoull(str,NULL,(*flags)==GJDLG_PERCENTS?10:16);
 else
    *addr = (*flags) == GJDLG_RELATIVE ||
	    (*flags) == GJDLG_REL_EOF ? (unsigned long)strtol(str,NULL,(*flags)==GJDLG_PERCENTS?10:16):
	    strtoul(str,NULL,(*flags)==GJDLG_PERCENTS?10:16);
 }
 twFocusWin(_using);
 return ret;
}

bool __FASTCALL__ GetStringDlg(char * buff,const char * title,const char *subtitle,const char *prompt)
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int key;
  bool ret;
  TWindow * wdlg,*ewnd;
  char estr[81];
  wdlg = CrtDlgWndnls(title,78,2);
  if(subtitle) twSetFooterAttr(wdlg,subtitle,TW_TMODE_RIGHT,dialog_cset.footer);
  twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;
  X1 += 2;
  X2 -= 2;
  Y1 += 2;
  Y2 = Y1;
  ewnd = CreateEditor(X1,Y1,X2,Y2,TWS_CURSORABLE | TWS_NLSOEM);
  twFocusWin(wdlg);
  twGotoXY(wdlg,2,1); twPutS(wdlg,prompt);
  strcpy(estr,buff);
  twShowWin(ewnd);
  twFocusWin(ewnd);
  while(1)
  {
   key = xeditstring(ewnd,estr,NULL,76,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = false; break; }
   else
     if(key == KE_ENTER) { ret = true; break; }
  }
  if(ret) strcpy(buff,estr);
  CloseWnd(ewnd);
  CloseWnd(wdlg);
  return ret;
}

static void  __FASTCALL__ FFStaticPaint(TWindow * wdlg,char * fname,char * st,char *end,unsigned long flg)
{
  int len,i;
  TWindow * _using = twFocusedWin();
    twFocusWin(wdlg);
    if(!(flg & FSDLG_USEBITNS))
    {
      len = strlen(fname);
      twGotoXY(wdlg,3,7); twPutS(wdlg,fname); for(i = len;i < 71;i++) twPutChar(wdlg,TWC_MED_SHADE);
    }
    len = strlen(st);
    twGotoXY(wdlg,8,4); twPutS(wdlg,st);
    for(i = len;i < 18;i++)
	twPutChar(wdlg,TWC_MED_SHADE);
    len = strlen(end);
    twGotoXY(wdlg,35,4); twPutS(wdlg,end);
    for(i = len;i < 18;i++)
	twPutChar(wdlg,TWC_MED_SHADE);
    if(flg & FSDLG_USEMODES)
    {
      twSetColorAttr(wdlg,dialog_cset.group.active);
      twGotoXY(wdlg,54,1); twPutS(wdlg,msgTypeComments[0]);
      twGotoXY(wdlg,61,1); twPutS(wdlg,flg & FSDLG_ASMMODE ? " Asm              " : " Bin              ");
      if(!(flg & FSDLG_ASMMODE)) twSetColorAttr(wdlg,dialog_cset.group.disabled);
      for(i = 1;i < 5;i++) { twGotoXY(wdlg,54,i + 1); twPutS(wdlg,msgTypeComments[i]); }
      twGotoXY(wdlg,56,2); twPutChar(wdlg,flg & FSDLG_STRUCTS ? TWC_CHECK_CHAR : TWC_DEF_FILLER);
      twGotoXY(wdlg,56,4); twPutChar(wdlg,flg & FSDLG_COMMENT ? TWC_DEF_FILLER : TWC_RADIO_CHAR);
      twGotoXY(wdlg,56,5); twPutChar(wdlg,flg & FSDLG_COMMENT ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twSetColorAttr(wdlg,dialog_cset.main);
    }
    else
    if(flg & FSDLG_USEBITNS)
    {
      twSetColorAttr(wdlg,dialog_cset.group.active);
      for(i = 0;i < 4;i++) { twGotoXY(wdlg,54,i + 1); twPutS(wdlg,msgTypeBitness[i]); }
      twGotoXY(wdlg,54,5); twPutS(wdlg,msgTypeBitness[i]);
      twGotoXY(wdlg,56,2); twPutChar(wdlg,(flg & FSDLG_BTNSMASK) == 0 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(wdlg,56,3); twPutChar(wdlg,(flg & FSDLG_BTNSMASK) == 1 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(wdlg,56,4); twPutChar(wdlg,(flg & FSDLG_BTNSMASK) == 2 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twGotoXY(wdlg,56,5); twPutChar(wdlg,(flg & FSDLG_BTNSMASK) == 3 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      twSetColorAttr(wdlg,dialog_cset.main);
    }
    twFocusWin(_using);
}

static const char * fs1_txt[] =
{
  "      ",
  "Mode  ",
  "Coment",
  "Struct",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * fs2_txt[] =
{
  "      ",
  "Bitnes",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "      ",
  "Escape"
};

static const char * fs3_txt[] =
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

const char ** fs_txt;

static void drawFSPrompt( void )
{
   __drawSinglePrompt(fs_txt);
}

bool __FASTCALL__ GetFStoreDlg(const char *title,char * fname,unsigned long * flags,
		   __filesize_t * start,__filesize_t * end,
		   const char *prompt)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,YY1,XX2,YY2;
 TWindow *wdlg = CrtDlgWndnls(title,78,8);
 TWindow *ewnd[3];
 int i,active,oactive,_lastbyte, neditors;
 bool redraw;
 unsigned attr,stx = 0;
 char startdig[19],enddig[19];
 unsigned mlen[3] = { 19, 19, 71 };
 const char * legal[3] = { &legalchars[2], &legalchars[2], NULL };
 char *wbuff[3];

 if((*flags) & FSDLG_USEMODES) fs_txt = fs1_txt;
 else
 if((*flags) & FSDLG_USEBITNS) fs_txt = fs2_txt;
 else                          fs_txt = fs3_txt;
 twSetFooterAttr(wdlg," [Enter] - Run ",TW_TMODE_CENTER,dialog_cset.footer);

 twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
 X1 = x1;
 Y1 = y1;

 wbuff[0] = startdig;
 wbuff[1] = enddig;
 wbuff[2] = fname;

 neditors = 2;
 XX1 = X1 + 3;
 YY1 = Y1 + 7;
 XX2 = XX1 + 70;
 YY2 = YY1;
 if(!((*flags) & FSDLG_USEBITNS))
 {
   ewnd[2] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
   neditors++;
 }
 XX1 += 5;
 XX2  = XX1 + 17;
 YY1 -= 3;
 YY2  = YY1;
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 XX1 += 27;
 XX2 = XX1 + 17;
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(wdlg);
 twGotoXY(wdlg,1,2);  twPutS(wdlg,TYPE_HEX_FORM);
 if(prompt)    { twGotoXY(wdlg,3,6);  twPutS(wdlg,prompt); }
 twGotoXY(wdlg,1,4);  twPutS(wdlg,START_PRMT);
 twGotoXY(wdlg,27,4); twPutS(wdlg,LENGTH_PRMT);
 ultoa(*start,startdig,16);
 ultoa(*end,enddig,16);
 FFStaticPaint(wdlg,fname,startdig,enddig,*flags);
 active = 0;
 oactive = 1;
 redraw = true;
 neditors--;
 while(1)
 {
   if(active != oactive)
   {
     twHideWin(ewnd[oactive]);
     twShowWin(ewnd[active]);
     twFocusWin(ewnd[active]);
     oactive = active;
     stx = 0;
   }
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!redraw) attr |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(ewnd[active],wbuff[active],legal[active],&mlen[active],1,&stx,attr,NULL,drawFSPrompt);
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_ENTER || _lastbyte == KE_F(10))
									  break;
   redraw = true;
   switch(_lastbyte)
   {
     case KE_TAB         : active++; break;
     case KE_SHIFT_TAB   : active--; break;
     case KE_LEFTARROW   :
     case KE_RIGHTARROW  : redraw = false; break;
     case KE_F(2)        : if((*flags) & FSDLG_USEMODES) *flags ^= FSDLG_ASMMODE;
			   else
			    if((*flags) & FSDLG_USEBITNS)
			    {
			      unsigned long val;
			      val = ((*flags) & FSDLG_BTNSMASK);
			      val++;
			      val &= FSDLG_BTNSMASK;
			      (*flags) &= ~FSDLG_BTNSMASK;
			      (*flags) |= val;
			    }
			   break;
     case KE_F(3)        : if(((*flags) & FSDLG_USEMODES) && ((*flags) & FSDLG_ASMMODE))
							 *flags ^= FSDLG_COMMENT;
			   break;
     case KE_F(4)        : if(((*flags) & FSDLG_USEMODES) && ((*flags) & FSDLG_ASMMODE))
							 *flags ^= FSDLG_STRUCTS;
			   break;
     default:              break;
   }
   if(active < 0) active = neditors;
   if(active > neditors) active = 0;
   if(redraw) FFStaticPaint(wdlg,fname,startdig,enddig,*flags);
 }
 CloseWnd(wdlg);
 for(i = 0;i < neditors+1;i++) CloseWnd(ewnd[i]);
 *start = strtoul(startdig,NULL,16);
 *end = strtoul(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}

static void  __FASTCALL__ FFStaticPaintInsDel(TWindow * wdlg,char * st,char *end)
{
  int len,i;
  TWindow * _using = twFocusedWin();
    twFocusWin(wdlg);
    len = strlen(st);
    twGotoXY(wdlg,8,4); twPutS(wdlg,st);
    for(i = len;i < 18;i++)
	twPutChar(wdlg,TWC_MED_SHADE);
    len = strlen(end);
    twGotoXY(wdlg,35,4); twPutS(wdlg,end);
    for(i = len;i < 18;i++)
	twPutChar(wdlg,TWC_MED_SHADE);
    twFocusWin(_using);
}

bool __FASTCALL__ GetInsDelBlkDlg(const char *title,__filesize_t * start,__fileoff_t * size)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,YY1,XX2,YY2;
 TWindow * wdlg = CrtDlgWndnls(title,78,5);
 TWindow * ewnd[2];
 int i,active,oactive,_lastbyte;
 unsigned stx = 0,attr;
 bool redraw;
 char startdig[11],enddig[11];
 const char * legal[2] = { &legalchars[2], legalchars };
 unsigned mlen[2] = { 19, 19 };
 char *wbuff[2];

 twGetWinPos(wdlg,&x1,&y1,&x2,&y2);
 X1 = x1;
 Y1 = y1;

 wbuff[0] = startdig;
 wbuff[1] = enddig;
 XX1 = X1 + 8;
 YY2 = YY1 = Y1 + 4;
 XX2 = XX1 + 17;
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 XX1 += 27;
 XX2 = XX1 + 17;
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWS_CURSORABLE | TWS_NLSOEM);
 twFocusWin(wdlg);
 twGotoXY(wdlg,1,2); twPutS(wdlg,TYPE_HEX_FORM);
 twGotoXY(wdlg,1,4);  twPutS(wdlg,START_PRMT);
 twGotoXY(wdlg,27,4); twPutS(wdlg,LENGTH_PRMT);
 twinDrawFrameAttr(wdlg,53,1,78,5,TW_UP3D_FRAME,dialog_cset.main);
 twGotoXY(wdlg,55,2); twPutS(wdlg,"Remarks:             ");
 twGotoXY(wdlg,55,3); twPutS(wdlg,"+(pos) - insert block");
 twGotoXY(wdlg,55,4); twPutS(wdlg,"-(neg) - delete block");
 ultoa(*start,startdig,16);
 if(*size < 0)
 {
   ltoa(labs(*size),&enddig[1],16);
   enddig[0] = '-';
 }
 else ltoa(*size,enddig,16);
 FFStaticPaintInsDel(wdlg,startdig,enddig);
 active = 0;
 oactive = 1;
 redraw = true;
 while(1)
 {
   if(active != oactive)
   {
     twHideWin(ewnd[oactive]);
     twShowWin(ewnd[active]);
     twFocusWin(ewnd[active]);
     oactive = active;
     stx = 0;
   }
   attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
   if(!redraw) attr |= __ESS_NOREDRAW;
   _lastbyte = eeditstring(ewnd[active],wbuff[active],legal[active],&mlen[active],1,&stx,attr,NULL,NULL);
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_ENTER || _lastbyte == KE_F(10))
									 break;
   redraw = true;
   switch(_lastbyte)
   {
     case KE_TAB        : active++; break;
     case KE_SHIFT_TAB  : active--; break;
     case KE_LEFTARROW  :
     case KE_RIGHTARROW : redraw = false; break;
     default: break;
   }
   if(active != oactive) active = active > 1 ? 0 : 1;
   if(redraw) FFStaticPaintInsDel(wdlg,startdig,enddig);
 }
 CloseWnd(wdlg);
 for(i = 0;i < 2;i++) CloseWnd(ewnd[i]);
 *start = strtoul(startdig,NULL,16);
 *size = strtol(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}
} // namespace beye

