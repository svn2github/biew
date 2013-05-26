#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_I
 * @file        plugins/disasm.c
 * @brief       This file contains universal interface for any disassembler.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        12.09.2000
 * @note        Adding virtual address as argument of jump and call insns
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Reworked inline assemblers support
**/
#include <algorithm>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "beye.h"
#include "colorset.h"
#include "udn.h"
#include "plugins/hexmode.h"
#include "plugins/disasm.h"
#include "beyeutil.h"
#include "reg_form.h"
#include "bconsole.h"
#include "editor.h"
#include "codeguid.h"
#include "search.h"
#include "tstrings.h"
#include "libbeye/file_ini.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/kbd_code.h"

#include "plugin.h"

namespace	usr {
    extern const Disassembler_Info ix86_disassembler_info;
    extern const Disassembler_Info data_disassembler_info;
    extern const Disassembler_Info avr_disassembler_info;
    extern const Disassembler_Info arm_disassembler_info;
    extern const Disassembler_Info ppc_disassembler_info;
    extern const Disassembler_Info java_disassembler_info;

DisMode::DisMode(const Bin_Format& b,binary_stream& h,TWindow& _main_wnd,CodeGuider& _code_guider,udn& u)
	:Plugin(b,h,_main_wnd,_code_guider,u)
	,DefDisasmSel(__DEFAULT_DISASM)
	,HiLight(1)
	,code_guider(_code_guider)
	,DisasmPrepareMode(false)
	,main_wnd(_main_wnd)
	,main_handle(h)
	,second_handle(&h)
	,bin_format(b)
	,_udn(u)
{
    size_t i,sz;
    unsigned def_platform;
    list.push_back(&data_disassembler_info);
    list.push_back(&ix86_disassembler_info);
    list.push_back(&java_disassembler_info);
    list.push_back(&avr_disassembler_info);
    list.push_back(&arm_disassembler_info);
    list.push_back(&ppc_disassembler_info);
    CurrStrLenBuff = new unsigned char [beye_context().tconsole().vio_height()];
    PrevStrLenAddr = new unsigned long [beye_context().tconsole().vio_height()];
    dis_comments   = new char [Comm_Size];
    second_handle = main_handle.dup();

    def_platform = DISASM_DATA;
    def_platform = bin_format.query_platform();
    sz=list.size();
    for(i=0;i<sz;i++) {
	if(list[i]->type == def_platform) {
	    activeDisasm=list[i]->query_interface(bin_format,*second_handle,*this);
	    DefDisasmSel = i;
	    break;
	}
    }
    if(!activeDisasm) {
	activeDisasm = list[0]->query_interface(bin_format,*second_handle,*this);
	DefDisasmSel = 0;
    }
    accept_actions();
}

DisMode::~DisMode()
{
    delete activeDisasm;
    delete CurrStrLenBuff;
    delete PrevStrLenAddr;
    delete dis_comments;
    delete disCodeBuffer;
    delete disCodeBufPredict;
    if(second_handle!=&main_handle) delete second_handle;
}

DisMode::e_flag DisMode::flags() const { return UseCodeGuide | Disasm | Has_SearchEngine; }

static const char* txt[] = { "", "Disasm", "", "", "", "AResol", "PanMod", "ResRef", "HiLght", "UsrNam" };
const char* DisMode::prompt(unsigned idx) const {
    if(activeDisasm && idx!=1 && idx<5) {
	if(!idx) return activeDisasm->prompt(idx);
	return activeDisasm->prompt(idx-1);
    }
    return (idx < 10) ? txt[idx] : "";
}

bool DisMode::action_F2() /* disSelect_Disasm */
{
    size_t i,nModes=list.size();
    const char *modeName[nModes];
    int retval;

    for(i = 0;i < nModes;i++) modeName[i] = list[i]->name;
    retval = ListBox(modeName,nModes," Select disassembler: ",LB_SELECTIVE|LB_USEACC,DefDisasmSel);
    if(retval != -1) {
	delete activeDisasm;
	activeDisasm = list[retval]->query_interface(bin_format,*second_handle,*this);
	DefDisasmSel = retval;
	accept_actions();
	return true;
    }
    return false;
}

void DisMode::fill_prev_asm_page(__filesize_t bound,unsigned predist)
{
    __filesize_t distin,addr;
    unsigned j;
    unsigned totallen;
    tAbsCoord height = main_wnd.client_height();
    char addrdet;
    e_ref showref;
    if(!predist) predist = height*disMaxCodeLen;
    predist = (predist/16)*16+16;   /** align on 16-byte boundary */
    distin = bound >= predist ? bound - predist : 0;
    memset(PrevStrLenAddr,TWC_DEF_FILLER,height*sizeof(long));
    PrevStrCount = 0;
    totallen = 0;
    showref = disNeedRef;
    addrdet = hexAddressResolv;
    hexAddressResolv = 0;
    disNeedRef = Ref_None;
    DisasmPrepareMode = true;
    for(j = 0;;j++) {
	DisasmRet dret;
	addr = distin + totallen;
	main_handle.seek(addr,binary_stream::Seek_Set);
	main_handle.read(disCodeBuffer,disMaxCodeLen);
	dret = disassembler(distin,(unsigned char*)disCodeBuffer,__DISF_SIZEONLY);
	if(addr >= bound) break;
	totallen += dret.codelen;
	if(j < height) PrevStrLenAddr[j] = addr;
	else {
	    ::memmove(PrevStrLenAddr,&PrevStrLenAddr[1],(height - 1)*sizeof(long));
	    PrevStrLenAddr[height - 1] = addr;
	}
    }
    PrevStrCount = j < height ? j : height;
    LastPrevLen = PrevStrCount ? bound - PrevStrLenAddr[PrevStrCount - 1] : 0;
    disNeedRef = showref;
    DisasmPrepareMode = false;
    hexAddressResolv = addrdet;
}

void DisMode::prepare_asm_lines(int keycode,__filesize_t cfpos)
{
    tAbsCoord height = main_wnd.client_height();
    switch(keycode) {
	case KE_DOWNARROW:
		PrevStrLen = CurrStrLenBuff[0];
		CurrStrLen = CurrStrLenBuff[1];
		if((unsigned)PrevStrCount < height) PrevStrCount++;
		else                                ::memmove(PrevStrLenAddr,&PrevStrLenAddr[1],sizeof(long)*(height - 1));
		PrevStrLenAddr[PrevStrCount - 1] = cfpos - CurrStrLenBuff[0];
		PrevPageSize = PrevStrLenAddr[PrevStrCount - 1] - PrevStrLenAddr[0] + PrevStrLen;
		break;
	case KE_PGDN:
		{
		    size_t i;
		    unsigned size;
		    __filesize_t prevpos;
		    size = Summ(CurrStrLenBuff,height);
		    prevpos = cfpos - size;
		    size = 0;
		    for(i = 0;i < height;i++) {
			size += CurrStrLenBuff[i];
			PrevStrLenAddr[i] = prevpos + size;
		    }
		    PrevStrLen = CurrStrLenBuff[height - 1];
		    PrevPageSize = size;
		    PrevStrCount = height;
		}
		break;
	case KE_UPARROW:
		fill_prev_asm_page(cfpos,(unsigned)(PrevPageSize + 15));
		goto Calc;
	default:
		fill_prev_asm_page(cfpos,0);
	Calc:
		if(PrevStrCount) {
		    PrevStrLen = LastPrevLen;
		    PrevPageSize = PrevStrLenAddr[PrevStrCount - 1] + LastPrevLen - PrevStrLenAddr[0];
		} else {
		    PrevStrLen = 0;
		    PrevPageSize = 0;
		}
		break;
    }
}

unsigned DisMode::paint( unsigned keycode, unsigned textshift )
{
    int i,I,Limit,dir,orig_commpos,orig_commoff;
    size_t j,len,len_64;
    __filesize_t cfpos,flen,TopCFPos;
    static __filesize_t amocpos = 0L;
    uint8_t outstr[__TVIO_MAXSCREENWIDTH];
    uint8_t savstring[20];
    ColorAttr cattr;
    flen = main_handle.flength();
    cfpos = TopCFPos = main_handle.tell();
    if(keycode == KE_UPARROW) {
	char addrdet;
	e_ref showref;
	DisasmRet dret;
	showref = disNeedRef;
	addrdet = hexAddressResolv;
	disNeedRef = Ref_None; hexAddressResolv = 0;
	main_handle.seek(cfpos,binary_stream::Seek_Set);
	main_handle.read(disCodeBuffer,disMaxCodeLen);
	DisasmPrepareMode = true;
	dret = disassembler(cfpos,(unsigned char*)disCodeBuffer,__DISF_SIZEONLY);
	if(cfpos + dret.codelen != amocpos && cfpos && amocpos) keycode = KE_SUPERKEY;
	DisasmPrepareMode = false;
	disNeedRef = showref;
	hexAddressResolv = addrdet;
    }
    prepare_asm_lines(keycode,cfpos);
    if(amocpos != cfpos || keycode == KE_SUPERKEY || keycode == KE_JUSTFIND) {
	tAbsCoord height = main_wnd.client_height();
	tAbsCoord width = main_wnd.client_width();
	code_guider.reset_go_address(keycode);
	I = 0;
	main_wnd.freeze();
	if(keycode == KE_UPARROW) {
	    dir = -1;
	    Limit = -1;
	    /* All checks we have done above */
	    main_wnd.scroll_down(1,1);
	    ::memmove(&CurrStrLenBuff[1],CurrStrLenBuff,height-1);
	} else {
	    dir = 1;
	    Limit = height;
	}
	if(keycode == KE_DOWNARROW) {
	    if(CurrStrLenBuff[1]) {
		I = height-1;
		main_wnd.scroll_up(I,1);
		::memmove(CurrStrLenBuff,&CurrStrLenBuff[1],I);
		cfpos += Summ(CurrStrLenBuff,I);
	    } else {
		main_wnd.refresh();
		goto bye;
	    }
	}
	if(cfpos > flen) cfpos = flen;
	amocpos = cfpos;
	for(i = I;i != Limit;i+=1*dir) {
	    DisasmRet dret;
	    ::memset(outstr,TWC_DEF_FILLER,__TVIO_MAXSCREENWIDTH);
	    DisasmCurrLine = i;
	    if(cfpos < flen) {
		len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
		::memset(disCodeBuffer,0,disMaxCodeLen);
		main_handle.seek(cfpos,binary_stream::Seek_Set);
		main_handle.read((any_t*)disCodeBuffer,len);
		dret = disassembler(cfpos,(unsigned char*)disCodeBuffer,__DISF_NORMAL);
		if(i == 0) CurrStrLen = dret.codelen;
		CurrStrLenBuff[i] = dret.codelen;
		main_wnd.set_color(browser_cset.main);
		len_64=HA_LEN();
		::memcpy(outstr,code_guider.encode_address(cfpos,hexAddressResolv).c_str(),len_64);
		len = 0;
		if(disPanelMode < Panel_Full) {
		    static uint8_t _clone;
		    len = len_64;
		    main_wnd.write(1,i + 1,outstr,len);
		    if(!hexAddressResolv) {
			main_wnd.set_color(disasm_cset.family_id);
			_clone = activeDisasm->clone_short_name(dret.pro_clone);
			main_wnd.write(len_64,i + 1,&_clone,1);
			main_wnd.set_color(browser_cset.main);
		    }
		}
		if(disPanelMode < Panel_Medium) {
		    unsigned full_off,med_off,tmp_off;
		    ColorAttr opc;
		    med_off = disMaxCodeLen*2+1;
		    full_off = med_off+len_64;
		    for(j = 0;j < dret.codelen;j++,len+=2) ::memcpy(&outstr[len],Get2Digit(disCodeBuffer[j]),2);
		    tmp_off = disPanelMode < Panel_Full ? full_off : med_off;
		    if(len < tmp_off) len = tmp_off;
		    opc =HiLight == 2 ? activeDisasm->get_alt_opcode_color(dret.pro_clone) :
			 HiLight == 1 ? activeDisasm->get_opcode_color(dret.pro_clone) : disasm_cset.opcodes;
		    main_wnd.set_color(opc);
		    main_wnd.write(disPanelMode < Panel_Full ? len_64+1 : 1,
				i + 1,
				&outstr[len_64],
				disPanelMode < Panel_Full ? len - (len_64+1) : len - 1);
		    if(isHOnLine(cfpos,dret.codelen)) HiLightSearch(main_wnd,cfpos,len_64,dret.codelen,i,(const char*)&outstr[len_64],HLS_USE_DOUBLE_WIDTH);
		}
		main_wnd.set_color(browser_cset.main);
		main_wnd.write(len,i + 1,(const uint8_t*)" ",1);  len++;
		cattr =	HiLight == 2 ?  activeDisasm->get_alt_insn_color(dret.pro_clone) :
			HiLight == 1 ?  activeDisasm->get_insn_color(dret.pro_clone) :
					browser_cset.main;
		main_wnd.set_color(cattr);
		j = strlen(dret.str);
		/* Here adding commentaries */
		savstring[0] = 0;
		orig_commoff = orig_commpos = 0;
		const char*codeguid_image = code_guider.image();
		if(j > 5)
		    if(dret.str[j-5] == codeguid_image[0] &&
			dret.str[j-4] == codeguid_image[1] &&
			dret.str[j-3] == codeguid_image[2] &&
			dret.str[j-1] == codeguid_image[4] &&
			dis_severity > DisMode::CommSev_None) {
			    int new_idx;
			    orig_commpos = new_idx = j-5;
			    orig_commoff = len;
			    ::strcpy((char*)savstring,&dret.str[new_idx]);
			    dret.str[new_idx--] = 0;
			    while(dret.str[new_idx] == ' ' && new_idx) new_idx--;
			    if(dret.str[new_idx] != ' ') new_idx++;
			    dret.str[new_idx] = 0;
			    j = ::strlen(dret.str);
		    }
		main_wnd.write(len,i+1,(const uint8_t*)dret.str,j); len += j;
		if(dis_severity > DisMode::CommSev_None) {
		    main_wnd.set_color(disasm_cset.comments);
		    main_wnd.goto_xy(len,i+1);
		    main_wnd.puts(" ; "); len+=3;
		    j = orig_commpos-len;
		    j = std::min(j,strlen(dis_comments));
		    main_wnd.write(len,i+1,(const uint8_t*)dis_comments,j);
		    len += j;
		    if(savstring[0]) {
			main_wnd.goto_xy(len,i+1);
			main_wnd.clreol();
			main_wnd.set_color(cattr);
			len = orig_commoff + orig_commpos;
			main_wnd.write(len,i+1,savstring,5);
			len += 5;
		    }
		}
		main_wnd.set_color(browser_cset.main);
		if(len < width) {
		    main_wnd.goto_xy(len,i + 1);
		    main_wnd.clreol();
		}
		cfpos += dret.codelen;
		main_handle.seek(cfpos,binary_stream::Seek_Set);
	    } else {
		main_wnd.write(1,i + 1,outstr,width);
		CurrStrLenBuff[i] = 0;
	    }
	}
	main_wnd.refresh();
	main_wnd.set_color(browser_cset.main);
	lastbyte = TopCFPos + Summ(CurrStrLenBuff,height);
	CurrPageSize = lastbyte-TopCFPos;
    }
    bye:
    return textshift;
}

bool DisMode::action_F1() { return activeDisasm->action_F1(); }
bool DisMode::action_F3() { return activeDisasm->action_F3(); }
bool DisMode::action_F4() { return activeDisasm->action_F4(); }
bool DisMode::action_F5() { return activeDisasm->action_F5(); }

unsigned long DisMode::prev_page_size() const { return PrevPageSize; }
unsigned long DisMode::curr_page_size() const { return CurrPageSize; }
unsigned long DisMode::prev_line_width() const { return PrevStrLen; }
unsigned long DisMode::curr_line_width() const { return CurrStrLen; }
const char*   DisMode::misckey_name() const { return "Modify"; }

void DisMode::misckey_action() /* disEdit */
{
    unsigned len_64;
    TWindow * ewnd;
    len_64=HA_LEN();
    if(!main_handle.flength()) { beye_context().ErrMessageBox(NOTHING_EDIT,""); return; }
    ewnd = new(zeromem) TWindow(len_64+1,2,disMaxCodeLen*2+1,beye_context().tconsole().vio_height()-2,TWindow::Flag_Has_Cursor);
    ewnd->set_color(browser_cset.edit.main); ewnd->clear();
    edit_x = edit_y = 0;
    EditMode = EditMode ? false : true;
    if(editInitBuffs(disMaxCodeLen,NULL,0)) {
	full_asm_edit(ewnd);
	editDestroyBuffs();
    }
    EditMode = EditMode ? false : true;
    delete ewnd;
    beye_context().PaintTitle();
}

static const char *refsdepth_names[] =
{
   "~None",
   "~Calls/jmps only (navigation)",
   "~All",
   "Reference ~prediction (mostly full)"
};

bool DisMode::action_F8() /* disReferenceResolving */
{
    size_t nModes;
    int retval;
    bool ret;
    nModes = sizeof(refsdepth_names)/sizeof(char *);
    retval = ListBox(refsdepth_names,nModes," Reference resolving depth: ",LB_SELECTIVE|LB_USEACC,disNeedRef);
    if(retval != -1) {
	disNeedRef = e_ref(retval);
	ret = true;
    } else ret = false;
    return ret;
}

static const char *panmod_names[] =
{
   "~Full",
   "~Medium",
   "~Wide"
};

bool DisMode::action_F7() /* disSelectPanelMode */
{
    unsigned nModes;
    int i;
    nModes = sizeof(panmod_names)/sizeof(char *);
    i = ListBox(panmod_names,nModes," Select panel mode: ",LB_SELECTIVE|LB_USEACC,disPanelMode);
    if(i != -1) {
	disPanelMode = e_panel(i);
	return true;
    }
    return false;
}

static const char *hilight_names[] =
{
   "~Mono",
   "~Highlight",
   "~Alt Highlight"
};

bool DisMode::action_F9() /* disSelectHiLight */
{
    unsigned nModes;
    int i;
    nModes = sizeof(hilight_names)/sizeof(char *);
    i = ListBox(hilight_names,nModes," Select highlight mode: ",LB_SELECTIVE|LB_USEACC,HiLight);
    if(i != -1) {
	HiLight = i;
	return true;
    }
    return false;
}

bool DisMode::action_F10() { return _udn.names(); }

bool DisMode::detect() { return true; }

bool DisMode::def_asm_action(int _lastbyte,int start) const
{
    int _index;
    bool redraw,dox;
    char xx;
    redraw = true;
    xx = edit_x / 2;
    dox = true;
    _index = start + xx;
    switch(_lastbyte) {
	case KE_F(4)   : EditorMem.buff[_index] = ~EditorMem.buff[_index]; break;
	case KE_F(5)   : EditorMem.buff[_index] |= edit_XX; break;
	case KE_F(6)   : EditorMem.buff[_index] &= edit_XX; break;
	case KE_F(7)   : EditorMem.buff[_index] ^= edit_XX; break;
	case KE_F(8)   : EditorMem.buff[_index]  = edit_XX; break;
	case KE_F(9)   : EditorMem.buff[_index] = EditorMem.save[_index]; break;
	default        : redraw = edit_defaction(_lastbyte); dox = false; break;
    }
    if(dox) { xx++; edit_x = xx*2; }
    return redraw;
}


void DisMode::disasm_screen(TWindow* ewnd,__filesize_t cp,__filesize_t flen,int st,int stop,int start)
{
    int i,j,len,lim,len_64;
    uint8_t outstr[__TVIO_MAXSCREENWIDTH+1],outstr1[__TVIO_MAXSCREENWIDTH+1];
    tAbsCoord width = main_wnd.client_width();
    DisasmRet dret;
    ewnd->freeze();
    for(i = st;i < stop;i++) {
	if(start + cp < flen) {
	    len_64=HA_LEN();
	    ::memcpy(outstr,code_guider.encode_address(cp + start,hexAddressResolv).c_str(),len_64);
	    main_wnd.set_color(browser_cset.main);
	    main_wnd.write(1,i + 1,outstr,len_64-1);
	    dret = disassembler(cp + start,&EditorMem.buff[start],__DISF_NORMAL);
	    EditorMem.alen[i] = dret.codelen;
	    ::memset(outstr,TWC_DEF_FILLER,width);
	    ::memset(outstr1,TWC_DEF_FILLER,width);
	    len = 0; for(j = 0;j < EditorMem.alen[i];j++) { ::memcpy(&outstr1[len],Get2Digit(EditorMem.save[start + j]),2); len += 2; }
	    len = 0; for(j = 0;j < EditorMem.alen[i];j++) { ::memcpy(&outstr[len],Get2Digit(EditorMem.buff[start + j]),2); len += 2; }
	    ewnd->set_focus();
	    len = disMaxCodeLen*2;
	    for(j = 0;j < len;j++) {
		ewnd->set_color(outstr[j] == outstr1[j] ? browser_cset.edit.main : browser_cset.edit.change);
		ewnd->write(j + 1,i + 1,&outstr[j],1);
	    }
	    len = ::strlen(dret.str);
	    ::memset(outstr,TWC_DEF_FILLER,width);
	    ::memcpy(outstr,dret.str,len);
	    main_wnd.set_color(browser_cset.main);
	    lim = disMaxCodeLen*2+len_64+1;
	    main_wnd.write(lim+1,i + 1,outstr,width-lim);
	    start += EditorMem.alen[i];
	} else {
	    main_wnd.goto_xy(1,i + 1);
	    main_wnd.clreol();
	    ewnd->set_focus();
	    ewnd->goto_xy(1,i + 1);
	    ewnd->clreol();
	    EditorMem.alen[i] = 0;
	}
    }
    ewnd->refresh();
}

int DisMode::full_asm_edit(TWindow * ewnd)
{
    int j,_lastbyte,start;
    unsigned rlen,len,flg;
    __filesize_t flen;
    unsigned max_buff_size = disMaxCodeLen*beye_context().tconsole().vio_height();
    tAbsCoord height = main_wnd.client_height();
    bool redraw = false;
    char outstr[__TVIO_MAXSCREENWIDTH],owork[__TVIO_MAXSCREENWIDTH];

    flen = main_handle.flength();
    edit_cp = main_handle.tell();
    start = 0;

    rlen = (__filesize_t)edit_cp + max_buff_size < flen ? max_buff_size : (unsigned)(flen - edit_cp);
    main_handle.seek(edit_cp,binary_stream::Seek_Set);
    main_handle.read((any_t*)EditorMem.buff,rlen);
    ::memcpy(EditorMem.save,EditorMem.buff,max_buff_size);
    ::memset(EditorMem.alen,TWC_DEF_FILLER,height);

    disasm_screen(ewnd,edit_cp,flen,0,height,start);
    beye_context().paint_Etitle(0,true);
    start = 0;
    ewnd->show();
    TWindow::set_cursor_type(TWindow::Cursor_Normal);
    while(1) {
	ewnd->set_focus();

	len = 0; for(j = 0;j < EditorMem.alen[edit_y];j++) { ::memcpy(&owork[len],Get2Digit(EditorMem.save[start + j]),2); len += 2; }
	len = 0; for(j = 0;j < EditorMem.alen[edit_y];j++) { ::memcpy(&outstr[len],Get2Digit(EditorMem.buff[start + j]),2); len += 2; }
	flg = __ESS_FILLER_7BIT | __ESS_WANTRETURN | __ESS_HARDEDIT;
	if(!redraw) flg |= __ESS_NOREDRAW;
	_lastbyte = eeditstring(ewnd,outstr,&legalchars[2],&len,(unsigned)(edit_y + 1),(unsigned *)&edit_x,flg,owork,NULL);
	CompressHex(&EditorMem.buff[start],outstr,EditorMem.alen[edit_y],false);
	switch(_lastbyte) {
	    case KE_CTL_F(4):
		{
		    AsmRet aret;
		    char code[81];
		    code[0]='\0';
		    if(GetStringDlg(code,list[DefDisasmSel]->name,
					 NULL,"Enter assembler instruction:")) {
			aret = activeDisasm->assembler(code);
			if(aret.err_code) {
			    const char *message="Syntax error";
			    if (aret.insn[0]) {
				message=(const char*)aret.insn;
			    }
			    beye_context().ErrMessageBox(message,"");
			    continue;
			} else {
			    int i;
			    char bytebuffer[3];

			    for (i=aret.insn_len-1; i>=0; i--) {
				sprintf(bytebuffer, "%02x", aret.insn[i]);
				ungotstring(bytebuffer);
			    }
			}
		    }
		    break;
		}
	    case KE_F(1)    : ExtHelp(); continue;
	    case KE_CTL_F(1): activeDisasm->action_F1(); continue;
	    case KE_CTL_F(2): beye_context().select_sysinfo(); continue;
	    case KE_CTL_F(3): beye_context().select_tool(); continue;
	    case KE_F(2)    :
		{
		    std::ofstream fs;
		    std::string fname;
		    fname = main_handle.filename();
		    fs.open(fname.c_str(),std::ios_base::binary);
		    if(fs.is_open()) {
			fs.seekp(edit_cp,std::ios_base::beg);
			if(!fs.write((const char*)EditorMem.buff,rlen))
			    beye_context().errnoMessageBox(WRITE_FAIL,"",errno);
			fs.close();
			main_handle.reread();
		    } else beye_context().errnoMessageBox("Can't reopen","",errno);
		}
	    case KE_F(10):
	    case KE_ESCAPE: goto bye;
	    default: redraw = def_asm_action(_lastbyte,start); break;
	}
	CheckYBounds();
	start = edit_y ? Summ(EditorMem.alen,edit_y) : 0;
	if(redraw) {
	    DisasmRet dret;
	    dret = disassembler(edit_cp + start,&EditorMem.buff[start],__DISF_NORMAL);
	    EditorMem.alen[edit_y] = dret.codelen;
	    disasm_screen(ewnd,edit_cp,flen,0,height,0);
	}
	beye_context().paint_Etitle(start + edit_x/2,true);
	CheckXYBounds();
	start = edit_y ? Summ(EditorMem.alen,edit_y) : 0;
    }
    bye:
    TWindow::set_cursor_type(TWindow::Cursor_Off);
    return _lastbyte;
}

void DisMode::help() const
{
   activeDisasm->show_short_help();
}

void DisMode::read_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	tmps=bctx.read_profile_string(ini,"Beye","Browser","LastSubMode","0");
	DefDisasmSel = (int)::strtoul(tmps.c_str(),NULL,10);
	if(DefDisasmSel >= list.size()) DefDisasmSel = 0;
	hexAddressResolv=ReadIniAResolv(ini);
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode7","0");
	disPanelMode = e_panel((int)::strtoul(tmps.c_str(),NULL,10));
	if(disPanelMode > Panel_Full) disPanelMode = Panel_Wide;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode8","0");
	disNeedRef = e_ref((int)::strtoul(tmps.c_str(),NULL,10));
	if(disNeedRef > Ref_Predict) disNeedRef = Ref_None;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode9","0");
	HiLight = (int)strtoul(tmps.c_str(),NULL,10);
	if(HiLight > 2) HiLight = 2;
	if(activeDisasm) delete activeDisasm;
	activeDisasm = list[DefDisasmSel]->query_interface(bin_format,*second_handle,*this);
	accept_actions();
	activeDisasm->read_ini(ini);
    }
}

