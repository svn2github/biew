#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
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
#include "bmfile.h"
#include "tstrings.h"
#include "plugins/hexmode.h"
#include "plugins/disasm.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "reg_form.h"
#include "libbeye/bbio.h"
#include "libbeye/bswap.h"
#include "libbeye/twin.h"
#include "libbeye/kbd_code.h"


namespace beye {
extern const Plugin_Info disMode;
static bool ChSize( void )
{
 __fileoff_t psize,tile = 0;
 if(Get16DigitDlg(" Change size of file ","Num. of bytes (+-dec):",3,(unsigned long long*)&tile))
 {
  if(tile != 0)
  {
    psize = BMGetFLength();
    psize += tile;
    if(psize > 0)
    {
       bool ret;
       int my_errno = 0;
       const char *fname = BMName();
       BFile* bHandle;
       bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
       if(bHandle == &bNull)
       {
	 err:
	 errnoMessageBox(RESIZE_FAIL,NULL,my_errno);
	 return false;
       }
       ret = bHandle->chsize(psize);
       my_errno = errno;
       delete bHandle;
       if(ret == false) goto err;
       BMReRead();
       return ret;
    }
    else ErrMessageBox("Invalid new length",NULL);
  }
 }
 return false;
}

static bool  __FASTCALL__ InsBlock(BFile* bHandle,__filesize_t start,__fileoff_t psize)
{
   char *buffer;
   __filesize_t tile,oflen,flen,crpos,cwpos;
   unsigned numtowrite;
   oflen = bHandle->flength();
   flen = oflen + psize;
   tile = oflen - start;
   buffer = new char [51200U];
   if(!buffer) return 0;
   if(!bHandle->chsize(oflen+psize))
   {
     ErrMessageBox(EXPAND_FAIL,NULL);
     delete buffer;
     return false;
   }
   crpos = oflen-std::min(tile,__filesize_t(51200U));
   cwpos = flen-std::min(tile,__filesize_t(51200U));
   numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
   while(tile)
   {
     bHandle->seek(crpos,BFile::Seek_Set);
     bHandle->read_buffer(buffer,numtowrite);
     bHandle->seek(cwpos,BFile::Seek_Set);
     bHandle->write_buffer(buffer,numtowrite);
     tile -= numtowrite;
     numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
     crpos -= numtowrite;
     cwpos -= numtowrite;
   }
   tile = oflen - start;
   cwpos = start;
   memset(buffer,0,51200U);
   while(psize)
   {
     numtowrite = (unsigned)std::min(psize,__fileoff_t(51200U));
     bHandle->seek(cwpos,BFile::Seek_Set);
     bHandle->write_buffer(buffer,numtowrite);
     psize -= numtowrite;
     cwpos += numtowrite;
   }
   delete buffer;
   return true;
}

static bool  __FASTCALL__ DelBlock(BFile* bHandle,__filesize_t start,__fileoff_t psize)
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
   while(tile)
   {
     numtowrite = (unsigned)std::min(tile,__filesize_t(51200U));
     bHandle->seek(crpos,BFile::Seek_Set);
     bHandle->read_buffer(buffer,numtowrite);
     bHandle->seek(cwpos,BFile::Seek_Set);
     bHandle->write_buffer(buffer,numtowrite);
     tile -= numtowrite;
     crpos += numtowrite;
     cwpos += numtowrite;
   }
   delete buffer;
   if(!bHandle->chsize(oflen+psize))
   {
     ErrMessageBox(TRUNC_FAIL,NULL);
     delete buffer;
   }
   return true;
}

static bool InsDelBlock( void )
{
 __filesize_t start;
 static __fileoff_t psize;
 bool ret = false;
 start = BMGetCurrFilePos();
 if(GetInsDelBlkDlg(" Insert or delete block to/from file ",&start,&psize))
 {
    __filesize_t fpos;
    BFile* bHandle;
    const char *fname;
    fpos = BMGetCurrFilePos();
    if(start > BMGetFLength()) { ErrMessageBox("Start is outside of file",NULL); return 0; }
    if(!psize) return 0;
    if(psize < 0) if(start+labs(psize) > BMGetFLength()) { ErrMessageBox("Use change size operation instead of block deletion",NULL); return 0; }
    fname = BMName();
    bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
    if(bHandle == &bNull)
    {
      errnoMessageBox(OPEN_FAIL,NULL,errno);
    }
    else
    {
      if(psize < 0) ret = DelBlock(bHandle,start,psize);
      else          ret = InsBlock(bHandle,start,psize);
      delete bHandle;
      BMReRead();
    }
    BMSeek(fpos,BFile::Seek_Set);
 }
 return ret;
}

