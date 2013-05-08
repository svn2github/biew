#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_I
 * @file        plugins/textmode.c
 * @brief       This file contains implementation of text viewer with different submodes.
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
#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "beye.h"
#include "plugins/textmode.h"
#include "colorset.h"
#include "bmfile.h"
#include "bin_util.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "bconsole.h"
#include "search.h"
#include "reg_form.h"
#include "libbeye/kbd_code.h"

#include "plugin.h"

namespace	usr {
enum {
    TEXT_TAB     =8,
    MOD_PLAIN    =0,
    MOD_BINARY   =1,
    MOD_MAXMODE  =1
};

typedef struct tagTSTR
{
  unsigned long st;
  unsigned long end;
}TSTR;

typedef struct tag_shash_s
{
    unsigned start;
    unsigned total;
}shash_t;

typedef struct tag_acontext_hl_s
{
    Color color;
    long  start_off;
    long  end_off;
}acontext_hl_t;

typedef struct tag_context_hl_s
{
    Color color;
    char  *start_seq;
    char  *end_seq;
}context_hl_t;

typedef struct tag_keyword_hl_s
{
    Color color;
    char  *keyword;
    unsigned len; /* for accelerating */
}keyword_hl_t;

static struct tag_syntax_hl_s
{
   char *name;
   context_hl_t *context;
   keyword_hl_t *keyword;
   shash_t       kwd_hash[256];
   ColorAttr     op_hash[256];
   unsigned context_num,keyword_num;
   unsigned maxkwd_len;
}syntax_hl;

extern const REGISTRY_NLS RussianNLS;
static const REGISTRY_NLS *nls_set[] =
{
  &RussianNLS
};

static const unsigned	MAX_STRLEN=1000; /**< defines maximal length of string */
static unsigned char	word_set[UCHAR_MAX+1];
static unsigned char	wset[UCHAR_MAX+1];
static char*		escape;
static char		detected_syntax_name[FILENAME_MAX+1];

    class TextMode : public Plugin {
	public:
	    TextMode(CodeGuider& code_guider);
	    virtual ~TextMode();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual bool		action_F2();
	    virtual bool		action_F3();
	    virtual bool		action_F4();
	    virtual bool		action_F8();
	    virtual bool		action_F9();
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
	    virtual unsigned		convert_cp(char *str,unsigned len, bool use_fs_nls);
	private:
	    unsigned			tab2space(tvioBuff* dest,unsigned int alen,char* str,unsigned int len,unsigned int shift,unsigned* n_tabs,long lstart) const;
	    void			paint_search(HLInfo* cptr,unsigned int shift,int i,int size,int _bin_mode) const;
	    void			prepare_lines(int keycode);
	    void			fill_curr_page(__filesize_t lval,__filesize_t flen) const;
	    void			fill_prev_page(__filesize_t lval) const;
	    __filesize_t		forward_scan_cr(__filesize_t cp,__filesize_t flen) const;
	    __filesize_t		back_scan_cr(__filesize_t cp) const;
	    unsigned char		nls_read_byte(__filesize_t cp) const;
	    ColorAttr			hl_get_ctx(long off,int *is_valid, long *end_ctx) const;
	    ColorAttr			hl_find_kwd(const char *str,Color col,unsigned *st_len) const;
	    void			read_syntaxes();
	    void			markup_ctx();
	    bool			test_leading_escape(__fileoff_t cpos) const;
	    bool			is_legal_word_char(unsigned char ch) const { return (bool)word_set[(unsigned char)ch]; }

	    acontext_hl_t*		acontext; /* means active context*/
	    unsigned long		acontext_num;

	    int				HiLight;

	    unsigned			bin_mode; /**< points to currently selected mode text mode */
	    binary_stream*			txtHandle; /**< Own handle of BBIO stream. (For speed). */

	    TSTR			*tlines,*ptlines;
	    unsigned int		maxstrlen; /**< contains maximal length of string which can be displayed without wrapping */
	    bool			wmode; /**< Wrap mode flag */
	    unsigned long		PrevPageSize,CurrPageSize,PrevStrLen,CurrStrLen;
	    unsigned			strmaxlen; /**< contains size of largest string on currently displayed page. (It for KE_END key) */
	    char*			buff;
	    __filesize_t		tmocpos;