void DisMode::save_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    char tmps[10];
    ::sprintf(tmps,"%i",DefDisasmSel);
    bctx.write_profile_string(ini,"Beye","Browser","LastSubMode",tmps);
    WriteIniAResolv(ini,hexAddressResolv,1);
    ::sprintf(tmps,"%i",disPanelMode);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode7",tmps);
    ::sprintf(tmps,"%i",disNeedRef);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode8",tmps);
    ::sprintf(tmps,"%i",HiLight);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode9",tmps);
    activeDisasm->save_ini(ini);
}

DisasmRet DisMode::disassembler(__filesize_t ulShift,MBuffer buffer,unsigned flg)
{
    dis_comments[0] = 0;
    dis_severity = DisMode::CommSev_None;
    return activeDisasm->disassembler(ulShift,buffer,flg);
}

unsigned DisMode::get_symbol_size() const { return 1; }
unsigned DisMode::get_max_symbol_size() const { return activeDisasm->max_insn_len(); }

__filesize_t DisMode::search_engine(TWindow *pwnd, __filesize_t start,
					__filesize_t *slen, unsigned flg,
					bool is_continue, bool *is_found)
{
    DisasmRet dret;
    __filesize_t tsize, retval, flen, dfpos, cfpos, sfpos; /* If search have no result */
    char *disSearchBuff;
    unsigned len, lw, proc, pproc, pmult;
    int cache[UCHAR_MAX+1];
    cfpos = sfpos = main_handle.tell();
    flen = main_handle.flength();
    retval = FILESIZE_MAX;
    disSearchBuff  = new char [1002+Comm_Size];
    DumpMode = true;
    if(!disSearchBuff) {
	MemOutBox("Disassembler search initialization");
	goto bye;
    }
    cfpos = start;
    tsize = flen;
    pmult = 100;
    if(tsize > FILESIZE_MAX/100) { tsize /= 100; pmult = 1; }
    pproc = proc = 0;
    /* Attempt to balance disassembler output */
    prepare_asm_lines(KE_SUPERKEY, cfpos);
    lw = prev_line_width();
    if(cfpos && cfpos >= lw) cfpos -= lw;
    if(!(is_continue && (flg & SF_REVERSE))) cfpos += curr_line_width();
    /* end of attempt */
    fillBoyerMooreCache(cache, (char*)search_buff, search_len, flg & SF_CASESENS);
    while(1) {
	proc = (unsigned)((cfpos*pmult)/tsize);
	if(proc != pproc) {
	    if(!ShowPercentInWnd(pwnd,pproc=proc))  break;
	}
	if(flg & SF_REVERSE) {
	    prepare_asm_lines(KE_UPARROW, cfpos);
	    lw = prev_line_width();
	    if(cfpos && lw && cfpos >= lw) {
		len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
		::memset(disCodeBuffer,0,disMaxCodeLen);
		dfpos = cfpos;
		main_handle.seek(cfpos,binary_stream::Seek_Set);
		main_handle.read((any_t*)disCodeBuffer,len);
		dret = disassembler(cfpos,(unsigned char*)disCodeBuffer,__DISF_NORMAL);
		cfpos -= lw;
	    } else break;
	} else {
	    len = cfpos + disMaxCodeLen < flen ? disMaxCodeLen : (int)(flen - cfpos);
	    ::memset(disCodeBuffer,0,disMaxCodeLen);
	    dfpos = cfpos;
	    main_handle.seek(cfpos,binary_stream::Seek_Set);
	    main_handle.read((any_t*)disCodeBuffer,len);
	    dret = disassembler(cfpos,(unsigned char*)disCodeBuffer,__DISF_NORMAL);
	    cfpos += dret.codelen;
	    if(cfpos >= flen) break;
	}
	::strcpy(disSearchBuff, dret.str);
	::strcat(disSearchBuff, dis_comments);
	if(strFind(disSearchBuff, strlen(disSearchBuff), search_buff, search_len, cache, flg)) {
	    *is_found = true;
	    retval = dfpos;
	    *slen = dret.codelen;
	    break;
	}
    }
    delete disSearchBuff;
    bye:
    main_handle.seek(sfpos, binary_stream::Seek_Set);
    DumpMode = false;
    return retval;
}

