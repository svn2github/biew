#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_I
 * @file        plugins/hexmode.c
 * @brief       This file contains implementation of hexadecimal mode viewers.
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

#include "beye.h"
#include "libbeye/bswap.h"
#include "plugins/hexmode.h"
#include "colorset.h"
#include "bconsole.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "listbox.h"
#include "search.h"
#include "udn.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"

#include "plugin.h"

namespace	usr {
    class hexView : public Opaque {
	public:
	    hexView(binary_stream& h);
	    virtual ~hexView();

	    virtual char*		get(__filesize_t) const = 0;
	    virtual unsigned char	width(const TWindow&,bool is_file64) const = 0;
	    virtual unsigned		size() const = 0;
	    virtual unsigned		hexlen() const = 0;
	protected:
	    binary_stream&	handle() const { return _handle; } __PURE_FUNC__;
	private:
	    binary_stream&	_handle;
    };

    class bitView : public hexView {
	public:
	    bitView(binary_stream& h);
	    virtual ~bitView();

	    virtual char*		get(__filesize_t) const;
	    virtual unsigned char	width(const TWindow&,bool is_file64) const __PURE_FUNC__;
	    virtual unsigned		size() const __CONST_FUNC__;
	    virtual unsigned		hexlen() const __CONST_FUNC__;

	    static hexView*		query_interface(binary_stream&);
    };

    class byteView : public hexView {
	public:
	    byteView(binary_stream& h);
	    virtual ~byteView();

	    virtual char*		get(__filesize_t) const;
	    virtual unsigned char	width(const TWindow&,bool is_file64) const __PURE_FUNC__;
	    virtual unsigned		size() const __CONST_FUNC__;
	    virtual unsigned		hexlen() const __CONST_FUNC__;

	    static hexView*		query_interface(binary_stream&);
    };

    class wordView : public hexView {
	public:
	    wordView(binary_stream& h);
	    virtual ~wordView();

	    virtual char*		get(__filesize_t) const;
	    virtual unsigned char	width(const TWindow&,bool is_file64) const __PURE_FUNC__;
	    virtual unsigned		size() const __CONST_FUNC__;
	    virtual unsigned		hexlen() const __CONST_FUNC__;

	    static hexView*		query_interface(binary_stream&);
    };

    class dwordView : public hexView {
	public:
	    dwordView(binary_stream& h);
	    virtual ~dwordView();

	    virtual char*		get(__filesize_t) const;
	    virtual unsigned char	width(const TWindow&,bool is_file64) const __PURE_FUNC__;
	    virtual unsigned		size() const __CONST_FUNC__;
	    virtual unsigned		hexlen() const __CONST_FUNC__;

	    static hexView*		query_interface(binary_stream&);
    };

    struct xView_Info {
	const char*		name;
	hexView*		(*query_interface)(binary_stream& h);
    };

    class HexMode : public Plugin {
	public:
	    HexMode(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& code_guider,udn&,Search&);
	    virtual ~HexMode();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual bool		action_F2();
	    virtual bool		action_F3();
	    virtual bool		action_F6();
	    virtual bool		action_F7();
	    virtual bool		action_F8();
	    virtual bool		action_F10();

	    virtual bool		detect();
	    virtual e_flag		flags() const;
	    virtual plugin_position	paint(unsigned keycode,unsigned textshift);

	    virtual unsigned		get_symbol_size() const;
	    virtual unsigned		get_max_line_length() const; /**< Returns max symbol size in bytes for selected NLS codepage */
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
	    void		check_width_corr();
	    int			full_hex_edit(Editor&,TWindow * txtwnd,TWindow *hexwnd) const;

