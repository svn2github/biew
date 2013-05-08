#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_I
 * @file        plugins/binmode.c
 * @brief       This file contains implementation of binary mode viewer.
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
#include <errno.h>

#include "beye.h"
#include "colorset.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "bin_util.h"
#include "bmfile.h"
#include "reg_form.h"
#include "editor.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"

#include "plugin.h"

namespace	usr {
    enum {
	MOD_PLAIN  =0,
	MOD_BINARY =1,
	MOD_REVERSE=2,
	MOD_MAXMODE=2
    };
    class BinMode : public Plugin {
	public:
	    BinMode(CodeGuider& code_guider);
	    virtual ~BinMode();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual bool		action_F2();
	    virtual bool		action_F6();
	    virtual bool		action_F7();
	    virtual bool		action_F10();

	    virtual bool		detect();
	    virtual e_flag		flags() const;
	    virtual unsigned		paint(unsigned keycode,unsigned textshift);

	    virtual unsigned		get_symbol_size() const;
	    virtual unsigned		get_max_line_length() const;
	    virtual const char*		misckey_name() const;
	    virtual void		misckey_action();
	    virtual unsigned long	prev_page_size() const;
	    virtual unsigned long	curr_page_size() const;
	    virtual unsigned long	prev_line_width() const;
	    virtual unsigned long	curr_line_width() const;
	    virtual void		help() const;
	    virtual void		read_ini(Ini_Profile& );
	    virtual void		save_ini(Ini_Profile& );
	private:
	    static void	save_video(Opaque& _this,unsigned char *buff,unsigned size);

