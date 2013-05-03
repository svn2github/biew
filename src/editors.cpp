#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
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
#include "bmfile.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "editor.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace beye {
__fileoff_t edit_cp = 0;
struct tag_emem EditorMem;

int edit_x,edit_y;
unsigned char edit_XX = 0;

void ExtHelp( void )
{
 TWindow * _using = twFocusedWin();
 hlpDisplay(2);
 twFocusWin(_using);
}

void __FASTCALL__ PaintETitle( int shift,bool use_shift )
{
  TWindow * _using = twFocusedWin();
  unsigned eidx;
  char byte,obyte;
  twFocusWin(TitleWnd);
  twFreezeWin(TitleWnd);
  twGotoXY(TitleWnd,1,1);
  twClrEOL(TitleWnd);
  twPrintF(TitleWnd,"%08lX: ",edit_cp + shift);
  eidx = use_shift ? (unsigned)shift : edit_y*EditorMem.width+edit_x;
  byte  = EditorMem.buff[eidx];
  obyte = EditorMem.save[eidx];
  if(byte != obyte) twSetColorAttr(TitleWnd,title_cset.change);
  twPrintF(TitleWnd,"%c %02XH %sH %sB "
	   ,byte ? byte : ' '
	   ,byte & 0x00FF
	   ,Get2SignDig(byte)
	   ,GetBinary(byte));
  twSetColorAttr(TitleWnd,title_cset.main);
  if(byte != obyte)
  {
    twPrintF(TitleWnd,"ORIGINAL: %c %02XH %sH %sB "
	     ,obyte ? obyte : ' '
	     ,obyte & 0x00FF
	     ,Get2SignDig(obyte)
	     ,GetBinary(obyte));
  }
  else
    twPrintF(TitleWnd,"                                ");
  twPrintF(TitleWnd,"MASK: %sH"
	   ,Get2Digit(edit_XX));
  twRefreshWin(TitleWnd);
  twFocusWin(_using);
}

bool __FASTCALL__ editInitBuffs(unsigned width,unsigned char *buff,unsigned size)
{
 __filesize_t flen,cfp,ssize;
 unsigned i,msize;
 msize = tvioWidth*tvioHeight;
 EditorMem.buff = new unsigned char [msize];
 EditorMem.save = new unsigned char [msize];
 EditorMem.alen = new unsigned char [tvioHeight];
 if((!EditorMem.buff) || (!EditorMem.save) || (!EditorMem.alen))
 {
   if(EditorMem.buff) delete EditorMem.buff;
   if(EditorMem.save) delete EditorMem.save;
   if(EditorMem.alen) delete EditorMem.alen;
   MemOutBox("Editor initialization");
   return false;
 }
 memset(EditorMem.buff,TWC_DEF_FILLER,msize);
 memset(EditorMem.save,TWC_DEF_FILLER,msize);
 flen = BMGetFLength();
 edit_cp = cfp = BMGetCurrFilePos();
 EditorMem.width = width;
 if(buff)
 {
    EditorMem.size = size;
    memcpy(EditorMem.buff,buff,size);
 }
 else
 {
    EditorMem.size = (unsigned)((__filesize_t)msize > (flen-cfp) ? (flen-cfp) : msize);
    BMReadBufferEx(EditorMem.buff,EditorMem.size,cfp,BFile::Seek_Set);
    BMSeek(cfp,BFile::Seek_Set);
 }
 memcpy(EditorMem.save,EditorMem.buff,EditorMem.size);
 /** initialize EditorMem.alen */
 ssize = flen-cfp;
 for(i = 0;i < tvioHeight;i++)
 {
    EditorMem.alen[i] = ssize >= width ? width : ssize;
    ssize -= std::min(ssize,__filesize_t(width));
 }
 return true;
}

void __FASTCALL__ editDestroyBuffs( void )
{
  delete EditorMem.buff;
  delete EditorMem.save;
  delete EditorMem.alen;
}

void __FASTCALL__ CheckBounds( void )
{
  tAbsCoord height = twGetClientHeight(MainWnd);
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  if(!EditorMem.alen[edit_y]) edit_y--;
  if(edit_x >= EditorMem.alen[edit_y]) edit_x = EditorMem.alen[edit_y] - 1;
}

void __FASTCALL__ CheckYBounds( void )
{
  tAbsCoord height = twGetClientHeight(MainWnd);
  if(edit_y < 0) edit_y = 0;
  if((unsigned)edit_y > height - 1) edit_y = height - 1;
  while(!EditorMem.alen[edit_y]) edit_y--;
}

void __FASTCALL__ CheckXYBounds( void )
{
   CheckYBounds();
   if(edit_x < 0) edit_x = EditorMem.alen[--edit_y]*2;
   if(edit_x >= EditorMem.alen[edit_y]*2) { edit_x = 0; edit_y++; }
   CheckYBounds();
}

