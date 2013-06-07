#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mz.c
 * @brief       This file contains implementation of MZ file format.
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
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "udn.h"
#include "colorset.h"
#include "codeguid.h"
#include "tstrings.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mz.h"
#include "beye.h"

namespace	usr {
static const char* txt[]={"MZHelp","","","","","","","","",""};
const char* MZ_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t MZ_Parser::va2pa(__filesize_t va) const { return va >= HeadSize ? va + HeadSize : 0L; }
__filesize_t MZ_Parser::pa2va(__filesize_t pa) const { return pa >= HeadSize ? pa - HeadSize : 0L; }

std::string MZ_Parser::QueryAddInfo( unsigned char *memmap ) const
{
    char rbuff[41];
    unsigned long idl;
    unsigned short idw,idw0;
    idl = ((unsigned long *)memmap)[0];
    idw0 = ((unsigned short *)memmap)[0];
    idw = ((unsigned short *)memmap)[2];
    if(memcmp(memmap,"RJSX",4) == 0) { ArjARC: return "ARJ self-extracting archive"; }
    else if(memcmp(memmap,"LZ09",4) == 0) return "LZEXE 0.90 compressed executable";
    else if(memcmp(memmap,"LZ91",4) == 0) return "LZEXE 0.91 compressed executable";
    else if(memmap[2] == 0xFB) {
	char hi,low;
	hi = (memmap[3] >> 4) & 0x0F;
	low = memmap[3] & 0x0F;
	sprintf(rbuff,"Borland TLINK version: %u.%u",(unsigned)hi,(unsigned)low);
	return rbuff;
    } else if(memcmp(&memmap[2],"PKLITE",6) == 0) {
	char hi, low;
	low = memmap[0];
	hi =  memmap[1] & 0x0F;
	sprintf(rbuff,"PKLITE v%u.%u compressed executable",(unsigned)hi,(unsigned)low);
	return rbuff;
    } else if(memcmp(&memmap[9],"LHarc's SFX",11) == 0) return "LHarc 1.x self-extracting archive";
    else if(memcmp(&memmap[8],"LHa's SFX",9) == 0) return "LHa 2.x self-extracting archive";
    else if(idl == 0x018A0001L && idw == 0x1565) return "TopSpeed 3.0 CRUNCH compressed file";
    else if(idl == 0x00020001L && idw == 0x0700) return "PKARCK 3.5 self-extracting-archive";
    else if(idw0 == 0x000F && memmap[2] == 0xA7) return "BSA (Soviet archiver) selft-extarcting";
    else if(memcmp(&memmap[4],"SFX by LARC",11) == 0) return "LARC self-extracting archive";
    else if(memcmp(&memmap[8],"LH's SFX",8) == 0) return "LH self-extracting archive";
    else {
	unsigned i;
	for(i = 0;i < 1000-6;i++)
	    if(memmap[i] == 'a' && memcmp(&memmap[i+1],"RJsfX",5) == 0) goto ArjARC;
    }
    return "";
}

std::string MZ_Parser::QueryAddInfo() const
{
    unsigned char *memmap;
    memmap = new unsigned char[1000];
    if(memmap) {
	std::string ret;
	__filesize_t fpos;
	fpos = _main_handle.tell();
	_main_handle.seek(0x1C,binary_stream::Seek_Set);
	_main_handle.read(memmap,1000);
	_main_handle.seek(fpos,binary_stream::Seek_Set);
	ret = QueryAddInfo(memmap);
	delete memmap;
	return ret;
    }
    return "";
}

__filesize_t MZ_Parser::show_header() const
{
    unsigned keycode;
    TWindow * hwnd;
    __filesize_t newcpos,fpos;
    unsigned long FPageCnt;
    std::string addinfo;
    fpos = _bctx.tell();
    keycode = 16;
    if(is_new_exe(_main_handle)) keycode++;
    addinfo = QueryAddInfo();
    if(!addinfo.empty()) keycode++;
    hwnd = CrtDlgWndnls(" Old Exe Header ",43,keycode-1);
    FPageCnt =  ((long)mz.mzPageCount - 1)*512;
    hwnd->goto_xy(1,1);
    hwnd->printf(
	  "Signature            = 'MZ'\n"
	  "Part Last Page       = %hu [ bytes ]\n"
	  "Page count           = %hu [ pages ]\n"
	  "Relocations count    = %hu\n"
	  "Header size          = %hu [ paragraphs ]\n"
	  "Minimum memory       = %04hXH [ paragraphs ]\n"
	  "Maximum memory       = %04hXH [ paragraphs ]\n"
	  "SS : SP              = %04hX:%04hXH\n"
	  "Check summ           = %hu\n"
	  "CS : IP              = %04hX:%04hXH\n"
	  "Table offset         = %04hXH [ bytes ]\n"
	  "Overlay Number       = %hu\n"
	  ,mz.mzPartLastPage
	  ,mz.mzPageCount
	  ,mz.mzRelocationCount
	  ,mz.mzHeaderSize
	  ,mz.mzMinMem
	  ,mz.mzMaxMem
	  ,mz.mzRelocationSS,mz.mzExeSP
	  ,mz.mzCheckSumm
	  ,mz.mzRelocationCS,mz.mzExeIP
	  ,mz.mzTableOffset
	  ,mz.mzOverlayNumber);
    newcpos = HeadSize;
    newcpos += (((unsigned long)mz.mzRelocationCS) << 4) + (unsigned long)mz.mzExeIP;
    hwnd->set_color(dialog_cset.entry);
    hwnd->printf(">Entry Point         = %08lXH",newcpos); hwnd->clreol();
    hwnd->set_color(dialog_cset.addinfo);
    hwnd->printf("\nModule Length        = %lu [ bytes ]",(FPageCnt - HeadSize) + mz.mzPartLastPage);
    hwnd->clreol();
    hwnd->set_color(dialog_cset.main);
    hwnd->printf("\nImage offset         = %08lXH",(long)HeadSize);
    if(_headshift) {
	hwnd->set_color(dialog_cset.altinfo);
	hwnd->printf("\nNew EXE header shift = %08lXH",(long)_headshift);
	hwnd->clreol();
    }
    if(!addinfo.empty()) {
	hwnd->set_color(dialog_cset.extrainfo);
	hwnd->printf("\n%s",addinfo.c_str());
	hwnd->clreol();
    }
    while(1) {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = newcpos; break; }
	else if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    delete hwnd;
    return fpos;
}

void MZ_Parser::BuildMZChain()
{
    unsigned i;
    __filesize_t fpos;
    TWindow* w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
    w->goto_xy(1,1);
    w->puts(BUILD_REFS);
    fpos = _main_handle.tell();
    for(i = 0;i < mz.mzRelocationCount;i++) {
	MZ_Reloc value(__codelen);
	unsigned off,seg,j;
	__filesize_t ptr;
	j = mz.mzTableOffset + i*4;
	_main_handle.seek(j,binary_stream::Seek_Set);
	off = _main_handle.read(type_word);
	seg = _main_handle.read(type_word);
	ptr = (((long)seg) << 4) + off + (((long)mz.mzHeaderSize) << 4);
	value.laddr = ptr;
	CurrMZChain.insert(value);
    }
    _main_handle.seek(fpos,binary_stream::Seek_Set);
    delete w;
}

bool MZ_Parser::isMZReferenced(__filesize_t shift,char len)
{
    if(mz.mzRelocationCount) {
	__filesize_t mz_size;
	mz_size = (long)(mz.mzPageCount)*512 + mz.mzPartLastPage;
	if(shift <= mz_size && shift >= ((unsigned long)mz.mzHeaderSize) << 4) {
	    MZ_Reloc key(__codelen);
	    key.laddr = shift;
	    __codelen = len;
	    return CurrMZChain.find(key) != CurrMZChain.end();
	}
    }
    return false;
}

std::string MZ_Parser::bind(const DisMode& parent,__filesize_t ulShift,Bin_Format::bind_type flags,int codelen,__filesize_t r_sh)
{
    std::string str;
    bool ret = false;
    if(flags & Bin_Format::Try_Pic) return "";
    if(CurrMZChain.empty()) BuildMZChain();
    if(isMZReferenced(ulShift,codelen)) {
	unsigned wrd;
	_main_handle.seek(ulShift,binary_stream::Seek_Set);
	wrd = _main_handle.read(type_word);
	str=Get4Digit(wrd);
	str+="+PID";
	ret = true;
    }
    if(!DumpMode && !EditMode && (flags & Bin_Format::Try_Label) && codelen == 4) {
	r_sh += (((__filesize_t)mz.mzHeaderSize) << 4);
	Symbol_Info rc = __udn.find(r_sh);
	if(rc.pa!=Plugin::Bad_Address) str=rc.name;
	else str=Get8Digit(r_sh);
	_code_guider.add_go_address(parent,str,r_sh);
    }
    return str;
}

/* Special case: this module must not use init and destroy */
MZ_Parser::MZ_Parser(BeyeContext& b,binary_stream& h,CodeGuider& __code_guider,udn& u)
	    :Binary_Parser(b,h,__code_guider,u)
	    ,_bctx(b)
	    ,_main_handle(h)
	    ,_code_guider(__code_guider)
	    ,__udn(u)
{
    unsigned char id[2];
    _main_handle.seek(0,binary_stream::Seek_Set);
    _main_handle.read(id,sizeof(id));
    if((id[0] == 'M' && id[1] == 'Z') ||
     (id[0] == 'Z' && id[1] == 'M')) {
	_main_handle.seek(2,binary_stream::Seek_Set);
	_main_handle.read((any_t*)&mz,sizeof(MZHEADER));
	HeadSize = ((unsigned long)mz.mzHeaderSize) << 4;
	_headshift = is_new_exe(h);
    } else throw bad_format_exception();
}
MZ_Parser::~MZ_Parser() {}
int MZ_Parser::query_platform() const { return DISASM_CPU_IX86; }

std::string MZ_Parser::address_resolving(__filesize_t cfpos)
{
    std::string addr;
    if(cfpos < sizeof(MZHEADER)+2) {
	addr="MZH :";
	addr+=Get4Digit(cfpos);
    } else if(cfpos >= sizeof(MZHEADER)+2 && cfpos < sizeof(MZHEADER)+2+(mz.mzRelocationCount<<2)) {
	addr="MZRl:";
	addr+=Get4Digit(cfpos - sizeof(MZHEADER));
    } else if(cfpos >= HeadSize) {
	addr = ".";
	addr+=Get8Digit(MZ_Parser::pa2va(cfpos));
    }
    return addr;
}

__filesize_t MZ_Parser::action_F1()
{
    Beye_Help bhelp(_bctx);
    if(bhelp.open(true)) {
	bhelp.run(10013);
	bhelp.close();
    }
    return _bctx.tell();
}

__filesize_t MZ_Parser::is_new_exe(binary_stream& main_handle)
{
    __filesize_t ret;
    char id[2];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(id,sizeof(id));
    main_handle.seek(0x3C,binary_stream::Seek_Set);
    ret=main_handle.read(type_dword);
    if(!( id[0] == 'M' && id[1] == 'Z' && ret > 0x02L)) ret = 0;
    return (__filesize_t)ret;
}

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) MZ_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info mz_info = {
    "MZ (Old DOS-exe)",	/**< plugin name */
    query_interface
};
} // namespace	usr
