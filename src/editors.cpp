#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        editors.c
 * @brief       This file contains low level editor implementation of BEYE project.
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
#include <algorithm>

#include <string.h>
#include <errno.h>

#include "colorset.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "editor.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/kbd_code.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
void Editor::show_help() const
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(2);
	bhelp.close();
    }
}

void Editor::paint_title( int shift,bool use_shift ) const
{
    unsigned eidx;
    char byte,obyte;
    TWindow& twnd = bctx.title_wnd();
    twnd.freeze();
    twnd.goto_xy(1,1);
    twnd.clreol();
    twnd.printf("%08lX: ",edit_cp + shift);
    eidx = use_shift ? (unsigned)shift : edit_y*EditorMem.width+edit_x;
    byte  = EditorMem.buff[eidx];
    obyte = EditorMem.save[eidx];
    if(byte != obyte) twnd.set_color(title_cset.change);
    twnd.printf("%c %02XH %sH %sB "
	   ,byte ? byte : ' '
	   ,byte & 0x00FF
	   ,Get2SignDig(byte)
	   ,GetBinary(byte).c_str());
    twnd.set_color(title_cset.main);
    if(byte != obyte) {
	twnd.printf("ORIGINAL: %c %02XH %sH %sB "
	     ,obyte ? obyte : ' '
	     ,obyte & 0x00FF
	     ,Get2SignDig(obyte)
	     ,GetBinary(obyte).c_str());
    } else twnd.printf("                                ");
    twnd.printf("MASK: %sH"
	   ,Get2Digit(edit_XX));
    twnd.refresh();
}

void Editor::init(unsigned width,const unsigned char *buff,unsigned size)
{
    __filesize_t flen,cfp,ssize;
    unsigned i,msize;
    msize = ewnd.width()*ewnd.height();
    EditorMem.buff = new unsigned char [msize];
    EditorMem.save = new unsigned char [msize];
    EditorMem.alen = new unsigned char [ewnd.height()];

    memset(EditorMem.buff,TWC_DEF_FILLER,msize);
    memset(EditorMem.save,TWC_DEF_FILLER,msize);
    flen = bctx.flength();
    edit_cp = cfp = bctx.tell();
    EditorMem.width = width;
    if(buff) {
	EditorMem.size = size;
	memcpy(EditorMem.buff,buff,size);
    } else {
	EditorMem.size = (unsigned)((__filesize_t)msize > (flen-cfp) ? (flen-cfp) : msize);
	bctx.bm_file().seek(cfp,binary_stream::Seek_Set);
	bctx.bm_file().read(EditorMem.buff,EditorMem.size);
	bctx.bm_file().seek(cfp,binary_stream::Seek_Set);
    }
    memcpy(EditorMem.save,EditorMem.buff,EditorMem.size);
    /** initialize EditorMem.alen */
    ssize = flen-cfp;
    for(i = 0;i < ewnd.height();i++) {
	EditorMem.alen[i] = ssize >= width ? width : ssize;
	ssize -= std::min(ssize,__filesize_t(width));
    }
}

Editor::Editor(BeyeContext& bc,TWindow& w,unsigned width)
	:bctx(bc)
	,edit_cp(0)
	,edit_XX(0)
	,ewnd(w)
{ init(width,NULL,0); }
Editor::Editor(BeyeContext& bc,TWindow& w,unsigned width,const unsigned char *buff,unsigned size)
	:bctx(bc)
	,edit_cp(0)
	,edit_XX(0)
	,ewnd(w)
{ init(width,buff,size); }

Editor::~Editor() {
    delete EditorMem.buff;
    delete EditorMem.save;
    delete EditorMem.alen;
}

void Editor::goto_xy(unsigned x,unsigned y) {
    edit_x=x;
    edit_y=y;
}

unsigned Editor::where_x() const { return edit_x; }
unsigned Editor::where_y() const { return edit_y; }
const editor_mem& Editor::get_mem() const { return EditorMem; }
editor_mem& Editor::get_mem() { return EditorMem; }
uint8_t Editor::get_template() const { return edit_XX; }

void Editor::CheckBounds()
{
  tAbsCoord height = bctx.main_wnd().client_height();
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  if(!EditorMem.alen[edit_y]) edit_y--;
  if(edit_x >= EditorMem.alen[edit_y]) edit_x = EditorMem.alen[edit_y] - 1;
}

void Editor::CheckYBounds()
{
  tAbsCoord height = bctx.main_wnd().client_height();
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  while(!EditorMem.alen[edit_y]) edit_y--;
}

void Editor::CheckXYBounds()
{
   CheckYBounds();
   if(edit_x < 0) edit_x = EditorMem.alen[--edit_y]*2;
   if(edit_x >= EditorMem.alen[edit_y]*2) { edit_x = 0; edit_y++; }
   CheckYBounds();
}