void __FASTCALL__ editSaveContest( void )
{
  BFile* bHandle;
  const char *fname;
  fname = BMName();
  bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
  if(bHandle == &bNull)
  {
      err:
      errnoMessageBox(WRITE_FAIL,NULL,errno);
      return;
  }
  bHandle->seek(edit_cp,BFile::Seek_Set);
  if(!bHandle->write_buffer((any_t*)EditorMem.buff,EditorMem.size)) goto err;
  delete bHandle;
  BMReRead();
}

bool __FASTCALL__ edit_defaction(int _lastbyte)
{
 bool redraw;
  redraw = false;
   switch(_lastbyte)
   {
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

bool __FASTCALL__ editDefAction(int _lastbyte)
{
 bool redraw = true;
 int eidx;
 eidx = edit_y*EditorMem.width+edit_x;
   switch(_lastbyte)
   {
     case KE_F(4)     : EditorMem.buff[eidx] = ~EditorMem.buff[eidx]; break;
     case KE_F(5)     : EditorMem.buff[eidx] |= edit_XX; break;
     case KE_F(6)     : EditorMem.buff[eidx] &= edit_XX; break;
     case KE_F(7)     : EditorMem.buff[eidx] ^= edit_XX; break;
     case KE_F(8)     : EditorMem.buff[eidx]  = edit_XX; break;
     case KE_F(9)     : EditorMem.buff[eidx] = EditorMem.save[eidx]; break;
     default        : redraw = edit_defaction(_lastbyte); edit_x--; break;
   }
   edit_x++;
   return redraw;
}

int __FASTCALL__ FullEdit(TWindow* ewnd,TWindow* hexwnd,Opaque& _this,void (*save_func)(Opaque& _this,unsigned char *,unsigned))
{
    size_t i,j;
    unsigned mlen;
    unsigned int _lastbyte;
    unsigned flags;
    tAbsCoord height = twGetClientHeight(MainWnd);
    bool redraw;
    char attr = __ESS_HARDEDIT | __ESS_WANTRETURN;
    twSetColorAttr(ewnd,browser_cset.edit.main);
    __MsSetState(false);
    for(i = 0;i < height;i++) {
	for(j = 0;j < EditorMem.alen[i];j++) {
	    unsigned eidx;
	    eidx = i*EditorMem.width+j;
	    twSetColorAttr(ewnd,EditorMem.buff[eidx] == EditorMem.save[eidx] ? browser_cset.edit.main : browser_cset.edit.change);
	    twDirectWrite(ewnd,j + 1,i + 1,&EditorMem.buff[eidx],1);
	}
	if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width) {
	    twGotoXY(ewnd,EditorMem.alen[i] + 1,i + 1); twClrEOL(MainWnd);
	}
    }
    __MsSetState(true);
    PaintETitle(edit_y*EditorMem.width + edit_x,0);
    twSetCursorType(TW_CUR_NORM);
    redraw = true;
    if(hexwnd) {
	char work[__TVIO_MAXSCREENWIDTH];
	int len;
	twSetColorAttr(hexwnd,browser_cset.main);
	twFreezeWin(hexwnd);
	for(i = 0;i < height;i++) {
	    mlen = EditorMem.alen[i];
	    len = ExpandHex(work,&EditorMem.buff[i*EditorMem.width],mlen,2);
	    twDirectWrite(hexwnd,11,i + 1,work,len);
	    if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width) {
		twGotoXY(hexwnd,11+len,i + 1);
		twClrEOL(hexwnd);
	    }
	}
	twRefreshWin(hexwnd);
    }
    twShowWin(ewnd);
    twFocusWin(ewnd);
    while(1) {
	unsigned eidx;
	eidx = edit_y*EditorMem.width;
	mlen = EditorMem.alen[edit_y];
	flags = attr;
	if(!redraw) flags |= __ESS_NOREDRAW;
	_lastbyte = eeditstring(ewnd,(char *)&EditorMem.buff[eidx],NULL,&mlen,(unsigned)(edit_y + 1),
				(unsigned *)&edit_x,flags,(char *)&EditorMem.save[eidx], NULL);
	switch(_lastbyte) {
	    case KE_F(1)   : ExtHelp(); continue;
	    case KE_F(2)   : save_func?save_func(_this,EditorMem.buff,EditorMem.size):editSaveContest();
	    case KE_F(10)  :
	    case KE_ESCAPE : goto bye;
	    case KE_TAB : if(ewnd) goto bye;
	    default     : redraw = editDefAction(_lastbyte); break;
	}
	CheckBounds();
	if(redraw) {
	    if(hexwnd) {
		char work[__TVIO_MAXSCREENWIDTH];
		int len;
		len = ExpandHex(work,&EditorMem.buff[edit_y*EditorMem.width],mlen,2);
		twDirectWrite(hexwnd,11,edit_y + 1,work,len);
	    }
	}
	PaintETitle(edit_y*EditorMem.width + edit_x,0);
    }
bye:
    twSetCursorType(TW_CUR_OFF);
    return _lastbyte;
}
} // namespace beye