static char ff_fname[FILENAME_MAX+1] = "beye.$$$";
static char xlat_fname[FILENAME_MAX+1];
static __filesize_t ff_startpos = 0L,ff_len = 0L;

static void  __FASTCALL__ printObject(FILE *fout,unsigned obj_num,char *oname,int oclass,int obitness,__filesize_t size)
{
  const char *name,*btn;
  char onumname[30];
  switch(obitness)
  {
    case DAB_USE16: btn = "USE16"; break;
    case DAB_USE32: btn = "USE32"; break;
    case DAB_USE64: btn = "USE64"; break;
    case DAB_USE128:btn = "USE128"; break;
    case DAB_USE256:btn = "USE256"; break;
    default: btn = "";
  }

  name = oname[0] ? oname : oclass == OC_DATA ? "DUMP_DATA" :
			    oclass == OC_CODE ? "DUMP_TEXT" :
			    "Unknown";
  if(!oname[0]) { sprintf(onumname,"%s%u",name,obj_num); name = onumname; }
  fprintf(fout,"\nSEGMENT %s BYTE PUBLIC %s '%s'\n; size: %llu bytes\n\n"
	      ,name
	      ,btn
	      ,oclass == OC_DATA ? "DATA" : oclass == OC_CODE ? "CODE" : "NoObject"
	      ,size);
}

static void  __FASTCALL__ printHdr(FILE * fout,const Bin_Format& fmt)
{
  const char *cptr,*cptr1,*cptr2;
  time_t tim;
  cptr = cptr1 = ";"; cptr2 = "";
  time(&tim);
  fprintf(fout,"%s\n%sDisassembler dump of \'%s\'\n"
	       "%sRange : %16llXH-%16llXH\n"
	       "%sWritten by %s\n"
	       "%sDumped : %s\n"
	       "%sFormat : %s\n"
	       "%s\n\n"
	      ,cptr1,cptr,BMName()
	      ,cptr,ff_startpos,ff_startpos+ff_len
	      ,cptr,BEYE_VER_MSG
	      ,cptr,ctime(&tim)
	      ,cptr,fmt.name()
	      ,cptr2);
}

static unsigned  __FASTCALL__ printHelpComment(char *buff,MBuffer codebuff,DisasmRet *dret,DisMode::e_severity dis_severity,const char* dis_comments)
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

inline const char* GET_FUNC_CLASS(unsigned x) { return x == SC_LOCAL ? "private" : "public"; }

static void  __FASTCALL__ make_addr_column(char *buff,__filesize_t offset)
{
   if(hexAddressResolv)
   {
     buff[0] = 0;
     beye_context().bin_format().address_resolving(buff,offset);
   }
   else sprintf(buff,"L%s",Get8Digit(offset));
   strcat(buff,":");
}

static void __make_dump_name(const char *end)
{
 /* construct name */
 char *p;
 strcpy(ff_fname,BMName());
 p = strrchr(ff_fname,'.');
 if(!p) p = &ff_fname[strlen(ff_fname)];
 strcpy(p,end);
}