	    unsigned	virtWidthCorr;
	    unsigned	bin_mode; /**< points to currently selected mode text mode */

    };

BinMode::BinMode(CodeGuider& code_guider)
	:Plugin(code_guider)
	,virtWidthCorr(0)
	,bin_mode(MOD_PLAIN)
{}
BinMode::~BinMode() {}

static const char* txt[] = { "", "BinMod", "", "", "", "", "<<<   ", "   >>>", "", "UsrNam" };
const char* BinMode::prompt(unsigned idx) const {
    return (idx < 10) ? txt[idx] : "";
}

static const char * mod_names[] =
{
   "~Plain text",
   "~Video dump",
   "~Reversed video dump"
};

bool BinMode::action_F2() /* select mode */
{
    unsigned nModes;
    int i;
    nModes = sizeof(mod_names)/sizeof(char *);
    i = SelBoxA(mod_names,nModes," Select binary mode: ",bin_mode);
    if(i != -1) {
	bin_mode = i;
	return true;
    }
    return false;
}

bool BinMode::action_F6() /* binDecVirtWidth */
{
    if(virtWidthCorr < tvioWidth-1) { virtWidthCorr++; return true; }
    return false;
}

bool BinMode::action_F7() /* binIncVirtWidth */
{
    if(virtWidthCorr) { virtWidthCorr--; return true; }
    return false;
}

bool BinMode::action_F10() { return udnUserNames(); }

bool BinMode::detect() { return true; }

BinMode::e_flag BinMode::flags() const { return None; }

unsigned BinMode::paint( unsigned keycode,unsigned tshift )
{
    static __filesize_t bmocpos = 0L;
    __filesize_t _index;
    __filesize_t limit,flen,cfp;
    int len;
    unsigned BWidth,_b_width,count;
    size_t j;
    HLInfo hli;
    tvioBuff it;
    char buffer[__TVIO_MAXSCREENWIDTH*2];
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    tAbsCoord width,height;

    it.chars=chars;
    it.oem_pg=oem_pg;
    it.attrs=attrs;
    cfp  = BMGetCurrFilePos();
    width = MainWnd->client_width();
    BWidth = MainWnd->client_width()-virtWidthCorr;
    height = MainWnd->client_height();
    if(bin_mode==MOD_PLAIN) _b_width=1;
    else _b_width=2;
    if(cfp != bmocpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND) {
	bmocpos = cfp;
	flen = BMGetFLength();
	limit = flen - BWidth;
	if(flen < (__filesize_t)BWidth) BWidth = (int)(limit = flen);
	MainWnd->freeze();
	for(j = 0;j < height;j++) {
	    count=BWidth*_b_width;
	    _index = cfp + j*count;
	    len = _index < limit ? (int)count : _index < flen ? (int)(flen - _index) : 0;
	    if(len) { lastbyte = _index + len; BMReadBufferEx((any_t*)buffer,len,_index,BFile::Seek_Set); }
	    if(bin_mode!=MOD_PLAIN) {
		unsigned i,ii;
		for(i=ii=0;i<BWidth;i++) {
		    chars[i]=buffer[ii++];
		    attrs[i]=buffer[ii++];
		}
		::memset(oem_pg,0,tvioWidth);
		if(bin_mode==MOD_REVERSE) {
		    t_vchar *t;
		    t=it.chars;
		    it.chars=it.attrs;
		    it.attrs=(ColorAttr*)t;
		}
		count=len/2;
		::memset(&it.chars[count],TWC_DEF_FILLER,tvioWidth-count);
		::memset(&it.attrs[count],browser_cset.main,tvioWidth-count);
	    } else ::memset(&buffer[len],TWC_DEF_FILLER,tvioWidth-len);
	    if(isHOnLine(_index,width)) {
		hli.text = buffer;
		HiLightSearch(MainWnd,_index,0,BWidth,j,&hli,HLS_NORMAL);
	    } else {
		if(bin_mode==MOD_PLAIN)
		    MainWnd->direct_write(1,j + 1,buffer,width);
		else
		    MainWnd->write(1,j + 1,&it,width);
	    }
	}
	MainWnd->refresh();
    }
    return tshift;
}

void BinMode::help() const
{
   hlpDisplay(1000);
}

unsigned long BinMode::prev_page_size() const { return (MainWnd->client_width()-virtWidthCorr)*MainWnd->client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_page_size() const { return (MainWnd->client_width()-virtWidthCorr)*MainWnd->client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::prev_line_width() const{ return (MainWnd->client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_line_width() const{ return (MainWnd->client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }

const char*   BinMode::misckey_name() const { return "Modify"; }

void BinMode::save_video(Opaque& _this,unsigned char *buff,unsigned size)
{
    BinMode& it = static_cast<BinMode&>(_this);
    BFile* bHandle;
    std::string fname;
    unsigned i;
    fname = BMName();
    bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
    if(bHandle == &bNull) {
	err:
	errnoMessageBox(WRITE_FAIL,"",errno);
	return;
    }
    bHandle->seek(BMGetCurrFilePos(),BFile::Seek_Set);
    if(it.bin_mode==MOD_REVERSE) bHandle->seek(1,BFile::Seek_Cur);
    for(i=0;i<size;i++) {
	if(!bHandle->write_byte(buff[i])) goto err;
	bHandle->seek(1,BFile::Seek_Cur);
    }
    delete bHandle;
    BMReRead();
}

void BinMode::misckey_action() /* EditBin */
{
    TWindow *ewin;
    bool inited;
    if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,""); return; }
    ewin = WindowOpen(1,2,tvioWidth-virtWidthCorr,tvioHeight-1,TWindow::Flag_Has_Cursor);
    ewin->set_color(browser_cset.edit.main); ewin->clear();
    drawEditPrompt();
    ewin->set_focus();
    edit_x = edit_y = 0;
    if(bin_mode==MOD_PLAIN) inited=editInitBuffs(tvioWidth-virtWidthCorr,NULL,0);
    else {
	unsigned long flen,cfp;
	unsigned i,size,msize = tvioWidth*tvioHeight;
	unsigned char *buff = new unsigned char [msize*2];
	if(buff) {
	    flen = BMGetFLength();
	    cfp = BMGetCurrFilePos();
	    size = (unsigned)((unsigned long)msize > (flen-cfp) ? (flen-cfp) : msize);
	    BMReadBufferEx(buff,size*2,cfp,BFile::Seek_Set);
	    BMSeek(cfp,BFile::Seek_Set);
	    for(i=0;i<size;i++) buff[i]=bin_mode==MOD_BINARY?buff[i*2]:buff[i*2+1];
	    inited=editInitBuffs(tvioWidth-virtWidthCorr,buff,size);
	    delete buff;
	} else {
	    MemOutBox("Editor initialization");
	    inited=false;
	}
    }
    if(inited) {
	FullEdit(ewin,NULL,*this,bin_mode==MOD_PLAIN?NULL:save_video);
	editDestroyBuffs();
    }
    delete ewin;
    beye_context().PaintTitle();
}

void BinMode::read_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	tmps=bctx.read_profile_string(ini,"Beye","Browser","LastSubMode","0");
	bin_mode = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(bin_mode > MOD_MAXMODE) bin_mode = MOD_MAXMODE;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","VirtWidthCorr","0");
	virtWidthCorr = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(virtWidthCorr>tvioWidth-1) virtWidthCorr=tvioWidth-1;
    }
}

void BinMode::save_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    char tmps[10];
    /** Nullify LastSubMode */
    ::sprintf(tmps,"%i",bin_mode);
    bctx.write_profile_string(ini,"Beye","Browser","LastSubMode",tmps);
    ::sprintf(tmps,"%u",virtWidthCorr);
    bctx.write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

unsigned BinMode::get_symbol_size() const { return bin_mode==MOD_PLAIN?1:2; }
unsigned BinMode::get_max_line_length() const { return MainWnd->client_width(); }

static Plugin* query_interface(CodeGuider& code_guider) { return new(zeromem) BinMode(code_guider); }

extern const Plugin_Info binMode = {
    "~Binary mode",	/**< plugin name */
    query_interface
};

} // namespace	usr
