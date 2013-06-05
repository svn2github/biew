#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
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
#include <string>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "colorset.h"
#include "tstrings.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "udn.h"
#include "libbeye/kbd_code.h"
#include "libbeye/twindow.h"
#include "beye.h"

namespace	usr {
bool __FASTCALL__ Get2DigitDlg(const std::string& title,const std::string& text,unsigned char *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 TWindow * hwnd,*ewnd;
 bool ret;
 int retval;
 char str[3] = "";
 hwnd = CrtDlgWndnls(title,24,1);
 hwnd->get_pos(x1,y1,x2,y2);
 hwnd->goto_xy(1,1); hwnd->puts(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 X1 += 22;
 Y1 += 1;
 X2 = X1 + 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd->show();
 ewnd->set_focus();
 if(*xx) sprintf(str,"%X",(unsigned int)*xx);
 while(1)
 {
   retval = xeditstring(ewnd,str,&legalchars[2],2,NULL);
   if(retval == KE_ESCAPE || retval == KE_F(10)) { ret = false; break; }
   else
     if(retval == KE_ENTER) { ret = true; break; }
 }
 delete ewnd;
 delete hwnd;
 if(ret) *xx = (unsigned char)strtoul(str,NULL,16);
 return ret;
}

enum {
    HEX     =0x00,
    UNSIGN  =0x00,
    SIGN    =0x01,
    DECIMAL =0x02
};

bool __FASTCALL__ Get8DigitDlg(const std::string& title,const std::string& text,char attr,unsigned long *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 10 : 8;
 bool ret;
 char decleg[13];
 char str[12] = "";
 const char* legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 hwnd = CrtDlgWndnls(title,34,1);
 hwnd->get_pos(x1,y1,x2,y2);
 hwnd->goto_xy(1,1); hwnd->puts(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 33;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd->show();
 ewnd->set_focus();
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
 delete ewnd;
 delete hwnd;
 if(ret) *xx = attr & SIGN ? (unsigned long)strtol(str,NULL,base) : strtoul(str,NULL,base);
 return ret;
}

bool        __FASTCALL__ Get16DigitDlg(const std::string& title,const std::string& text,char attr,
					unsigned long long int *xx)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd;
 char base = attr & DECIMAL ? 10 : 16;
 char len  = attr & DECIMAL ? 20 : 16;
 bool ret;
 char decleg[13];
 char str[20] = "";
 const char* legals;
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 len += attr & SIGN ? 1 : 0;
 hwnd = CrtDlgWndnls(title,44,1);
 hwnd->get_pos(x1,y1,x2,y2);
 hwnd->goto_xy(1,1); hwnd->puts(text);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X2 = X1 + 43;
 X1 = X2 - (len - 1);
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd->show();
 ewnd->set_focus();
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
 delete ewnd;
 delete hwnd;
 if(ret) *xx = attr & SIGN ? (unsigned long long int)strtoll(str,NULL,base) : strtoull(str,NULL,base);
 return ret;
}

static void  __FASTCALL__ paintJumpDlg(TWindow *wdlg,unsigned long flags)
{
  wdlg->set_color(dialog_cset.group.active);
  wdlg->goto_xy(4,2); wdlg->putch(flags == GJDLG_FILE_TOP ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  wdlg->goto_xy(4,3); wdlg->putch(flags == GJDLG_RELATIVE ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  wdlg->goto_xy(4,4); wdlg->putch(flags == GJDLG_REL_EOF  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  wdlg->goto_xy(4,5); wdlg->putch(flags == GJDLG_VIRTUAL  ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
  wdlg->goto_xy(4,6); wdlg->putch(flags == GJDLG_PERCENTS ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
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

static void drawJumpPrompt()
{
   __drawSinglePrompt(jmptxt);
}


bool __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags)
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,X2,Y2;
 int key;
 TWindow * hwnd,*ewnd;
 unsigned len = HA_LEN()-1,stx = 0;
 bool ret,update;
 static char str[21] = "";
 const char * legals;
 char declegals[13];
 unsigned attr;
 hwnd = CrtDlgWndnls(" Jump within file ",beye_context().is_file64()?34:26,6);
 memcpy(declegals,legalchars,12);
 hwnd->get_pos(x1,y1,x2,y2);
 hwnd->goto_xy(2,1); hwnd->puts("Enter offset :");
 hwnd->set_color(dialog_cset.group.active);
 hwnd->goto_xy(2,2); hwnd->puts(" ( ) - From top of file ");
 hwnd->goto_xy(2,3); hwnd->puts(" ( ) - From current pos ");
 hwnd->goto_xy(2,4); hwnd->puts(" ( ) - Relatively EOF   ");
 hwnd->goto_xy(2,5); hwnd->puts(" ( ) - Virtual          ");
 hwnd->goto_xy(2,6); hwnd->puts(" ( ) - Percents         ");
 hwnd->set_color(dialog_cset.main);
 X1 = x1;
 Y1 = y1;
 X2 = x2;
 Y2 = y2;
 Y1 += 1;
 X1 += 17;
 X2 = X1 + len - 1;
 Y2 = Y1;
 ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd->show();
 ewnd->set_focus();
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
      case KE_F(1):
		    {
			Beye_Help bhelp;
			if(bhelp.open(true)) {
			    bhelp.run(6);
			    bhelp.close();
			}
		    }

		     update = false;
		     break;
      case KE_F(5):  if(beye_context()._udn().select(*addr)) {
			if(beye_context().is_file64()) sprintf(str,"%016llX",*addr);
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
 delete ewnd;
 delete hwnd;
 if(ret)
 {
 if(beye_context().is_file64())
    *addr = (*flags) == GJDLG_RELATIVE ||
	    (*flags) == GJDLG_REL_EOF ? (unsigned long long int)strtoll(str,NULL,(*flags)==GJDLG_PERCENTS?10:16):
					strtoull(str,NULL,(*flags)==GJDLG_PERCENTS?10:16);
 else
    *addr = (*flags) == GJDLG_RELATIVE ||
	    (*flags) == GJDLG_REL_EOF ? (unsigned long)strtol(str,NULL,(*flags)==GJDLG_PERCENTS?10:16):
	    strtoul(str,NULL,(*flags)==GJDLG_PERCENTS?10:16);
 }
 return ret;
}

bool __FASTCALL__ GetStringDlg(char * buff,const std::string& title,const std::string& subtitle,const std::string& prompt)
{
  tAbsCoord x1,y1,x2,y2;
  tRelCoord X1,Y1,X2,Y2;
  int key;
  bool ret;
  TWindow * wdlg,*ewnd;
  char estr[81];
  wdlg = CrtDlgWndnls(title,78,2);
  if(!subtitle.empty()) wdlg->set_footer(subtitle,TWindow::TMode_Right,dialog_cset.footer);
  wdlg->get_pos(x1,y1,x2,y2);
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;
  X1 += 2;
  X2 -= 2;
  Y1 += 2;
  Y2 = Y1;
  ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
  wdlg->goto_xy(2,1); wdlg->puts(prompt);
  strcpy(estr,buff);
  ewnd->show();
  ewnd->set_focus();
  while(1)
  {
   key = xeditstring(ewnd,estr,NULL,76,NULL);
   if(key == KE_ESCAPE || key == KE_F(10)) { ret = false; break; }
   else
     if(key == KE_ENTER) { ret = true; break; }
  }
  if(ret) strcpy(buff,estr);
  delete ewnd;
  delete wdlg;
  return ret;
}

static void  __FASTCALL__ FFStaticPaint(TWindow * wdlg,const std::string& fname,char * st,char *end,unsigned long flg)
{
  int len,i;
    if(!(flg & FSDLG_USEBITNS))
    {
      len = fname.length();
      wdlg->goto_xy(3,7); wdlg->puts(fname); for(i = len;i < 71;i++) wdlg->putch(TWC_MED_SHADE);
    }
    len = strlen(st);
    wdlg->goto_xy(8,4); wdlg->puts(st);
    for(i = len;i < 18;i++)
	wdlg->putch(TWC_MED_SHADE);
    len = strlen(end);
    wdlg->goto_xy(35,4); wdlg->puts(end);
    for(i = len;i < 18;i++)
	wdlg->putch(TWC_MED_SHADE);
    if(flg & FSDLG_USEMODES)
    {
      wdlg->set_color(dialog_cset.group.active);
      wdlg->goto_xy(54,1); wdlg->puts(msgTypeComments[0]);
      wdlg->goto_xy(61,1); wdlg->puts(flg & FSDLG_ASMMODE ? " Asm              " : " Bin              ");
      if(!(flg & FSDLG_ASMMODE)) wdlg->set_color(dialog_cset.group.disabled);
      for(i = 1;i < 5;i++) { wdlg->goto_xy(54,i + 1); wdlg->puts(msgTypeComments[i]); }
      wdlg->goto_xy(56,2); wdlg->putch(flg & FSDLG_STRUCTS ? TWC_CHECK_CHAR : TWC_DEF_FILLER);
      wdlg->goto_xy(56,4); wdlg->putch(flg & FSDLG_COMMENT ? TWC_DEF_FILLER : TWC_RADIO_CHAR);
      wdlg->goto_xy(56,5); wdlg->putch(flg & FSDLG_COMMENT ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      wdlg->set_color(dialog_cset.main);
    }
    else
    if(flg & FSDLG_USEBITNS)
    {
      wdlg->set_color(dialog_cset.group.active);
      for(i = 0;i < 4;i++) { wdlg->goto_xy(54,i + 1); wdlg->puts(msgTypeBitness[i]); }
      wdlg->goto_xy(54,5); wdlg->puts(msgTypeBitness[i]);
      wdlg->goto_xy(56,2); wdlg->putch((flg & FSDLG_BTNSMASK) == 0 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      wdlg->goto_xy(56,3); wdlg->putch((flg & FSDLG_BTNSMASK) == 1 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      wdlg->goto_xy(56,4); wdlg->putch((flg & FSDLG_BTNSMASK) == 2 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      wdlg->goto_xy(56,5); wdlg->putch((flg & FSDLG_BTNSMASK) == 3 ? TWC_RADIO_CHAR : TWC_DEF_FILLER);
      wdlg->set_color(dialog_cset.main);
    }
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

static void drawFSPrompt()
{
   __drawSinglePrompt(fs_txt);
}

bool __FASTCALL__ GetFStoreDlg(const std::string& title,char* fname,unsigned long * flags,
		   __filesize_t * start,__filesize_t * end,
		   const std::string& prompt)
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
 wdlg->set_footer(" [Enter] - Run ",TWindow::TMode_Center,dialog_cset.footer);

 wdlg->get_pos(x1,y1,x2,y2);
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
   ewnd[2] = CreateEditor(XX1,YY1,XX2,YY2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
   neditors++;
 }
 XX1 += 5;
 XX2  = XX1 + 17;
 YY1 -= 3;
 YY2  = YY1;
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 XX1 += 27;
 XX2 = XX1 + 17;
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 wdlg->goto_xy(1,2);  wdlg->puts(TYPE_HEX_FORM);
 if(!prompt.empty())  { wdlg->goto_xy(3,6);  wdlg->puts(prompt); }
 wdlg->goto_xy(1,4);  wdlg->puts(START_PRMT);
 wdlg->goto_xy(27,4); wdlg->puts(LENGTH_PRMT);
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
     ewnd[oactive]->hide();
     ewnd[active]->show();
     ewnd[active]->set_focus();
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
 delete wdlg;
 for(i = 0;i < neditors+1;i++) delete ewnd[i];
 *start = strtoul(startdig,NULL,16);
 *end = strtoul(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}

static void  __FASTCALL__ FFStaticPaintInsDel(TWindow * wdlg,const std::string& st,const std::string& end)
{
  int len,i;
    len = st.length();
    wdlg->goto_xy(8,4); wdlg->puts(st);
    for(i = len;i < 18;i++)
	wdlg->putch(TWC_MED_SHADE);
    len = end.length();
    wdlg->goto_xy(35,4); wdlg->puts(end);
    for(i = len;i < 18;i++)
	wdlg->putch(TWC_MED_SHADE);
}

bool __FASTCALL__ GetInsDelBlkDlg(const std::string& title,__filesize_t * start,__fileoff_t * size)
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

 wdlg->get_pos(x1,y1,x2,y2);
 X1 = x1;
 Y1 = y1;

 wbuff[0] = startdig;
 wbuff[1] = enddig;
 XX1 = X1 + 8;
 YY2 = YY1 = Y1 + 4;
 XX2 = XX1 + 17;
 ewnd[0] = CreateEditor(XX1,YY1,XX2,YY2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 XX1 += 27;
 XX2 = XX1 + 17;
 ewnd[1] = CreateEditor(XX1,YY1,XX2,YY2,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 wdlg->goto_xy(1,2); wdlg->puts(TYPE_HEX_FORM);
 wdlg->goto_xy(1,4);  wdlg->puts(START_PRMT);
 wdlg->goto_xy(27,4); wdlg->puts(LENGTH_PRMT);
 wdlg->draw_frame(53,1,78,5,TWindow::UP3D_FRAME,dialog_cset.main);
 wdlg->goto_xy(55,2); wdlg->puts("Remarks:             ");
 wdlg->goto_xy(55,3); wdlg->puts("+(pos) - insert block");
 wdlg->goto_xy(55,4); wdlg->puts("-(neg) - delete block");
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
     ewnd[oactive]->hide();
     ewnd[active]->show();
     ewnd[active]->set_focus();
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
 delete wdlg;
 for(i = 0;i < 2;i++) delete ewnd[i];
 *start = strtoul(startdig,NULL,16);
 *size = strtol(enddig,NULL,16);
 return !(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10));
}
} // namespace	usr

