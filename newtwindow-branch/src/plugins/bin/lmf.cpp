#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/lmf.c
 * @brief       This file contains implementation of lmf (QNX4 executable file)
 *              file format decoder.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Andrew Golovnia
 * @since       2001
 * @note        Development, fixes and improvements
 * @todo        wc 10.6 debug information support!!! (see lmf.tgz)
**/
#include <algorithm>
#include <sstream>
#include <iomanip>

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "udn.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "listbox.h"
#include "tstrings.h"
#include "plugins/disasm.h"
#include "plugins/bin/lmf.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    enum {
	MAXREC		=200,
	MINREC		=20,
	MAXSEG		=50,
	MAXSEGFRAMES	=50
    };
    struct lmf_headers_list {		/* LMF file frame */
	lmf_header header;		/* Header of frame */
	lmf_data data;			/* Data info */
	lmf_resource res;		/* Resource info */
	uint32_t file_pos;		/* Frame header file position */
    };

    struct lmf_xdef {		/* Extra definition */
	lmf_definition def;		/* Standard definition */
	uint32_t seg[MAXSEG];	/* Segments lengthes list */
    };

    class LMF_Parser : public Binary_Parser {
	public:
	    LMF_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~LMF_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F9();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	    virtual Bin_Format::bitness	query_bitness(__filesize_t) const;
	    virtual std::string		address_resolving(__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	private:
	    void			failed_lmf() const;
	    std::vector<std::string>	lmf_ReadSecHdr(binary_stream& handle,size_t nnames) const;

	    lmf_headers_list*	hl;
	    lmf_xdef		xdef;
	    int			xdef_len;
	    unsigned		seg_num;
	    uint32_t		reccnt;
	    uint32_t		recmax;
	    uint32_t		reclast;
	    uint32_t		segbase[MAXSEG];

	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;

	    static const char*	lmftypes[];
    };
static const char* txt[]={"LMFHlp","","","","","","","","SecLst",""};
const char* LMF_Parser::prompt(unsigned idx) const { return txt[idx]; }

const char* LMF_Parser::lmftypes[]={
	"definition",
	"comment",
	"text",
	"fixup seg",
	"fixup x87",
	"eof",
	"resource",
	"end data",
	"fixup linear",
	"ph resource",
	"unknown"};

void LMF_Parser::failed_lmf() const
{
	/* lmf corruption message */
}

inline size_t __CONST_FUNC__ DEFSIZE() { return sizeof(lmf_definition); }
inline size_t __CONST_FUNC__ DATSIZE() { return sizeof(lmf_data); }
inline size_t __CONST_FUNC__ HDRSIZE() { return sizeof(lmf_header); }

LMF_Parser::LMF_Parser(BeyeContext& b,binary_stream& _h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,_h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(_h)
	    ,_udn(u)
{
    lmf_xdef _xdef;
    int32_t j,p=0;
    uint32_t i;
    binary_packet bp(1);
/*	lmf_data d;*/
    lmf_header h;
    main_handle.seek(0,binary_stream::Seek_Set);
    bp=main_handle.read(sizeof h); memcpy(&h,bp.data(),bp.size());
    if(bp.size()!=sizeof(h)) throw bad_format_exception();
	/* Test a first heder */
    if(h.rec_type!=_LMF_DEFINITION_REC||h.zero1!=0||/*h.spare!=0||*/
		h.data_nbytes<DEFSIZE()+2*sizeof(long)||
		(h.data_nbytes-DEFSIZE())%4!=0) throw bad_format_exception();
    i=j=(h.data_nbytes-DEFSIZE())/4;
    main_handle.seek(6,binary_stream::Seek_Set);
    bp=main_handle.read(std::min(sizeof(lmf_xdef),size_t(h.data_nbytes))); memcpy(&_xdef,bp.data(),bp.size());
    if(!bp.size()!=std::min(sizeof(lmf_xdef),size_t(h.data_nbytes))) throw bad_format_exception();
	/* Test a definition record */
    if(_xdef.def.version_no!=400||_xdef.def.code_index>i||_xdef.def.stack_index>i||
		_xdef.def.heap_index>i||_xdef.def.argv_index>i||_xdef.def.zero2!=0)
		throw bad_format_exception();
    if(_xdef.def.cpu%100!=86||(_xdef.def.fpu!=0&&_xdef.def.fpu%100!=87))
		throw bad_format_exception();
    if(_xdef.def.cflags&_PCF_FLAT&&_xdef.def.flat_offset==0) throw bad_format_exception();
    if(_xdef.def.stack_nbytes==0) throw bad_format_exception();
    for(i=0;i<4;i++) if(_xdef.def.zero1[i]!=0) throw bad_format_exception();
    while(1) {
	/* Test other headers */
	p+=HDRSIZE()+h.data_nbytes;
	main_handle.seek(p,binary_stream::Seek_Set);
	bp=main_handle.read(sizeof h); memcpy(&h,bp.data(),bp.size());
	if(!bp.size()!=sizeof(h)) throw bad_format_exception();
	if(h.rec_type==_LMF_DEFINITION_REC||h.data_nbytes==0||h.zero1!=0/*||h.spare!=0*/) throw bad_format_exception();
	if(h.rec_type==_LMF_EOF_REC) break;
    }

    main_handle.seek(0,binary_stream::Seek_Set);
    bp=main_handle.read(sizeof h); memcpy(&h,bp.data(),bp.size());
	/* Test a first heder */
    i=j=(h.data_nbytes-DEFSIZE())/4;
    xdef_len=h.data_nbytes;
    main_handle.seek(6,binary_stream::Seek_Set);
    bp=main_handle.read(std::min(sizeof(lmf_xdef),size_t(h.data_nbytes))); memcpy(&xdef,bp.data(),bp.size());
    while(1) {
	/* Test other headers */
	p+=HDRSIZE()+h.data_nbytes;
	main_handle.seek(p,binary_stream::Seek_Set);
	bp=main_handle.read(sizeof h); memcpy(&h,bp.data(),bp.size());
	if(h.rec_type==_LMF_EOF_REC) break;
    }

	uint32_t l;
	int32_t pos=0;
	hl=new lmf_headers_list[MINREC];
	if(hl==NULL) return;
	recmax=MINREC;
	reccnt=0;
	seg_num=(xdef_len-DEFSIZE())/4;
	for(i=0;i<seg_num;i++) segbase[i]=0;
	for(i=0;;i++)
	{
		if((unsigned)i==recmax)
		{
			hl=(lmf_headers_list*)mp_realloc(hl,(recmax+=MINREC)*sizeof(lmf_headers_list));
			if(hl==NULL) throw bad_format_exception();
		}
		main_handle.seek(pos,binary_stream::Seek_Set);
		bp=main_handle.read(HDRSIZE()); memcpy(&hl[i].header,bp.data(),bp.size());
		if(bp.size()!=HDRSIZE())
			throw bad_format_exception();
		hl[i].file_pos=pos;
		switch(hl[i].header.rec_type)
		{
		case _LMF_DATA_REC:
		case _LMF_FIXUP_SEG_REC:
		case _LMF_FIXUP_LINEAR_REC:
			main_handle.seek(pos+HDRSIZE(),binary_stream::Seek_Set);
			bp=main_handle.read(DATSIZE()); memcpy(&hl[i].data,bp.data(),bp.size());
			if(bp.size()!=DATSIZE())
			    throw bad_format_exception();
			l=hl[i].data.index;
			if(l>=seg_num)
			    throw bad_format_exception();
			break;
		case _LMF_RESOURCE_REC:
			main_handle.seek(pos+HDRSIZE(),binary_stream::Seek_Set);
			bp=main_handle.read(sizeof(lmf_resource)); memcpy(&hl[i].res,bp.data(),bp.size());
			if(bp.size()!=sizeof(lmf_resource))
			    throw bad_format_exception();
			break;
		case _LMF_EOF_REC:
			reclast=i;
			goto outloop;
		case _LMF_DEFINITION_REC:
		case _LMF_COMMENT_REC:
		case _LMF_FIXUP_80X87_REC:
		case _LMF_ENDDATA_REC:	/* todo: decode license information (name) */
		case _LMF_PHRESOURCE:
			break;				/* Ignore this records */
		default:
			throw bad_format_exception();
		}
		pos+=hl[i].header.data_nbytes+HDRSIZE();
	}
outloop:
	if(xdef.def.cflags&_PCF_FLAT)
	{
		segbase[0]=xdef.def.stack_nbytes;
		for(i=1;i<=seg_num;i++)
			segbase[i]=(segbase[i-1]+
				((xdef.seg[i-1]&0x0fffffff)+4095))&0xfffff000;
	}
}

LMF_Parser::~LMF_Parser()
{
	delete hl;
}

int LMF_Parser::query_platform() const
{
	return DISASM_CPU_IX86;
}

Bin_Format::bitness LMF_Parser::query_bitness(__filesize_t pa) const
{
	UNUSED(pa);
	if(xdef.def.cflags&_PCF_32BIT) return Bin_Format::Use32;
	else return Bin_Format::Use16;
}

std::string LMF_Parser::address_resolving(__filesize_t cfpos)
{
    unsigned i;
    std::string addr;
    std::ostringstream oss;
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
    for(i=0;i<=reclast;i++) {
	if(hl[i].file_pos<=cfpos&& cfpos<hl[i].file_pos+hl[i].header.data_nbytes+HDRSIZE()) {
	    if(cfpos<hl[i].file_pos+HDRSIZE()) {
		oss<<"H"<<std::hex<<std::setfill('0')<<std::setw(2)<<i<<";"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos);
		addr=oss.str();
	    } else
		switch(hl[i].header.rec_type) {
		    case _LMF_DEFINITION_REC:
			oss<<"Def:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			addr=oss.str();
			break;
		    case _LMF_COMMENT_REC:
			if(cfpos<hl[i].file_pos+HDRSIZE()+DATSIZE())
			    oss<<"Com:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			    addr=oss.str();
			break;
		    case _LMF_DATA_REC:
		    case _LMF_FIXUP_SEG_REC:
			if(cfpos<hl[i].file_pos+HDRSIZE()+DATSIZE()) {
			    oss<<((hl[i].header.rec_type==_LMF_DATA_REC)?"Dat:":"Fix:")<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			    addr=oss.str();
			}
			return "";
			break;
		    case _LMF_FIXUP_80X87_REC:
			oss<<"F87:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			addr=oss.str();
			break;
		    case _LMF_EOF_REC:
			oss<<"Eof:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			addr=oss.str();
			break;
		    case _LMF_RESOURCE_REC:
			oss<<"Res:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			addr=oss.str();
			break;
		    case _LMF_ENDDATA_REC:
			oss<<"EnD:"<<std::hex<<std::setfill('0')<<std::setw(4)<<(cfpos-hl[i].file_pos-HDRSIZE());
			addr=oss.str();
			break;
		    default: return "";
		}
		return addr;
	}
    }
    return "";
}

__filesize_t LMF_Parser::va2pa(__filesize_t va) const
{
	unsigned i,j;
	int seclen;
	__filesize_t addr=0;
	__filesize_t newva=0;
	if(xdef.def.cflags&_PCF_32BIT)
	{
		for(i=0;i<seg_num;i++)
			if(va>segbase[i]&&va<segbase[i+1])
			{
				newva=va-segbase[i];
				break;
			}
		if(newva>(xdef.seg[i]&0x0fffffff)) return 0;
	}
	else
	{
		i=va>>19;
		newva=va&0xffff;
		if(i>seg_num||newva>(xdef.seg[i]&0x0ffff)) return 0;
	}
	for(j=1;j<reclast;j++)
	{
		seclen=hl[j].header.data_nbytes-DATSIZE();
		if(hl[j].header.rec_type==_LMF_DATA_REC&&
			hl[j].data.index==i&&
			hl[j].data.offset<=newva&&
			hl[j].data.offset+seclen>newva) break;
	}
	if(i==reclast) return 0;
	addr=hl[j].file_pos+newva-hl[j].data.offset+HDRSIZE()+DATSIZE();

	return addr;
}

__filesize_t LMF_Parser::pa2va(__filesize_t pa) const
{
	unsigned i;
	int seclen;
	__filesize_t addr=0;
	for(i=1;i<reclast;i++)
	{
		seclen=hl[i].header.data_nbytes-DATSIZE();
		if(hl[i].file_pos<=pa&&
			hl[i].file_pos+seclen>pa)
		{
			if(hl[i].header.rec_type==_LMF_DATA_REC) break;
			else return 0;
		}
	}
	addr=hl[i].data.offset+pa-(hl[i].file_pos+HDRSIZE()+DATSIZE());
	if(xdef.def.cflags&_PCF_32BIT)
		addr+=segbase[hl[i].data.index];
	else
		addr|=(hl[i].data.index<<19)|((xdef.def.cflags&_PCF_PRIVMASK)<<14);
	return addr;
}

std::vector<std::string> LMF_Parser::lmf_ReadSecHdr(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    std::ostringstream oss;
    unsigned i;
    UNUSED(handle);
    UNUSED(nnames);
    for(i=0;i<=reclast;i++) {
	switch(hl[i].header.rec_type) {
	    case _LMF_DEFINITION_REC:
		oss<<std::setw(2)<<(i+1)<<std::left<<std::setw(17)<<lmftypes[hl[i].header.rec_type]<<std::right
		    <<"<"<<seg_num<<"> "<<((xdef.def.cflags&_PCF_32BIT)?"32-bit":"16-bit")<<" "<<((xdef.def.cflags&_PCF_FLAT)?"flat model":"");
			break;
	    case _LMF_DATA_REC:
	    case _LMF_FIXUP_SEG_REC:
	    case _LMF_FIXUP_LINEAR_REC:
		oss<<std::setw(2)<<(i+1)<<std::left<<std::setw(18)<<lmftypes[hl[i].header.rec_type]<<std::right
		    <<" ("<<(((xdef.seg[hl[i].data.index]>>28)==_LMF_CODE)?"code":"data")<<")"
		    <<std::setw(2)<<hl[i].data.index<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)hl[i].data.offset;
		if(xdef.def.cflags&_PCF_32BIT)
		    oss<<std::hex<<std::setfill('0')<<std::setw(8)<<(unsigned long)hl[i].data.offset+hl[i].header.data_nbytes-HDRSIZE()-DATSIZE();
		else
		    oss<<std::hex<<std::setfill('0')<<std::setw(4)<<(unsigned long)hl[i].data.offset+hl[i].header.data_nbytes-HDRSIZE()-DATSIZE();
		break;
	    case _LMF_RESOURCE_REC:
		oss<<std::setw(2)<<(i+1)<<" "<<std::left<<std::setw(18)<<lmftypes[hl[i].header.rec_type]<<((hl[i].res.resource_type==0)?"(usage)":"");
		break;
	    default:
		oss<<std::setw(2)<<(i+1)<<" "<<std::left<<std::setw(18)<<((hl[i].header.rec_type<10)?lmftypes[hl[i].header.rec_type]:lmftypes[10]);
	}
	rc.push_back(oss.str());
    }
    return rc;
}