	    CodeGuider&		code_guider;
	    unsigned		virtWidthCorr;
	    unsigned		hmode;
	    TWindow&		main_wnd;
	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    const Bin_Format&	bin_format;
	    udn&		_udn;
	    bool		is_file64;
	    hexView*		hex_viewer;
	    Search&		search;
    };

static const struct xView_Info xView_Info[] = {
  { "B~it",         bitView::query_interface   },
  { "~Byte",        byteView::query_interface  },
  { "~Word",        wordView::query_interface  },
  { "~Double word", dwordView::query_interface }
};

unsigned	hexAddressResolv;
static unsigned hendian;

hexView::hexView(binary_stream& h)
	:_handle(h)
{
}

hexView::~hexView() {}

bitView::bitView(binary_stream& h)
	:hexView(h)
{
}
bitView::~bitView() {}
char* bitView::get(__filesize_t val) const {
    char id;
    handle().seek(val,binary_stream::Seek_Set);
    id=handle().read(type_byte);
    return GetBinary(id);
}
unsigned bitView::size() const { return 1; }
unsigned bitView::hexlen() const { return 8; }

byteView::byteView(binary_stream& h)
	:hexView(h)
{
}
byteView::~byteView() {}
char* byteView::get(__filesize_t val) const {
    char id;
    handle().seek(val,binary_stream::Seek_Set);
    id=handle().read(type_byte);
    return Get2Digit(id);
}
unsigned byteView::size() const { return 1; }
unsigned byteView::hexlen() const { return 2; }

wordView::wordView(binary_stream& h)
	:hexView(h)
{
}
wordView::~wordView() {}
char* wordView::get(__filesize_t val) const {
    unsigned short v;
    handle().seek(val,binary_stream::Seek_Set);
    v = handle().read(type_word);
    if(hendian==1) v=le2me_16(v);
    else
    if(hendian==2) v=be2me_16(v);
    return Get4Digit(v);
}
unsigned wordView::size() const { return 2; }
unsigned wordView::hexlen() const { return 4; }

dwordView::dwordView(binary_stream& h)
	:hexView(h)
{
}
dwordView::~dwordView() {}
char* dwordView::get(__filesize_t val) const {
    unsigned long v;
    handle().seek(val,binary_stream::Seek_Set);
    v = handle().read(type_dword);
    if(hendian==1) v=le2me_32(v);
    else
    if(hendian==2) v=be2me_32(v);
    return Get8Digit(v);
}
unsigned dwordView::size() const { return 4; }
unsigned dwordView::hexlen() const { return 8; }

unsigned char bitView::width(const TWindow& main_wnd,bool is_file64) const { return main_wnd.width()-(is_file64?18:10)/(8+1+1); }
unsigned char byteView::width(const TWindow& main_wnd,bool is_file64) const { return main_wnd.width()-(is_file64?18:10)/(12+1+4)*4; } /* always round on four-column boundary */
unsigned char wordView::width(const TWindow& main_wnd,bool is_file64) const { return main_wnd.width()-(is_file64?18:10)/(4+1+2); }
unsigned char dwordView::width(const TWindow& main_wnd,bool is_file64) const { return main_wnd.width()-(is_file64?18:10)/(8+1+4); }

hexView* bitView::query_interface(binary_stream& h) { return new(zeromem) bitView(h); }
hexView* byteView::query_interface(binary_stream& h) { return new(zeromem) byteView(h); }
hexView* wordView::query_interface(binary_stream& h) { return new(zeromem) wordView(h); }
hexView* dwordView::query_interface(binary_stream& h) { return new(zeromem) dwordView(h); }

HexMode::HexMode(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& _code_guider,udn& u,Search& s)
	:Plugin(bc,b,h,_main_wnd,_code_guider,u,s)
	,code_guider(_code_guider)
	,virtWidthCorr(0)
	,hmode(1)
	,main_wnd(_main_wnd)
	,bctx(bc)
	,main_handle(h)
	,bin_format(b)
	,_udn(u)
	,is_file64(bc.is_file64())
	,hex_viewer(xView_Info[hmode].query_interface(h))
	,search(s)
{
}

HexMode::~HexMode() { delete hex_viewer; }

HexMode::e_flag HexMode::flags() const { return None; }

static const char* txt[] = { "", "HexMod", "Endian", "", "", "AResol", "<<<   ", "   >>>", "", "UsrNam" };
const char* HexMode::prompt(unsigned idx) const {
    return (idx < 10) ? txt[idx] : "";
}

plugin_position HexMode::paint( unsigned keycode,unsigned textshift )
{
    int i,I,Limit,dir;
    uint8_t outstr[__TVIO_MAXSCREENWIDTH+1];
    unsigned char HWidth;
    unsigned scrHWidth;
    __filesize_t sindex,cpos,flen,lindex,SIndex;
    static __filesize_t hmocpos = 0L;
    int __inc,hexlen;
    plugin_position rc;

    cpos = main_handle.tell();
    if(hmocpos != cpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND) {
	tAbsCoord height = main_wnd.client_height();
	tAbsCoord width = main_wnd.client_width();
	main_wnd.freeze();
	HWidth = hex_viewer->width(main_wnd,is_file64)-virtWidthCorr;
	if(!(hmocpos == cpos + HWidth || hmocpos == cpos - HWidth)) keycode = KE_SUPERKEY;
	hmocpos = cpos;
	__inc = hex_viewer->size();
	hexlen = hex_viewer->hexlen();
	flen = main_handle.flength();
	scrHWidth = HWidth*__inc;
	if(flen < HWidth) HWidth = flen;
	if(keycode == KE_UPARROW) {
	    I = height-1;
	    dir = -1;
	    Limit = -1;
	    if((__filesize_t)HWidth <= cpos) {
		main_wnd.scroll_down(1,1);
		I = 0;
	    } else goto full_redraw;
	} else if(keycode == KE_DOWNARROW && flen >= HWidth) {
	    I = height-1;
	    dir = 1;
	    Limit = height;
	    main_wnd.scroll_up(I,1);
	} else {
	    full_redraw:
	    I = 0;
	    dir = 1;
	    Limit = height;
	}
	SIndex = cpos + HWidth*I;
	lindex = flen - SIndex;
	/* This loop is called only when line or screen is repainting */
	for(i = I,sindex = SIndex;i != Limit;i += 1*dir,sindex += scrHWidth*dir) {
	    ::memset(outstr,TWC_DEF_FILLER,width);
	    if(sindex < flen) {
		int freq,j,rwidth,xmin,len;
		lindex = (flen - sindex)/__inc;
		rwidth = lindex > HWidth ? HWidth : (int)lindex;
		len = is_file64?18:10;
		::memcpy(outstr,code_guider.encode_address(sindex,hexAddressResolv).c_str(),len);
		for(j = 0,freq = 0,lindex = sindex;j < rwidth;j++,lindex += __inc,freq++) {
		    ::memcpy(&outstr[len],hex_viewer->get(lindex),hexlen);
		    len += hexlen + 1;
		    if(hmode == 1) if(freq == 3) { freq = -1; len++; }
		}
		main_handle.seek(sindex,binary_stream::Seek_Set);
		main_handle.read((any_t*)&outstr[width - scrHWidth],rwidth*__inc);
		xmin = main_wnd.width()-scrHWidth;
		main_wnd.write(1,i + 1,outstr,xmin);
		if(search.is_inline(sindex,scrHWidth)) search.hilight(main_wnd,sindex,xmin,width,i,(const char*)&outstr[xmin],Search::HL_Normal);
		else  main_wnd.write(xmin + 1,i + 1,&outstr[xmin],width - xmin);
	    } else main_wnd.write(1,i + 1,outstr,width);
	}
	rc.lastbyte = lindex + __inc;
	main_wnd.refresh();
    }
    rc.textshift = textshift;
    return rc;
}

void HexMode::help() const
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(1002);
	bhelp.close();
    }
}

