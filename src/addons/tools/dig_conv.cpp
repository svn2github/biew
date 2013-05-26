#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_addons
 * @file        addons/tools/dig_conv.c
 * @brief       This file contains implementation of digital convertor.
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

#include "addons/addon.h"

#include "bconsole.h"
#include "beyeutil.h"
#include "colorset.h"
#include "reg_form.h"
#include "libbeye/libbeye.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace	usr {
    class DigitalConverter_Addon : public Addon {
	public:
	    DigitalConverter_Addon();
	    virtual ~DigitalConverter_Addon();
	
	    virtual void	run();
	private:
	    int			GetFullBin(uintmax_t value,char* buff);
	    int			Dig2Str(uintmax_t value,char * buff,int action);
	    uintmax_t		Str2Dig(char * buff,int action);
	    void		DCStaticPaint(TWindow * wdlg,char * wbuff,intmax_t digit,unsigned *mlen);
    };

DigitalConverter_Addon::DigitalConverter_Addon() {}
DigitalConverter_Addon::~DigitalConverter_Addon() {}

int DigitalConverter_Addon::GetFullBin(uintmax_t value,char * buff)
{
 char byte,*b;
 bool started = false;
 buff[0] = 0;
 byte = (value >> 60) & 0x0F;
 if(byte) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 56) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 52) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 48) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 44) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 40) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 36) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 32) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 28) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 24) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 20) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 16) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 12) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 8) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); started = true; }
 byte = (value >> 4) & 0x0F;
 if(byte || started) { b = GetBinary(byte); strcat(buff,&b[4]); }
 byte = value & 0x0F;
 b = GetBinary(byte); strcat(buff,&b[4]);
 return strlen(buff);
}

int DigitalConverter_Addon::Dig2Str(uintmax_t value,char * buff,int action)
{
 if(action == 0) return strlen(ulltoa(value,buff,16));
 if(action == 1) return strlen(lltoa(value,buff,8));
 if(action == 2) return strlen(ulltoa(value,buff,10));
 if(action == 3) return strlen(lltoa(value,buff,10));
 if(action == 4) return GetFullBin(value,buff);
 if(action == 5) return strlen(ulltoa(value,buff,2));
 return 0;
}

uintmax_t DigitalConverter_Addon::Str2Dig(char * buff,int action)
{
 if(action == 0) return strtoull(buff,NULL,16);
 if(action == 1) return strtoull(buff,NULL,8);
 if(action == 2) return strtoull(buff,NULL,10);
 if(action == 3) return strtoll(buff,NULL,10);
 if(action == 4) return strtoull(buff,NULL,2);
 return 0;
}

void DigitalConverter_Addon::DCStaticPaint(TWindow * wdlg,char * wbuff,intmax_t digit,unsigned *mlen)
{
 int rlen;
 tAbsCoord x1,y1,x2,y2;
 unsigned i,w;
    wdlg->get_pos(x1,y1,x2,y2);
    w=x2-x1;
    rlen = Dig2Str(digit,wbuff,0);
    wdlg->goto_xy(3,4); wdlg->puts(wbuff); for(i = rlen;i < mlen[0];i++)  wdlg->putch('±'); wdlg->puts("   [Hex]");
    rlen = Dig2Str(digit,wbuff,1);
    wdlg->goto_xy(w-11-mlen[0],4); wdlg->puts(wbuff); for(i = rlen;i < mlen[1];i++)  wdlg->putch('±'); wdlg->puts(" [Oct]");
    rlen = Dig2Str(digit,wbuff,2);
    wdlg->goto_xy(3,6); wdlg->puts(wbuff); for(i = rlen;i < mlen[2];i++)  wdlg->putch('±'); wdlg->puts(" [Dec]");
    rlen = Dig2Str(digit,wbuff,3);
    wdlg->goto_xy(w-11-mlen[0],6); wdlg->puts(wbuff); for(i = rlen;i < mlen[3];i++) wdlg->putch('±'); wdlg->puts(" [+-Dec]");
    rlen = Dig2Str(digit,wbuff,4);
    wdlg->goto_xy(5,8); wdlg->puts(wbuff); for(i = rlen;i < mlen[4];i++)  wdlg->putch('±'); wdlg->puts(" [Bin]");
}

