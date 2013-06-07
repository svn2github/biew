#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        fileutil.c
 * @brief       This file contains file utilities of BEYE project.
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
#include <iomanip>
#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "beye.h"
#include "editor.h"
#include "tstrings.h"
#include "plugins/hexmode.h"
#include "plugins/disasm.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "listbox.h"
#include "libbeye/bbio.h"
#include "libbeye/bswap.h"
#include "libbeye/twindow.h"
#include "libbeye/kbd_code.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/osdep/system.h"

namespace	usr {
extern const Plugin_Info disMode;

    class FileUtilities : public Opaque {
	public:
	    FileUtilities(BeyeContext&);
	    virtual ~FileUtilities();

	    virtual bool	run() = 0;
	protected:
	    __filesize_t	ff_startpos,ff_len;
	    BeyeContext&	bctx;
    };

    class ChSize : public FileUtilities {
	public:
	    ChSize(BeyeContext&);
	    virtual ~ChSize();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
    };

FileUtilities::FileUtilities(BeyeContext& b):bctx(b) {}
FileUtilities::~FileUtilities() {}

ChSize::ChSize(BeyeContext& b):FileUtilities(b) {}
ChSize::~ChSize() {}
bool ChSize::run()
{
    __fileoff_t psize,tile = 0;
    if(Get16DigitDlg(" Change size of file ","Num. of bytes (+-dec):",3,(unsigned long long*)&tile)) {
	if(tile == 0) { bctx.ErrMessageBox("Invalid new length",""); return false; }
	psize = bctx.flength();
	psize += tile;
	if(psize > 0) {
	    bool ret;
	    int my_errno = 0;
	    std::string fname = bctx.bm_file().filename();
	    binary_stream* bHandle;
	    bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
	    if(bHandle == NULL) {
err:
		bctx.errnoMessageBox(RESIZE_FAIL,"",my_errno);
		return false;
	    }
	    ret = bHandle->chsize(psize);
	    my_errno = errno;
	    delete bHandle;
	    if(ret == false) goto err;
	    bctx.bm_file().reread();
	    return ret;
	}
    }
    return false;
}
FileUtilities*	ChSize::query_interface(BeyeContext& b) { return new(zeromem) ChSize(b); }

    class InsDelBlock : public FileUtilities {
	public:
	    InsDelBlock(BeyeContext&);
	    virtual ~InsDelBlock();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
	private:
	    bool		InsBlock(binary_stream* bHandle,__filesize_t start,__fileoff_t psize) const;
	    bool		DelBlock(binary_stream* bHandle,__filesize_t start,__fileoff_t psize) const;
    };

InsDelBlock::InsDelBlock(BeyeContext& b):FileUtilities(b) {}
InsDelBlock::~InsDelBlock() {}

bool InsDelBlock::InsBlock(binary_stream* bHandle,__filesize_t start,__fileoff_t psize) const
{
    char *buffer;
    __filesize_t tile,oflen,flen,crpos,cwpos;
    unsigned numtowrite;
    oflen = bHandle->flength();
    flen = oflen + psize;
    tile = oflen - start;
    buffer = new char [51200U];
    if(!buffer) return 0;
    if(!bHandle->chsize(oflen+psize)) {
	bctx.ErrMessageBox(EXPAND_FAIL,"");
	delete buffer;
	return false;
    }
    crpos = oflen-std::min(tile,__filesize_t(51200U));
    cwpos = flen-std::min(tile,__filesize_t(51200U));
    numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
    while(tile) {
	bHandle->seek(crpos,binary_stream::Seek_Set);
	bHandle->read(buffer,numtowrite);
	bHandle->seek(cwpos,binary_stream::Seek_Set);
	bHandle->write(buffer,numtowrite);
	tile -= numtowrite;
	numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
	crpos -= numtowrite;
	cwpos -= numtowrite;
    }
    tile = oflen - start;
    cwpos = start;
    memset(buffer,0,51200U);
    while(psize) {
	numtowrite = (unsigned)std::min(psize,__fileoff_t(51200U));
	bHandle->seek(cwpos,binary_stream::Seek_Set);
	bHandle->write(buffer,numtowrite);
	psize -= numtowrite;
	cwpos += numtowrite;
    }
    delete buffer;
    return true;
}

bool InsDelBlock::DelBlock(binary_stream* bHandle,__filesize_t start,__fileoff_t psize) const
{
    char *buffer;
    __filesize_t tile,oflen,crpos,cwpos;
    unsigned numtowrite;
    oflen = bHandle->flength();
    tile = oflen - start;
    buffer = new char [51200U];
    if(!buffer) return false;
    crpos = start-psize; /** psize is negative value */
    cwpos = start;
    while(tile) {
	numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
	bHandle->seek(crpos,binary_stream::Seek_Set);
	bHandle->read(buffer,numtowrite);
	bHandle->seek(cwpos,binary_stream::Seek_Set);
	bHandle->write(buffer,numtowrite);
	tile -= numtowrite;
	crpos += numtowrite;
	cwpos += numtowrite;
    }
    delete buffer;
    if(!bHandle->chsize(oflen+psize)) {
	bctx.ErrMessageBox(TRUNC_FAIL,"");
	delete buffer;
    }
    return true;
}

bool InsDelBlock::run()
{
    __filesize_t start;
    static __fileoff_t psize;
    bool ret = false;
    start = bctx.tell();
    if(GetInsDelBlkDlg(" Insert or delete block to/from file ",&start,&psize)) {
	__filesize_t fpos;
	binary_stream* bHandle;
	std::string fname;
	fpos = bctx.tell();
	if(start > bctx.flength()) { bctx.ErrMessageBox("Start is outside of file",""); return 0; }
	if(!psize) return 0;
	if(psize < 0) if(start+labs(psize) > bctx.flength()) { bctx.ErrMessageBox("Use change size operation instead of block deletion",""); return 0; }
	fname = bctx.bm_file().filename();
	bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
	if(bHandle == NULL) bctx.errnoMessageBox(OPEN_FAIL,"",errno);
	else {
	    if(psize < 0) ret = DelBlock(bHandle,start,psize);
	    else          ret = InsBlock(bHandle,start,psize);
	    delete bHandle;
	    bctx.bm_file().reread();
	}
	bctx.bm_file().seek(fpos,binary_stream::Seek_Set);
    }
    return ret;
}
FileUtilities*	InsDelBlock::query_interface(BeyeContext& b) { return new(zeromem) InsDelBlock(b); }