	    const REGISTRY_NLS*		activeNLS;
	    unsigned			defNLSSet;
    };

bool TextMode::test_leading_escape(__fileoff_t cpos) const {
    char tmps[MAX_STRLEN];
    __fileoff_t epos = BMGetCurrFilePos(),spos;
    unsigned escl = ::strlen(escape);
    spos=(cpos-1)-escl;
    if(escl && spos>=0) {
	BMReadBufferEx(tmps,escl,spos,binary_stream::Seek_Set);
	BMSeek(epos,binary_stream::Seek_Set);
	return ::memcmp(tmps,escape,escl)==0;
    }
    return 0;
}

ColorAttr TextMode::hl_find_kwd(const char *str,Color col,unsigned *st_len) const
{
    int found,res;
    unsigned i,len,st,end;
    ColorAttr defcol=LOGFB_TO_PHYS(col,BACK_COLOR(text_cset.normal));
    *st_len=0;
    if(syntax_hl.kwd_hash[(unsigned char)str[0]].start!=UINT_MAX) {
	st=syntax_hl.kwd_hash[(unsigned char)str[0]].start;
	end=st+syntax_hl.kwd_hash[(unsigned char)str[0]].total;
	for(i=st;i<end;i++) {
	    len=syntax_hl.keyword[i].len;
	    found=0;
	    if(len>1) {
		res=::memcmp(&str[1],&syntax_hl.keyword[i].keyword[1],len-1);
		if(res==0) found=1;
	    } else found=1;
	    if(found) {
		defcol=LOGFB_TO_PHYS(syntax_hl.keyword[i].color,BACK_COLOR(text_cset.normal));
		*st_len=len;
		break;
	    }
	}
    }
    return defcol;
}

void TextMode::markup_ctx()
{
    long fptr,fpos,flen;
    unsigned i,len;
    int found;
    char tmps[MAX_STRLEN],ktmps[MAX_STRLEN],ch,chn;
    const char *sseq,*eseq;
    TWindow *hwnd;
    hwnd=PleaseWaitWnd();
    flen=BMGetFLength();
    fpos=BMGetCurrFilePos();
    BMSeek(0,binary_stream::Seek_Set);
    acontext_num=0;
    for(fptr=0;fptr<flen;fptr++) {
	tmps[0]=0;
	ch=BMReadByte();
	found=0;
	if(ch=='\r') ch='\n';
	for(i=0;i<syntax_hl.context_num;i++) {
	    unsigned ss_idx;
	    ss_idx=0;
	    sseq=syntax_hl.context[i].start_seq;
	    if(sseq[0]=='\n') {
		if(fptr==0) ss_idx=1;
		else {
		    long ccpos;
		    ccpos=BMGetCurrFilePos();
		    chn=BMReadByteEx(fptr-1,binary_stream::Seek_Set);
		    BMSeek(ccpos, binary_stream::Seek_Set);
		    if(chn=='\n' || chn=='\r') ss_idx=1;
		}
	    }
	    if(ch==sseq[ss_idx]) {
		__fileoff_t cpos;
		len=::strlen(sseq);
		cpos=BMGetCurrFilePos();
		found=0;
		if(len>(ss_idx+1)) {
		    BMReadBuffer(tmps,len-(ss_idx+1));
		    if(::memcmp(tmps,&sseq[ss_idx+1],len-(ss_idx+1))==0) found=1;
		}
		else found=1;
		/*avoid markup escape sequences */
		if(found && escape) found = !test_leading_escape(cpos);
		if(found) {
		    /* avoid context markup if it's equal one of keywords */
		    unsigned st_len;
		    long ckpos;
		    ktmps[0]=ch;
		    ::memcpy(&ktmps[1],tmps,len-(ss_idx+1));
		    if(syntax_hl.maxkwd_len>(len-ss_idx)) {
			ckpos=BMGetCurrFilePos();
			BMReadBuffer(&ktmps[len],syntax_hl.maxkwd_len-(len-ss_idx));
			BMSeek(ckpos,binary_stream::Seek_Set);
		    }
		    hl_find_kwd(ktmps,Color(0),&st_len);
		    if(st_len) found=0;
		}
		if(found) {
		    if(!acontext) acontext=new acontext_hl_t;
		    else	  acontext=(acontext_hl_t*)mp_realloc(acontext,sizeof(acontext_hl_t)*(acontext_num+1));
		    acontext[acontext_num].color=(Color)LOGFB_TO_PHYS(syntax_hl.context[i].color,BACK_COLOR(text_cset.normal));
		    acontext[acontext_num].start_off=fptr-ss_idx;
		    acontext[acontext_num].end_off=flen;
		    fptr+=len;
		    BMSeek(fptr,binary_stream::Seek_Set);
		    /* try find end */
		    eseq=syntax_hl.context[i].end_seq;
		    len=::strlen(eseq);
		    for(;fptr<flen;fptr++) {
			ch=BMReadByte();
			if(ch==eseq[0]) {
			    long ecpos;
			    ecpos=BMGetCurrFilePos();
			    found=0;
			    if(len>1) {
				BMReadBuffer(tmps,len-1);
				if(::memcmp(tmps,&eseq[1],len-1)==0) found=1;
			    }
			    else found=1;
			    if(found && escape) found=!test_leading_escape(ecpos);
			    if(found) {
				fptr+=len;
				BMSeek(fptr,binary_stream::Seek_Set);
				acontext[acontext_num].end_off=fptr;
				break;
			    }
			    else BMSeek(ecpos,binary_stream::Seek_Set);
			}
		    }
		    acontext_num++;
		    fptr--;
		}
		else BMSeek(cpos,binary_stream::Seek_Set);
	    }
	    if(found) break;
	}
    }
    BMSeek(fpos,binary_stream::Seek_Set);
    delete hwnd;
}

static void unfmt_str(unsigned char *str)
{
   long result;
   unsigned base,i;
   unsigned char *src,*dest,ch,temp[MAX_STRLEN];
   src=dest=str;
   while(*src)
   {
	ch=*src;
	src++;
	if(ch == '\\')
	{
	    if(*src== '\\');
	    else
	    if(*src=='n') { src++; ch='\n'; }
	    else
	    if(*src=='t') { src++; ch='\t'; }
	    else
	    {
		base=10;
		i=0;
		if(*src=='0') { src++; base=8; }
		if(*src=='x' || *src=='X') { base=16; src++; while(isxdigit(*src)) temp[i++]=*src++; }
		else { while(isdigit(*src)) temp[i++]=*src++; }
		temp[i]=0;
		result=strtol((char*)temp,NULL,base);
		ch=result;
	    }
	}
	*dest++=ch;
   }
   *dest=0;
}

static bool __FASTCALL__ txtFiUserFunc1(IniInfo * info,any_t* data)
{
    UNUSED(data);
  const char* p=NULL;
  if(strcmp(info->section,"Extensions")==0) {
	p = strrchr(beye_context().ArgVector1.c_str(),'.');
	if(p) {
	    p++;
	    if(strcmp(p,info->item)==0) {
		strcpy(detected_syntax_name,info->value);
		return true;
	    }
	}
  }
  if(strcmp(info->section,"Names")==0) {
	const char *pp;
	p = strrchr(beye_context().ArgVector1.c_str(),'/');
	pp = strrchr(beye_context().ArgVector1.c_str(),'\\');
	p=std::max(p,pp);
	if(p) p++;
	else  p=beye_context().ArgVector1.c_str();
	if(memcmp(p,info->item,strlen(info->item))==0) {
	    strcpy(detected_syntax_name,info->value);
	    return true;
	}
  }
  if(strcmp(info->section,"Context")==0) {
	long off,fpos;
	unsigned i,ilen;
	int found,softmode;
	off = atol(info->item);
	char stmp[4096];
	strncpy(stmp,info->value,sizeof(stmp));
	char* value=strstr(stmp,"-->");
	if(!value) { ErrMessageBox("Missing separator in main context definition",""); return true; }
	*value=0;
	softmode=0;
	if(strcmp(info->subsection,"Soft")==0) softmode=1;
	unfmt_str((unsigned char*)stmp);
	ilen=strlen(stmp);
	fpos=BMGetCurrFilePos();
	BMSeek(off,binary_stream::Seek_Set);
	found=1;
	for(i=0;i<ilen;i++) {
	    unsigned char ch;
	    ch=BMReadByte();
	    if(softmode) while(isspace(ch)) { ch=BMReadByte(); if(BMEOF()) { found = 0; break; }}
	    if(ch != stmp[i]) {
		found=0;
		break;
	    }
	}
	BMSeek(fpos,binary_stream::Seek_Set);
	if(found) {
	    strcpy(detected_syntax_name,value+3);
	    return true;
	}
  }
  return false;
}

static Color  __FASTCALL__ getCtxColorByName(const char *subsection,const char *item,Color cdef,bool *err)
{
    PrgWordCSet *cset;
    *err=0;
    cset=NULL;
    if(::strcmp(subsection,"Comments")==0) cset=&prog_cset.comments;
    else if(::strcmp(subsection,"Preproc")==0) cset=&prog_cset.preproc;
    else if(::strcmp(subsection,"Constants")==0) cset=&prog_cset.constants;
    else if(::strcmp(subsection,"Keywords")==0) cset=&prog_cset.keywords;
    else if(::strcmp(subsection,"Operators")==0) cset=&prog_cset.operators;
    else ErrMessageBox("Unknown context subsection definition",subsection);
    if(cset) {
	if(::strcmp(item,"base")==0) return Color(cset->base);
	if(::strcmp(item,"extended")==0) return Color(cset->extended);
	if(::strcmp(item,"reserved")==0) return Color(cset->reserved);
	if(::strcmp(item,"alt")==0) return Color(cset->alt);
	ErrMessageBox("Unknown context class definition",item);
    }
    *err=1;
    return cdef;
}

static Color  __FASTCALL__ getKwdColorByName(const char *item,Color cdef,bool *err)
{
    *err=0;
    if(::strcmp(item,"base")==0) return Color(prog_cset.keywords.base);
    if(::strcmp(item,"extended")==0) return Color(prog_cset.keywords.extended);
    if(::strcmp(item,"reserved")==0) return Color(prog_cset.keywords.reserved);
    if(::strcmp(item,"alt")==0) return Color(prog_cset.keywords.alt);
    ErrMessageBox("Unknown keyword class definition",item);
    *err=1;
    return cdef;
}

static Color  __FASTCALL__ getOpColorByName(const char *item,Color cdef,bool *err)
{
    *err=0;
    if(::strcmp(item,"base")==0) return Color(prog_cset.operators.base);
    if(::strcmp(item,"extended")==0) return Color(prog_cset.operators.extended);
    if(::strcmp(item,"reserved")==0) return Color(prog_cset.operators.reserved);
    if(::strcmp(item,"alt")==0) return Color(prog_cset.operators.alt);
    ErrMessageBox("Unknown keyword class definition",item);
    *err=1;
    return cdef;
}

static const char *last_syntax_err="";
static bool __FASTCALL__ txtFiUserFunc2(IniInfo * info,any_t* data)
{
    UNUSED(data);
    char *p;
    bool err;
    Color cdef=FORE_COLOR(text_cset.normal);
    err=false;
    if(::strcmp(info->section,"General")==0) {
	if(::strcmp(info->item,"Name")==0) {
	    syntax_hl.name=new char [strlen(info->value)+1];
	    ::strcpy(syntax_hl.name,info->value);
	}
	if(strcmp(info->item,"Escape")==0) {
	    escape=new char [strlen(info->value)+1];
	    ::strcpy(escape,info->value);
	}
	if(strcmp(info->item,"WSet")==0) {
	    unsigned i,len,rlen;
	    len=::strlen(info->value);
	    for(i=0;i<len;i++) word_set[(unsigned char)info->value[i]]=1;
	    rlen=std::min(unsigned(UCHAR_MAX),len);
	    ::memcpy(wset,info->value,rlen);
	    wset[rlen]=0;
	}
    }
    if(::strcmp(info->section,"Context")==0) {
	Color col;
	col = getCtxColorByName(info->subsection,info->item,cdef,&err);
	if(!err) {
	    if(!syntax_hl.context) syntax_hl.context=new context_hl_t;
	    else syntax_hl.context=(context_hl_t*)realloc(syntax_hl.context,sizeof(context_hl_t)*(syntax_hl.context_num+1));
	    syntax_hl.context[syntax_hl.context_num].color=col;
	    p=::strstr((char*)info->value,"...");
	    if(!p) { last_syntax_err="Missing separator in context definition"; return true; }
	     *p=0;
	    p+=3;
	    unfmt_str((unsigned char*)info->value);
	    unfmt_str((unsigned char*)p);
	    syntax_hl.context[syntax_hl.context_num].start_seq=new char [strlen(info->value)+1];
	    ::strcpy(syntax_hl.context[syntax_hl.context_num].start_seq,info->value);
	    syntax_hl.context[syntax_hl.context_num].end_seq=new char [strlen(p)+1];
	    ::strcpy(syntax_hl.context[syntax_hl.context_num].end_seq,p);
	    syntax_hl.context_num++;
	} else last_syntax_err=beye_context().last_skin_error.c_str();
    }
    if(::strcmp(info->section,"Keywords")==0) {
	Color col;
	col = getKwdColorByName(info->item,cdef,&err);
	if(!err) {
	    if(!syntax_hl.keyword) syntax_hl.keyword=new keyword_hl_t;
	    else syntax_hl.keyword=(keyword_hl_t*)realloc(syntax_hl.keyword,sizeof(keyword_hl_t)*(syntax_hl.keyword_num+1));
	    syntax_hl.keyword[syntax_hl.keyword_num].color=col;
	    unfmt_str((unsigned char*)info->value);
	    syntax_hl.keyword[syntax_hl.keyword_num].len=strlen(info->value);
	    syntax_hl.keyword[syntax_hl.keyword_num].keyword=new char [syntax_hl.keyword[syntax_hl.keyword_num].len+1];
	    ::strcpy(syntax_hl.keyword[syntax_hl.keyword_num].keyword,info->value);
	    if(syntax_hl.keyword[syntax_hl.keyword_num].len > syntax_hl.maxkwd_len)
		syntax_hl.maxkwd_len=syntax_hl.keyword[syntax_hl.keyword_num].len;
	    syntax_hl.keyword_num++;
	}
	else last_syntax_err=beye_context().last_skin_error.c_str();
    }
    if(::strcmp(info->section,"Operators")==0) {
	Color col;
	col = getOpColorByName(info->item,cdef,&err);
	if(!err) {
	    unfmt_str((unsigned char*)info->value);
	    if(::strlen(info->value)>1) { last_syntax_err="Too long operator has been found"; return true; }
	    syntax_hl.op_hash[(unsigned char)info->value[0]]=LOGFB_TO_PHYS(col,BACK_COLOR(text_cset.normal));
	} else last_syntax_err=beye_context().last_skin_error.c_str();
    }
    return err?true:false;
}

static tCompare __FASTCALL__ cmp_ctx(const any_t* e1,const any_t* e2)
{
    const char *k1,*k2;
    int sl1,sl2,res;
    k1=(*(const context_hl_t  *)e1).start_seq;
    k2=(*(const context_hl_t  *)e2).start_seq;
    sl1=::strlen((*(const context_hl_t  *)e1).start_seq);
    sl2=::strlen((*(const context_hl_t  *)e2).start_seq);
    res=::memcmp(k1,k2,std::min(sl1,sl2));
    return res==0?__CmpLong__(sl2,sl1):res;
}

static tCompare __FASTCALL__ cmp_kwd(const any_t* e1,const any_t* e2)
{
    const char *k1,*k2;
    int sl1,sl2,res;
    k1=(*(const keyword_hl_t  *)e1).keyword;
    k2=(*(const keyword_hl_t  *)e2).keyword;
    sl1=::strlen((*(const keyword_hl_t  *)e1).keyword);
    sl2=::strlen((*(const keyword_hl_t  *)e2).keyword);
    res=::memcmp(k1,k2,std::min(sl1,sl2));
    return res==0?__CmpLong__(sl2,sl1):res;
}

void TextMode::read_syntaxes()
{
    if(binary_stream::exists(beye_context().syntax_name)) {
	Ini_Parser::scan(beye_context().syntax_name.c_str(),txtFiUserFunc1,NULL);
	if(detected_syntax_name[0]) {
	    char tmp[FILENAME_MAX+1];
	    char *p;
	    char *pp;
	    ::strcpy(tmp,beye_context().syntax_name.c_str());
	    p=::strrchr(tmp,'/');
	    pp=::strrchr(tmp,'\\');
	    p=std::max(p,pp);
	    if(p) { p++; ::strcpy(p,detected_syntax_name); }
	    ::strcpy(detected_syntax_name,tmp);
	    if(binary_stream::exists(detected_syntax_name)) {
		unsigned i,total;
		int phash;
		::memset(word_set,0,sizeof(word_set));
		Ini_Parser::scan(detected_syntax_name,txtFiUserFunc2,NULL);
		if(last_syntax_err[0]) ErrMessageBox(last_syntax_err,"");
		/* put longest strings on top */
		HQSort(syntax_hl.context,syntax_hl.context_num,sizeof(context_hl_t),cmp_ctx);
		HQSort(syntax_hl.keyword,syntax_hl.keyword_num,sizeof(keyword_hl_t),cmp_kwd);
		/* Fill keyword's hash */
		phash=-1;
		total=0;
		for(i=0;i<sizeof(syntax_hl.kwd_hash)/sizeof(syntax_hl.kwd_hash[0]);i++)
		    syntax_hl.kwd_hash[i].start=syntax_hl.kwd_hash[i].total=UINT_MAX;
		for(i=0;i<syntax_hl.keyword_num;i++) {
		    if(syntax_hl.keyword[i].keyword[0]!=phash) {
			if(phash!=-1) syntax_hl.kwd_hash[phash].total=total;
			phash=(unsigned char)(syntax_hl.keyword[i].keyword[0]);
			syntax_hl.kwd_hash[phash].start=i;
			total=0;
		    }
		    total++;
		}
		if(phash!=-1) syntax_hl.kwd_hash[phash].total=total;
		if(syntax_hl.context_num) markup_ctx();
	    }
	}
    }
}

ColorAttr TextMode::hl_get_ctx(long off,int *is_valid, long *end_ctx) const
{
    unsigned long ii;
    *is_valid=0;
    *end_ctx=BMGetFLength();
    for(ii=0;ii<acontext_num;ii++)
    {
	if(acontext[ii].start_off <= off && off < acontext[ii].end_off)
	{
	    *is_valid=1;
	    *end_ctx=acontext[ii].end_off;
	    return acontext[ii].color;
	}
	if(off < acontext[ii].start_off) { *end_ctx=acontext[ii].start_off; break; }
    }
    return 0;
}

static const char * mod_names[] =
{
   "~Plain text",
   "~All characters 8bit (as is)"
};

void __FASTCALL__ txt_cvt_full(char * str,int size,const unsigned char *tmpl)
{
  int i;
  for(i = 0;i < size;i++) str[i] = tmpl[(unsigned char)str[i]];
}

void __FASTCALL__ txt_cvt_hi80(char * str,unsigned size,const unsigned char *tmpl)
{
 size_t i;
 unsigned char cc;
 for(i = 0;i < size;i++)
 {
   cc = str[i];
   str[i] = cc >= 0x80 ? tmpl[cc-0x80] : cc;
 }
}

void __FASTCALL__ txt_cvt_lo80(char * str,unsigned size,const unsigned char *tmpl)
{
 size_t i;
 unsigned char cc;
 for(i = 0;i < size;i++)
 {
   cc = str[i];
   str[i] = cc < 0x80 ? tmpl[cc] : cc;
 }
}

unsigned char TextMode::nls_read_byte(__filesize_t cp) const
{
    char nls_buff[256];
    unsigned sym_size;
    sym_size = activeNLS->get_symbol_size();
    txtHandle->seek(cp,binary_stream::Seek_Set);
    txtHandle->read(nls_buff,sym_size);
    activeNLS->convert_buffer(nls_buff,sym_size,true);
    return (unsigned char)nls_buff[0];
}

__filesize_t TextMode::back_scan_cr( __filesize_t cp ) const
{
    __filesize_t lval;
    unsigned int freq;
    unsigned cp_symb_len;
    char ch;
    cp_symb_len = activeNLS->get_symbol_size();
    ch = nls_read_byte(cp);
    if((ch == '\n' || ch == '\r') && cp) {
	char ch1;
	cp-=cp_symb_len;
	ch1 = nls_read_byte(cp);
	if((ch1 == '\n' || ch1 == '\r') && ch != ch1) cp-=cp_symb_len;
    }
    for(lval = freq = 0;cp;cp-=cp_symb_len) {
	ch = nls_read_byte(cp);
	if(ch == '\n' || ch == '\r') {
	    lval = cp + cp_symb_len;
	    break;
	}
	freq++;
	if(freq >= maxstrlen) { lval = cp; break; }
    }
    return lval;
}

__filesize_t TextMode::forward_scan_cr( __filesize_t cp ,__filesize_t flen) const
{
    __filesize_t lval;
    unsigned int freq = 0;
    unsigned cp_symb_len;
    cp_symb_len = activeNLS->get_symbol_size();

    for(;cp < flen;cp+=cp_symb_len) {
	unsigned char ch;
	ch = nls_read_byte(cp);
	if(ch == '\n' || ch == '\r') {
	    char ch1;
	    lval = cp + cp_symb_len;
	    ch1 = nls_read_byte(lval);
	    if((ch1 == '\n' || ch1 == '\r') && ch != ch1)  lval+=cp_symb_len;
	    return lval;
	}
	freq++;
	if(freq > maxstrlen) return cp;
    }
    return flen;
}

void TextMode::fill_prev_page(__filesize_t lval) const
{
    unsigned cp_symb_len;
    int i;
    cp_symb_len = activeNLS->get_symbol_size();
    for(i = MainWnd->client_height() - 1;i >= 0;i--) {
	ptlines[i].end = lval;
	if(lval >= cp_symb_len) lval = back_scan_cr(lval - cp_symb_len);
	ptlines[i].st = lval;
    }
}

void TextMode::fill_curr_page(__filesize_t lval,__filesize_t flen) const
{
    size_t i;
    tAbsCoord height = MainWnd->client_height();
    for(i = 0;i < height;i++) {
	tlines[i].st = lval;
	if(lval < flen) lval = forward_scan_cr(lval,flen);
	tlines[i].end = lval;
    }
}

void TextMode::prepare_lines(int keycode)
{
    int size,size1,h,height = MainWnd->client_height();
    unsigned cp_symb_len;
    __filesize_t lval,flen,cp = BMGetCurrFilePos();
    cp_symb_len = activeNLS->get_symbol_size();
    flen = BMGetFLength();
    /** search begin of first string */
    h=height-1;
    size = sizeof(TSTR)*h;
    size1 = size + sizeof(TSTR);
    if(keycode == KE_UPARROW) {
	/** i.e. going down */
	if(tlines[0].st) {
	    ::memmove(&tlines[1],tlines,size);
	    tlines[0] = ptlines[h];
	    ::memmove(&ptlines[1],ptlines,size);
	    lval = ptlines[1].st;
	    ptlines[0].end = lval;
	    if(lval >= cp_symb_len) lval-=cp_symb_len;
	    ptlines[0].st = back_scan_cr(lval);
	}
    } else if(keycode == KE_DOWNARROW) {
	/** i.e. going up */
	if(tlines[0].end < flen) {
	    ::memmove(ptlines,&ptlines[1],size);
	    ptlines[h] = tlines[0];
	    ::memmove(tlines,&tlines[1],size);
	    lval = tlines[h - 1].end;
	    tlines[h].st = lval;
	    tlines[h].end = forward_scan_cr(lval,flen);
        }
    } else if(keycode == KE_PGUP) {
	if(ptlines[0].st) {
	    ::memcpy(tlines,ptlines,size1);
	    lval = tlines[0].st;
	    fill_prev_page(lval);
	} else goto CommonPart;
    } else if(keycode == KE_PGDN) {
	if(tlines[h].end < flen) {
	    ::memcpy(ptlines,tlines,size1);
	    lval = ptlines[h].end;
	    fill_curr_page(lval,flen);
	} else goto CommonPart;
    } else if(cp != tmocpos || keycode == KE_SUPERKEY) {
	CommonPart:
	lval = back_scan_cr(cp);
	if(cp != lval) cp = lval;
	if(cp) fill_prev_page(lval);
	else { PrevStrLen = PrevPageSize = 2; memset(ptlines,0,sizeof(TSTR)*height); }
	/** scan forward */
	fill_curr_page(lval,flen);
    }
    tmocpos = cp;
    PrevStrLen = ptlines[h].end - ptlines[h].st + cp_symb_len;
    PrevPageSize = ptlines[h].end - ptlines[0].st + cp_symb_len;
}

unsigned TextMode::tab2space(tvioBuff * dest,unsigned int alen,char * str,unsigned int len,unsigned int shift,unsigned *n_tabs,long lstart) const
{
    long end_ctx;
    unsigned char ch,defcol;
    size_t i,size,j,k;
    unsigned int freq, end_kwd, st_len;
    int in_ctx,in_kwd;
    ColorAttr ctx_color=text_cset.normal;
    if(n_tabs) *n_tabs = 0;
    in_ctx=0;
    in_kwd=0;
    end_kwd=0; end_ctx=0;
    st_len=0;
    for(i = 0,freq = 0,k = 0;i < len;i++,freq++) {
	defcol = text_cset.normal;
	ch = str[i];
	if(dest && HiLight) {
	    if(end_ctx) {
		if((lstart+(long)i)>=end_ctx) goto rescan;
	    } else {
		rescan:
		ctx_color=hl_get_ctx(lstart+i,&in_ctx,&end_ctx);
	    }
	    if(!in_ctx && i>=end_kwd) {
		ctx_color = hl_find_kwd(&str[i],(Color)defcol,&st_len);
		in_kwd=st_len?1:0;
		if(is_legal_word_char(str[i+st_len])||(i && is_legal_word_char(str[i-1]))) in_kwd=0;
		if(in_kwd) end_kwd=i+st_len;
		else       end_kwd=strspn(&str[i],(char*)wset);
	    }
	    if(in_kwd || in_ctx) defcol=ctx_color;
	}
	if(ch < 32) {
	    switch(ch) {
		default:
		case 0: /**NUL  end string*/
		case 1: /*SOH  start of heading*/
		case 2: /**SOT  start of text*/
		case 3: /**ETX  end of text*/
		case 4: /**EOT  end of transmission*/
		case 5: /**ENQ  enquiry*/
		case 6: /**ACK  acknowledge*/
		case 7: /**BEL  bell*/
		case 11: /**VT  virtical tab*/
		case 12: /**FF  form feed*/
		case 14: /**SO  shift out*/
		case 15: /**SI  shift in*/
		case 16: /**DLE data line escape*/
		case 17: /**DC1 dev ctrl 1 (X-ON)*/
		case 18: /**DC2 dev ctrl 2*/
		case 19: /**DC3 dev ctrl 3 (X-OFF)*/
		case 20: /**DC4 dev ctrl 4*/
		case 21: /**NAK negative acknowledge*/
		case 22: /**SYN synhronous idel*/
		case 23: /**ETB end transmission block*/
		case 24: /**CAN cancel*/
		case 25: /**EM  end of medium*/
		case 26: /**SUB substitude*/
		case 27: /**ESC escape*/
		case 28: /**FS  file separator*/
		case 29: /**GS  group separator*/
		case 30: /**RS  record separator*/
		case 31: /**US  unit separator*/
			break;
		case 9: /**HT   horizontal tab*/
		    {
			size = TEXT_TAB - (freq%TEXT_TAB);
			for(j = 0;j < size;j++,freq++) {
			    if(k < alen && freq >= shift) {
				if(dest) {
				    dest->chars[k] = ' ';
				    dest->oem_pg[k] = 0;
				    dest->attrs[k] = defcol;
				}
				k++;
			    }
			}
			if(n_tabs) (*n_tabs)++;
		    }
		    freq--;
		    break;
		case 10: /**LF  line feed*/
		case 13: /**CR  cariage return*/
			goto End;
		case 8:  /**BS  backspace*/
		    {
			char pch;
			pch = i ? str[i-1] : str[0];
			switch(pch) {
			    case '_': defcol = text_cset.underline; break;
			    case '-': defcol = text_cset.strikethrough; break;
			    default:  defcol = text_cset.bold;
			}
			if(i < len) ch = str[++i];
			if(k) k--;
			freq--;
		    }
		    if(freq >= shift) goto DefChar;
		    break;
	    }
	} else {
	    DefChar:
	    if(k < alen && freq >= shift) {
		if(dest) {
		    if(!(in_ctx||in_kwd) && HiLight) defcol=syntax_hl.op_hash[ch];
		    dest->chars[k] = ch;
		    dest->oem_pg[k] = 0;
		    dest->attrs[k] = defcol;
		}
		k++;
	    }
	}
    }
    End:
    return k;
}

void TextMode::paint_search(HLInfo * cptr,unsigned int shift,int i,int size,int _bin_mode) const
{
    int sh,she;
    __fileoff_t save,savee;
    unsigned cp_symb_len,loc_st,loc_end;
    cp_symb_len = activeNLS->get_symbol_size();
    loc_st = FoundTextSt > tlines[i].st ? (unsigned)(FoundTextSt - tlines[i].st) : 0;
    loc_end = (unsigned)(FoundTextEnd - tlines[i].st);
    if(_bin_mode) {
	sh = loc_st - shift;
	she = loc_end - shift;
    } else {
	sh = tab2space(NULL,UINT_MAX,buff,loc_st/cp_symb_len,shift,NULL,0L)*cp_symb_len;
	she= tab2space(NULL,UINT_MAX,buff,loc_end/cp_symb_len,shift,NULL,0L)*cp_symb_len;
    }
    save = FoundTextSt;
    savee= FoundTextEnd;
    FoundTextSt = tlines[i].st + shift + sh/cp_symb_len;
    FoundTextEnd = tlines[i].st + shift + she/cp_symb_len;
    HiLightSearch(MainWnd,tlines[i].st + shift,0,size,i,cptr,_bin_mode == MOD_BINARY ? HLS_NORMAL : HLS_USE_BUFFER_AS_VIDEO);
    FoundTextSt = save;
    FoundTextEnd = savee;
}

static void  __FASTCALL__ drawBound(TWindow* w,int x,int y,char ch)
{
  w->goto_xy(x,y);
  w->set_color(browser_cset.bound);
  w->putch(ch);
  w->set_color(browser_cset.main);
}

TextMode::TextMode(CodeGuider& code_guider)
	:Plugin(code_guider)
	,HiLight(1)
	,bin_mode(MOD_PLAIN)
	,txtHandle(&bNull)
	,maxstrlen(MAX_STRLEN)
	,activeNLS(&RussianNLS)
{
    buff = new char [MAX_STRLEN];
    tlines = new TSTR[__TVIO_MAXSCREENWIDTH];
    ptlines = new TSTR[__TVIO_MAXSCREENWIDTH];
    if((!buff) || (!tlines) || !(ptlines)) {
	MemOutBox("Text mode initialization");
	::exit(EXIT_FAILURE);
    }
    binary_stream& bh = BMbioHandle();
    if((txtHandle = bh.dup()) == &bNull) txtHandle = &bh;
    ::memset(&syntax_hl,0,sizeof(syntax_hl));
    /* Fill operator's hash */
    ::memset(syntax_hl.op_hash,text_cset.normal,sizeof(syntax_hl.op_hash));
    if(detect()) read_syntaxes();
}

TextMode::~TextMode() {
    unsigned i;
    delete buff;
    delete tlines;
    delete ptlines;
    binary_stream& bh = BMbioHandle();
    if(txtHandle != &bh) { delete txtHandle; txtHandle = &bNull; }
    if(syntax_hl.name) delete syntax_hl.name;
    if(escape) delete escape;
    escape=NULL;
    if(syntax_hl.context) {
	for(i=0;i<syntax_hl.context_num;i++) { delete syntax_hl.context[i].start_seq; delete syntax_hl.context[i].end_seq; }
	delete syntax_hl.context;
    }
    if(syntax_hl.keyword) {
	for(i=0;i<syntax_hl.keyword_num;i++) { delete syntax_hl.keyword[i].keyword; }
        delete syntax_hl.keyword;
    }
    delete acontext;
    acontext=NULL;
    acontext_num=0;
    ::memset(&syntax_hl,0,sizeof(syntax_hl));
}

static const char* txt[] = { "", "CodPag", "TxMode", "NLSSet", "", "", "", "TxType", "HiLght", "UsrNam" };
const char* TextMode::prompt(unsigned idx) const {
    return (idx < 10) ? txt[idx] : "";
}

unsigned TextMode::convert_cp(char *_buff,unsigned size,bool use_fs_nls)
{
    return activeNLS->convert_buffer(_buff,size,use_fs_nls);
}

unsigned TextMode::paint( unsigned keycode, unsigned shift )
{
    int hilightline;
    size_t i;
    unsigned size,rsize,rshift;
    __filesize_t cpos;
    unsigned cp_symb_len,len,tmp,textmaxlen;
    tAbsCoord height = MainWnd->client_height();
    HLInfo hli;
    tvioBuff it;
    t_vchar chars[__TVIO_MAXSCREENWIDTH];
    t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    it.chars = chars;
    it.oem_pg = oem_pg;
    it.attrs = attrs;
    cp_symb_len = activeNLS->get_symbol_size();
    strmaxlen = 0;
    if(shift%cp_symb_len) {
	if(keycode == KE_RIGHTARROW || keycode == KE_PGDN) shift+=shift%cp_symb_len;
	else shift=(shift/cp_symb_len)*cp_symb_len;
    }
    maxstrlen = wmode ? tvioWidth : (MAX_STRLEN / cp_symb_len) - 3;
    cpos = BMGetCurrFilePos();
    if(cpos%cp_symb_len) {
	if(keycode == KE_RIGHTARROW || keycode == KE_PGDN) cpos+=cpos%cp_symb_len;
	else cpos=(cpos/cp_symb_len)*cp_symb_len;
    }
    BMSeek(cpos,binary_stream::Seek_Set);
    if(!(keycode == KE_LEFTARROW || keycode == KE_RIGHTARROW)) prepare_lines(keycode);
    hilightline = -1;
    for(i = 0;i < height;i++) {
	len = (unsigned)(tlines[i].end - tlines[i].st);
	if(len > strmaxlen) strmaxlen = len;
	if(isHOnLine(tlines[i].st,len)) {
	    hilightline = i;
	    if(keycode == KE_JUSTFIND) {
		if(bin_mode != MOD_BINARY) {
		    unsigned n_tabs,b_ptr,b_lim;
		    len = std::min(MAX_STRLEN,FoundTextSt > tlines[i].st ? (int)(FoundTextSt-tlines[i].st):unsigned(0));
		    BMReadBufferEx((any_t*)buff,len,tlines[i].st,binary_stream::Seek_Set);
		    len = convert_cp(buff,len,false);
		    for(b_lim=len,b_ptr = 0;b_ptr < len;b_ptr+=2,b_lim-=2) {
			shift = tab2space(NULL,UINT_MAX,&buff[b_ptr],b_lim,0,&n_tabs,0L);
			if(shift) shift-=cp_symb_len;
			if(shift < (unsigned)(tvioWidth/2)) break;
		    }
		    shift = tab2space(NULL,UINT_MAX,buff,b_ptr,0,NULL,0L);
		} else if(!isHOnLine((tlines[i].st+shift)*cp_symb_len,std::min(len,tvioWidth))) {
		    shift = ((unsigned)(FoundTextSt - tlines[i].st)-tvioWidth/2)/cp_symb_len;
		    if((int)shift < 0) shift = 0;
		    if(shift%cp_symb_len) shift+=shift%cp_symb_len;
		}
	    }
	    break;
	}
    }
    textmaxlen = maxstrlen - 2;
    MainWnd->freeze();
    for(i = 0;i < height;i++) {
	len = (int)(tlines[i].end - tlines[i].st);
	if(isHOnLine(tlines[i].st,len)) hilightline = i;
	rshift = bin_mode != MOD_BINARY ? 0 : shift;
	rsize = size = len - rshift;
	if(len > rshift) {
	    BMReadBufferEx((any_t*)buff,size,tlines[i].st + rshift,binary_stream::Seek_Set);
	    rsize = size = convert_cp(buff,size,false);
	    if(bin_mode != MOD_BINARY) {
		rsize = size = tab2space(&it,__TVIO_MAXSCREENWIDTH,buff,size,shift,NULL,tlines[i].st);
		tmp = size + shift;
		if(strmaxlen < tmp) strmaxlen = tmp;
		if(textmaxlen < tmp) textmaxlen = tmp;
	    }
	    if(size > tvioWidth) size = tvioWidth;
	    if(i == (unsigned)hilightline) {
		if(bin_mode == MOD_BINARY) hli.text = buff;
		else                       hli.buff = it;
		paint_search(&hli,shift,i,size,bin_mode == MOD_BINARY);
	    } else {
		if(bin_mode == MOD_BINARY) MainWnd->direct_write(1,i+1,buff,size);
		else                       MainWnd->write(1,i + 1,&it,size);
	    }
	    if(rsize < tvioWidth) {
		MainWnd->goto_xy(1 + rsize,i + 1);
		MainWnd->clreol();
	    } else if(rsize > tvioWidth) drawBound(MainWnd,tvioWidth,i + 1,TWC_RT_ARROW);
	} else {
	    MainWnd->goto_xy(1,i + 1);
	    MainWnd->clreol();
	}
	if(shift) drawBound(MainWnd,1,i + 1,TWC_LT_ARROW);
	lastbyte = tlines[i].st;
	lastbyte += bin_mode == MOD_BINARY ? shift + size : rshift + len;
    }
    MainWnd->refresh();
    tmp = textmaxlen - tvioWidth + 2;
    if(shift > tmp) shift = tmp;
    if(!tlines[1].st) tlines[1].st = tlines[0].end;
    CurrStrLen = tlines[0].end - tlines[0].st;
    CurrPageSize = lastbyte - tlines[0].st;
    BMSeek(cpos,binary_stream::Seek_Set);
    return shift;
}

void TextMode::help() const
{
   hlpDisplay(1001);
}

unsigned long TextMode::prev_page_size() const { return PrevPageSize; }
unsigned long TextMode::curr_page_size() const { return CurrPageSize; }
unsigned long TextMode::prev_line_width() const { return PrevStrLen; }
unsigned long TextMode::curr_line_width() const { return CurrStrLen; }
const char*   TextMode::misckey_name() const { return wmode ? "Unwrap" : "Wrap  "; }
void          TextMode::misckey_action() { wmode = wmode ? false : true; }

bool TextMode::action_F2() /* txtSelectCP */
{
    return activeNLS->select_table();
}

bool TextMode::action_F3() /* txtSelectMode */
{
    unsigned nModes;
    int i;
    nModes = sizeof(mod_names)/sizeof(char *);
    i = SelBoxA(mod_names,nModes," Select text mode: ",bin_mode);
    if(i != -1) {
	bin_mode = i;
	return true;
    }
    return false;
}

static const char *hilight_names[] =
{
   "~Mono",
   "~Highlight"
};
bool TextMode::action_F9() /* txtSelectHiLight */
{
    unsigned nModes;
    int i;
    nModes = sizeof(hilight_names)/sizeof(char *);
    i = SelBoxA(hilight_names,nModes," Select highlight mode: ",HiLight);
    if(i != -1) {
	HiLight = i;
	return true;
    }
    return false;
}

bool TextMode::action_F4() /* txtSelectNLS */
{
    const char *modeName[sizeof(nls_set)/sizeof(REGISTRY_NLS *)];
    size_t i,nModes;
    int retval;

    nModes = sizeof(nls_set)/sizeof(REGISTRY_NLS *);
    for(i = 0;i < nModes;i++) modeName[i] = nls_set[i]->set_name;
    retval = SelBoxA(modeName,nModes," Select NLS set: ",defNLSSet);
    if(retval != -1) {
	if(activeNLS->term) activeNLS->term();
	activeNLS = nls_set[retval];
	if(activeNLS->init) activeNLS->init();
	defNLSSet = retval;
	return true;
    }
    return false;
}


inline bool  __FASTCALL__ isBinByte(unsigned char ch)
{
  return ch < 32 && !isspace(ch & 0xFF) && ch != 0x08 && ch != 0x1A;
}

bool TextMode::action_F10() { return udnUserNames(); }

bool TextMode::detect()
{
    size_t maxl,i;
    bool bin = false;
    __filesize_t flen,fpos;
    maxl = 1000;
    flen = BMGetFLength();
    fpos=BMGetCurrFilePos();
    if(maxl > flen) maxl = (size_t)flen;
    for(i = 0;i < maxl;i++) {
	char ch;
	ch = BMReadByteEx(i,binary_stream::Seek_Set);
	if((bin=isBinByte(ch))!=false) break;
    }
    if(bin==false) bin_mode = MOD_PLAIN;
    BMSeek(fpos,binary_stream::Seek_Set);
    return bin == false;
}

void TextMode::read_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    std::string tmps;
    if(bctx.is_valid_ini_args()) {
	int w_m;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode4","0");
	defNLSSet = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(defNLSSet > sizeof(nls_set)/sizeof(REGISTRY_NLS *)) defNLSSet = 0;
	activeNLS = nls_set[defNLSSet];
	if(activeNLS->init) activeNLS->init();
	activeNLS->read_ini(ini);
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode3","0");
	bin_mode = (unsigned)::strtoul(tmps.c_str(),NULL,10);
	if(bin_mode > MOD_MAXMODE) bin_mode = 0;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","MiscMode","0");
	w_m = (int)::strtoul(tmps.c_str(),NULL,10);
	wmode = w_m ? true : false;
	tmps=bctx.read_profile_string(ini,"Beye","Browser","SubSubMode9","0");
	HiLight = (int)::strtoul(tmps.c_str(),NULL,10);
	if(HiLight > 1) HiLight = 1;
    }
}

