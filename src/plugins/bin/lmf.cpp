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

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "reg_form.h"
#include "udn.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bconsole.h"
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
	    LMF_Parser(binary_stream&,CodeGuider&,udn&);
	    virtual ~LMF_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F9();

	    virtual __filesize_t	show_header();
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	private:
	    void			failed_lmf() const;
	    std::vector<std::string>	lmf_ReadSecHdr(binary_stream& handle,size_t nnames);

	    lmf_headers_list*	hl;
	    lmf_xdef		xdef;
	    int			xdef_len;
	    unsigned		seg_num;
	    uint32_t		reccnt;
	    uint32_t		recmax;
	    uint32_t		reclast;
	    uint32_t		segbase[MAXSEG];

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

inline size_t DEFSIZE() { return sizeof(lmf_definition); }
inline size_t DATSIZE() { return sizeof(lmf_data); }
inline size_t HDRSIZE() { return sizeof(lmf_header); }

LMF_Parser::LMF_Parser(binary_stream& _h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(_h,code_guider,u)
	    ,main_handle(_h)
	    ,_udn(u)
{
    uint32_t i;
    int32_t j,p=0;
/*	lmf_data d;*/
    lmf_header h;
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&h,sizeof h);
	/* Test a first heder */
    i=j=(h.data_nbytes-DEFSIZE())/4;
    xdef_len=h.data_nbytes;
    main_handle.seek(6,binary_stream::Seek_Set);
    main_handle.read(&xdef,std::min(sizeof(lmf_xdef),size_t(h.data_nbytes)));
    while(1) {
	/* Test other headers */
	p+=HDRSIZE()+h.data_nbytes;
	main_handle.seek(p,binary_stream::Seek_Set);
	main_handle.read(&h,sizeof h);
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
			if(hl==NULL) return;
		}
		main_handle.seek(pos,binary_stream::Seek_Set);
		if(!main_handle.read(&hl[i].header,HDRSIZE()))
			{ failed_lmf(); return; }
		hl[i].file_pos=pos;
		switch(hl[i].header.rec_type)
		{
		case _LMF_DATA_REC:
		case _LMF_FIXUP_SEG_REC:
		case _LMF_FIXUP_LINEAR_REC:
			main_handle.seek(pos+HDRSIZE(),binary_stream::Seek_Set);
			if(!main_handle.read(&hl[i].data,DATSIZE()))
				{ failed_lmf(); return; }
			l=hl[i].data.index;
			if(l>=seg_num)
				{ failed_lmf(); return; }
			break;
		case _LMF_RESOURCE_REC:
			main_handle.seek(pos+HDRSIZE(),binary_stream::Seek_Set);
			if(!main_handle.read(&hl[i].res,sizeof(lmf_resource)))
				{ failed_lmf(); return; }
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
			{ failed_lmf(); return; }
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

	return;
}

LMF_Parser::~LMF_Parser()
{
	delete hl;
}

int LMF_Parser::query_platform() const
{
	return DISASM_CPU_IX86;
}

int LMF_Parser::query_bitness(__filesize_t pa) const
{
	UNUSED(pa);
	if(xdef.def.cflags&_PCF_32BIT) return DAB_USE32;
	else return DAB_USE16;
}

bool LMF_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
	unsigned i;
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
	for(i=0;i<=reclast;i++)
	{
		if(hl[i].file_pos<=cfpos&&
			cfpos<hl[i].file_pos+hl[i].header.data_nbytes+HDRSIZE())
		{
			if(cfpos<hl[i].file_pos+HDRSIZE()) {
			    addr="H";
			    addr+=Get2Digit(i);
			    addr+=";";
			    addr+=Get4Digit(cfpos-hl[i].file_pos);
			}
			else
				switch(hl[i].header.rec_type)
				{
				case _LMF_DEFINITION_REC:
					addr="Def:";
					addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				case _LMF_COMMENT_REC:
					if(cfpos<hl[i].file_pos+HDRSIZE()+DATSIZE())
					    addr="Com:";
					    addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				case _LMF_DATA_REC:
				case _LMF_FIXUP_SEG_REC:
					if(cfpos<hl[i].file_pos+HDRSIZE()+DATSIZE()) {
						addr=(hl[i].header.rec_type==_LMF_DATA_REC)?
							"Dat:":"Fix:";
						addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					}
/*					else
						if(((xdef.seg[hl[i].data.index]>>28)&0xf)==_LMF_CODE)
							sprintf(addr,"C:%06X",(cfpos-hl[i].file_pos+
								hl[i].data.offset-HDRSIZE()-
								DATSIZE()));
						else
							sprintf(addr,"D:%06X",(cfpos-hl[i].file_pos+
								hl[i].data.offset-HDRSIZE()-
								DATSIZE()));*/
					return false;
					break;
				case _LMF_FIXUP_80X87_REC:
					addr="F87:";
					addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				case _LMF_EOF_REC:
					addr="Eof:";
					addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				case _LMF_RESOURCE_REC:
					addr="Res:";
					addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				case _LMF_ENDDATA_REC:
					addr="EnD:";
					addr+=Get4Digit(cfpos-hl[i].file_pos-HDRSIZE());
					break;
				default:
					return false;
				}
			return true;
		}
	}
	return false;
}

