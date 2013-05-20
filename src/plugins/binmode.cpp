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
#include "udn.h"
#include "reg_form.h"
#include "editor.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/osdep/tconsole.h"

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
	    BinMode(const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& code_guider,udn&);
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
	    static void		save_video(Opaque& _this,unsigned char *buff,unsigned size);

	    unsigned		virtWidthCorr;
	    unsigned		bin_mode; /**< points to currently selected mode text mode */
	    TWindow&		main_wnd;
	    binary_stream&	main_handle;
	    const Bin_Format&	bin_format;
	    udn&		_udn;
    };

BinMode::BinMode(const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& code_guider,udn& u)
	:Plugin(b,h,_main_wnd,code_guider,u)
	,virtWidthCorr(0)
	,bin_mode(MOD_PLAIN)
	,main_wnd(_main_wnd)
	,main_handle(h)
	,bin_format(b)
	,_udn(u)
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
    i = ListBox(mod_names,nModes," Select binary mode: ",LB_SELECTIVE|LB_USEACC,bin_mode);
    if(i != -1) {
	bin_mode = i;
	return true;
    }
    return false;
}

bool BinMode::action_F6() /* binDecVirtWidth */
{
    if(virtWidthCorr < beye_context().tconsole().vio_width()-1) { virtWidthCorr++; return true; }
    return false;
}

bool BinMode::action_F7() /* binIncVirtWidth */
{
    if(virtWidthCorr) { virtWidthCorr--; return true; }
    return false;
}

bool BinMode::action_F10() { return _udn.names(); }

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
    cfp  = main_handle.tell();
    width = main_wnd.client_width();
    BWidth = main_wnd.client_width()-virtWidthCorr;
    height = main_wnd.client_height();
    if(bin_mode==MOD_PLAIN) _b_width=1;
    else _b_width=2;
    if(cfp != bmocpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND) {
	bmocpos = cfp;
	flen = main_handle.flength();
	limit = flen - BWidth;
	if(flen < (__filesize_t)BWidth) BWidth = (int)(limit = flen);
	main_wnd.freeze();
	for(j = 0;j < height;j++) {
	    count=BWidth*_b_width;
	    _index = cfp + j*count;
	    len = _index < limit ? (int)count : _index < flen ? (int)(flen - _index) : 0;
	    if(len) {
		lastbyte = _index + len;
		main_handle.seek(_index,binary_stream::Seek_Set);
		main_handle.read((any_t*)buffer,len);
	    }
	    if(bin_mode!=MOD_PLAIN) {
		unsigned i,ii;
		for(i=ii=0;i<BWidth;i++) {
		    chars[i]=buffer[ii++];
		    attrs[i]=buffer[ii++];
		}
		::memset(oem_pg,0,beye_context().tconsole().vio_width());
		if(bin_mode==MOD_REVERSE) {
		    t_vchar *t;
		    t=it.chars;
		    it.chars=it.attrs;
		    it.attrs=(ColorAttr*)t;
		}
		count=len/2;
		::memset(&it.chars[count],TWC_DEF_FILLER,beye_context().tconsole().vio_width()-count);
		::memset(&it.attrs[count],browser_cset.main,beye_context().tconsole().vio_width()-count);
	    } else ::memset(&buffer[len],TWC_DEF_FILLER,beye_context().tconsole().vio_width()-len);
	    if(isHOnLine(_index,width)) {
		hli.text = buffer;
		HiLightSearch(main_wnd,_index,0,BWidth,j,&hli,HLS_NORMAL);
	    } else {
		if(bin_mode==MOD_PLAIN)
		    main_wnd.direct_write(1,j + 1,buffer,width);
		else
		    main_wnd.write(1,j + 1,&it,width);
	    }
	}
	main_wnd.refresh();
    }
    return tshift;
}

void BinMode::help() const
{
   hlpDisplay(1000);
}

unsigned long BinMode::prev_page_size() const { return (main_wnd.client_width()-virtWidthCorr)*main_wnd.client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_page_size() const { return (main_wnd.client_width()-virtWidthCorr)*main_wnd.client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::prev_line_width() const{ return (main_wnd.client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_line_width() const{ return (main_wnd.client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }

const char*   BinMode::misckey_name() const { return "Modify"; }

void BinMode::save_video(Opaque& _this,unsigned char *buff,unsigned size)
{
    BinMode& it = static_cast<BinMode&>(_this);
    std::ofstream fs;
    std::string fname;
    unsigned i;
    fname = beye_context().bm_file().filename();
    fs.open(fname.c_str(),std::ios_base::binary);
    if(!fs.is_open()) {
	err:
	beye_context().errnoMessageBox(WRITE_FAIL,"",errno);
	return;
    }
    fs.seekp(beye_context().tell(),std::ios_base::beg);
    if(it.bin_mode==MOD_REVERSE) fs.seekp(1,std::ios_base::cur);
    for(i=0;i<size;i++) {
	fs.put((uint8_t)buff[i]);
	if(!fs.good()) goto err;
	fs.seekp(1,std::ios_base::cur);
    }
    fs.close();
    beye_context().bm_file().reread();
}

void BinMode::misckey_action() /* EditBin */
{
    TWindow *ewin;
    bool inited;
    if(!main_handle.flength()) { beye_context().ErrMessageBox(NOTHING_EDIT,""); return; }
    ewin = new(zeromem) TWindow(1,2,beye_context().tconsole().vio_width()-virtWidthCorr,beye_context().tconsole().vio_height()-2,TWindow::Flag_Has_Cursor);
    ewin->set_color(browser_cset.edit.main); ewin->clear();
    drawEditPrompt();
    ewin->set_focus();
    edit_x = edit_y = 0;
    if(bin_mode==MOD_PLAIN) inited=editInitBuffs(beye_context().tconsole().vio_width()-virtWidthCorr,NULL,0);
    else {
	unsigned long flen,cfp;
	unsigned i,size,msize = beye_context().tconsole().vio_width()*beye_context().tconsole().vio_height();
	unsigned char *buff = new unsigned char [msize*2];
	if(buff) {
	    flen = main_handle.flength();
	    cfp = main_handle.tell();
	    size = (unsigned)((unsigned long)msize > (flen-cfp) ? (flen-cfp) : msize);
	    main_handle.seek(cfp,binary_stream::Seek_Set);
	    main_handle.read(buff,size*2);
	    main_handle.seek(cfp,binary_stream::Seek_Set);
	    for(i=0;i<size;i++) buff[i]=bin_mode==MOD_BINARY?buff[i*2]:buff[i*2+1];
	    inited=editInitBuffs(beye_context().tconsole().vio_width()-virtWidthCorr,buff,size);
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
	if(virtWidthCorr>beye_context().tconsole().vio_width()-1) virtWidthCorr=beye_context().tconsole().vio_width()-1;
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
unsigned BinMode::get_max_line_length() const { return main_wnd.client_width(); }

static Plugin* query_interface(const Bin_Format& b,binary_stream& h,TWindow& main_wnd,CodeGuider& code_guider,udn& u) { return new(zeromem) BinMode(b,h,main_wnd,code_guider,u); }

extern const Plugin_Info binMode = {
    "~Binary mode",	/**< plugin name */
    query_interface
};

} // namespace	usr