void TextMode::save_ini(Ini_Profile& ini)
{
    BeyeContext& bctx = beye_context();
    char tmps[10];
    ::sprintf(tmps,"%i",bin_mode);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode3",tmps);
    bctx.write_profile_string(ini,"Beye","Browser","MiscMode",wmode ? "1" : "0");
    ::sprintf(tmps,"%i",defNLSSet);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode4",tmps);
    ::sprintf(tmps,"%i",HiLight);
    bctx.write_profile_string(ini,"Beye","Browser","SubSubMode9",tmps);
    activeNLS->save_ini(ini);
}

bool TextMode::action_F8 () /* txtShowType */
{
    const char *type;
    if(syntax_hl.name) type=syntax_hl.name;
    else type="Unknown";
    TMessageBox(type," Detected text type: ");
    return false;
}

unsigned TextMode::get_symbol_size() const { return activeNLS->get_symbol_size(); }
unsigned TextMode::get_max_line_length() const { return strmaxlen; }
TextMode::e_flag TextMode::flags() const { return Text|Has_ConvertCP; }

static Plugin* query_interface(CodeGuider& code_guider) { return new(zeromem) TextMode(code_guider); }

extern const Plugin_Info textMode = {
    "~Text mode",	/**< plugin name */
    query_interface
};

} // namespace	usr