void Editor::save_context()
{
  std::ofstream fs;
  std::string fname;
  fname = bctx.bm_file().filename();
  fs.open(fname.c_str(),std::ios_base::binary);
  if(!fs.is_open()) {
      err:
      bctx.errnoMessageBox(WRITE_FAIL,"",errno);
      return;
  }
  fs.seekp(edit_cp,std::ios_base::beg);
  fs.write((const char*)EditorMem.buff,EditorMem.size);
  if(!fs.good()) goto err;
  fs.close();
  bctx.bm_file().reread();
}

bool Editor::default_navigation(int _lastbyte)
{
    bool redraw;
    redraw = false;
    switch(_lastbyte) {
	case KE_UPARROW  : edit_y--; break;
	case KE_DOWNARROW: edit_y++; break;
	case KE_ENTER:
	case KE_LEFTARROW:
	case KE_RIGHTARROW: break;
	case KE_F(3)     :
		      Get2DigitDlg(INIT_MASK,INPUT_MASK,&edit_XX);
		      break;
	default: redraw = true; break;
    }
    return redraw;
}

bool Editor::default_action(int _lastbyte)
{
    bool redraw = true;
    int eidx;
    eidx = edit_y*EditorMem.width+edit_x;
    switch(_lastbyte) {
	case KE_F(4): EditorMem.buff[eidx] = ~EditorMem.buff[eidx]; break;
	case KE_F(5): EditorMem.buff[eidx] |= edit_XX; break;
	case KE_F(6): EditorMem.buff[eidx] &= edit_XX; break;
	case KE_F(7): EditorMem.buff[eidx] ^= edit_XX; break;
	case KE_F(8): EditorMem.buff[eidx]  = edit_XX; break;
	case KE_F(9): EditorMem.buff[eidx] = EditorMem.save[eidx]; break;
	default     : redraw = default_navigation(_lastbyte); edit_x--; break;
    }
    edit_x++;
    return redraw;
}

int Editor::run(TWindow* hexwnd)
{
    size_t i,j;
    unsigned mlen;
    unsigned int _lastbyte;
    unsigned flags;
    tAbsCoord height = bctx.main_wnd().client_height();
    bool redraw;
    char attr = __ESS_HARDEDIT | __ESS_WANTRETURN;
    ewnd.set_color(browser_cset.edit.main);
    bctx.tconsole().mouse_set_state(false);
    for(i = 0;i < height;i++) {
	for(j = 0;j < EditorMem.alen[i];j++) {
	    unsigned eidx;
	    eidx = i*EditorMem.width+j;
	    ewnd.set_color(EditorMem.buff[eidx] == EditorMem.save[eidx] ? browser_cset.edit.main : browser_cset.edit.change);
	    ewnd.write(j + 1,i + 1,&EditorMem.buff[eidx],1);
	}
	if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width) {
	    ewnd.goto_xy(EditorMem.alen[i] + 1,i + 1);
	    ewnd.clreol();
	}
    }
    bctx.tconsole().mouse_set_state(true);
    paint_title(edit_y*EditorMem.width + edit_x,0);
    TWindow::set_cursor_type(TWindow::Cursor_Normal);
    redraw = true;
    if(hexwnd) {
	char work[__TVIO_MAXSCREENWIDTH];
	int len;
	hexwnd->set_color(browser_cset.main);
	hexwnd->freeze();
	for(i = 0;i < height;i++) {
	    mlen = EditorMem.alen[i];
	    len = ExpandHex(work,&EditorMem.buff[i*EditorMem.width],mlen,2);
	    hexwnd->write(11,i + 1,(const uint8_t*)work,len);
	    if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width) {
		hexwnd->goto_xy(11+len,i + 1);
		hexwnd->clreol();
	    }
	}
	hexwnd->refresh();
    }
    ewnd.show();
    ewnd.set_focus();
    while(1) {
	unsigned eidx;
	eidx = edit_y*EditorMem.width;
	mlen = EditorMem.alen[edit_y];
	flags = attr;
	if(!redraw) flags |= __ESS_NOREDRAW;
	_lastbyte = eeditstring(&ewnd,(char *)&EditorMem.buff[eidx],NULL,&mlen,(unsigned)(edit_y + 1),
				(unsigned *)&edit_x,flags,(char *)&EditorMem.save[eidx], NULL);
	switch(_lastbyte) {
	    case KE_F(1)   : show_help(); continue;
	    case KE_F(2)   : save_context();
	    case KE_F(10)  :
	    case KE_ESCAPE :
	    case KE_TAB : goto bye;
	    default     : redraw = default_action(_lastbyte); break;
	}
	CheckBounds();
	if(redraw) {
	    if(hexwnd) {
		char work[__TVIO_MAXSCREENWIDTH];
		int len;
		len = ExpandHex(work,&EditorMem.buff[edit_y*EditorMem.width],mlen,2);
		hexwnd->write(11,edit_y + 1,(const uint8_t*)work,len);
	    }
	}
	paint_title(edit_y*EditorMem.width + edit_x,0);
    }
bye:
    TWindow::set_cursor_type(TWindow::Cursor_Off);
    return _lastbyte;
}
} // namespace	usr

