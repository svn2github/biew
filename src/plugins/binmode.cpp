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
#include "listbox.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "search.h"
#include "udn.h"
#include "editor.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"

#include "plugin.h"

namespace	usr {
    enum {
	MOD_PLAIN  =0,
	MOD_BINARY =1,
	MOD_REVERSE=2,
	MOD_MAXMODE=2
    };
    class Bin_Editor;
    class BinMode : public Plugin {
	public:
	    BinMode(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& code_guider,udn&,Search&);
	    virtual ~BinMode();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual bool		action_F2();
	    virtual bool		action_F6();
	    virtual bool		action_F7();
	    virtual bool		action_F10();

	    virtual bool		detect();
	    virtual e_flag		flags() const;
	    virtual plugin_position	paint(unsigned keycode,unsigned textshift);

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
	protected:
	    friend class Bin_Editor;
	    void			save_video(unsigned char *buff,unsigned size);
	private:

	    unsigned		virtWidthCorr;
	    unsigned		bin_mode; /**< points to currently selected mode text mode */
	    TWindow&		main_wnd;
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    const Bin_Format&	bin_format;
	    udn&		_udn;
	    Search&		search;
    };

    class Bin_Editor : public Editor {
	public:
	    Bin_Editor(BeyeContext& bc,TWindow&,BinMode& parent,unsigned bin_mode,unsigned width);
	    Bin_Editor(BeyeContext& bc,TWindow&,BinMode& parent,unsigned bin_mode,unsigned width,const unsigned char *buff,unsigned size);
	    virtual ~Bin_Editor();

	    virtual void	save_contest();
	private:
	    BinMode&	parent;
	    unsigned	bin_mode;
    };

Bin_Editor::Bin_Editor(BeyeContext& bc,TWindow& w,BinMode& _parent,unsigned bmode,unsigned width)
	    :Editor(bc,w,width)
	    ,parent(_parent)
	    ,bin_mode(bmode)
{
}
Bin_Editor::Bin_Editor(BeyeContext& bc,TWindow& w,BinMode& _parent,unsigned bmode,unsigned width,const unsigned char *buff,unsigned size)
	    :Editor(bc,w,width,buff,size)
	    ,parent(_parent)
	    ,bin_mode(bmode)
{
}
Bin_Editor::~Bin_Editor() {}

void Bin_Editor::save_contest() {
    editor_mem emem = get_mem();
    if(bin_mode==MOD_PLAIN) Editor::save_context();
    else parent.save_video(emem.buff,emem.size);
}

BinMode::BinMode(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& code_guider,udn& u,Search& s)
	:Plugin(bc,b,h,_main_wnd,code_guider,u,s)
	,virtWidthCorr(0)
	,bin_mode(MOD_PLAIN)
	,main_wnd(_main_wnd)
	,bctx(bc)
	,main_handle(h)
	,bin_format(b)
	,_udn(u)
	,search(s)
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
    ListBox lb(bctx);
    i = lb.run(mod_names,nModes," Select binary mode: ",ListBox::Selective|ListBox::UseAcc,bin_mode);
    if(i != -1) {
	bin_mode = i;
	return true;
    }
    return false;
}

bool BinMode::action_F6() /* binDecVirtWidth */
{
    if(virtWidthCorr < main_wnd.width()-1) { virtWidthCorr++; return true; }
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

plugin_position BinMode::paint( unsigned keycode,unsigned tshift )
{
    static __filesize_t bmocpos = 0L;
    __filesize_t _index;
    __filesize_t limit,flen,cfp;
    int len;
    unsigned BWidth,_b_width,count;
    size_t j;
    uint8_t buffer[__TVIO_MAXSCREENWIDTH*2];
    uint8_t _chars[__TVIO_MAXSCREENWIDTH];
    ColorAttr _attrs[__TVIO_MAXSCREENWIDTH];
    uint8_t* chars=_chars;
    ColorAttr* attrs=_attrs;
    tAbsCoord width,height;
    plugin_position rc;

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
		rc.lastbyte = _index + len;
		main_handle.seek(_index,binary_stream::Seek_Set);
		binary_packet bp=main_handle.read(len); memcpy(buffer,bp.data(),bp.size());
	    }
	    if(bin_mode!=MOD_PLAIN) {
		unsigned i,ii;
		for(i=ii=0;i<BWidth;i++) {
		    chars[i]=buffer[ii++];
		    attrs[i]=buffer[ii++];
		}
		if(bin_mode==MOD_REVERSE) {
		    any_t* t;
		    t=chars;
		    chars=(uint8_t*)attrs;
		    attrs=(ColorAttr*)t;
		}
		count=len/2;
		::memset(&chars[count],TWC_DEF_FILLER,main_wnd.width()-count);
		::memset(&attrs[count],browser_cset.main,main_wnd.width()-count);
	    } else ::memset(&buffer[len],TWC_DEF_FILLER,main_wnd.width()-len);
	    if(search.is_inline(_index,width)) search.hilight(main_wnd,_index,0,BWidth,j,(const char*)buffer,Search::HL_Normal);
	    else {
		if(bin_mode==MOD_PLAIN)
		    main_wnd.write(1,j+1,buffer,width);
		else
		    main_wnd.write(1,j+1,chars,attrs,width);
	    }
	}
	main_wnd.refresh();
    }
    rc.textshift = tshift;
    return rc;
}