void DisMode::accept_actions()
{
    disMaxCodeLen = activeDisasm->max_insn_len();
    if(disCodeBuffer) delete disCodeBuffer;
    disCodeBuffer = new char [disMaxCodeLen];
    if(disCodeBufPredict) delete disCodeBufPredict;
    disCodeBufPredict = new char [disMaxCodeLen*PREDICT_DEPTH];
}

/** Common disassembler utility */

char * __FASTCALL__ TabSpace(char * str,unsigned nSpace)
{
  int i,mx;
  unsigned len;
  len = strlen(str);
  mx = std::max(len,nSpace);
  for(i = len;i < mx;i++) str[i] = TWC_DEF_FILLER;
  if(len >= nSpace) str[i++] = TWC_DEF_FILLER;
  str[i] = 0;
  return str;
}

void  __FASTCALL__ disSetModifier(char *str,const char *modf)
{
  unsigned i,len,mlen;
  i = 0;
  len = strlen(str);
  mlen = strlen(modf);
  while(str[i] != TWC_DEF_FILLER) i++;
  i++;
  memcpy(&str[i],modf,mlen);
  if(i+mlen > len) { str[i+mlen] = TWC_DEF_FILLER; str[i+mlen+1] = 0; }
}

bool DisMode::append_digits(binary_stream& handle,std::string& str,__filesize_t ulShift,int flg,char codelen,any_t*defval,e_disarg type)
{
 bool app;
 char comments[Comm_Size];
 const char *appstr;
 unsigned dig_type;
 __filesize_t fpos;
 fpos = handle.tell();
#ifndef NDEBUG
  if(ulShift >= main_handle.flength()-codelen)
  {
     char sout[75];
     static bool displayed = false;
     if(!displayed)
     {
       strncpy(sout,str.c_str(),sizeof(sout)-1);
       sout[sizeof(sout)-1] = 0;
       if(!strlen(sout)) strcpy(sout,"disAppendDigits");
       beye_context().ErrMessageBox(sout," Internal disassembler error detected ");
       displayed = true;
       COREDUMP();
     }
  }
#endif
  if(hexAddressResolv) flg |= APREF_SAVE_VIRT;
  app = disNeedRef >= Ref_All ? bin_format.bind(*this,str,ulShift,flg,codelen,0L) : false;
  if(app != true)
  {
    dig_type = type & 0x00FFU;
    comments[0] = 0;
    /* @todo Remove dependencies from 4-byte size of operand */
					 /* Only if immediate operand */
    if(((type & Arg_Imm) || (type & Arg_Disp) ||
	(type & Arg_IdxDisp) || (type & Arg_Rip)) &&
	disNeedRef >= Ref_Predict)    /* Only when reference prediction is on */
    {
      uint64_t _defval;
      unsigned fld_len=1;
      switch(dig_type)
      {
	default:
	case Arg_Byte: _defval = *(uint8_t *)defval;  fld_len=1; break;
	case Arg_Char: _defval = *(int8_t *)defval;   fld_len=1; break;
	case Arg_Word: _defval = *(uint16_t *)defval; fld_len=2; break;
	case Arg_Short:_defval = *(int16_t *)defval;  fld_len=2; break;
	case Arg_DWord:_defval = *(uint32_t *)defval; fld_len=4; break;
	case Arg_Long: _defval = *(int32_t *)defval;  fld_len=4; break;
	case Arg_QWord:_defval = *(uint64_t *)defval; fld_len=8; break;
	case Arg_LLong:_defval = *(int64_t *)defval;  fld_len=8; break;
      }
      if(_defval)         /* Do not perform operation on NULL */
      {
      __filesize_t pa,psym,__tmp;
      unsigned _class;
      if(type & Arg_Rip) {
	__tmp=bin_format.pa2va(ulShift);
	_defval += ((__tmp!=Plugin::Bad_Address) ? __tmp : ulShift)+fld_len;
      }
      pa = bin_format.va2pa(_defval);
      if(pa!=Plugin::Bad_Address)
      {
	/* 1. Try to determine immediate as offset to public symbol */
	if(type & Arg_Rip) app = bin_format.bind(*this,str,pa,flg,codelen,0L);
	if(app == true) goto next_step;
	if(dis_severity < CommSev_Func)
	{
	  strcpy(comments,".*");
	  std::string stmp;
	  psym = bin_format.get_public_symbol(stmp,_class,pa,false);
	  strcat(comments,stmp.c_str());
	  if(psym!=Plugin::Bad_Address) {
	    if(psym != pa) comments[0] = 0;
	    else
	    {
		dis_severity = CommSev_Func;
		strcpy(dis_comments,comments);
	    }
	  }
	}
	/* 2. Try to determine immediate as offset to string constant */
	comments[0] = 0;
	if(dis_severity < CommSev_StrPtr)
	{
	  size_t _index;
	  unsigned char rch;
	  _index = 0;
	  strcat(comments,"->\"");
	  for(_index = 3;_index < sizeof(comments)-5;_index++)
	  {
	    handle.seek(pa+_index-3,binary_stream::Seek_Set);
	    rch = handle.read(type_byte);
	    if(isprint(rch)) comments[_index] = rch;
	    else break;
	  }
	  if(!comments[3]) comments[0] = 0;
	  else
	  {
	    comments[_index++] = '"'; comments[_index] = 0;
	    dis_severity = CommSev_StrPtr;
	    strcpy(dis_comments,comments);
	  }
	}
      }
      }
    }
    next_step:
    comments[0] = 0;
    if(app == false)
    {
     switch(dig_type)
     {
      case Arg_LLong:
			 appstr = Get16SignDig(*(int64_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]) &&
			    isprint(((unsigned char *)defval)[2]) &&
			    isprint(((unsigned char *)defval)[3]) &&
			    isprint(((unsigned char *)defval)[4]) &&
			    isprint(((unsigned char *)defval)[5]) &&
			    isprint(((unsigned char *)defval)[6]) &&
			    isprint(((unsigned char *)defval)[7]))
			    sprintf(comments,"\"%c%c%c%c%c%c%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]
					    ,((unsigned char *)defval)[2]
					    ,((unsigned char *)defval)[3]
					    ,((unsigned char *)defval)[4]
					    ,((unsigned char *)defval)[5]
					    ,((unsigned char *)defval)[6]
					    ,((unsigned char *)defval)[7]);
			 break;
      case Arg_Long:  appstr = Get8SignDig(*(int32_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]) &&
			    isprint(((unsigned char *)defval)[2]) &&
			    isprint(((unsigned char *)defval)[3]))
			    sprintf(comments,"\"%c%c%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]
					    ,((unsigned char *)defval)[2]
					    ,((unsigned char *)defval)[3]);
			 break;
      case Arg_Short: appstr = Get4SignDig(*(int16_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]))
			    sprintf(comments,"\"%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]);
			 break;
      case Arg_Char:  appstr = Get2SignDig(*(char *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(*(unsigned char *)defval))
			    sprintf(comments,"'%c'",*(unsigned char *)defval);
			 break;
      case Arg_Byte:  appstr = Get2Digit(*(unsigned char *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(*(unsigned char *)defval))
			    sprintf(comments,"'%c'",*(unsigned char *)defval);
			 break;
      case Arg_Word:  appstr = Get4Digit(*(uint16_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]))
			    sprintf(comments,"\"%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]);
			 break;
      default:
      case Arg_DWord: appstr = Get8Digit(*(uint32_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]) &&
			    isprint(((unsigned char *)defval)[2]) &&
			    isprint(((unsigned char *)defval)[3]))
			    sprintf(comments,"\"%c%c%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]
					    ,((unsigned char *)defval)[2]
					    ,((unsigned char *)defval)[3]);
			 break;
      case Arg_QWord:
			 appstr = Get16Digit(*(uint64_t *)defval);
			 if(type & Arg_Imm &&
			    disNeedRef >= Ref_Predict &&
			    dis_severity < CommSev_String &&
			    isprint(((unsigned char *)defval)[0]) &&
			    isprint(((unsigned char *)defval)[1]) &&
			    isprint(((unsigned char *)defval)[2]) &&
			    isprint(((unsigned char *)defval)[3]) &&
			    isprint(((unsigned char *)defval)[4]) &&
			    isprint(((unsigned char *)defval)[5]) &&
			    isprint(((unsigned char *)defval)[6]) &&
			    isprint(((unsigned char *)defval)[7]))
			    sprintf(comments,"\"%c%c%c%c%c%c%c%c\""
					    ,((unsigned char *)defval)[0]
					    ,((unsigned char *)defval)[1]
					    ,((unsigned char *)defval)[2]
					    ,((unsigned char *)defval)[3]
					    ,((unsigned char *)defval)[4]
					    ,((unsigned char *)defval)[5]
					    ,((unsigned char *)defval)[6]
					    ,((unsigned char *)defval)[7]);
			 break;
    }
    str+=appstr;
   }
   if(comments[0])
   {
     dis_severity = CommSev_String;
     strcpy(dis_comments,comments);
   }
  }
  handle.seek(fpos,binary_stream::Seek_Set);
  return app;
}

