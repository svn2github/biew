#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        mainloop.c
 * @brief       This file is analog of message loop routine.
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

#include "beye.h"
#include "colorset.h"
#include "tstrings.h"
#include "reg_form.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "codeguid.h"
#include "search.h"
#include "setup.h"
#include "libbeye/kbd_code.h"
#include "libbeye/osdep/system.h"
#include "libbeye/osdep/tconsole.h"
#include "plugins/plugin.h"

namespace	usr {
__filesize_t lastbyte;
static __filesize_t OldCurrFilePos; /** means previous File position */
unsigned long CurrStrLen = 0;
unsigned long PrevStrLen = 2;
unsigned long CurrPageSize = 0;
unsigned long PrevPageSize = 0;

int textshift = 0;

int __FASTCALL__ isHOnLine(__filesize_t cp,int width)
{
  if(FoundTextSt == FoundTextEnd) return 0;
  return (FoundTextSt >= cp && FoundTextSt < cp + width)
	  || (FoundTextEnd > cp && FoundTextEnd < cp + width)
	  || (FoundTextSt <= cp && FoundTextEnd >= cp + width);
}

void __FASTCALL__ HiLightSearch(TWindow& out,__filesize_t cfp,tRelCoord minx,tRelCoord maxx,tRelCoord y,HLInfo *buff,unsigned flags)
{
 tvioBuff it;
 unsigned __len,width;
 int x;
 char attr;
 t_vchar chars[__TVIO_MAXSCREENWIDTH];
 t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
 ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
 it.chars = chars;
 it.oem_pg = oem_pg;
 it.attrs = attrs;
 width = (flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH ? maxx*2 : maxx-minx;
 attr = browser_cset.highline;
 if((flags & HLS_USE_BUFFER_AS_VIDEO) == HLS_USE_BUFFER_AS_VIDEO)
 {
   memcpy(chars,buff->buff.chars,width);
   memcpy(oem_pg,buff->buff.oem_pg,width);
   memset(attrs,attr,width);
 }
 else
 {
   memcpy(chars,buff->text,width);
   memset(oem_pg,0,width);
   memset(attrs,attr,width);
   beye_context().system().nls_prepare_oem_for_vio(&it,width);
 }
 x = (int)(FoundTextSt - cfp);
 if((flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH) x *= 2;
 __len = (unsigned)(FoundTextEnd - FoundTextSt);
 if((flags & HLS_USE_DOUBLE_WIDTH) == HLS_USE_DOUBLE_WIDTH) __len *= 2;
 if(__len > width - x) __len = width - x;
 if(x < 0) { __len += x; x = 0; }
 if(__len && x + __len <= width)
 {
   unsigned char end,st;
   st = x;
   end = (__len + x);
   attr = browser_cset.hlight;
   memset(&attrs[st],attr,end-st);
 }
 out.write(minx + 1,y + 1,&it,width);
}

void BeyeContext::draw_title() const
{
  unsigned percent;
  __filesize_t flen;
  flen = beye_context().flength();
  percent = flen ? (unsigned)(( lastbyte*100 )/flen) : 100;
  if(percent > 100) percent = 100;
  title_wnd().goto_xy(title_wnd().client_width()-4,1);
  title_wnd().printf("%u%%",percent);
  title_wnd().clreol();
}

const char legalchars[] = "+-0123456789ABCDEFabcdef";

void BeyeContext::main_loop()
{
    int ch;
    __filesize_t savep = 0,cfp,nfp,flen;
    unsigned long lwidth;
    bm_file().seek(LastOffset,binary_stream::Seek_Set);
    drawPrompt();
    textshift = active_mode().paint(KE_SUPERKEY,textshift);
    bm_file().seek(LastOffset,binary_stream::Seek_Set);
    draw_title();
    while(1) {
	unsigned che;
	ch = GetEvent(drawPrompt,MainActionFromMenu,NULL);
	nfp = cfp = OldCurrFilePos = tell();
	flen = flength();
	lwidth = active_mode().curr_line_width();
	che = ch & 0x00FF;
	if(((che >= '0' && che <= '9') ||
	    (che >= 'A' && che <= 'Z') ||
	    (che >= 'a' && che <= 'z') ||
	    ch == KE_BKSPACE) &&
	    (active_mode().flags() & Plugin::UseCodeGuide) == Plugin::UseCodeGuide) {
		nfp = codeguider().get_go_address(ch);
		goto GO;
        }
	switch(ch) {
	    case KE_CTL_F(1): if(active_mode().action_F1()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(2): if(active_mode().action_F2()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(3): if(active_mode().action_F3()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(4): if(active_mode().action_F4()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(5): if(active_mode().action_F5()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(6): if(active_mode().action_F6()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(7): if(active_mode().action_F7()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(8): if(active_mode().action_F8()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(9): if(active_mode().action_F9()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_CTL_F(10):if(active_mode().action_F10()) { ch = KE_SUPERKEY; drawPrompt(); } break;
	    case KE_ALT_F(1): nfp=bin_format().action_F1(); break;
	    case KE_ALT_F(2): nfp=bin_format().action_F2(); break;
	    case KE_ALT_F(3): nfp=bin_format().action_F3(); break;
	    case KE_ALT_F(4): nfp=bin_format().action_F4(); break;
	    case KE_ALT_F(5): nfp=bin_format().action_F5(); break;
	    case KE_ALT_F(6): nfp=bin_format().action_F6(); break;
	    case KE_ALT_F(7): nfp=bin_format().action_F7(); break;
	    case KE_ALT_F(8): nfp=bin_format().action_F8(); break;
	    case KE_ALT_F(9): nfp=bin_format().action_F9(); break;
	    case KE_ALT_F(10):nfp=bin_format().action_F10(); break;
	    case KE_SUPERKEY: goto DRAW;
	    case KE_F(1) : About();  continue;
	    default : continue;
	    case KE_SHIFT_F(1): active_mode().help(); break;
	    case KE_F(10):
	    case KE_ESCAPE : return;
	    case KE_ENTER:
		quick_select_mode();
		drawPrompt();
		ch = KE_SUPERKEY;
		break;
	    case KE_F(2):
		if(select_mode()) ch = KE_SUPERKEY; break;
	    case KE_F(3):
		if(new_source()) {
		    ch = KE_SUPERKEY;
		    FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
		    PaintTitle();
		}
		break;
	    case KE_F(4):
		__filesize_t sfp;
		sfp = tell();
		active_mode().misckey_action();
		ch = KE_SUPERKEY;
		PaintTitle();
		drawPrompt();
		bm_file().seek(sfp,binary_stream::Seek_Set);
		break;
	    case KE_F(5):
		{
		    static __filesize_t shift = 0;
		    static unsigned long flags = GJDLG_FILE_TOP;
		    if(GetJumpDlg(&shift,&flags)) {
		    switch(flags&0xFF) {
			default:
			case GJDLG_PERCENTS:
				shift=shift>100?100:shift;
				nfp = flength()*shift/100;
				break;
			case GJDLG_FILE_TOP:
				nfp = shift;
				break;
			case GJDLG_RELATIVE:
				nfp += (long)shift;
				break;
			case GJDLG_REL_EOF:
				nfp = flength()+(long)shift;
				break;
			case GJDLG_VIRTUAL:
				__filesize_t temp_fp;
				nfp = shift;
				temp_fp = bin_format().va2pa(shift);
				if(temp_fp==Plugin::Bad_Address) ErrMessageBox(NOT_ENTRY,"");
				else nfp = temp_fp;
				break;
		    }
		    if((active_mode().flags() & Plugin::UseCodeGuide) == Plugin::UseCodeGuide)
								 codeguider().add_back_address();
		    ch = KE_SUPERKEY;
		    }
	    }
	    break;
	    case KE_SHIFT_F(5): nfp = WhereAMI(nfp); break;
	    case KE_F(6):
		bm_file().reread();
		FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
		PaintTitle();
		break;
	    case KE_SHIFT_F(6): select_sysinfo(); break;
	    case KE_F(7): nfp = search(false); ch = KE_JUSTFIND; break;
	    case KE_SHIFT_F(7): nfp = search(true); ch = KE_JUSTFIND; break;
	    case KE_F(8):
		    nfp = bin_format().show_header();
		    break;
	    case KE_SHIFT_F(8): select_tool(); break;
	    case KE_F(9): {
			class Setup* setup = new class Setup;
			setup->run();
			delete setup;
		    }
		    break;
	    case KE_SHIFT_F(10):
		if(FileUtils()) {
		    FoundTextSt = FoundTextEnd; ch = KE_SUPERKEY;
		    PaintTitle();
		}
		break;
	    case KE_HOME: textshift = 0; break;
	    case KE_END:  textshift = active_mode().get_max_line_length() - tconsole().vio_width()/2; break;
	    case KE_UPARROW:
		nfp = cfp - active_mode().prev_line_width();
		break;
	    case KE_DOWNARROW:
		nfp = cfp + active_mode().curr_line_width();
		break;
	    case KE_RIGHTARROW:
		if((active_mode().flags() & Plugin::Text) == Plugin::Text)
		    textshift+=active_mode().get_symbol_size();
		else nfp = cfp + active_mode().get_symbol_size();
		break;
	    case KE_LEFTARROW:
		if((active_mode().flags() & Plugin::Text) == Plugin::Text)
		    textshift-=active_mode().get_symbol_size();
		else nfp = cfp - active_mode().get_symbol_size();
		if(textshift < 0) textshift = 0;
		break;
	    case KE_CTL_RIGHTARROW:
		if((active_mode().flags() & Plugin::Text) == Plugin::Text)
		    textshift+=8*active_mode().get_symbol_size();
		else nfp = cfp + 8*active_mode().get_symbol_size();
		break;
	    case KE_CTL_LEFTARROW:
		if((active_mode().flags() & Plugin::Text) == Plugin::Text)
		    textshift-=8*active_mode().get_symbol_size();
		else nfp = cfp - 8*active_mode().get_symbol_size();
		if(textshift < 0) textshift = 0;
		break;
	    case KE_PGUP:
		nfp = cfp - active_mode().prev_page_size();
		break;
	    case KE_PGDN:
		nfp = cfp + active_mode().curr_page_size();
		break;
	    case KE_CTL_PGUP:
		nfp = 0;
		break;
	    case KE_CTL_PGDN:
		nfp = flen;
		break;
	    case KE_CTL_(O): /** User screen */
		{
		    unsigned evt;
		    main_wnd().hide();
		    title_wnd().hide();
		    do {
			evt = GetEvent(drawEmptyPrompt,NULL,NULL);
		    }
		    while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_CTL_(O)));
		    main_wnd().show();
		    title_wnd().show();
		}
		continue;
	}
	GO:
	if(cfp != nfp && nfp != Plugin::Bad_Address) {
	    unsigned long twidth = ( active_mode().flags() & Plugin::Text ) == Plugin::Text ?
			   active_mode().get_symbol_size() :
			   ( active_mode().flags() & Plugin::Disasm ) == Plugin::Disasm ?
			   1 : lwidth;
	    __filesize_t p = flen - twidth;
	    if((__fileoff_t)nfp < 0) nfp = 0;
	    if(nfp > 0) if(nfp > p) nfp = p;
	}
	bm_file().seek(nfp,binary_stream::Seek_Set);
	DRAW:
	if((active_mode().flags() & Plugin::Text) != Plugin::Text) savep = tell();
	textshift = active_mode().paint(ch,textshift);
	if((active_mode().flags() & Plugin::Text) != Plugin::Text) bm_file().seek(savep,binary_stream::Seek_Set);
	draw_title();
    }
}
} // namespace	usr

