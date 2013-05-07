#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye_plugins_I
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
#include "bmfile.h"
#include "bin_util.h"
#include "reg_form.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

#include "plugin.h"

namespace beye {
    class HexMode : public Plugin {
	public:
	    HexMode(CodeGuider& code_guider);
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
	    virtual unsigned		paint(unsigned keycode,unsigned textshift);

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
	    int			full_hex_edit(TWindow * txtwnd,TWindow *hexwnd);

	    CodeGuider&	code_guider;
	    unsigned	virtWidthCorr;
	    unsigned	hmode;

    };
unsigned	hexAddressResolv;
static unsigned hendian;

HexMode::HexMode(CodeGuider& _code_guider)
	:Plugin(_code_guider)
	,code_guider(_code_guider)
	,virtWidthCorr(0)
	,hmode(1)
{}
HexMode::~HexMode() {}

HexMode::e_flag HexMode::flags() const { return None; }

static const char* txt[] = { "", "HexMod", "Endian", "", "", "AResol", "<<<   ", "   >>>", "", "UsrNam" };
const char* HexMode::prompt(unsigned idx) const {
    return (idx < 10) ? txt[idx] : "";
}

typedef char *( __FASTCALL__ *hexFunc)(__filesize_t);
typedef unsigned char ( __FASTCALL__ *sizeFunc) ();

typedef struct tag_hexView {
    const char *  name;
    hexFunc       func;
    sizeFunc      width;
    unsigned char size;
    unsigned char hardlen;
}hexView;

static char *  __FASTCALL__ GetB(__filesize_t val) { return GetBinary(BMReadByteEx(val,BFile::Seek_Set)); }
static char *  __FASTCALL__ Get2D(__filesize_t val) { return Get2Digit(BMReadByteEx(val,BFile::Seek_Set)); }
static char *  __FASTCALL__ Get4D(__filesize_t val)
{
    unsigned short v;
    v = BMReadWordEx(val,BFile::Seek_Set);
    if(hendian==1) v=le2me_16(v);
    else
    if(hendian==2) v=be2me_16(v);
    return Get4Digit(v);
}
static char *  __FASTCALL__ Get8D(__filesize_t val)
{
    unsigned long v;
    v = BMReadDWordEx(val,BFile::Seek_Set);
    if(hendian==1) v=le2me_32(v);
    else
    if(hendian==2) v=be2me_32(v);
    return Get8Digit(v);
}

static unsigned char  __FASTCALL__ sizeBit()  { return (tvioWidth-HA_LEN())/(8+1+1); }
static unsigned char  __FASTCALL__ sizeByte() { return ((tvioWidth-HA_LEN())/(12+1+4)*4); } /* always round on four-column boundary */
static unsigned char  __FASTCALL__ sizeWord() { return (tvioWidth-HA_LEN())/(4+1+2); }
static unsigned char  __FASTCALL__ sizeDWord(){ return (tvioWidth-HA_LEN())/(8+1+4); }

static const hexView hexViewer[] =
{
  { "B~it",         GetB,   sizeBit,   1, 8 },
  { "~Byte",        Get2D,  sizeByte,  1, 2 },
  { "~Word",        Get4D,  sizeWord,  2, 4 },
  { "~Double word", Get8D,  sizeDWord, 4, 8 }
};

unsigned HexMode::paint( unsigned keycode,unsigned textshift )
{
    int i,I,Limit,dir;
    char outstr[__TVIO_MAXSCREENWIDTH+1];
    unsigned char HWidth;
    unsigned scrHWidth;
    __filesize_t sindex,cpos,flen,lindex,SIndex;
    static __filesize_t hmocpos = 0L;
    int __inc,dlen;
    cpos = BMGetCurrFilePos();
    if(hmocpos != cpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND) {
	tAbsCoord height = twGetClientHeight(MainWnd);
	tAbsCoord width = twGetClientWidth(MainWnd);
	twFreezeWin(MainWnd);
	HWidth = hexViewer[hmode].width()-virtWidthCorr;
	if(!(hmocpos == cpos + HWidth || hmocpos == cpos - HWidth)) keycode = KE_SUPERKEY;
	hmocpos = cpos;
	__inc = hexViewer[hmode].size;
	dlen = hexViewer[hmode].hardlen;
	flen = BMGetFLength();
	scrHWidth = HWidth*__inc;
	if(flen < HWidth) HWidth = flen;
	if(keycode == KE_UPARROW) {
	    I = height-1;
	    dir = -1;
	    Limit = -1;
	    if((__filesize_t)HWidth <= cpos) {
		twScrollWinDn(MainWnd,1,1);
		I = 0;
	    } else goto full_redraw;
	} else if(keycode == KE_DOWNARROW && flen >= HWidth) {
	    I = height-1;
	    dir = 1;
	    Limit = height;
	    twScrollWinUp(MainWnd,I,1);
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
		len = HA_LEN();
		::memcpy(outstr,code_guider.encode_address(sindex,hexAddressResolv),len);
		for(j = 0,freq = 0,lindex = sindex;j < rwidth;j++,lindex += __inc,freq++) {
		    ::memcpy(&outstr[len],hexViewer[hmode].func(lindex),dlen);
		    len += dlen + 1;
		    if(hmode == 1) if(freq == 3) { freq = -1; len++; }
		}
		BMReadBufferEx((any_t*)&outstr[width - scrHWidth],rwidth*__inc,sindex,BFile::Seek_Set);
		xmin = tvioWidth-scrHWidth;
		twDirectWrite(MainWnd,1,i + 1,outstr,xmin);
		if(isHOnLine(sindex,scrHWidth)) {
		    HLInfo hli;
		    hli.text = &outstr[xmin];
		    HiLightSearch(MainWnd,sindex,xmin,width,i,&hli,HLS_NORMAL);
		} else  twDirectWrite(MainWnd,xmin + 1,i + 1,&outstr[xmin],width - xmin);
	    } else twDirectWrite(MainWnd,1,i + 1,outstr,width);
	}
	lastbyte = lindex + __inc;
	twRefreshWin(MainWnd);
    }
    return textshift;
}