__filesize_t LMF_Parser::va2pa(__filesize_t va)
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

__filesize_t LMF_Parser::pa2va(__filesize_t pa)
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

std::vector<std::string> LMF_Parser::lmf_ReadSecHdr(binary_stream& handle,size_t nnames)
{
    std::vector<std::string> rc;
	unsigned i;
	char tmp[30];
	char stmp[80];
	UNUSED(handle);
	UNUSED(nnames);
	for(i=0;i<=reclast;i++) {
		switch(hl[i].header.rec_type) {
		case _LMF_DEFINITION_REC:
			sprintf(tmp,"%s %s",
				(xdef.def.cflags&_PCF_32BIT)?"32-bit":"16-bit",
				(xdef.def.cflags&_PCF_FLAT)?"flat model":"");
			sprintf(stmp," %2d %-17s<%2d> %s",i+1,
				lmftypes[hl[i].header.rec_type],seg_num,tmp);
			break;
		case _LMF_DATA_REC:
		case _LMF_FIXUP_SEG_REC:
		case _LMF_FIXUP_LINEAR_REC:
			sprintf(tmp,"%s (%s)",lmftypes[hl[i].header.rec_type],
				((xdef.seg[hl[i].data.index]>>28)==_LMF_CODE)?
					"code":"data");
			if(xdef.def.cflags&_PCF_32BIT)
				sprintf(stmp," %2d %-18s %2d %08lX to %08lX",
					i+1,
					tmp,
					hl[i].data.index,
					(unsigned long)hl[i].data.offset,
					(unsigned long)hl[i].data.offset+
						hl[i].header.data_nbytes-HDRSIZE()-DATSIZE());
			else
				sprintf(stmp," %2d %-18s %2d %04lX to %04lX        ",
					i+1,
					tmp,
					hl[i].data.index,
					(unsigned long)hl[i].data.offset,
					(unsigned long)hl[i].data.offset+
						hl[i].header.data_nbytes-HDRSIZE()-DATSIZE());
			break;
		case _LMF_RESOURCE_REC:
			sprintf(tmp,"%s%s",lmftypes[hl[i].header.rec_type],
				(hl[i].res.resource_type==0)?"(usage)":"");
			sprintf(stmp," %2d %-18s",
				i+1,tmp);
			break;
		default:
			sprintf(stmp," %2d %-18s",i+1,
				(hl[i].header.rec_type<10)?
					lmftypes[hl[i].header.rec_type]:lmftypes[10]);
		}
		rc.push_back(stmp);
	}
	return rc;
}