unsigned long HexMode::prev_page_size() const { return (hex_viewer->width(main_wnd,is_file64)-virtWidthCorr)*hex_viewer->size()*main_wnd.client_height(); }
unsigned long HexMode::curr_page_size() const { return (hex_viewer->width(main_wnd,is_file64)-virtWidthCorr)*hex_viewer->size()*main_wnd.client_height(); }
unsigned long HexMode::prev_line_width() const { return (hex_viewer->width(main_wnd,is_file64)-virtWidthCorr)*hex_viewer->size(); }
unsigned long HexMode::curr_line_width() const { return (hex_viewer->width(main_wnd,is_file64)-virtWidthCorr)*hex_viewer->size(); }

const char* HexMode::misckey_name() const { return hmode == 1 ? "Modify" : "      "; }
void HexMode::misckey_action () /* EditHex */
{
    TWindow * ewnd[2];
    bool has_show[2];
    int active = 0,oactive = 0;
    unsigned bound;
    tAbsCoord width = main_wnd.client_width();
    if(hmode != 1) return;
    if(!main_handle.flength()) { bctx.ErrMessageBox(NOTHING_EDIT,""); return; }
    bound = (width-(hex_viewer->width(main_wnd,is_file64)-virtWidthCorr))-is_file64?18:10;
    ewnd[0] = new(zeromem) TWindow((is_file64?18:10)+1,2,bound,main_wnd.height()-2,TWindow::Flag_Has_Cursor);
    ewnd[0]->set_color(browser_cset.edit.main); ewnd[0]->clear();
    ewnd[1] = new(zeromem) TWindow(bound+(is_file64?18:10)+1,2,width-bound+2,main_wnd.height()-2,TWindow::Flag_Has_Cursor);
    ewnd[1]->set_color(browser_cset.edit.main); ewnd[1]->clear();
    drawEditPrompt();
    has_show[0] = has_show[1] = false;
    Editor* editor = new(zeromem) Editor(bctx,*ewnd[1],hex_viewer->width(main_wnd,is_file64)-virtWidthCorr);
    editor->goto_xy(0,0);
    while(1) {
	int _lastbyte;
	if(active != oactive) {
	    ewnd[oactive]->hide();
	    if(has_show[active]) ewnd[active]->show();
	    oactive = active;
	}
	ewnd[active]->set_focus();
	if(!active) _lastbyte = full_hex_edit(*editor,&main_wnd,ewnd[0]);
	else        _lastbyte = editor->run(&main_wnd);
	has_show[active] = true;
	if(_lastbyte == KE_TAB) active = active ? 0 : 1;
	else break;
    }
    delete editor;

    delete ewnd[0];
    delete ewnd[1];
    bctx.PaintTitle();
}