static bool FStore( void )
{
    BeyeContext& bctx = beye_context();
    unsigned long flags;
    char *tmp_buff;
    __filesize_t endpos,cpos;
    tmp_buff = new char [0x1000];
    if(!tmp_buff) {
	 MemOutBox("temporary buffer initialization");
	 return false;
    }
    flags = FSDLG_USEMODES | FSDLG_BINMODE | FSDLG_COMMENT;
    DumpMode = true;
    ff_startpos = BMGetCurrFilePos();
    if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
    __make_dump_name(".$$$");
    if(GetFStoreDlg(" Save information to file ",ff_fname,&flags,&ff_startpos,&ff_len,FILE_PRMT)) {
	endpos = ff_startpos + ff_len;
	endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
	if(endpos > ff_startpos) {
	    TWindow *progress_wnd;
	    unsigned prcnt_counter,oprcnt_counter;
	    cpos = BMGetCurrFilePos();
	    progress_wnd = PercentWnd("Saving ..."," Save block to file ");
	    if(!(flags & FSDLG_ASMMODE)) { /** Write in binary mode */
		BFile* _bioHandle;
		bhandle_t handle;
		__filesize_t wsize,crpos,pwsize,awsize;
		unsigned rem;
		wsize = endpos - ff_startpos;
		if(__IsFileExists(ff_fname) == false) handle = __OsCreate(ff_fname);
		else {
		    handle = __OsOpen(ff_fname,FO_READWRITE | SO_DENYNONE);
		    if(handle == NULL_HANDLE) handle = __OsOpen(ff_fname,FO_READWRITE | SO_COMPAT);
		    if(handle == NULL_HANDLE) {
			use_err:
			errnoMessageBox("Can't use file",NULL,errno);
			goto Exit;
		    }
		    __OsTruncFile(handle,0L);
		}
		__OsClose(handle);
		_bioHandle = new BFile;
		bool rc = _bioHandle->open(ff_fname,FO_READWRITE | SO_DENYNONE,BBIO_CACHE_SIZE,bctx.fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
		if(rc == false)  rc = _bioHandle->open(ff_fname,FO_READWRITE | SO_COMPAT,BBIO_CACHE_SIZE,bctx.fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
		if(rc == false)  goto use_err;
		crpos = ff_startpos;
		_bioHandle->seek(0L,SEEKF_START);
		prcnt_counter = oprcnt_counter = 0;
		pwsize = 0;
		awsize = wsize;
		while(wsize) {
		    unsigned real_size;
		    rem = (unsigned)std::min(wsize,__filesize_t(4096));
		    if(!BMReadBufferEx(tmp_buff,rem,crpos,BFile::Seek_Set)) {
			errnoMessageBox(READ_FAIL,NULL,errno);
			delete _bioHandle;
			goto Exit;
		    }
		    real_size = (bctx.active_mode().flags()&Plugin::Has_ConvertCP) ? bctx.active_mode().convert_cp((char *)tmp_buff,rem,true) : rem;
		    if(!_bioHandle->write_buffer(tmp_buff,real_size)) {
			errnoMessageBox(WRITE_FAIL,NULL,errno);
			delete _bioHandle;
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
		delete _bioHandle;
	    } else { /** Write in disassembler mode */
		FILE * fout = NULL;
		unsigned char *codebuff;
		char *file_cache = NULL,*tmp_buff2 = NULL;
		unsigned MaxInsnLen;
		char func_name[300],obj_name[300],data_dis[300];
		__filesize_t func_pa,stop;
		unsigned func_class;
		__filesize_t awsize,pwsize;
		bool has_string;

		__filesize_t obj_start,obj_end;
		int obj_class,obj_bitness;
		unsigned obj_num;

		extern const Plugin_Info disMode;
		DisMode* dismode;
		if(bctx.mode_info()!=&disMode)
		    dismode = static_cast<DisMode*>(disMode.query_interface(bctx.codeguider()));
		else
		    dismode = static_cast<DisMode*>(&bctx.active_mode());
		MaxInsnLen = dismode->get_max_symbol_size();
		codebuff = new unsigned char [MaxInsnLen];
		if(!codebuff) {
		    MemOutBox("Disasm initialization");
		    goto dis_exit;
		}
		tmp_buff2 = new char [0x1000];
		file_cache = new char [BBIO_SMALL_CACHE_SIZE];
		fout = fopen(ff_fname,"wt");
		if(fout == NULL) {
		    errnoMessageBox(WRITE_FAIL,NULL,errno);
		    delete codebuff;
		    goto Exit;
		}
		if(file_cache) setvbuf(fout,file_cache,_IOFBF,BBIO_SMALL_CACHE_SIZE);
		if(flags & FSDLG_COMMENT) {
		    printHdr(fout,bctx.bin_format());
		}
		if(flags & FSDLG_STRUCTS) {
		    obj_num = bctx.bin_format().get_object_attribute(ff_startpos,obj_name,
						sizeof(obj_name),&obj_start,
						&obj_end,&obj_class,&obj_bitness);
		    obj_name[sizeof(obj_name)-1] = 0;
		}
#if 0
		    if(!obj_num) goto defobj;
		} else {
		    defobj:
		    obj_num = 0;
		    obj_start = 0;
		    obj_end = BMGetFLength();
		    obj_name[0] = 0;
		    obj_class = OC_CODE;
		    obj_bitness = bctx.bin_format().query_bitness(ff_startpos);
		}
#endif
		if(flags & FSDLG_STRUCTS) printObject(fout,obj_num,obj_name,obj_class,obj_bitness,obj_end - obj_start);
		func_pa = 0;
		if(flags & FSDLG_STRUCTS) {
		    func_pa = bctx.bin_format().get_public_symbol(func_name,sizeof(func_name),
						&func_class,ff_startpos,true);
		    func_name[sizeof(func_name)-1] = 0;
		    if(func_pa!=Bin_Format::Bad_Address) {
			fprintf(fout,"%s %s:\n"
					,GET_FUNC_CLASS(func_class)
					,func_name);
			if(func_pa < ff_startpos && flags & FSDLG_COMMENT) {
				fprintf(fout,"; ...\n");
			}
		    }
		    func_pa = bctx.bin_format().get_public_symbol(func_name,sizeof(func_name),
						&func_class,ff_startpos,false);
		    if(func_pa==Bin_Format::Bad_Address) func_pa=0;
		    func_name[sizeof(func_name)-1] = 0;
		}
		prcnt_counter = oprcnt_counter = 0;
		awsize = endpos - ff_startpos;
		pwsize = 0;
		has_string = false;
		while(1) {
		    DisasmRet dret;
		    int len;
		    if(flags & FSDLG_STRUCTS) {
			if(ff_startpos >= obj_end) {
			    obj_num = bctx.bin_format().get_object_attribute(ff_startpos,obj_name,
						sizeof(obj_name),&obj_start,
						&obj_end,&obj_class,
						&obj_bitness);
			    obj_name[sizeof(obj_name)-1] = 0;
			    printObject(fout,obj_num,obj_name,obj_class,obj_bitness,obj_end - obj_start);
			}
			if(obj_class == OC_NOOBJECT) {
			    __filesize_t diff;
			    fprintf(fout,"; L%016llXH-L%016llXH - no object\n",obj_start,obj_end);
			    dret.codelen = std::min(__filesize_t(UCHAR_MAX),obj_end - ff_startpos);
			    /** some functions can placed in virtual area of objects
				mean at end of true data, but before next object */
			    while(func_pa && func_pa >= obj_start && func_pa < obj_end && func_pa > ff_startpos) {
				diff = func_pa - ff_startpos;
				if(diff) fprintf(fout,"resb %16llXH\n",diff);
				fprintf(fout,"%s %s: ;at offset - %16llXH\n"
					,GET_FUNC_CLASS(func_class)
					,func_name
					,func_pa);
				ff_startpos = func_pa;
				func_pa = bctx.bin_format().get_public_symbol(func_name,sizeof(func_name),
						&func_class,ff_startpos,false);
				if(func_pa==Bin_Format::Bad_Address) func_pa=0;
				func_name[sizeof(func_name)-1] = 0;
				if(func_pa == ff_startpos) {
				    fprintf(fout,"...Probably internal error of beye...\n");
				    break;
				}
			    }
			    diff = obj_end - ff_startpos;
			    if(diff) fprintf(fout,"resb %16llXH\n",diff);
			    ff_startpos = obj_end;
			    goto next_obj;
			}
			if(func_pa) {
			    int not_silly;
			    not_silly = 0;
			    while(ff_startpos == func_pa) {
				/* print out here all public labels */
				fprintf(fout,"%s %s:\n"
					,GET_FUNC_CLASS(func_class)
					,func_name);
				func_pa = bctx.bin_format().get_public_symbol(func_name,sizeof(func_name),
						&func_class,ff_startpos,false);
				if(func_pa==Bin_Format::Bad_Address) func_pa=0;
				func_name[sizeof(func_name)-1] = 0;
				not_silly++;
				if(not_silly > 100) {
				    fprintf(fout,"; [snipped out] ...\n");
				    break;
				}
			    }
			}
		    }
		    memset(codebuff,0,MaxInsnLen);
		    BMReadBufferEx((any_t*)codebuff,MaxInsnLen,ff_startpos,BFile::Seek_Set);
		    if(obj_class == OC_CODE) dret = dismode->disassembler(ff_startpos,codebuff,__DISF_NORMAL);
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
			switch(obj_bitness) {
			    case DAB_USE16: dis_data_len = 2; break;
			    case DAB_USE32: dis_data_len = 4; break;
			    case DAB_USE64: dis_data_len = 8; break;
			    case DAB_USE128: dis_data_len = 16; break;
			    case DAB_USE256: dis_data_len = 32; break;
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
		    stop = func_pa ? std::min(func_pa,obj_end) : obj_end;
		    if(flags & FSDLG_STRUCTS) {
			if(stop && stop > ff_startpos && ff_startpos + dret.codelen > stop) {
			    unsigned lim,ii;
			    make_addr_column(tmp_buff,ff_startpos);
			    strcat(tmp_buff," db ");
			    lim = (unsigned)(stop-ff_startpos);
			    if(lim > MaxInsnLen) lim = MaxInsnLen;
			    for(ii = 0;ii < lim;ii++) sprintf(&tmp_buff[strlen(tmp_buff)],"%s ",Get2Digit(codebuff[ii]));
			    dret.codelen = lim;
			} else goto normline;
		    } else {
			normline:
			make_addr_column(tmp_buff,ff_startpos);
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
		    strcat(tmp_buff2 ? tmp_buff2 : tmp_buff,"\n");
		    if(fputs(tmp_buff2 ? tmp_buff2 : tmp_buff,fout) == EOF) {
			errnoMessageBox(WRITE_FAIL,NULL,errno);
			goto dis_exit;
		    }
		    if(flags & FSDLG_STRUCTS) {
			if(stop && ff_startpos != stop && ff_startpos + dret.codelen > stop)
			    dret.codelen = stop - ff_startpos;
		    }
		    if(!dret.codelen) {
			ErrMessageBox("Internal fatal error"," Put structures ");
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
		fclose(fout);
		if(file_cache) delete file_cache;
		if(tmp_buff2) delete tmp_buff2;
		if(bctx.mode_info()!=&disMode)	delete dismode;
	    } /** END: Write in disassembler mode */
	    Exit:
	    CloseWnd(progress_wnd);
	    BMSeek(cpos,BFile::Seek_Set);
	} else  ErrMessageBox("Start position > end position!",NULL);
    }
    delete tmp_buff;
    DumpMode = false;
    return false;
}

static bool FRestore( void )
{
 __filesize_t endpos,cpos;
 unsigned long flags;
 bool ret;
 ret = false;
 flags = FSDLG_NOMODES;
 __make_dump_name(".$$$");
 if(GetFStoreDlg(" Restore information from file ",ff_fname,&flags,&ff_startpos,&ff_len,FILE_PRMT))
 {
   __filesize_t flen,lval;
   bhandle_t handle;
   BFile* bHandle;
   const char *fname;
   endpos = ff_startpos + ff_len;
   handle = __OsOpen(ff_fname,FO_READONLY | SO_DENYNONE);
   if(handle == NULL_HANDLE) handle = __OsOpen(ff_fname,FO_READONLY | SO_COMPAT);
   if(handle == NULL_HANDLE) goto err;
   flen = __FileLength(handle);
   __OsClose(handle);
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     __filesize_t wsize,cwpos;
     unsigned remaind;
     any_t*tmp_buff;
     handle = __OsOpen(ff_fname,FO_READONLY | SO_DENYNONE);
     if(handle == NULL_HANDLE) handle = __OsOpen(ff_fname,FO_READONLY | SO_COMPAT);
     if(handle == NULL_HANDLE)
     {
	err:
	errnoMessageBox(OPEN_FAIL,NULL,errno);
	return false;
     }
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     __OsSeek(handle,0L,SEEKF_START);
     tmp_buff = new char [4096];
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return false;
     }
     fname = BMName();
     bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       while(wsize)
       {
	 remaind = (unsigned)std::min(wsize,__filesize_t(4096));
	 if((unsigned)__OsRead(handle,tmp_buff,remaind) != remaind)
	 {
	   errnoMessageBox(READ_FAIL,NULL,errno);
	   __OsClose(handle);
	   ret = false;
	   goto bye;
	 }
	 bHandle->seek(cwpos,BFile::Seek_Set);
	 if(!bHandle->write_buffer(tmp_buff,remaind))
	 {
	   errnoMessageBox(WRITE_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 wsize -= remaind;
	 cwpos += remaind;
       }
       bye:
       delete bHandle;
       BMReRead();
     }
     else errnoMessageBox(OPEN_FAIL,NULL,errno);
     delete (char*)tmp_buff;
     __OsClose(handle);
     BMSeek(cpos,BFile::Seek_Set);
     ret = true;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void  __FASTCALL__ CryptFunc(char * buff,unsigned len,char *pass)
{
  char ch,cxor;
  unsigned i,j;
  unsigned bigkey_idx;
  unsigned passlen;
  char big_key[UCHAR_MAX];
  cxor = 0;
  passlen = strlen(pass);
  memset(big_key,0,sizeof(big_key));
  for(j = 0;j < passlen;j++) cxor += pass[j]+j;
  cxor ^= passlen + len;
  for(j = i = 0;i < UCHAR_MAX;i++,j++)
  {
    if(j > passlen) j = 0;
    bigkey_idx = (pass[j] + i) * cxor;
    if(bigkey_idx > UCHAR_MAX) bigkey_idx = (bigkey_idx & 0xFF) ^ ((bigkey_idx >> 8) & 0xFF);
    big_key[i] = bigkey_idx;
  }
  for(bigkey_idx = j = i = 0;i < len;i++,bigkey_idx++,j++)
  {
   unsigned short _xor;
   if(bigkey_idx > UCHAR_MAX)
   {
     /** rotate of big key */
     ch = big_key[0];
     memmove(big_key,&big_key[1],UCHAR_MAX-1);
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
  memmove(pass,&pass[1],passlen-1);
  pass[passlen-1] = ch;
}

static bool CryptBlock( void )
{
 __filesize_t endpos,cpos;
 unsigned long flags;
 char pass[81];
 bool ret;
 ret = false;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 pass[0] = 0;
 flags = FSDLG_NOMODES;
 if(GetFStoreDlg(" (De)Crypt block of file ",pass,&flags,&ff_startpos,&ff_len,"Input password (WARNING! password will be displayed):"))
 {
   __filesize_t flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(!pass[0]) { ErrMessageBox("Password can't be empty",NULL); return false; }
   if(endpos > ff_startpos)
   {
     __filesize_t wsize,cwpos;
     unsigned remaind;
     const char *fname;
     BFile* bHandle;
     any_t*tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     tmp_buff = new char [4096];
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return false;
     }
     fname = BMName();
     bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bHandle->seek(ff_startpos,SEEK_SET);
       while(wsize)
       {
	 remaind = (unsigned)std::min(wsize,__filesize_t(4096));
	 if(!bHandle->read_buffer(tmp_buff,remaind))
	 {
	   errnoMessageBox(READ_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 CryptFunc((char*)tmp_buff,remaind,pass);
	 bHandle->seek(cwpos,BFile::Seek_Set);
	 if(!(bHandle->write_buffer(tmp_buff,remaind)))
	 {
	   errnoMessageBox(WRITE_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 wsize -= remaind;
	 cwpos += remaind;
       }
       bye:
       delete bHandle;
       BMReRead();
     }
     delete (char*)tmp_buff;
     BMSeek(cpos,BFile::Seek_Set);
     ret = true;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void  __FASTCALL__ EndianifyBlock(char * buff,unsigned len, int type)
{
  unsigned i, step;
  if(!type) return; /* for now */
  switch(type)
  {
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
  for(i = 0;i < len;i+=step, buff+=step)
  {
    switch(type)
    {
      default:
      case 3: *((uint64_t *)buff) = bswap_64(*((uint64_t *)buff));
	      break;
      case 2: *((uint32_t *)buff) = bswap_32(*((uint32_t *)buff));
	      break;
      case 1:
	      *((uint16_t *)buff) = bswap_16(*((uint16_t *)buff));
	      break;
    }
  }
}

static bool ReverseBlock( void )
{
 __filesize_t endpos,cpos;
 unsigned long flags;
 bool ret;
 ret = false;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 flags = FSDLG_USEBITNS;
 if(GetFStoreDlg(" Endianify block of file ",NULL,&flags,&ff_startpos,&ff_len,NULL))
 {
   __filesize_t flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     __filesize_t wsize,cwpos;
     unsigned remaind;
     const char *fname;
     BFile* bHandle;
     any_t*tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     tmp_buff = new char [4096];
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return false;
     }
     fname = BMName();
     bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bHandle->seek(ff_startpos,SEEK_SET);
       while(wsize)
       {
	 remaind = (unsigned)std::min(wsize,__filesize_t(4096));
	 if(!bHandle->read_buffer(tmp_buff,remaind))
	 {
	   errnoMessageBox(READ_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 EndianifyBlock((char*)tmp_buff,remaind, flags & FSDLG_BTNSMASK);
	 bHandle->seek(cwpos,BFile::Seek_Set);
	 if(!(bHandle->write_buffer(tmp_buff,remaind)))
	 {
	   errnoMessageBox(WRITE_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 wsize -= remaind;
	 cwpos += remaind;
       }
       bye:
       delete bHandle;
       BMReRead();
     }
     delete (char*)tmp_buff;
     BMSeek(cpos,BFile::Seek_Set);
     ret = true;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static void  __FASTCALL__ TranslateBlock(char * buff,unsigned len, const unsigned char *xlt)
{
  unsigned i;
  for(i = 0;i < len;i++)
  {
    buff[i] = xlt[(int)buff[i]];
  }
}


static bool XLatBlock( void )
{
 unsigned char xlt[256];
 __filesize_t endpos,cpos;
 unsigned long flags;
 bool ret;
 ret = false;
 ff_startpos = BMGetCurrFilePos();
 if(!ff_len) ff_len = BMGetFLength() - ff_startpos;
 flags = FSDLG_NOMODES;
 if(!xlat_fname[0])
 {
   strcpy(xlat_fname,__get_rc_dir("beye"));
   strcat(xlat_fname,"xlt");
 }
 if(GetFStoreDlg(" Table Look-up Translation ",xlat_fname,&flags,&ff_startpos,&ff_len,XLAT_PRMT))
 {
   __filesize_t flen,lval;
   endpos = ff_startpos + ff_len;
   flen = BMGetFLength();
   lval = endpos - ff_startpos;
   endpos = lval > flen ? flen + ff_startpos : endpos;
   endpos = endpos > BMGetFLength() ? BMGetFLength() : endpos;
   if(endpos > ff_startpos)
   {
     __filesize_t wsize,cwpos;
     unsigned remaind;
     const char *fname;
     BFile* bHandle,* xHandle;
     any_t*tmp_buff;
     cpos = BMGetCurrFilePos();
     wsize = endpos - ff_startpos;
     cwpos = ff_startpos;
     /* Parse xlat file */
     xHandle = BeyeContext::beyeOpenRO(xlat_fname,BBIO_SMALL_CACHE_SIZE);
     if(xHandle == &bNull)
     {
       ErrMessageBox("Can't open xlat file", NULL);
       return false;
     }
     if(xHandle->flength() != 320)
     {
       ErrMessageBox("Size of xlat file is not 320 bytes", NULL);
       delete xHandle;
       return false;
     }
     xHandle->read_buffer(xlt, 16);
     if(memcmp(xlt, "Beye Xlat Table.", 16) != 0)
     {
       ErrMessageBox("It seems that xlat file is corrupt", NULL);
       delete xHandle;
       return false;
     }
     xHandle->seek(0x40, SEEKF_START);
     xHandle->read_buffer(xlt, 256);
     delete xHandle;
     tmp_buff = new char [4096];
     if(!tmp_buff)
     {
       MemOutBox("temporary buffer initialization");
       return false;
     }
     fname = BMName();
     bHandle = BeyeContext::beyeOpenRW(fname,BBIO_SMALL_CACHE_SIZE);
     if(bHandle != &bNull)
     {
       bHandle->seek(ff_startpos,SEEK_SET);
       while(wsize)
       {
	 remaind = (unsigned)std::min(wsize,__filesize_t(4096));
	 if(!bHandle->read_buffer(tmp_buff,remaind))
	 {
	   errnoMessageBox(READ_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 TranslateBlock((char*)tmp_buff,remaind, xlt);
	 bHandle->seek(cwpos,BFile::Seek_Set);
	 if(!(bHandle->write_buffer(tmp_buff,remaind)))
	 {
	   errnoMessageBox(WRITE_FAIL,NULL,errno);
	   ret = false;
	   goto bye;
	 }
	 wsize -= remaind;
	 cwpos += remaind;
       }
       bye:
       delete bHandle;
       BMReRead();
     }
     delete (char*)tmp_buff;
     BMSeek(cpos,BFile::Seek_Set);
     ret = true;
   }
   else ErrMessageBox("Start position > end position!",NULL);
 }
 return ret;
}

static bool FileInfo( void )
{
  TWindow* wnd;
  struct stat statbuf;
  unsigned evt;
  char attr[14];
  char stimes[3][80];
  memset(&statbuf,0,sizeof(struct stat));
  stat(BMName(),&statbuf);
  memset(attr,'-',sizeof(attr));
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
  wnd = CrtDlgWndnls(" File information: ",tvioWidth-5,13);
  twGotoXY(wnd,1,1);
  strcpy(stimes[0],ctime(&statbuf.st_ctime));
  strcpy(stimes[1],ctime(&statbuf.st_mtime));
  strcpy(stimes[2],ctime(&statbuf.st_atime));
  twPrintF(wnd,
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
	   ,beye_context().short_name()
	   ,beye_context().bin_format().name()
	   ,BMGetFLength()
	   ,attr
	   ,stimes[0]
	   ,stimes[1]
	   ,stimes[2]
	   ,(unsigned)statbuf.st_dev
	   ,(unsigned)statbuf.st_ino
	   ,(unsigned)statbuf.st_nlink
	   ,(unsigned)statbuf.st_uid
	   ,(unsigned)statbuf.st_gid);
  do
  {
    evt = GetEvent(drawEmptyPrompt,NULL,wnd);
  }
  while(!(evt == KE_ESCAPE || evt == KE_F(10)));
  CloseWnd(wnd);
  return false;
}

static const char* fu_names[] =
{
  "~File information...",
  "C~hange size of file",
  "~Save block as...",
  "~Restore block from...",
  "~Insert/delete block...",
  "~Crypt/decrypt block...",
  "~Endianify block...",
  "~Xlat block..."
};

typedef bool (*FileFunc)( void );

static FileFunc fu_funcs[] =
{
  FileInfo,
  ChSize,
  FStore,
  FRestore,
  InsDelBlock,
  CryptBlock,
  ReverseBlock,
  XLatBlock
};

bool FileUtils( void )
{
  size_t nUtils;
  int retval;
  bool ret;
  static unsigned def_sel = 0;
  nUtils = sizeof(fu_names)/sizeof(char *);
  retval = SelBoxA(const_cast<char**>(fu_names),nUtils," File utilities: ",def_sel);
  if(retval != -1)
  {
     TWindow * w;
     w = PleaseWaitWnd();
     ret = (*fu_funcs[retval])();
     CloseWnd(w);
     def_sel = retval;
     return ret;
  }
  return false;
}
} // namespace beye