__filesize_t LMF_Parser::action_F9()
{
    __filesize_t fpos=beye_context().tell();
    int ret;
    std::string title = " Num Type              Seg Virtual addresses   ";
    ssize_t nnames = reclast+1;
    int flags = LB_SELECTIVE;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = lmf_ReadSecHdr(main_handle,nnames);
    delete w;
    if(objs.empty()) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = ListBox(objs,title,flags,-1);
exit:
    if(ret!=-1) fpos=hl[ret].file_pos;
    return fpos;
}

__filesize_t LMF_Parser::show_header()
{
	unsigned i,j,k;
	__filesize_t fpos;
	TWindow *w;
	char hdr[81];
	char tmp[30];
	unsigned keycode;
/*	unsigned long entrya;*/
	fpos = beye_context().tell();
	sprintf(hdr," QNX%d Load Module Format Header ",xdef.def.version_no/100);
	sprintf(tmp,"%s%sPrivity=%d%s%s",
		(xdef.def.cflags&_PCF_LONG_LIVED)?"Long lived, ":"",
		(xdef.def.cflags&_PCF_32BIT)?"32-bit, ":"",
		(xdef.def.cflags&_PCF_PRIVMASK)>>2,
		(xdef.def.cflags&_PCF_FLAT)?", Flat model":"",
		(xdef.def.cflags&_PCF_NOSHARE)?", NoShare":"");
	if(strlen(tmp)>30) j=5;
	else j=1;
	k=seg_num+j+1;
	if(k<14) k=14;
	w=CrtDlgWndnls(hdr,64,k);
	w->goto_xy(1,1);
	w->printf(
		"Version       = %d.%02d\n"
		"Code flags    = %04XH\n"
		"(%s)\n"
		"CPU/FPU       = %d/%d\n",
		xdef.def.version_no/100,xdef.def.version_no%100,xdef.def.cflags,
		tmp,xdef.def.cpu,xdef.def.fpu);
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
  hlpDisplay(10015);
  return beye_context().tell();
}

static bool probe(binary_stream& main_handle)
{
    lmf_xdef xdef;
    int32_t j,p=0;
    uint32_t i;
/*	lmf_data d;*/
    lmf_header h;
    main_handle.seek(0,binary_stream::Seek_Set);
    if(!main_handle.read(&h,sizeof h)) return false;
	/* Test a first heder */
    if(h.rec_type!=_LMF_DEFINITION_REC||h.zero1!=0||/*h.spare!=0||*/
		h.data_nbytes<DEFSIZE()+2*sizeof(long)||
		(h.data_nbytes-DEFSIZE())%4!=0) return false;
    i=j=(h.data_nbytes-DEFSIZE())/4;
    main_handle.seek(6,binary_stream::Seek_Set);
    if(!main_handle.read(&xdef,std::min(sizeof(lmf_xdef),size_t(h.data_nbytes)))) return false;
	/* Test a definition record */
    if(xdef.def.version_no!=400||xdef.def.code_index>i||xdef.def.stack_index>i||
		xdef.def.heap_index>i||xdef.def.argv_index>i||xdef.def.zero2!=0)
		return false;
    if(xdef.def.cpu%100!=86||(xdef.def.fpu!=0&&xdef.def.fpu%100!=87))
		return false;
    if(xdef.def.cflags&_PCF_FLAT&&xdef.def.flat_offset==0) return false;
    if(xdef.def.stack_nbytes==0) return false;
    for(i=0;i<4;i++) if(xdef.def.zero1[i]!=0) return false;
    while(1) {
	/* Test other headers */
	p+=HDRSIZE()+h.data_nbytes;
	main_handle.seek(p,binary_stream::Seek_Set);
	if(!main_handle.read(&h,sizeof h)) return false;
	if(h.rec_type==_LMF_DEFINITION_REC||h.data_nbytes==0||h.zero1!=0/*||h.spare!=0*/) return false;
	if(h.rec_type==_LMF_EOF_REC) break;
    }
    return true;
}


static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) LMF_Parser(h,_parent,u); }
extern const Binary_Parser_Info lmf_info = {
    "lmf (QNX4 executable file)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