void HexMode::check_width_corr()
{
    if(virtWidthCorr>(unsigned)hex_viewer->width(main_wnd,is_file64)-1) virtWidthCorr=hex_viewer->width(main_wnd,is_file64)-1;
}

bool HexMode::action_F2() /* hexSelectMode */
{
    std::vector<std::string> names;
    size_t i,nModes;
    int retval;
    nModes = sizeof(xView_Info)/sizeof(struct xView_Info);
    for(i = 0;i < nModes;i++) names.push_back(xView_Info[i].name);
    ListBox lb(bctx);
    retval = lb.run(names," Select hexadecimal mode: ",ListBox::Selective|ListBox::UseAcc,hmode);
    if(retval != -1) {
	hmode = retval;
	delete hex_viewer;
	hex_viewer = xView_Info[hmode].query_interface(main_handle);
	check_width_corr();
	return true;
    }
    return false;
}

static const char *nendian[] =
{
  "~Native",
  "~Little",
  "~Big"
};
bool HexMode::action_F3 () /* hexSelectEndian */
{
    size_t nModes;
    int retval;
    nModes = sizeof(nendian)/sizeof(char *);
    ListBox lb(bctx);
    retval = lb.run(nendian,nModes," Select endian mode: ",ListBox::Selective|ListBox::UseAcc,hendian);
    if(retval != -1) {
	hendian = retval;
	return true;
    }
    return false;
}

static const char *aresolv[] =
{
  "~Global (global file offset)",
  "~Local (local offset within blocks and virtual addresses)"
};

bool hexAddressResolution(unsigned& har)
{
    unsigned nModes;
    int i;
    nModes = sizeof(aresolv)/sizeof(char *);
    ListBox lb(beye_context());
    i = lb.run(aresolv,nModes," Select address resolving: ",ListBox::Selective|ListBox::UseAcc,(unsigned)har);
    if(i != -1) {
	har = i ? true : false;
	return true;
    }
    return false;
}

bool HexMode::action_F6() { return hexAddressResolution(hexAddressResolv); }

bool HexMode::action_F7() /* hexDecVirtWidth */
{
    if(virtWidthCorr < (unsigned)hex_viewer->width(main_wnd,is_file64)-1) { virtWidthCorr++; return true; }
    return false;
}

bool HexMode::action_F8() /* hexIncVirtWidth */
{
    if(virtWidthCorr) { virtWidthCorr--; return true; }
    return false;
}

bool HexMode::action_F10() { return _udn.names(); }

bool HexMode::detect() { return true; }