bool DisMode::append_faddr(binary_stream& handle,std::string& str,__fileoff_t ulShift,__fileoff_t distin,__filesize_t r_sh,e_disaddr type,unsigned seg,char codelen)
{
 e_ref needref;
 __filesize_t fpos;
 size_t modif_to;
 DisasmRet dret;
 bool appended = false;
 int flg;
 fpos = handle.tell();
 memset(&dret,0,sizeof(DisasmRet));
 /* Prepare insn type */
 if(disNeedRef > Ref_None)
 {
   /* Forward prediction: ulShift = offset of binded field but r_sh is
      pointer where this field is referenced. */
   memset(disCodeBufPredict,0,disMaxCodeLen*PREDICT_DEPTH);
   handle.seek(r_sh, binary_stream::Seek_Set);
   handle.read(disCodeBufPredict,disMaxCodeLen*PREDICT_DEPTH);
   dret = disassembler(r_sh,(MBuffer)disCodeBufPredict,__DISF_GETTYPE);
 }
#ifndef NDEBUG
  if(ulShift >= (__fileoff_t)main_handle.flength()-codelen)
  {
     char sout[75];
     static bool displayed = false;
     if(!displayed)
     {
       strncpy(sout,str.c_str(),sizeof(sout)-1);
       sout[sizeof(sout)-1] = 0;
       if(!strlen(sout)) strcpy(sout,"disAppendFAddr");
       beye_context().ErrMessageBox(sout," Internal disassembler error detected ");
       displayed = true;
       COREDUMP();
     }
  }
#endif
 if(disNeedRef > Ref_None)
 {
   if(dret.pro_clone == __INSNT_JMPPIC || dret.pro_clone == __INSNT_JMPRIP) goto try_pic; /* skip defaults for PIC */
   flg = APREF_TRY_LABEL;
   if(hexAddressResolv) flg |= APREF_SAVE_VIRT;
   appended=bin_format.bind(*this,str,ulShift,flg,codelen,r_sh);
   if(!appended)
   {
      /*
	 Forwarding references.
	 Dereferencing ret instruction.
	 Idea and PE implementation by "Kostya Nosov" <k-nosov@yandex.ru>
      */
       if(dret.pro_clone == __INSNT_JMPVVT) /* jmp (mod r/m) */
       {
	    if(bin_format.bind(*this,str,r_sh+dret.field,APREF_TRY_LABEL,dret.codelen,r_sh))
	    {
	      appended = true;
	      modif_to = str.find(' ');
	      if(modif_to!=std::string::npos)
	      {
		while(str[modif_to] == ' ') modif_to++;
		str[modif_to-1] = '*';
	      }
	      if(!DumpMode && !EditMode) code_guider.add_go_address(*this,str,r_sh);
	    }
       }
       else
       if(dret.pro_clone == __INSNT_JMPPIC) /* jmp [ebx+offset] */
       {
	    try_pic:
	    if(dret.pro_clone == __INSNT_JMPRIP) goto try_rip;
	    if(bin_format.bind(*this,str,r_sh+dret.field,APREF_TRY_PIC,dret.codelen,r_sh))
	    {
	      appended = true; /* terminate appending any info anyway */
	      if(!DumpMode && !EditMode) code_guider.add_go_address(*this,str,r_sh);
	    }
       }
       else
       if(dret.pro_clone == __INSNT_JMPRIP) /* calln *(jmp [rip+offset]) */
       {
	__filesize_t _defval,_fpos,pa,__tmp;
	unsigned long app;
		try_rip:
		_fpos = main_handle.tell();
		main_handle.seek(r_sh+dret.field,binary_stream::Seek_Set);
		_defval = dret.codelen==8 ? main_handle.read(type_qword):
					    main_handle.read(type_dword);
		main_handle.seek(_fpos,binary_stream::Seek_Set);
	__tmp=bin_format.pa2va(r_sh+dret.field);
	_defval += (__tmp!=Plugin::Bad_Address ?
		    __tmp :
		    r_sh+dret.field)+dret.codelen;
	__tmp = bin_format.va2pa(_defval);
	pa = (__tmp!=Plugin::Bad_Address) ? __tmp : _defval;
	app=bin_format.bind(*this,str,pa,APREF_TRY_LABEL,dret.codelen,0L);
	if(app)
	{
	  appended = true; /* terminate appending any info anyway */
	  modif_to = str.find(' ');
	  if(modif_to!=std::string::npos)
	  {
	    while(str[modif_to] == ' ') modif_to++;
	    str[modif_to-1] = '*';
	  }
	  if(!DumpMode && !EditMode) code_guider.add_go_address(*this,str,r_sh);
	}
       }
   }
 }
   /*
      Idea and PE release of "Kostya Nosov" <k-nosov@yandex.ru>:
      make virtual address as argument of "jxxx" and "callx"
   */
 if(!appended)
 {
   if(hexAddressResolv)
   {
     r_sh = r_sh ? r_sh : (__filesize_t)ulShift;
     std::string stmp;
     appended = bin_format.address_resolving(stmp,r_sh);
     str+=stmp;
   }
   if(!appended)
   {
     needref = disNeedRef;
     disNeedRef = Ref_None;
     if(r_sh <= main_handle.flength())
     {
       const char * cptr;
       char lbuf[20];
       cptr = DumpMode ? "L" : "file:";
       str+=cptr;
	if(beye_context().is_file64())
	    sprintf(lbuf,"%016llX",r_sh);
	else
	    sprintf(lbuf,"%08lX",(unsigned long)r_sh);
       str+=lbuf;
       appended = true;
     }
     else
     {
       const char * pstr = "";
       if(type & UseSeg)
       {
	 str+=Get4Digit(seg);
	 str+=":";
       }
       if(!type) pstr = Get2SignDig((char)distin);
       else
	 if(type & Near16)
	      pstr = type & UseSeg ? Get4Digit((unsigned)distin) :
				     Get4SignDig((unsigned)distin);
	 else
	  if(type & Near32)   pstr = Get8SignDig(distin);
	  else
	   if(type & Near64) pstr = Get16SignDig(distin);
       str+=pstr;
     }
     disNeedRef = needref;
   }
   if(disNeedRef >= Ref_Predict && dis_severity < CommSev_InsnRef)
   {
     const char * comms;
     comms = NULL;
     switch(dret.pro_clone)
     {
	case __INSNT_RET:   comms = "RETURN"; break;
	case __INSNT_LEAVE: comms = "LEAVE"; break;
	default:            break;
     }
     if(comms)
     {
	dis_severity = CommSev_InsnRef;
	strcpy(dis_comments,comms);
     }
   }
   if(appended && !DumpMode && !EditMode) code_guider.add_go_address(*this,str,r_sh);
 }
 handle.seek(fpos,binary_stream::Seek_Set);
 return appended;
}

bool hexAddressResolution(unsigned& har);
bool DisMode::action_F6() { return hexAddressResolution(hexAddressResolv); }
unsigned DisMode::get_max_line_length() const { return get_max_symbol_size(); }

static Plugin* query_interface(const Bin_Format& b,binary_stream& h,TWindow& main_wnd,CodeGuider& code_guider,udn& u) { return new(zeromem) DisMode(b,h,main_wnd,code_guider,u); }

extern const Plugin_Info disMode = {
    "~Disassembler",	/**< plugin name */
    query_interface
};
AsmRet	Disassembler::assembler(const char *str) {
    AsmRet ret = {NULL, ASM_SYNTAX, 0 };
    UNUSED(str);
    beye_context().ErrMessageBox("Sorry, no assembler available","");
    return ret;
}
} // namespace	usr