void BinMode::help() const
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(1000);
	bhelp.close();
    }
}

unsigned long BinMode::prev_page_size() const { return (main_wnd.client_width()-virtWidthCorr)*main_wnd.client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_page_size() const { return (main_wnd.client_width()-virtWidthCorr)*main_wnd.client_height()*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::prev_line_width() const{ return (main_wnd.client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }
unsigned long BinMode::curr_line_width() const{ return (main_wnd.client_width()-virtWidthCorr)*(bin_mode==MOD_PLAIN?1:2); }

const char*   BinMode::misckey_name() const { return "Modify"; }

void BinMode::save_video(unsigned char *buff,unsigned size)
{
    std::ofstream fs;
    std::string fname;
    unsigned i;
    fname = bctx.bm_file().filename();
    fs.open(fname.c_str(),std::ios_base::binary);
    if(!fs.is_open()) {
	err:
	bctx.errnoMessageBox(WRITE_FAIL,"",errno);
	return;
    }
    fs.seekp(bctx.tell(),std::ios_base::beg);
    if(bin_mode==MOD_REVERSE) fs.seekp(1,std::ios_base::cur);
    for(i=0;i<size;i++) {
	fs.put((uint8_t)buff[i]);
	if(!fs.good()) goto err;
	fs.seekp(1,std::ios_base::cur);
    }
    fs.close();
    bctx.bm_file().reread();
}

void BinMode::misckey_action() /* EditBin */
{
    Bin_Editor* editor;
    TWindow *ewin;
    if(!main_handle.flength()) { bctx.ErrMessageBox(NOTHING_EDIT,""); return; }
    ewin = new(zeromem) TWindow(1,2,main_wnd.width()-virtWidthCorr,main_wnd.height(),TWindow::Flag_Has_Cursor);
    ewin->set_color(browser_cset.edit.main); ewin->clear();
    drawEditPrompt();
    ewin->set_focus();
    if(bin_mode==MOD_PLAIN) editor=new(zeromem) Bin_Editor(bctx,*ewin,*this,bin_mode,main_wnd.width()-virtWidthCorr);
    else {
	unsigned long flen,cfp;
	unsigned i,size,msize = main_wnd.width()*main_wnd.height();
	unsigned char *buff = new unsigned char [msize*2];
	flen = main_handle.flength();
	cfp = main_handle.tell();
	size = (unsigned)((unsigned long)msize > (flen-cfp) ? (flen-cfp) : msize);
	main_handle.seek(cfp,binary_stream::Seek_Set);
	binary_packet bp=main_handle.read(size*2); memcpy(buff,bp.data(),bp.size());
	main_handle.seek(cfp,binary_stream::Seek_Set);
	for(i=0;i<size;i++) buff[i]=bin_mode==MOD_BINARY?buff[i*2]:buff[i*2+1];
	editor=new(zeromem) Bin_Editor(bctx,*ewin,*this,bin_mode,main_wnd.width()-virtWidthCorr,buff,size);
	delete buff;
    }
    editor->goto_xy(0,0);
    editor->run();
    delete editor;
    delete ewin;
    bctx.PaintTitle();
}

void BinMode::read_ini(Ini_Profile& ini)
{
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	tmps=bctx.read_profile_string(ini,"Beye","Browser","LastSubMode","0");
	bin_mode = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(bin_mode > MOD_MAXMODE) bin_mode = MOD_MAXMODE;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","VirtWidthCorr","0");
	virtWidthCorr = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(virtWidthCorr>main_wnd.width()-1) virtWidthCorr=main_wnd.width()-1;
    }
}

void BinMode::save_ini(Ini_Profile& ini)
{
    char tmps[10];
    /** Nullify LastSubMode */
    ::sprintf(tmps,"%i",bin_mode);
    bctx.write_profile_string(ini,"Beye","Browser","LastSubMode",tmps);
    ::sprintf(tmps,"%u",virtWidthCorr);
    bctx.write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

unsigned BinMode::get_symbol_size() const { return bin_mode==MOD_PLAIN?1:2; }
unsigned BinMode::get_max_line_length() const { return main_wnd.client_width(); }

static Plugin* query_interface(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& main_wnd,CodeGuider& code_guider,udn& u,Search& s) { return new(zeromem) BinMode(bc,b,h,main_wnd,code_guider,u,s); }

extern const Plugin_Info binMode = {
    "~Binary mode",	/**< plugin name */
    query_interface
};

} // namespace	usr