void HexMode::help() const
{
   hlpDisplay(1002);
}

unsigned long HexMode::prev_page_size() const { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size*twGetClientHeight(MainWnd); }
unsigned long HexMode::curr_page_size() const { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size*twGetClientHeight(MainWnd); }
unsigned long HexMode::prev_line_width() const { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size; }
unsigned long HexMode::curr_line_width() const { return (hexViewer[hmode].width()-virtWidthCorr)*hexViewer[hmode].size; }

const char* HexMode::misckey_name() const { return hmode == 1 ? "Modify" : "      "; }
void HexMode::misckey_action () /* EditHex */
{
    TWindow * ewnd[2];
    bool has_show[2];
    int active = 0,oactive = 0;
    unsigned bound;
    tAbsCoord width = twGetClientWidth(MainWnd);
    if(hmode != 1) return;
    if(!BMGetFLength()) { ErrMessageBox(NOTHING_EDIT,""); return; }
    bound = width-(hexViewer[hmode].width()-virtWidthCorr);
    ewnd[0] = WindowOpen(HA_LEN()+1,2,bound,tvioHeight-1,TWS_CURSORABLE);
    twSetColorAttr(ewnd[0],browser_cset.edit.main); twClearWin(ewnd[0]);
    ewnd[1] = WindowOpen(bound+1,2,width,tvioHeight-1,TWS_CURSORABLE);
    twSetColorAttr(ewnd[1],browser_cset.edit.main); twClearWin(ewnd[1]);
    drawEditPrompt();
    has_show[0] = has_show[1] = false;
    if(editInitBuffs(hexViewer[hmode].width()-virtWidthCorr,NULL,0)) {
	edit_x = edit_y = 0;
	while(1) {
	    int _lastbyte;
	    if(active != oactive) {
		twHideWin(ewnd[oactive]);
		if(has_show[active]) twShowWin(ewnd[active]);
		oactive = active;
	    }
	    twFocusWin(ewnd[active]);
	    if(!active) _lastbyte = full_hex_edit(MainWnd,ewnd[0]);
	    else        _lastbyte = FullEdit(ewnd[1],MainWnd,*this,NULL);
	    has_show[active] = true;
	    if(_lastbyte == KE_TAB) active = active ? 0 : 1;
	    else break;
	}
	editDestroyBuffs();
    }
    CloseWnd(ewnd[0]);
    CloseWnd(ewnd[1]);
    beye_context().PaintTitle();
}