    class FStore : public FileUtilities {
	public:
	    FStore(BeyeContext&);
	    virtual ~FStore();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
	protected:
	    virtual void		__make_dump_name(const std::string& end);

	    std::string		ff_fname;
	private:
	    void		printObject(std::ofstream& fout,const Object_Info& obj) const;
	    void		printHdr(std::ofstream& fout,const Bin_Format& fmt) const;
	    std::string		GET_FUNC_CLASS(Symbol_Info::symbol_class x) const { return x == Symbol_Info::Local ? "private" : "public"; }
	    void		make_addr_column(std::string& buff,__filesize_t offset) const;
	    unsigned		printHelpComment(char *buff,MBuffer codebuff,DisasmRet *dret,DisMode::e_severity dis_severity,const char* dis_comments) const;
    };

FStore::FStore(BeyeContext& b):FileUtilities(b),ff_fname("beye.$$$") {}
FStore::~FStore() {}

void FStore::printObject(std::ofstream& fout,const Object_Info& obj) const
{
    const char *btn;
    std::string name;
    char onumname[30];
    switch(obj.bitness) {
	case Bin_Format::Use16: btn = "USE16"; break;
	case Bin_Format::Use32: btn = "USE32"; break;
	case Bin_Format::Use64: btn = "USE64"; break;
	case Bin_Format::Use128:btn = "USE128"; break;
	case Bin_Format::Use256:btn = "USE256"; break;
	default: btn = "";
    }

    name = !obj.name.empty() ? obj.name : obj._class == Object_Info::Data ? "DUMP_DATA" :
			obj._class == Object_Info::Code ? "DUMP_TEXT" :
			"Unknown";
    if(obj.name.empty()) { sprintf(onumname,"%s%u",name.c_str(),obj.number); name = onumname; }
    fout<<std::endl<<"SEGMENT "<<name<<" BYTE PUBLIC "<<btn<<" '"<<(obj._class == Object_Info::Data ? "DATA" : obj._class == Object_Info::Code ? "CODE" : "NoObject")<<"'"<<std::endl;
    fout<<"; size: "<<std::dec<<(obj.end-obj.start)<<" bytes"<<std::endl<<std::endl;
}

void FStore::printHdr(std::ofstream& fout,const Bin_Format& fmt) const
{
    const char *cptr,*cptr1,*cptr2;
    time_t tim;
    cptr = cptr1 = ";"; cptr2 = "";
    time(&tim);
    fout<<cptr1<<std::endl;
    fout<<cptr<<"Disassembler dump of \'"<<bctx.bm_file().filename()<<"\'"<<std::endl;
    fout<<cptr<<"Range : "<<std::hex<<ff_startpos<<"H-"<<std::hex<<(ff_startpos+ff_len)<<"H"<<std::endl;
    fout<<cptr<<"Written by "<<BEYE_VER_MSG<<std::endl;
    fout<<cptr<<"Dumped : "<<ctime(&tim)<<std::endl;
    fout<<cptr<<"Format : "<<fmt.name()<<std::endl;
    fout<<cptr2<<std::endl<<std::endl;
}

unsigned FStore::printHelpComment(char *buff,MBuffer codebuff,DisasmRet *dret,DisMode::e_severity dis_severity,const char* dis_comments) const
{
    unsigned len,j;
    if(dis_severity > DisMode::CommSev_None) {
	len = 3+::strlen(dis_comments);
	::strcat(buff,dis_comments);
	::strcat(buff," ; ");
    } else len = 0;
    for(j = 0;j < dret->codelen;j++) {
	::memcpy((char *)&buff[len],(char *)Get2Digit(codebuff[j]),2);
	len += 2;
    }
    buff[len] = 0;
    return len;
}

void FStore::make_addr_column(std::string& buff,__filesize_t offset) const
{
    if(hexAddressResolv) buff=bctx.bin_format().address_resolving(offset);
    else {
	buff="L";
	buff+=Get8Digit(offset);
    }
    buff+=":";
}

void FStore::__make_dump_name(const std::string& end)
{
    /* construct name */
    ff_fname=bctx.bm_file().filename();
    ff_fname=ff_fname.substr(0,ff_fname.rfind('.'));
    ff_fname+=end;
}

bool FStore::run()
{
    unsigned long flags;
    char *tmp_buff;
    __filesize_t endpos,cpos;
    tmp_buff = new char [0x1000];
    flags = FSDLG_USEMODES | FSDLG_BINMODE | FSDLG_COMMENT;
    DumpMode = true;
    ff_startpos = bctx.tell();
    if(!ff_len) ff_len = bctx.flength() - ff_startpos;
    __make_dump_name(".$$$");
    char ffname[4096];
    strcpy(ffname,ff_fname.c_str());
    if(GetFStoreDlg(" Save information to file ",ffname,&flags,&ff_startpos,&ff_len,FILE_PRMT)) {
	ff_fname=ffname;
	endpos = ff_startpos + ff_len;
	endpos = endpos > bctx.flength() ? bctx.flength() : endpos;
	if(endpos > ff_startpos) {
	    TWindow *progress_wnd;
	    unsigned prcnt_counter,oprcnt_counter;
	    cpos = bctx.tell();
	    progress_wnd = PercentWnd("Saving ..."," Save block to file ");
	    if(!(flags & FSDLG_ASMMODE)) { /** Write in binary mode */
		std::ofstream fs;
		__filesize_t wsize,crpos,pwsize,awsize;
		unsigned rem;
		wsize = endpos - ff_startpos;
		fs.open(ff_fname.c_str(),std::ios_base::out|std::ios_base::binary);
		if(!fs.good())  {
		    bctx.errnoMessageBox("Can't use file","",errno);
		    goto Exit;
		}
		crpos = ff_startpos;
		fs.seekp(0L,std::ios_base::beg);
		prcnt_counter = oprcnt_counter = 0;
		pwsize = 0;
		awsize = wsize;
		while(wsize) {
		    unsigned real_size;
		    rem = (unsigned)std::min(wsize,__filesize_t(4096));
		    bctx.bm_file().seek(crpos,binary_stream::Seek_Set);
		    if(!bctx.bm_file().read(tmp_buff,rem)) {
			bctx.errnoMessageBox(READ_FAIL,"",errno);
			goto Exit;
		    }
		    real_size = (bctx.active_mode().flags()&Plugin::Has_ConvertCP) ? bctx.active_mode().convert_cp((char *)tmp_buff,rem,true) : rem;
		    fs.write(tmp_buff,real_size);
		    if(!fs.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			goto Exit;
		    }
		    wsize -= rem;
		    crpos += rem;
		    pwsize += rem;
		    prcnt_counter = (unsigned)((pwsize*100)/awsize);
		    if(prcnt_counter != oprcnt_counter) {
			oprcnt_counter = prcnt_counter;
			if(!ShowPercentInWnd(progress_wnd,prcnt_counter)) break;
		    }
		}
	    } else { /** Write in disassembler mode */
		std::ofstream fout;
		unsigned char *codebuff;
		char *file_cache = NULL,*tmp_buff2 = NULL;
		unsigned MaxInsnLen;
		char data_dis[300];
		__filesize_t stop;
		Symbol_Info psym;
		__filesize_t awsize,pwsize;
		bool has_string;

		Object_Info obj;

		extern const Plugin_Info disMode;
		DisMode* dismode;
		if(bctx.mode_info()!=&disMode)
		    dismode = static_cast<DisMode*>(disMode.query_interface(bctx,bctx.bin_format(),bctx.bm_file(),bctx.main_wnd(),bctx.codeguider(),bctx._udn(),bctx.search()));
		else
		    dismode = static_cast<DisMode*>(&bctx.active_mode());
		MaxInsnLen = dismode->get_max_symbol_size();
		codebuff = new unsigned char [MaxInsnLen];
		tmp_buff2 = new char [0x1000];
		file_cache = new char [BBIO_SMALL_CACHE_SIZE];
		fout.open(ff_fname.c_str(),std::ios_base::out);
		if(!fout.is_open()) {
		    bctx.errnoMessageBox(WRITE_FAIL,"",errno);
		    delete codebuff;
		    goto Exit;
		}
//		if(file_cache) setvbuf(fout,file_cache,_IOFBF,BBIO_SMALL_CACHE_SIZE);
		if(flags & FSDLG_COMMENT) {
		    printHdr(fout,bctx.bin_format());
		}
		if(flags & FSDLG_STRUCTS) obj = bctx.bin_format().get_object_attribute(ff_startpos);
#if 0
		if(!obj.number) goto defobj;
		else {
		    defobj:
		    obj.number = 0;
		    obj.start = 0;
		    obj.end = bctx.flength();
		    obj.name[0] = 0;
		    obj._class = Object_Info::Code;
		    obj.bitness = bctx.bin_format().query_bitness(ff_startpos);
		}
#endif
		if(flags & FSDLG_STRUCTS) printObject(fout,obj);
		psym.pa = Plugin::Bad_Address;
		if(flags & FSDLG_STRUCTS) {
		    psym = bctx.bin_format().get_public_symbol(ff_startpos,true);
		    if(psym.pa!=Plugin::Bad_Address) {
			fout<<GET_FUNC_CLASS(psym._class)<<" "<<psym.name<<":"<<std::endl;
			if(psym.pa < ff_startpos && flags & FSDLG_COMMENT) {
				fout<<"; ..."<<std::endl;
			}
		    }
		    psym = bctx.bin_format().get_public_symbol(ff_startpos,false);
		}
		prcnt_counter = oprcnt_counter = 0;
		awsize = endpos - ff_startpos;
		pwsize = 0;
		has_string = false;
		while(1) {
		    DisasmRet dret;
		    int len;
		    if(flags & FSDLG_STRUCTS) {
			if(ff_startpos >= obj.end) {
			    obj = bctx.bin_format().get_object_attribute(ff_startpos);
			    printObject(fout,obj);
			}
			if(obj._class == Object_Info::NoObject) {
			    __filesize_t diff;
			    fout<<"; L"<<std::hex<<std::setfill('0')<<std::setw(16)<<obj.start<<"H-L"<<std::hex<<std::setfill('0')<<std::setw(16)<<obj.end<<"H - no object"<<std::endl;
			    dret.codelen = std::min(__filesize_t(UCHAR_MAX),obj.end - ff_startpos);
			    /** some functions can placed in virtual area of objects
				mean at end of true data, but before next object */
			    while(psym.pa!=Plugin::Bad_Address && psym.pa >= obj.start && psym.pa < obj.end && psym.pa > ff_startpos) {
				diff = psym.pa - ff_startpos;
				if(diff) fout<<"resb "<<std::dec<<diff<<"H"<<std::endl;
				fout<<GET_FUNC_CLASS(psym._class)<<" "<<psym.name<<": ;at offset - "<<std::hex<<psym.pa<<"H"<<std::endl;
				ff_startpos = psym.pa;
				psym = bctx.bin_format().get_public_symbol(ff_startpos,false);
				if(psym.pa==ff_startpos) {
				    fout<<"...Probably internal error of beye..."<<std::endl;
				    break;
				}
			    }
			    diff = obj.end - ff_startpos;
			    if(diff) fout<<"resb "<<std::hex<<diff<<"H"<<std::endl;
			    ff_startpos = obj.end;
			    goto next_obj;
			}
			if(psym.pa!=Plugin::Bad_Address) {
			    int not_silly;
			    not_silly = 0;
			    while(ff_startpos == psym.pa) {
				/* print out here all public labels */
				fout<<GET_FUNC_CLASS(psym._class)<<" "<<psym.name<<std::endl;
				psym = bctx.bin_format().get_public_symbol(ff_startpos,false);
				not_silly++;
				if(not_silly > 100) {
				    fout<<"; [snipped out] ..."<<std::endl;
				    break;
				}
			    }
			}
		    }
		    memset(codebuff,0,MaxInsnLen);
		    bctx.bm_file().seek(ff_startpos,binary_stream::Seek_Set);
		    bctx.bm_file().read((any_t*)codebuff,MaxInsnLen);
		    if(obj._class == Object_Info::Code) dret = dismode->disassembler(ff_startpos,codebuff,__DISF_NORMAL);
		    else { /** Data object */
			unsigned dis_data_len,ifreq,data_len;
			char coll_str[__TVIO_MAXSCREENWIDTH];
			size_t cstr_idx = 0;
			dis_data_len = std::min(sizeof(coll_str)-1,size_t(MaxInsnLen));
			for(cstr_idx = 0;cstr_idx < dis_data_len;cstr_idx++) {
			    if(isprint(codebuff[cstr_idx])) {
				coll_str[cstr_idx] = codebuff[cstr_idx];
			    } else break;
			}
			coll_str[cstr_idx] = 0;
			switch(obj.bitness) {
			    case Bin_Format::Use16: dis_data_len = 2; break;
			    case Bin_Format::Use32: dis_data_len = 4; break;
			    case Bin_Format::Use64: dis_data_len = 8; break;
			    case Bin_Format::Use128: dis_data_len = 16; break;
			    case Bin_Format::Use256: dis_data_len = 32; break;
			    default:         dis_data_len = 1; break;
			}
			data_len = 0;
			sprintf(data_dis,"db  ");
			if(cstr_idx > 1) {
			    sprintf(&data_dis[strlen(data_dis)],"'%s'",coll_str);
			    dret.codelen = cstr_idx;
			    has_string = true;
			} else {
			    for(ifreq = 0;ifreq < dis_data_len;ifreq++) {
				if(isprint(codebuff[ifreq]) && isprint(codebuff[ifreq+1])) break;
				if(isprint(codebuff[ifreq]) && has_string) sprintf(&data_dis[strlen(data_dis)],"'%c',",codebuff[ifreq]);
				else		sprintf(&data_dis[strlen(data_dis)],"%02Xh,",codebuff[ifreq]);
				data_len++;
				has_string = false;
			    }
			    dret.codelen = data_len;
			}
			dret.str = data_dis;
			dret.pro_clone = 0;
			dismode->dis_severity = DisMode::CommSev_None;
		    }
		    stop = psym.pa!=Plugin::Bad_Address ? std::min(psym.pa,obj.end) : obj.end;
		    if(flags & FSDLG_STRUCTS) {
			if(stop && stop > ff_startpos && ff_startpos + dret.codelen > stop) {
			    unsigned lim,ii;
			    std::string stmp=(char*)tmp_buff;
			    make_addr_column(stmp,ff_startpos);
			    strcpy(tmp_buff,stmp.c_str());
			    strcat(tmp_buff," db ");
			    lim = (unsigned)(stop-ff_startpos);
			    if(lim > MaxInsnLen) lim = MaxInsnLen;
			    for(ii = 0;ii < lim;ii++) sprintf(&tmp_buff[strlen(tmp_buff)],"%s ",Get2Digit(codebuff[ii]));
			    dret.codelen = lim;
			} else goto normline;
		    } else {
			normline:
			std::string stmp=(char*)tmp_buff;
			make_addr_column(stmp,ff_startpos);
			strcpy(tmp_buff,stmp.c_str());
			sprintf(&tmp_buff[strlen(tmp_buff)]," %s",dret.str);
		    }
		    len = strlen(tmp_buff);
		    if(flags & FSDLG_COMMENT) {
			if(len < 48) {
			    memset(&tmp_buff[len],' ',48-len);
			    len = 48;
			    tmp_buff[len] = 0;
			}
			strcat(tmp_buff,"; ");
			len += 2;
			len += printHelpComment(&((char *)tmp_buff)[len],codebuff,&dret,dismode->dis_severity,dismode->dis_comments);
		    }
		    if(tmp_buff2) {
			szSpace2Tab(tmp_buff2,tmp_buff);
			szTrimTrailingSpace(tmp_buff2);
		    }
		    fout<<(tmp_buff2 ? tmp_buff2 : tmp_buff)<<std::endl;
		    if(!fout.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			goto dis_exit;
		    }
		    if(flags & FSDLG_STRUCTS) {
			if(stop && ff_startpos != stop && ff_startpos + dret.codelen > stop)
			    dret.codelen = stop - ff_startpos;
		    }
		    if(!dret.codelen) {
			bctx.ErrMessageBox("Internal fatal error"," Put structures ");
			goto dis_exit;
		    }
		    ff_startpos += dret.codelen;
		    next_obj:
		    if(ff_startpos >= endpos) break;
		    pwsize += dret.codelen;
		    prcnt_counter = (unsigned)((pwsize*100)/awsize);
		    if(prcnt_counter != oprcnt_counter) {
			oprcnt_counter = prcnt_counter;
			if(!ShowPercentInWnd(progress_wnd,prcnt_counter)) break;
		    }
		}
		dis_exit:
		delete codebuff;
		fout.close();
		if(file_cache) delete file_cache;
		if(tmp_buff2) delete tmp_buff2;
		if(bctx.mode_info()!=&disMode)	delete dismode;
	    } /** END: Write in disassembler mode */
	    Exit:
	    delete progress_wnd;
	    bctx.bm_file().seek(cpos,binary_stream::Seek_Set);
	} else  bctx.ErrMessageBox("Start position > end position!","");
    }
    delete tmp_buff;
    DumpMode = false;
    return false;
}
FileUtilities*	FStore::query_interface(BeyeContext& b) { return new(zeromem) FStore(b); }