int HexMode::full_hex_edit(Editor& editor,TWindow* txtwnd,TWindow* hexwnd) const
{
    size_t i,j;
    unsigned mlen;
    unsigned int _lastbyte;
    unsigned flg;
    tAbsCoord height = txtwnd->client_height();
    tAbsCoord width = txtwnd->client_width();
    char work[__TVIO_MAXSCREENWIDTH],owork[__TVIO_MAXSCREENWIDTH];
    bool redraw;

    txtwnd->set_focus();
    txtwnd->set_color(browser_cset.main);
    txtwnd->freeze();
    editor_mem& emem = editor.get_mem();
    for(i = 0;i < height;i++) {
	txtwnd->write(width - emem.width + 1,i + 1,&emem.buff[i*emem.width],emem.alen[i]);
	if((unsigned)emem.alen[i] + 1 < emem.width) {
	    txtwnd->goto_xy(width - emem.width + emem.alen[i] + 2,i + 1); txtwnd->clreol();
        }
    }
    txtwnd->refresh();

    hexwnd->set_color(browser_cset.edit.main);
    for(i = 0;i < height;i++) {
	unsigned eidx;
	eidx = i*emem.width;
	ExpandHex(work,&emem.buff[eidx],emem.alen[i],1);
	mlen = ExpandHex(owork,&emem.save[eidx],emem.alen[i],1);
	for(j = 0;j < mlen;j++) {
	    hexwnd->set_color(work[j] == owork[j] ? browser_cset.edit.main : browser_cset.edit.change);
	    hexwnd->write(j + 1,i + 1,(const uint8_t*)&work[j],1);
	}
	if(mlen + 1 < emem.width) {
	    hexwnd->goto_xy(mlen + 1,i + 1);
	    hexwnd->clreol();
	}
    }
    redraw = true;
    editor.paint_title(editor.where_y()*emem.width + editor.where_x(),0);
    hexwnd->show();
    TWindow::set_cursor_type(TWindow::Cursor_Normal);
    while(1) {
	unsigned eidx;
	mlen = emem.alen[editor.where_y()];
	eidx = editor.where_y()*emem.width;
	ExpandHex(work,&emem.buff[eidx],mlen,1);
	mlen = ExpandHex(owork,&emem.save[eidx],mlen,1);
	editor.goto_xy(editor.where_x()*3,editor.where_y());
	flg = __ESS_WANTRETURN | __ESS_HARDEDIT | __ESS_ASHEX;
	if(!redraw) flg |= __ESS_NOREDRAW;
	unsigned edit_x = editor.where_x();
	_lastbyte = eeditstring(hexwnd,work,&legalchars[2],&mlen,(unsigned)(editor.where_y() + 1),(unsigned *)&edit_x,
			  flg,owork,NULL);
	edit_x/=3;
	editor.goto_xy(edit_x,editor.where_y());
	CompressHex(&emem.buff[eidx],work,mlen/3,true);
	switch(_lastbyte) {
	    case KE_F(1)   : editor.show_help(); continue;
	    case KE_F(2)   : editor.save_context();
	    case KE_F(10)  :
	    case KE_ESCAPE :
	    case KE_TAB : goto bye;
	    default     : redraw = editor.default_action(_lastbyte); break;
	}
	editor.CheckBounds();
	if(redraw) txtwnd->write(width - emem.width + 1,editor.where_y() + 1,&emem.buff[eidx],mlen/3);
	editor.paint_title(eidx + editor.where_x(),0);
    }
    bye:
    TWindow::set_cursor_type(TWindow::Cursor_Off);
    return _lastbyte;
}

unsigned __FASTCALL__ ReadIniAResolv(BeyeContext& bctx, Ini_Profile& ini )
{
    std::string tmps;
    tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode6","0");
    hexAddressResolv = (unsigned)::strtoul(tmps.c_str(),NULL,10);
    if(hexAddressResolv > 1) hexAddressResolv = 0;
    return hexAddressResolv;
}

void __FASTCALL__ WriteIniAResolv(BeyeContext& bctx, Ini_Profile& ini, unsigned har, unsigned virt_width_corr)
{
    char tmps[10];
    ::sprintf(tmps,"%i",har);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode6",tmps);
    ::sprintf(tmps,"%u",virt_width_corr);
    bctx.write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

void HexMode::read_ini(Ini_Profile& ini)
{
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	tmps=bctx.read_profile_string(ini,"Beye","Browser","LastSubMode","2");
	hmode = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(hmode > 3) hmode = 1;
	delete hex_viewer;
	hex_viewer = xView_Info[hmode].query_interface(main_handle);
	hexAddressResolv=ReadIniAResolv(bctx,ini);
	tmps=bctx.read_profile_string(ini,"Beye","Browser","VirtWidthCorr","0");
	virtWidthCorr = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	check_width_corr();
    }
}

void HexMode::save_ini(Ini_Profile&  ini)
{
    char tmps[10];
    ::sprintf(tmps,"%i",hmode);
    bctx.write_profile_string(ini,"Beye","Browser","LastSubMode",tmps);
    WriteIniAResolv(bctx,ini,hexAddressResolv,virtWidthCorr);
    ::sprintf(tmps,"%u",virtWidthCorr);
    bctx.write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

unsigned HexMode::get_symbol_size() const { return 1; }
unsigned HexMode::get_max_line_length() const { return hex_viewer->width(main_wnd,is_file64); }

static Plugin* query_interface(BeyeContext& bc,const Bin_Format& b,binary_stream& h,TWindow& main_wnd,CodeGuider& code_guider,udn& u,Search& s) { return new(zeromem) HexMode(bc,b,h,main_wnd,code_guider,u,s); }

extern const Plugin_Info hexMode = {
    "~Hexadecimal mode",	/**< plugin name */
    query_interface
};

} // namespace	usr