void HexMode::check_width_corr()
{
  if(virtWidthCorr>(unsigned)hexViewer[hmode].width()-1) virtWidthCorr=hexViewer[hmode].width()-1;
}

bool HexMode::action_F2() /* hexSelectMode */
{
    const char *names[sizeof(hexViewer)/sizeof(hexView)];
    size_t i,nModes;
    int retval;
    nModes = sizeof(hexViewer)/sizeof(hexView);
    for(i = 0;i < nModes;i++) names[i] = hexViewer[i].name;
    retval = SelBoxA(names,nModes," Select hexadecimal mode: ",hmode);
    if(retval != -1) {
	hmode = retval;
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
    retval = SelBoxA(nendian,nModes," Select endian mode: ",hendian);
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
    i = SelBoxA(aresolv,nModes," Select address resolving: ",(unsigned)har);
    if(i != -1) {
	har = i ? true : false;
	return true;
    }
    return false;
}

bool HexMode::action_F6() { return hexAddressResolution(hexAddressResolv); }

bool HexMode::action_F7() /* hexDecVirtWidth */
{
    if(virtWidthCorr < (unsigned)hexViewer[hmode].width()-1) { virtWidthCorr++; return true; }
    return false;
}

bool HexMode::action_F8() /* hexIncVirtWidth */
{
    if(virtWidthCorr) { virtWidthCorr--; return true; }
    return false;
}

bool HexMode::action_F10() { return udnUserNames(); }

bool HexMode::detect() { return true; }

int HexMode::full_hex_edit(TWindow* txtwnd,TWindow* hexwnd)
{
    size_t i,j;
    unsigned mlen;
    unsigned int _lastbyte;
    unsigned flg;
    tAbsCoord height = twGetClientHeight(txtwnd);
    tAbsCoord width = twGetClientWidth(txtwnd);
    char work[__TVIO_MAXSCREENWIDTH],owork[__TVIO_MAXSCREENWIDTH];
    bool redraw;

    twFocusWin(txtwnd);
    twSetColorAttr(txtwnd,browser_cset.main);
    twFreezeWin(txtwnd);
    for(i = 0;i < height;i++) {
	twDirectWrite(txtwnd,width - EditorMem.width + 1,i + 1,&EditorMem.buff[i*EditorMem.width],EditorMem.alen[i]);
	if((unsigned)EditorMem.alen[i] + 1 < EditorMem.width) {
	    twGotoXY(txtwnd,width - EditorMem.width + EditorMem.alen[i] + 2,i + 1); twClrEOL(txtwnd);
        }
    }
    twRefreshWin(txtwnd);

    twSetColorAttr(hexwnd,browser_cset.edit.main);
    for(i = 0;i < height;i++) {
	unsigned eidx;
	eidx = i*EditorMem.width;
	ExpandHex(work,&EditorMem.buff[eidx],EditorMem.alen[i],1);
	mlen = ExpandHex(owork,&EditorMem.save[eidx],EditorMem.alen[i],1);
	for(j = 0;j < mlen;j++) {
	    twSetColorAttr(hexwnd,work[j] == owork[j] ? browser_cset.edit.main : browser_cset.edit.change);
	    twDirectWrite(hexwnd,j + 1,i + 1,&work[j],1);
	}
	if(mlen + 1 < EditorMem.width) {
	    twGotoXY(hexwnd,mlen + 1,i + 1);
	    twClrEOL(hexwnd);
	}
    }
    redraw = true;
    PaintETitle(edit_y*EditorMem.width + edit_x,0);
    twShowWin(hexwnd);
    twSetCursorType(TW_CUR_NORM);
    while(1) {
	unsigned eidx;
	mlen = EditorMem.alen[edit_y];
	eidx = edit_y*EditorMem.width;
	ExpandHex(work,&EditorMem.buff[eidx],mlen,1);
	mlen = ExpandHex(owork,&EditorMem.save[eidx],mlen,1);
	edit_x*=3;
	flg = __ESS_WANTRETURN | __ESS_HARDEDIT | __ESS_ASHEX;
	if(!redraw) flg |= __ESS_NOREDRAW;
	_lastbyte = eeditstring(hexwnd,work,&legalchars[2],&mlen,(unsigned)(edit_y + 1),(unsigned *)&edit_x,
			  flg,owork,NULL);
	edit_x/=3;
	CompressHex(&EditorMem.buff[eidx],work,mlen/3,true);
	switch(_lastbyte) {
	    case KE_F(1)   : ExtHelp(); continue;
	    case KE_F(2)   : editSaveContest();
	    case KE_F(10)  :
	    case KE_ESCAPE :
	    case KE_TAB : goto bye;
	    default     : redraw = editDefAction(_lastbyte); break;
	}
	CheckBounds();
	if(redraw) twDirectWrite(txtwnd,width - EditorMem.width + 1,edit_y + 1,&EditorMem.buff[eidx],mlen/3);
	PaintETitle(eidx + edit_x,0);
    }
    bye:
    twSetCursorType(TW_CUR_OFF);
    return _lastbyte;
}

unsigned __FASTCALL__ ReadIniAResolv( Ini_Profile& ini )
{
    std::string tmps;
    tmps=beye_context().read_profile_string(ini,"Beye","Browser","SubSubMode6","0");
    hexAddressResolv = (unsigned)::strtoul(tmps.c_str(),NULL,10);
    if(hexAddressResolv > 1) hexAddressResolv = 0;
    return hexAddressResolv;
}

void __FASTCALL__ WriteIniAResolv( Ini_Profile& ini, unsigned har, unsigned virt_width_corr)
{
    char tmps[10];
    ::sprintf(tmps,"%i",har);
    beye_context().write_profile_string(ini,"Beye","Browser","SubSubMode6",tmps);
    ::sprintf(tmps,"%u",virt_width_corr);
    beye_context().write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

void HexMode::read_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	tmps=bctx.read_profile_string(ini,"Beye","Browser","LastSubMode","2");
	hmode = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(hmode > 3) hmode = 1;
	hexAddressResolv=ReadIniAResolv(ini);
	tmps=bctx.read_profile_string(ini,"Beye","Browser","VirtWidthCorr","0");
	virtWidthCorr = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	check_width_corr();
    }
}

void HexMode::save_ini(Ini_Profile&  ini)
{
    BeyeContext& bctx = beye_context();
    char tmps[10];
    ::sprintf(tmps,"%i",hmode);
    bctx.write_profile_string(ini,"Beye","Browser","LastSubMode",tmps);
    WriteIniAResolv(ini,hexAddressResolv,virtWidthCorr);
    ::sprintf(tmps,"%u",virtWidthCorr);
    bctx.write_profile_string(ini,"Beye","Browser","VirtWidthCorr",tmps);
}

unsigned HexMode::get_symbol_size() const { return 1; }
unsigned HexMode::get_max_line_length() const { return hexViewer[hmode].width(); }

static Plugin* query_interface(CodeGuider& code_guider) { return new(zeromem) HexMode(code_guider); }

extern const Plugin_Info hexMode = {
    "~Hexadecimal mode",	/**< plugin name */
    query_interface
};

} // namespace beye