__filesize_t LMF_Parser::action_F9()
{
    __filesize_t fpos=bctx.tell();
    int ret;
    std::string title = " Num Type              Seg Virtual addresses   ";
    ssize_t nnames = reclast+1;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = lmf_ReadSecHdr(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
exit:
    if(ret!=-1) fpos=hl[ret].file_pos;
    return fpos;
}

__filesize_t LMF_Parser::show_header() const
{
	unsigned i,j,k;
	__filesize_t fpos;
	TWindow* w;
	unsigned keycode;
	std::ostringstream oss,oss2;
/*	unsigned long entrya;*/
	fpos = bctx.tell();
	oss2<<" QNX"<<(xdef.def.version_no/100)<<" Load Module Format Header ";
	oss.str("");
	oss<<((xdef.def.cflags&_PCF_LONG_LIVED)?"Long lived, ":"")<<((xdef.def.cflags&_PCF_32BIT)?"32-bit, ":"")
	    <<"Privity="<<((xdef.def.cflags&_PCF_PRIVMASK)>>2)<<((xdef.def.cflags&_PCF_FLAT)?", Flat model":"")<<((xdef.def.cflags&_PCF_NOSHARE)?", NoShare":"");
	if(oss.str().length()>30) j=5;
	else j=1;
	k=seg_num+j+1;
	if(k<14) k=14;
	w=CrtDlgWndnls(oss2.str(),64,k);
	w->goto_xy(1,1);
	w->printf(
		"Version       = %d.%02d\n"
		"Code flags    = %04XH\n"
		"(%s)\n"
		"CPU/FPU       = %d/%d\n",
		xdef.def.version_no/100,xdef.def.version_no%100,xdef.def.cflags,
		oss.str().c_str(),xdef.def.cpu,xdef.def.fpu);
	w->printf(
		"Code index    = %d\n"
		"Stack index   = %d\n"
		"Heap index    = %d\n"
		"Argv index    = %d\n"
		"Code offset   = %08lXH\n"
		"Stack size    = %08lXH\n"
		"Heap size     = %08lXH\n"
		"Flat offset   = %08lXH\n"
		"Unmapped size = %08lXH\n",
		xdef.def.code_index,
		xdef.def.stack_index,
		xdef.def.heap_index,
		xdef.def.argv_index,
		xdef.def.code_offset,
		xdef.def.stack_nbytes,
		xdef.def.heap_nbytes,
		xdef.def.flat_offset,
		xdef.def.unmapped_size);
	w->goto_xy(35,j++);
	w->printf("Segments:");
	w->goto_xy(35,j++);
	w->printf("Num  Length     Type");
	for(i=0;i<seg_num;i++)
	{
		w->goto_xy(35,j+i);
		w->printf(" %2d  %08lXH  %s",i,xdef.seg[i]&0x0fffffff,
			((xdef.seg[i]>>28)==_LMF_CODE)?
				"code":"data");
	}
	w->goto_xy(1,14);
	w->set_color(dialog_cset.entry);
	w->printf(
		"Entry point   = seg:%d, offset:%08lXH",
		xdef.def.code_index,xdef.def.code_offset);
	w->printf("\n"); w->clreol();
	w->set_color(dialog_cset.main);
	while(1)
	{
		keycode=GetEvent(drawEmptyPrompt,NULL,w);
		if(keycode==KE_ENTER)
		{
			if(xdef.def.cflags&_PCF_32BIT)
				fpos=va2pa(segbase[xdef.def.code_index]+xdef.def.code_offset);
			else
				fpos=va2pa((xdef.def.code_index<<19)+xdef.def.code_offset);
			break;
		}
		else
			if(keycode==KE_ESCAPE||keycode==KE_F(10)) break;
	}
	delete w;
	return fpos;
}

__filesize_t LMF_Parser::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(10015);
	bhelp.close();
    }
    return bctx.tell();
}

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) LMF_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info lmf_info = {
    "lmf (QNX4 executable file)",	/**< plugin name */
    query_interface
};
} // namespace	usr