void DigitalConverter_Addon::run()
{
 tAbsCoord x1,y1,x2,y2;
 tRelCoord X1,Y1,XX1,XX2,YY1,YY2;
 TWindow * wdlg = CrtDlgWndnls(" Digital convertor ",78,9);
 unsigned mlen[5] = { 16, 22, 19, 20, 64 };
 TWindow * ewnd[5];
 int i,active,oactive,_lastbyte;
 unsigned attr,stx = 0,rlen,w;
 char wbuff[68];
 bool redraw;
 const char * legal[5];
 uintmax_t digit;
 char decleg[13],oleg[9],bleg[3];
 memcpy(decleg,legalchars,12);
 decleg[12] = '\0';
 memcpy(oleg,&legalchars[2],8);
 oleg[8] = '0';
 memcpy(bleg,oleg,2);
 bleg[2] = '\0';
 legal[0] = &legalchars[2];
 legal[1] = oleg;
 legal[2] = &decleg[2];
 legal[3] = decleg;
 legal[4] = bleg;
 wdlg->get_pos(x1,y1,x2,y2);
 w=x2-x1;
 X1 = x1;
 Y1 = y1;
 XX1 = X1 + 3;
 YY1 = Y1 + 4;
 XX2 = XX1 + (mlen[0]-1);
 YY2 = YY1;
 ewnd[0] = new(zeromem) TWindow(XX1,YY1,XX2-XX1+1,YY2-YY1+1,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd[0]->set_color(dialog_cset.editor.active);
 XX1 = X1 + (w-11-mlen[0]);
 XX2 = XX1 + (mlen[1]-1);
 ewnd[1] = new(zeromem) TWindow(XX1,YY1,XX2-XX1+1,YY2-YY1+1,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd[1]->set_color(dialog_cset.editor.active);
 XX1 = X1 + 3;
 YY1 = Y1 + 6;
 XX2 = XX1 + (mlen[2]-1);
 YY2 = YY1;
 ewnd[2] = new(zeromem) TWindow(XX1,YY1,XX2-XX1+1,YY2-YY1+1,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd[2]->set_color(dialog_cset.editor.active);
 XX1 = X1 + (w-11-mlen[0]);
 XX2 = XX1 + (mlen[3]-1);
 ewnd[3] = new(zeromem) TWindow(XX1,YY1,XX2-XX1+1,YY2-YY1+1,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd[3]->set_color(dialog_cset.editor.active);
 XX1 = X1 + 5;
 YY1 = Y1 + 8;
 XX2 = XX1 + (mlen[4]-1);
 YY2 = YY1;
 ewnd[4] = new(zeromem) TWindow(XX1,YY1,XX2-XX1+1,YY2-YY1+1,TWindow::Flag_Has_Cursor | TWindow::Flag_NLS);
 ewnd[4]->set_color(dialog_cset.editor.active);
 digit = 0;
 wdlg->goto_xy(3,2); wdlg->puts("Convert numbers between bases [16, 10, 8, 2]");
 DCStaticPaint(wdlg,wbuff,digit,mlen);
 oactive = 1;
 active =
 wbuff[0] = 0;
 digit = 0;
 attr = __ESS_NOTUPDATELEN | __ESS_WANTRETURN | __ESS_ENABLEINSERT;
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
   if(active == 4)
   {
     rlen=0;
   }
   rlen = Dig2Str(digit,wbuff,active == 4 ? 5 : active);
   if(stx > rlen) stx = rlen;
   NextCh:
   _lastbyte = eeditstring(ewnd[active],wbuff,legal[active],&mlen[active],1,&stx,attr,
			  NULL,NULL);
   if((char)_lastbyte == '-' || (char)_lastbyte == '+') goto NextCh;
   if(_lastbyte == KE_ESCAPE || _lastbyte == KE_F(10)) break;
   digit = Str2Dig(wbuff,active);
   redraw = false;
   switch(_lastbyte)
   {
     case KE_TAB        : active++; break;
     case KE_SHIFT_TAB  : active--; break;
     case KE_LEFTARROW  :
     case KE_RIGHTARROW : break;
     case KE_UPARROW    : active -= 2; break;
     case KE_DOWNARROW  : active += 2; break;
     default: redraw = true; break;
   }
   if(active < 0) active = 4;
   if(active > 4) active = 0;
   if(redraw) DCStaticPaint(wdlg,wbuff,digit,mlen);
 }
 delete wdlg;
 for(i = 0;i < 5;i++) delete ewnd[i];
}

static Addon* query_interface() { return new(zeromem) DigitalConverter_Addon(); }
extern const Addon_Info DigitalConvertor = {
    "~Digital convertor",
    query_interface
};
} // namespace	usr