    class FRestore : public FStore {
	public:
	    FRestore(BeyeContext&);
	    virtual ~FRestore();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
    };

FRestore::FRestore(BeyeContext& b):FStore(b) {}
FRestore::~FRestore() {}

bool FRestore::run()
{
    __filesize_t endpos,cpos;
    unsigned long flags;
    bool ret = false;
    flags = FSDLG_NOMODES;
    __make_dump_name(".$$$");
    char ffname[4096];
    strcpy(ffname,ff_fname.c_str());
    if(GetFStoreDlg(" Restore information from file ",ffname,&flags,&ff_startpos,&ff_len,FILE_PRMT)) {
	ff_fname=ffname;
	__filesize_t flen,lval;
	binary_stream* h = new(zeromem) binary_stream;
	std::string fname;
	endpos = ff_startpos + ff_len;
	if(!h->open(ff_fname,binary_stream::FO_READONLY | binary_stream::SO_DENYNONE)) {
	    if(!h->open(ff_fname,binary_stream::FO_READONLY | binary_stream::SO_COMPAT)) {
		goto err;
	    }
	}
	flen = h->flength();
	delete h;
	lval = endpos - ff_startpos;
	endpos = lval > flen ? flen + ff_startpos : endpos;
	endpos = endpos > bctx.flength() ? bctx.flength() : endpos;
	if(endpos > ff_startpos) {
	    __filesize_t wsize,cwpos;
	    unsigned remaind;
	    char* tmp_buff;
	    h = new(zeromem) binary_stream;
	    if(!h->open(ff_fname,binary_stream::FO_READONLY | binary_stream::SO_DENYNONE)) {
		if(!h->open(ff_fname,binary_stream::FO_READONLY | binary_stream::SO_COMPAT)) {
err:
		    delete h;
		    bctx.errnoMessageBox(OPEN_FAIL,"",errno);
		    return false;
		}
	    }
	    cpos = bctx.tell();
	    wsize = endpos - ff_startpos;
	    cwpos = ff_startpos;
	    h->seek(0L,binary_stream::Seek_Set);
	    tmp_buff = new char [4096];
	    fname = bctx.bm_file().filename();
	    std::fstream fs;
	    fs.open(fname.c_str(),std::ios_base::in|std::ios_base::out|std::ios_base::binary);
	    if(fs.is_open()) {
		while(wsize) {
		    remaind = (unsigned)std::min(wsize,__filesize_t(4096));
		    if(!h->read((any_t*)tmp_buff,remaind)) {
			bctx.errnoMessageBox(READ_FAIL,"",errno);
			delete h;
			ret = false;
			goto bye;
		    }
		    fs.seekp(cwpos,std::ios_base::beg);
		    fs.write(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    wsize -= remaind;
		    cwpos += remaind;
		}
bye:
		fs.close();
		bctx.bm_file().reread();
	    } else bctx.errnoMessageBox(OPEN_FAIL,"",errno);
	    delete tmp_buff;
	    delete h;
	    bctx.bm_file().seek(cpos,binary_stream::Seek_Set);
	    ret = true;
	} else bctx.ErrMessageBox("Start position > end position!","");
    }
    return ret;
}
FileUtilities*	FRestore::query_interface(BeyeContext& b) { return new(zeromem) FRestore(b); }

    class CryptBlock : public FileUtilities {
	public:
	    CryptBlock(BeyeContext&);
	    virtual ~CryptBlock();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
	private:
	    void		CryptFunc(char* buff,unsigned len,std::string& pass) const;
    };

CryptBlock::CryptBlock(BeyeContext& b):FileUtilities(b) {}
CryptBlock::~CryptBlock() {}

void CryptBlock::CryptFunc(char* buff,unsigned len,std::string& pass) const
{
    char ch,cxor;
    unsigned i,j;
    unsigned bigkey_idx;
    unsigned passlen;
    char big_key[UCHAR_MAX];
    cxor = 0;
    passlen = pass.length();
    ::memset(big_key,0,sizeof(big_key));
    for(j = 0;j < passlen;j++) cxor += pass[j]+j;
    cxor ^= passlen + len;
    for(j = i = 0;i < UCHAR_MAX;i++,j++) {
	if(j > passlen) j = 0;
	bigkey_idx = (pass[j] + i) * cxor;
	if(bigkey_idx > UCHAR_MAX) bigkey_idx = (bigkey_idx & 0xFF) ^ ((bigkey_idx >> 8) & 0xFF);
	big_key[i] = bigkey_idx;
    }
    for(bigkey_idx = j = i = 0;i < len;i++,bigkey_idx++,j++) {
	unsigned short _xor;
	if(bigkey_idx > UCHAR_MAX) {
	    /** rotate of big key */
	    ch = big_key[0];
	    ::memmove(big_key,&big_key[1],UCHAR_MAX-1);
	    big_key[UCHAR_MAX-1] = ch;
	    bigkey_idx = 0;
	}
	if(j > passlen) j = 0;
	_xor = (big_key[bigkey_idx] + i)*cxor;
	if(_xor > UCHAR_MAX) _xor = (_xor & 0xFF) ^ ((_xor >> 8) & 0xFF);
	buff[i] = buff[i] ^ _xor;
    }
    /** rotate of pass */
    ch = pass[0];
    pass = pass.substr(1);
    pass[passlen-1] = ch;
}

bool CryptBlock::run()
{
    __filesize_t endpos,cpos;
    unsigned long flags;
    char pass[81];
    bool ret;
    ret = false;
    ff_startpos = bctx.tell();
    if(!ff_len) ff_len = bctx.flength() - ff_startpos;
    pass[0] = 0;
    flags = FSDLG_NOMODES;
    if(GetFStoreDlg(" (De)Crypt block of file ",pass,&flags,&ff_startpos,&ff_len,"Input password (WARNING! password will be displayed):")) {
	std::string passwd = pass;
	__filesize_t flen,lval;
	endpos = ff_startpos + ff_len;
	flen = bctx.flength();
	lval = endpos - ff_startpos;
	endpos = lval > flen ? flen + ff_startpos : endpos;
	endpos = endpos > bctx.flength() ? bctx.flength() : endpos;
	if(!passwd[0]) { bctx.ErrMessageBox("Password can't be empty",""); return false; }
	if(endpos > ff_startpos) {
	    __filesize_t wsize,cwpos;
	    unsigned remaind;
	    std::string fname;
	    char* tmp_buff;
	    cpos = bctx.tell();
	    wsize = endpos - ff_startpos;
	    cwpos = ff_startpos;
	    tmp_buff = new char [4096];
	    fname = bctx.bm_file().filename();
	    std::fstream fs;
	    fs.open(fname.c_str(),std::ios_base::in|std::ios_base::out|std::ios_base::binary);
	    if(fs.is_open()) {
		fs.seekg(ff_startpos,std::ios_base::beg);
		while(wsize) {
		    remaind = (unsigned)std::min(wsize,__filesize_t(4096));
		    fs.read(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(READ_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    CryptFunc(tmp_buff,remaind,passwd);
		    fs.seekp(cwpos,std::ios_base::beg);
		    fs.write(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    wsize -= remaind;
		    cwpos += remaind;
		}
bye:
		fs.close();
		bctx.bm_file().reread();
	    }
	    delete tmp_buff;
	    bctx.bm_file().seek(cpos,binary_stream::Seek_Set);
	    ret = true;
	} else bctx.ErrMessageBox("Start position > end position!","");
    }
    return ret;
}
FileUtilities*	CryptBlock::query_interface(BeyeContext& b) { return new(zeromem) CryptBlock(b); }

    class EndianifyBlock : public FileUtilities {
	public:
	    EndianifyBlock(BeyeContext&);
	    virtual ~EndianifyBlock();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
	private:
	    void		EndianifyFunc(char* buff,unsigned len, int type) const;
    };

EndianifyBlock::EndianifyBlock(BeyeContext& b):FileUtilities(b) {}
EndianifyBlock::~EndianifyBlock() {}

void EndianifyBlock::EndianifyFunc(char* buff,unsigned len, int type) const
{
    unsigned i, step;
    if(!type) return; /* for now */
    switch(type) {
	default:
	case 3: step = 8;
	    break;
	case 2: step = 4;
	    break;
	case 1:
	    step = 2;
	    break;
    }
    len /= step;
    len *= step;
    for(i = 0;i < len;i+=step, buff+=step) {
	switch(type) {
	    default:
	    case 3:
		*((uint64_t *)buff) = bswap_64(*((uint64_t *)buff));
		break;
	    case 2:
		*((uint32_t *)buff) = bswap_32(*((uint32_t *)buff));
		break;
	    case 1:
		*((uint16_t *)buff) = bswap_16(*((uint16_t *)buff));
		break;
	}
    }
}

bool EndianifyBlock::run()
{
    __filesize_t endpos,cpos;
    unsigned long flags;
    bool ret;
    ret = false;
    ff_startpos = bctx.tell();
    if(!ff_len) ff_len = bctx.flength() - ff_startpos;
    flags = FSDLG_USEBITNS;
    if(GetFStoreDlg(" Endianify block of file ",NULL,&flags,&ff_startpos,&ff_len,NULL)) {
	__filesize_t flen,lval;
	endpos = ff_startpos + ff_len;
	flen = bctx.flength();
	lval = endpos - ff_startpos;
	endpos = lval > flen ? flen + ff_startpos : endpos;
	endpos = endpos > bctx.flength() ? bctx.flength() : endpos;
	if(endpos > ff_startpos) {
	    __filesize_t wsize,cwpos;
	    unsigned remaind;
	    std::string fname;
	    std::fstream fs;
	    char* tmp_buff;
	    cpos = bctx.tell();
	    wsize = endpos - ff_startpos;
	    cwpos = ff_startpos;
	    tmp_buff = new char [4096];
	    fname = bctx.bm_file().filename();
	    fs.open(fname.c_str(),std::ios_base::in|std::ios_base::out|std::ios_base::binary);
	    if(fs.is_open()) {
		fs.seekg(ff_startpos,std::ios_base::beg);
		while(wsize) {
		    remaind = (unsigned)std::min(wsize,__filesize_t(4096));
		    fs.read(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(READ_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    EndianifyFunc(tmp_buff,remaind, flags & FSDLG_BTNSMASK);
		    fs.seekp(cwpos,std::ios_base::beg);
		    fs.write(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    wsize -= remaind;
		    cwpos += remaind;
		}
bye:
		fs.close();
		bctx.bm_file().reread();
	    }
	    delete tmp_buff;
	    bctx.bm_file().seek(cpos,binary_stream::Seek_Set);
	    ret = true;
	} else bctx.ErrMessageBox("Start position > end position!","");
    }
    return ret;
}
FileUtilities*	EndianifyBlock::query_interface(BeyeContext& b) { return new(zeromem) EndianifyBlock(b); }

    class XLatBlock : public FileUtilities {
	public:
	    XLatBlock(BeyeContext&);
	    virtual ~XLatBlock();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
	private:
	    void		XLatFunc(char* buff,unsigned len, const unsigned char *xlt) const;
	    std::string		xlat_fname;
    };

XLatBlock::XLatBlock(BeyeContext& b):FileUtilities(b) {}
XLatBlock::~XLatBlock() {}

void XLatBlock::XLatFunc(char* buff,unsigned len, const unsigned char *xlt) const
{
    unsigned i;
    for(i = 0;i < len;i++) buff[i] = xlt[(int)buff[i]];
}

bool XLatBlock::run()
{
    unsigned char xlt[256];
    __filesize_t endpos,cpos;
    unsigned long flags;
    bool ret;
    ret = false;
    ff_startpos = bctx.tell();
    if(!ff_len) ff_len = bctx.flength() - ff_startpos;
    flags = FSDLG_NOMODES;
    if(xlat_fname.empty()) xlat_fname=bctx.system().get_rc_dir("beye")+"xlt";
    char ffname[4096];
    strcpy(ffname,xlat_fname.c_str());
    if(GetFStoreDlg(" Table Look-up Translation ",ffname,&flags,&ff_startpos,&ff_len,XLAT_PRMT)) {
	xlat_fname=ffname;
	__filesize_t flen,lval;
	endpos = ff_startpos + ff_len;
	flen = bctx.flength();
	lval = endpos - ff_startpos;
	endpos = lval > flen ? flen + ff_startpos : endpos;
	endpos = endpos > bctx.flength() ? bctx.flength() : endpos;
	if(endpos > ff_startpos) {
	    __filesize_t wsize,cwpos;
	    unsigned remaind;
	    std::string fname;
	    char* tmp_buff;
	    cpos = bctx.tell();
	    wsize = endpos - ff_startpos;
	    cwpos = ff_startpos;
	    /* Parse xlat file */
	    binary_stream* xHandle;
	    xHandle = BeyeContext::beyeOpenRO(xlat_fname,BBIO_SMALL_CACHE_SIZE);
	    if(xHandle == NULL) {
		bctx.ErrMessageBox("Can't open xlat file", "");
		return false;
	    }
	    if(xHandle->flength() != 320) {
		bctx.ErrMessageBox("Size of xlat file is not 320 bytes", "");
		delete xHandle;
		return false;
	    }
	    xHandle->read(xlt, 16);
	    if(memcmp(xlt, "Beye Xlat Table.", 16) != 0) {
		bctx.ErrMessageBox("It seems that xlat file is corrupt", "");
		delete xHandle;
		return false;
	    }
	    xHandle->seek(0x40, binary_stream::Seek_Set);
	    xHandle->read(xlt, 256);
	    delete xHandle;
	    tmp_buff = new char [4096];
	    std::fstream fs;
	    fname = bctx.bm_file().filename();
	    fs.open(fname.c_str(),std::ios_base::binary);
	    if(fs.is_open()) {
		fs.seekg(ff_startpos,std::ios_base::beg);
		while(wsize) {
		    remaind = (unsigned)std::min(wsize,__filesize_t(4096));
		    fs.read(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(READ_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    XLatFunc(tmp_buff,remaind, xlt);
		    fs.seekp(cwpos,std::ios_base::beg);
		    fs.write(tmp_buff,remaind);
		    if(!fs.good()) {
			bctx.errnoMessageBox(WRITE_FAIL,"",errno);
			ret = false;
			goto bye;
		    }
		    wsize -= remaind;
		    cwpos += remaind;
		}
bye:
		fs.close();
		bctx.bm_file().reread();
	    }
	    delete tmp_buff;
	    bctx.bm_file().seek(cpos,binary_stream::Seek_Set);
	    ret = true;
	} else bctx.ErrMessageBox("Start position > end position!","");
    }
    return ret;
}
FileUtilities*	XLatBlock::query_interface(BeyeContext& b) { return new(zeromem) XLatBlock(b); }

    class FileInfo : public FileUtilities {
	public:
	    FileInfo(BeyeContext&);
	    virtual ~FileInfo();

	    virtual bool	run();

	    static FileUtilities*	query_interface(BeyeContext&);
    };

FileInfo::FileInfo(BeyeContext& b):FileUtilities(b) {}
FileInfo::~FileInfo() {}

bool FileInfo::run()
{
    TWindow* wnd;
    struct stat statbuf;
    unsigned evt;
    char attr[14];
    char stimes[3][80];
    ::memset(&statbuf,0,sizeof(struct stat));
    ::stat(bctx.bm_file().filename().c_str(),&statbuf);
    ::memset(attr,'-',sizeof(attr));
    attr[sizeof(attr)-1] = 0;
#ifdef S_IXOTH /** Execute by other */
    if((statbuf.st_mode & S_IXOTH) == S_IXOTH) attr[12] = 'X';
#endif
#ifdef S_IWOTH /** Write by other */
    if((statbuf.st_mode & S_IWOTH) == S_IWOTH) attr[11] = 'W';
#endif
#ifdef S_IROTH /** Read by other */
    if((statbuf.st_mode & S_IROTH) == S_IROTH) attr[10] = 'R';
#endif
#ifdef S_IXGRP /** Execute by group */
    if((statbuf.st_mode & S_IXGRP) == S_IXGRP) attr[9] = 'X';
#endif
#ifdef S_IWGRP /** Write by group */
    if((statbuf.st_mode & S_IWGRP) == S_IWGRP) attr[8] = 'W';
#endif
#ifdef S_IRGRP /** Read by group */
    if((statbuf.st_mode & S_IRGRP) == S_IRGRP) attr[7] = 'R';
#endif
#ifdef S_IEXEC /** Execute by owner */
    if((statbuf.st_mode & S_IEXEC) == S_IEXEC) attr[6] = 'X';
#endif
#ifdef S_IWRITE /** Write by owner */
    if((statbuf.st_mode & S_IWRITE) == S_IWRITE) attr[5] = 'W';
#endif
#ifdef S_IREAD /** Read by owner */
    if((statbuf.st_mode & S_IREAD) == S_IREAD) attr[4] = 'R';
#endif
#ifdef S_ISVTX /** Save swapped text after use (obsolete) */
    if((statbuf.st_mode & S_ISVTX) == S_ISVTX) attr[3] = 'V';
#endif
#ifdef S_ISGID /** Set GID on execution */
    if((statbuf.st_mode & S_ISGID) == S_ISGID) attr[2] = 'G';
#endif
#ifdef S_ISUID /** Set UID on execution */
    if((statbuf.st_mode & S_ISUID) == S_ISUID) attr[1] = 'U';
#endif
#ifdef S_ISFIFO /** it FIFO */
    if(S_ISFIFO(statbuf.st_mode)) attr[0] = 'F';
#endif
#ifdef S_IFCHR /** it character device */
    if((statbuf.st_mode & S_IFCHR) == S_IFCHR) attr[0] = 'C';
#endif
#ifdef S_IFDIR /** it directory */
    if((statbuf.st_mode & S_IFDIR) == S_IFDIR) attr[0] = 'D';
#endif
#ifdef S_IFBLK /** it block device */
    if((statbuf.st_mode & S_IFBLK) == S_IFBLK) attr[0] = 'B';
#endif
#ifdef S_IFREG /** it regular file (not dir or node) */
    if((statbuf.st_mode & S_IFREG) == S_IFREG) attr[0] = 'f';
#endif
#ifdef S_IFLNK /** it symbolic link */
    if((statbuf.st_mode & S_IFLNK) == S_IFLNK) attr[0] = 'L';
#endif
#ifdef S_IFSOCK /** it socket */
    if((statbuf.st_mode & S_IFSOCK) == S_IFSOCK) attr[0] = 'S';
#endif
    wnd = CrtDlgWndnls(" File information: ",bctx.tconsole().vio_width()-5,13);
    wnd->goto_xy(1,1);
    ::strcpy(stimes[0],ctime(&statbuf.st_ctime));
    ::strcpy(stimes[1],ctime(&statbuf.st_mtime));
    ::strcpy(stimes[2],ctime(&statbuf.st_atime));
    wnd->printf(
	   "Name                          = %s\n"
	   "Type                          = %s\n"
	   "Length                        = %llu bytes\n"
	   "Attributes                    = %s\n"
	   "                                TUGVOwnGrpOth\n"
	   "Creation time                 = %s"
	   "Modification time             = %s"
	   "Last access time              = %s"
	   "Device containing file        = %u\n"
	   "File serial (inode) number    = %u\n"
	   "Number of hard links to file  = %u\n"
	   "User ID of the file owner     = %u\n"
	   "Group ID of the file owner    = %u"
	   ,bctx.short_name()
	   ,bctx.bin_format().name()
	   ,bctx.flength()
	   ,attr
	   ,stimes[0]
	   ,stimes[1]
	   ,stimes[2]
	   ,(unsigned)statbuf.st_dev
	   ,(unsigned)statbuf.st_ino
	   ,(unsigned)statbuf.st_nlink
	   ,(unsigned)statbuf.st_uid
	   ,(unsigned)statbuf.st_gid);
    do {
	evt = GetEvent(drawEmptyPrompt,NULL,wnd);
    }while(!(evt == KE_ESCAPE || evt == KE_F(10)));
    delete wnd;
    return false;
}
FileUtilities*	FileInfo::query_interface(BeyeContext& b) { return new(zeromem) FileInfo(b); }

    struct file_utilities {
	const char*	name;
	FileUtilities*	(*query_interface)(BeyeContext&);
    };

static const file_utilities utilities[] = {
  { "~File information...", FileInfo::query_interface },
  { "C~hange size of file", ChSize::query_interface },
  { "~Save block as...", FStore::query_interface },
  { "~Restore block from...", FRestore::query_interface },
  { "~Insert/delete block...", InsDelBlock::query_interface },
  { "~Crypt/decrypt block...", CryptBlock::query_interface },
  { "~Endianify block...", EndianifyBlock::query_interface },
  { "~Xlat block...", XLatBlock::query_interface }
};

bool FileUtils()
{
    BeyeContext& bctx = beye_context();
    size_t nUtils = sizeof(utilities)/sizeof(file_utilities);
    std::vector<std::string> names;
    int retval;
    bool ret;
    static unsigned def_sel = 0;
    for(unsigned i=0;i<nUtils;i++) names.push_back(utilities[i].name);
    ListBox lb(bctx);
    retval = lb.run(names," File utilities: ",ListBox::Selective|ListBox::UseAcc,def_sel);
    if(retval != -1) {
	TWindow* w = PleaseWaitWnd();
	FileUtilities* util = utilities[retval].query_interface(bctx);
	ret = util->run();
	delete util;
	delete w;
	def_sel = retval;
	return ret;
    }
    return false;
}
} // namespace	usr

