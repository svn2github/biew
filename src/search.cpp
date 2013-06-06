#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        search.c
 * @brief       This file contains implementation of file search interface.
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
#include <limits.h>
#include <stdlib.h>
#define  _CT_FTM
#include <ctype.h>

#include "beye.h"
#include "colorset.h"
#include "tstrings.h"
#include "search.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bbio.h"
#include "plugins/plugin.h"

extern void ReReadFile( int );

namespace	usr {
Search::Search(BeyeContext& b)
	:beyeSearchFlg(None)
	,bctx(b)
{
}

Search::~Search() {}

void Search::fillBoyerMooreCache(int *cache, const char *pattern,
				unsigned pattern_len, bool case_sens)
{
    size_t iptr;
    ::memset(cache,0,(UCHAR_MAX+1)*sizeof(int));
    for(iptr = 0;iptr < pattern_len;iptr++) {
	cache[ case_sens ? ((const unsigned char *)pattern)[iptr] :
			    toupper(((const unsigned char *)pattern)[iptr]) ] = iptr+1;
    }
}

enum {
    __LF_NORMAL    =0x0000, /**< Indicates normal search engine */
    __LF_NOSEEK    =0x0001, /**< Indicates that search must be performed at given offset only */
    __LF_NOLEFT    =0x0002, /**< Indicates that search engine should ignore left whitespace */
    __LF_NORIGHT   =0x0004, /**< Indicates that search engine should ignore right whitespace */
    __LF_HIDEPRCNT =0x8000  /**< Indicates that search engine must not display percents >*/
};
enum {
    __MAX_SYMBOL_SIZE=4
};
		   /** Performs single search (without templates) within file.
		     * @return                address of found sequence
		     *                        if global variable __found is true
		     *                        otherwise:
		     *                        0L - if sequence is not found
		     *                        FILESIZE_MAX - if sequence can not be
		     *                        found (EOF is reached)
		     * @param sfrom           indicates string where search must
		     *                        be performed. If NULL then search
		     *                        will be performed in file stream.
		     * @param slen            indicates length of sfrom
		     * @param flags           indicates __LF_* flags family
		     * @param start           indicates offset from which search
		     *                        must be performed
		     * @param scache          indicates Boyer-Moore cache.
		     *                        If NULL then will be used internal
		     *                        cache.
		     * @param pattern         indicates search pattern
		     * @param pattern_size    indicates size of search pattern
		     * @param beyeFlg         indicates global flags of Beye
		     *                        search engine.
		    **/
__filesize_t Search::___lfind(const char *sfrom,
			    unsigned slen,
			    unsigned flags,
			    __filesize_t start,
			    const int *scache,
			    const char *pattern,
			    unsigned pattern_size,
			    search_flags beyeFlg)
{
    __filesize_t flen, endscan, orig_start;
    __filesize_t tsize,cpos,findptr = FILESIZE_MAX,retval;
    char fbuff[MAX_SEARCH_SIZE*__MAX_SYMBOL_SIZE], nbuff[__MAX_SYMBOL_SIZE];
    unsigned proc,pproc,pmult,symb_size,bio_opt=0;
    int direct,icache[UCHAR_MAX+1];
    const int *cache;
    bool cond;
    unsigned char __search_len;
    unsigned char ch,ch1;
    char cbuff[MAX_SEARCH_SIZE];
    symb_size = bctx.active_mode().get_symbol_size();
    /*
     * Cache initialization for adapted Boyer-Moore search algorithm
    */
    __found = false;
    orig_start = start;
    retval = 0;
    if(!scache) {
	fillBoyerMooreCache(icache, pattern, pattern_size, beyeFlg & Case_Sens);
	cache = icache;
    } else cache = scache;
    flen = sfrom ? slen : bctx.flength();
    endscan = beyeFlg & Reverse ? 0 : flen;
    direct  = beyeFlg & Reverse ? -1 : 1;
    tsize = flen;
    pmult = 100;
    if(tsize > FILESIZE_MAX/100) { tsize /= 100; pmult = 1; }
    cond = false;
    pproc = proc = 0;
    /* seek to the last character of pattern by direction */
    start += beyeFlg & Reverse ? 0 : (pattern_size-1)*symb_size;
    if(!sfrom) {
	bio_opt = bctx.bm_file().get_optimization();
	bctx.bm_file().set_optimization((bio_opt & (~BBio_File::Opt_DirMask)) |
		      (beyeFlg & Reverse ? BBio_File::Opt_RBackScan : BBio_File::Opt_RForward));
    }
    start = (start/symb_size)*symb_size; /** align on symbol boundary */
    ::memcpy(cbuff,pattern,pattern_size);
    if(!(beyeFlg & Case_Sens)) memupr((any_t*)cbuff,pattern_size);
    for(cpos = start;start != endscan;start = direct == 1 ? start + pattern_size*symb_size : start - (pattern_size*symb_size),cpos=start) {
	/* If search direction is forward then start point at the end of pattern */
	if(direct == 1 && start*symb_size > flen) {
	    retval = FILESIZE_MAX;
	    break;
	}
	/* If search direction is backward then start point at the begin of pattern */
	if(direct == -1 && start < (pattern_size*symb_size)) {
	    retval = FILESIZE_MAX;
	    break;
	}
	proc = (unsigned)((cpos*pmult)/tsize);
	if(proc != pproc && !(flags & __LF_HIDEPRCNT)) {
	    if(!ShowPercentInWnd(prcntswnd,pproc=proc))  break;
	}
	if(sfrom) ::memcpy(nbuff,&sfrom[start],symb_size);
	else {
	    bctx.bm_file().seek(start,binary_stream::Seek_Set);
	    bctx.bm_file().read(nbuff,symb_size);
	}
	if((bctx.active_mode().flags() & Plugin::Text) == Plugin::Text) bctx.active_mode().convert_cp(nbuff,symb_size,false);
	ch = nbuff[0];
	if(!(beyeFlg & Case_Sens)) ch = toupper(ch);
	if(cache[ch]) {
	    if(pattern_size > 1) {
		findptr = start-(cache[ch]-1)*symb_size;
		if((flags & __LF_NOSEEK) && findptr != orig_start) break;
		if(sfrom) ::memcpy(fbuff,&sfrom[findptr],pattern_size*symb_size);
		else {
		    bctx.bm_file().seek(findptr,binary_stream::Seek_Set);
		    bctx.bm_file().read((any_t*)fbuff,pattern_size*symb_size);
		}
		if((bctx.active_mode().flags() & Plugin::Text) == Plugin::Text)
		    __search_len = bctx.active_mode().convert_cp((char *)fbuff,pattern_size*symb_size,false);
		else __search_len = pattern_size;
		if(!(beyeFlg & Case_Sens)) memupr((any_t*)fbuff,__search_len);
		if(::memcmp(fbuff,cbuff,__search_len) == 0) cond = true;
		else {
		    if(flags & __LF_NOSEEK) break;
		    else {
			start = direct == 1 ? start-(pattern_size-1)*symb_size :
				  start+(pattern_size-1)*symb_size;
			continue;
		    }
		}
	    } else {
		findptr = start;
		cond = true;
	    }
	    if((beyeFlg & Word_Only) && cond) {
		if(start && !(flags & __LF_NOLEFT)) {
		    if(sfrom) ::memcpy(nbuff,&sfrom[findptr-symb_size],symb_size);
		    else {
			bctx.bm_file().seek(findptr - symb_size,binary_stream::Seek_Set);
			bctx.bm_file().read(nbuff,symb_size);
		    }
		    if((bctx.active_mode().flags() & Plugin::Text) == Plugin::Text) bctx.active_mode().convert_cp(nbuff,symb_size,false);
		    ch = nbuff[0];
		} else ch = ' ';
		if(start + pattern_size < flen && !(flags & __LF_NORIGHT)) {
		    if(sfrom) ::memcpy(nbuff,&sfrom[findptr + (pattern_size*symb_size)],symb_size);
		    else {
			bctx.bm_file().seek(findptr + (pattern_size*symb_size),binary_stream::Seek_Set);
			bctx.bm_file().read(nbuff,symb_size);
		    }
		    if((bctx.active_mode().flags() & Plugin::Text) == Plugin::Text) bctx.active_mode().convert_cp(nbuff,symb_size,false);
		    ch1 = nbuff[0];
		} else ch1 = ' ';
		if(!(isseparate(ch) && isseparate(ch1))) cond = false;
	    }
	}
	if(cond) { __found = true; retval = findptr; break; }
	if(flags & __LF_NOSEEK) break;
    }
    if(!sfrom) bctx.bm_file().set_optimization(bio_opt);
    return retval;
}

__filesize_t Search::___adv_find(const char *sfrom,
				unsigned sfromlen,
				__filesize_t start,
				__filesize_t *slen,
				const int *scache,
				const char *pattern,
				unsigned pattern_size,
				search_flags beyeFlg)
{
  __filesize_t _found,found_st=FILESIZE_MAX,prev_found;
  __filesize_t stable_found;
  unsigned i, orig_i, last_search_len;
  unsigned orig_slen, t_count, flags;
  bool is_tmpl, has_question;
  bool always_prcnt;
  unsigned orig_direct;
  char cbuff[MAX_SEARCH_SIZE];
  char firstc;
  is_tmpl = (beyeFlg & Wild_Cards) == Wild_Cards;
  *slen = pattern_size;
  if(!is_tmpl) return ___lfind(sfrom, sfromlen, sfrom ? __LF_HIDEPRCNT : __LF_NORMAL,start, scache, pattern, pattern_size, beyeFlg);
  memcpy(cbuff, pattern, pattern_size);
  orig_slen = pattern_size;
  orig_direct = beyeFlg & Reverse;
  _found = 0L;
  restart:
  always_prcnt = false;
  i = 0;
  prev_found = start;
  flags = __LF_NORMAL;
  has_question = false;
  stable_found = FILESIZE_MAX;
  while(1)
  {
    orig_i = i;
    for(;i < pattern_size;i++)
    {
      if(cbuff[i] == '*' || cbuff[i] == '?')
      {
	flags |= __LF_NORIGHT;
	break;
      }
    }
    pattern_size = i-orig_i;
    last_search_len = pattern_size;
    if(pattern_size)
    {
      memcpy(cbuff,&pattern[orig_i],pattern_size);
      if((flags & __LF_NOSEEK && !always_prcnt) || sfrom) flags |= __LF_HIDEPRCNT;
      _found = ___lfind(sfrom, sfromlen, flags,start, NULL, cbuff, pattern_size, beyeFlg);
    }
    else
    {
      if(!orig_i && !(beyeFlg & Word_Only))
      {
	/* it means: first character is wildcard and it much better
	   to restart search engine */
	firstc = cbuff[0];
	pattern_size = orig_slen;
	if(orig_direct & Reverse) beyeFlg &= ~Reverse;
	else                         beyeFlg |= Reverse;
	memmove(cbuff, &pattern[1], --pattern_size);
	found_st = ___adv_find(sfrom, sfromlen, start,slen, scache, cbuff, pattern_size, beyeFlg);
	(*slen)++;
	if(__found)
	{
	  switch(firstc)
	  {
	    case '?': if(found_st) found_st--; break;
	    default:  (*slen)+= found_st; found_st = 0L; break;
	  }
	}
	goto exit;
      }
      else
      {
	__found = true;
	_found = prev_found;
	always_prcnt = true;
      }
    }
    flags = __LF_NORMAL; /* reseting flags immediately after search */
    if(__found)
    {
       if(!orig_i) stable_found = _found;
       t_count = 0;
       if(found_st == FILESIZE_MAX) found_st = _found;
       if(orig_i)
	if(pattern[orig_i-1] == '?' &&
	  prev_found+last_search_len+t_count != _found) /* special case for '?' */
	     found_st = _found-orig_i;
       still:
       switch(cbuff[i])
       {
	 case '?': while(cbuff[i] == '?') { t_count++; i++; }
		   flags = __LF_NOSEEK | __LF_NOLEFT;
		   has_question = true;
		   goto still;
	 case '*': while(cbuff[i] == '*') i++;
		   pattern_size = orig_slen-i;
		   memmove(cbuff, &pattern[i], pattern_size);
		   beyeFlg &= ~Reverse;
		   found_st = ___adv_find(sfrom, sfromlen,_found+last_search_len,slen, scache, cbuff, pattern_size, beyeFlg);
		   (*slen)++;
		   goto exit;
	 default: break;
       }
       start=_found+last_search_len+t_count;
       beyeFlg &= ~Reverse; /* Anyway: we have found a first subsequence.
				 For searching with _using template need
				 only forward search technology. */
       pattern_size = orig_slen;
    }
    else
    {
      if(!has_question || _found == FILESIZE_MAX)
      {
	found_st = FILESIZE_MAX;
	break;
      }
      else /* restarting search engine */
      {
	if(found_st == FILESIZE_MAX) break;
	pattern_size = orig_slen;
	if(orig_direct & Reverse) beyeFlg |= Reverse;
	else                         beyeFlg &= Reverse;
	start = beyeFlg & Reverse ? stable_found-1 : stable_found+1;
	memcpy(cbuff,pattern,pattern_size);
	found_st = FILESIZE_MAX;
	goto restart;
      }
    }
    if(i >= orig_slen) break;
  }
  if(found_st == FILESIZE_MAX) found_st = 0;
  /* Special case if last character is wildcard */
  if(cbuff[orig_slen-1] == '?') last_search_len++;
  *slen = _found+last_search_len-found_st;
  if(cbuff[orig_slen-1] == '*') (*slen) = FILESIZE_MAX - _found;
exit:
  pattern_size = orig_slen;
  if(orig_direct & Reverse) beyeFlg &= ~Reverse;
  else                         beyeFlg |= Reverse;
  return found_st;
}

__filesize_t Search::__adv_find(__filesize_t start,__filesize_t* slen) { return ___adv_find(NULL, 0, start, slen, NULL, (const char*)search_buff, search_len, beyeSearchFlg); }

char* Search::strFind(const char *str, unsigned str_len,
			const any_t*sbuff, unsigned sbuflen,
			const int *cache, search_flags flg)
{
    __filesize_t slen;
    unsigned long lretval;
    lretval = ___adv_find(str, str_len, 0, &slen, cache, (const char*)sbuff, sbuflen, flg & (~Reverse));
    return (char *)(__found ? &str[lretval] : 0);
}

void Search::SearchPaint(TWindow& wdlg,dialog_flags flags,search_flags sf_flags)
{
    wdlg.set_color(dialog_cset.group.active);
    if(sf_flags & As_Hex) wdlg.set_color(dialog_cset.group.disabled);
    else wdlg.set_color(dialog_cset.group.active);
    wdlg.goto_xy(4,4); wdlg.putch(Gebool(sf_flags & Case_Sens));
    wdlg.set_color(dialog_cset.group.active);
    wdlg.goto_xy(4,5); wdlg.putch(Gebool(sf_flags & Word_Only));
    wdlg.goto_xy(4,6); wdlg.putch(Gebool(sf_flags & Reverse));
    wdlg.goto_xy(46,4); wdlg.putch(Gebool(sf_flags & As_Hex));
    wdlg.set_color((!(flags == All_Features) || sf_flags & As_Hex)?dialog_cset.group.disabled:dialog_cset.group.active);
    wdlg.goto_xy(46,5); wdlg.putch(Gebool(sf_flags & Wild_Cards));
    wdlg.set_color(!((flags == All_Features) && (bctx.active_mode().flags()&Plugin::Has_SearchEngine))?dialog_cset.group.disabled:dialog_cset.group.active);
    wdlg.goto_xy(46,6); wdlg.putch(Gebool(sf_flags & Plugins));
    wdlg.set_color(dialog_cset.main);
}

void Search::SearchUpdate(TWindow& wdlg,dialog_flags _flags,search_flags sf_flags)
{
    wdlg.set_color((sf_flags & As_Hex)?dialog_cset.group.disabled:dialog_cset.group.active);
    wdlg.goto_xy(2,4); wdlg.puts(msgFindOpt[0]);
    wdlg.set_color(dialog_cset.group.active);
    wdlg.goto_xy(2,5); wdlg.puts(msgFindOpt[1]);
    wdlg.goto_xy(2,6); wdlg.puts(msgFindOpt[2]);
    wdlg.goto_xy(44,4); wdlg.puts(msgFindOpt2[0]);
    wdlg.set_color((!(_flags == All_Features) || sf_flags & As_Hex)?dialog_cset.group.disabled:dialog_cset.group.active);
    wdlg.goto_xy(44,5); wdlg.puts(msgFindOpt2[1]);
    wdlg.set_color(!((_flags == All_Features) && (bctx.active_mode().flags() & Plugin::Has_SearchEngine))?dialog_cset.group.disabled:dialog_cset.group.active);
    wdlg.goto_xy(44,6); wdlg.puts(msgFindOpt2[2]);
    wdlg.set_color(dialog_cset.main);
}

static const char * searchtxt[] =
{
  "Help  ",
  "CasSen",
  "WrdOnl",
  "ScanDr",
  "Hex   ",
  "Templt",
  "Plugin",
  "      ",
  "      ",
  "Escape"
};

static void drawSearchPrompt()
{
   __drawSinglePrompt(searchtxt);
}

bool Search::dialog(dialog_flags _flags, char* searchbuff,
			unsigned char* searchlen,
			search_flags& sf_flags)
{
    TWindow* hwnd,* ewnd;
    tAbsCoord x1,y1,x2,y2;
    tRelCoord X1,Y1,X2,Y2;
    unsigned x[2] = { 0, 0 };
    int rret, active;
    bool ret;
    int update;
    char attr[2] = { __ESS_FILLER_7BIT | __ESS_WANTRETURN | __ESS_ENABLEINSERT | __ESS_NON_C_STR,
		   __ESS_WANTRETURN | __ESS_ASHEX | __ESS_NON_C_STR };
    char ebuff1[MAX_SEARCH_SIZE],ebuff2[MAX_SEARCH_SIZE*3];
    char *ebuff[2];
    const char* legal[2];
    unsigned mlen[2],flags;
    int ch,i;
    hwnd = CrtDlgWndnls(FIND_STR,78,7);
    hwnd->set_footer(" [Enter] - Start ",TWindow::TMode_Center,dialog_cset.footer);
    hwnd->get_pos(x1,y1,x2,y2);
    X1 = x1+2;
    Y1 = y1+2;
    X2 = X1+75;
    Y2 = Y1;
    ewnd = CreateEditor(X1,Y1,X2,Y2,TWindow::Flag_Has_Cursor);
    ewnd->show();
    hwnd->draw_frame(1,3,78,7,TWindow::DN3D_FRAME,dialog_cset.main);
    hwnd->draw_frame(37,4,42,6,TWindow::UP3D_FRAME,dialog_cset.main);
    hwnd->goto_xy(38,5); hwnd->puts("BASE");
    hwnd->goto_xy(2,1);  hwnd->puts(TYPE_STR);
    SearchUpdate(*hwnd,_flags,sf_flags);
    legal[0] = NULL;
    legal[1] = &legalchars[2];
    ebuff1[0] = ebuff2[0] = '\0';
    ebuff[0] = ebuff1;
    ebuff[1] = ebuff2;

    if(searchlen) {
	::memcpy(ebuff[0],searchbuff,*searchlen);
	ExpandHex(ebuff[1],(unsigned char *)searchbuff,*searchlen,0);
    }
    rret = 2;
    ret = true;
    SearchPaint(*hwnd,_flags,sf_flags);
    update = 1;
    while(1) {
	mlen[0] = MAX_SEARCH_SIZE;
	mlen[1] = MAX_SEARCH_SIZE*3;
	active = sf_flags & As_Hex ? 1 : 0;
	flags = attr[active];
	if(!update) flags |= __ESS_NOREDRAW;
	ewnd->set_focus();
	ch = eeditstring(ewnd,ebuff[active],legal[active],&mlen[active],
		     active ? (*searchlen)*3 : (*searchlen),
		     &x[active],flags,NULL, drawSearchPrompt);
	update = 1;
	switch(ch) {
	    case KE_ENTER  :	if(searchlen) { rret = 1; ret = true; } else { rret = 0; ret = false; } break;
	    case KE_F(10)  :
	    case KE_ESCAPE :	rret = 0; ret = false; break;
	    case KE_F(2)   :	if(!(sf_flags&As_Hex)) sf_flags ^= Case_Sens;
				update = 0;
				break;
	    case KE_F(3)   :	sf_flags ^= Word_Only;
				update = 0;
				break;
	    case KE_F(4)   :	sf_flags ^= Reverse;
				update = 0;
				break;
	    case KE_F(1)  : {
				Beye_Help bhelp;
				if(bhelp.open(true)) {
				    bhelp.run(7);
				    bhelp.close();
				}
			    }
			    update = 0;
			    break;
	    case KE_F(5) :	sf_flags ^= As_Hex;
				update = 2;
				break;
	    case KE_F(6) :	if(!(sf_flags&As_Hex) && (_flags == All_Features)) sf_flags ^= Wild_Cards;
				update = 0;
				break;
	    case KE_F(7):	if(_flags == All_Features && (bctx.active_mode().flags() & Plugin::Has_SearchEngine))
				sf_flags ^= Plugins;
				update = 0;
				break;
	    case KE_LEFTARROW:
	    case KE_RIGHTARROW:
				update = 0;
				break;
	    default : break;
	}
	if(rret != 2) break;
	if(!active) { *searchlen = mlen[0]; memcpy(searchbuff,ebuff[0],mlen[0]); }
	else  { *searchlen = mlen[1] / 3; CompressHex((unsigned char *)searchbuff,ebuff[1],*searchlen,true); }
	if(searchlen) memcpy(ebuff[0],searchbuff,*searchlen);
	else     ebuff[0][0] = '\0';
	mlen[0] = *searchlen;
	ExpandHex(ebuff[1],(unsigned char *)searchbuff,*searchlen,0);
	mlen[1] = (*searchlen)*3;
	for(i = 0;i < 2;i++) if(x[i] > mlen[i]) x[i] = mlen[i];
	if(update>1) SearchUpdate(*hwnd,_flags,sf_flags);
	SearchPaint(*hwnd,_flags,sf_flags);
    }
    if(sf_flags & As_Hex) {
	sf_flags &= ~(Wild_Cards);
	sf_flags |= Case_Sens;
    }
    delete ewnd;
    delete hwnd;
    return ret;
}

__filesize_t Search::run( bool is_continue )
{
    __filesize_t found;
    __filesize_t fmem,lmem,slen, flen;
    bool ret;
    fmem = bctx.tell();
    flen = bctx.flength();
    ret = is_continue ? true : dialog(All_Features,(char *)search_buff,&search_len,beyeSearchFlg);
    if(ret && search_len) {
	prcntswnd = PercentWnd(PLEASE_WAIT,SEARCHING);
	lmem = fmem;
	if(FoundTextSt != FoundTextEnd) {
	    unsigned cp_symb_size;
	    if(is_continue) lmem = FoundTextSt;
	    cp_symb_size = bctx.active_mode().get_symbol_size();
	    if((beyeSearchFlg & Reverse) && lmem) lmem-=cp_symb_size;
	    else if(lmem < flen) lmem+=cp_symb_size;
	}
	__found = false;
	found = (beyeSearchFlg & Plugins) && (bctx.active_mode().flags() & Plugin::Has_SearchEngine) ?
	bctx.active_mode().search_engine(prcntswnd,lmem,&slen,beyeSearchFlg,is_continue,&__found):
	__adv_find(lmem,&slen);
	delete prcntswnd;
	if(__found) {
	    FoundTextSt = found;
	    FoundTextEnd = found + slen*bctx.active_mode().get_symbol_size();
	    /* it is not an error of search engine it is special case:
		adv_find function don't want to use a file stream directly */
	    if(FoundTextEnd > flen) FoundTextEnd = flen;
	    return found;
	} else bctx.ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
    }
    bctx.bm_file().seek(fmem,binary_stream::Seek_Set);
    return fmem;
}

void Search::set_flags(Search::search_flags f) { beyeSearchFlg = f; }
Search::search_flags Search::get_flags() const { return beyeSearchFlg; }

void Search::set_found(__filesize_t start,__filesize_t end) {
    FoundTextSt = start;
    FoundTextEnd = end;
}
void Search::reset() { FoundTextSt = FoundTextEnd; }

int Search::is_inline(__filesize_t cp,int width) const
{
    if(FoundTextSt == FoundTextEnd) return 0;
    return (FoundTextSt >= cp && FoundTextSt < cp + width)
	  || (FoundTextEnd > cp && FoundTextEnd < cp + width)
	  || (FoundTextSt <= cp && FoundTextEnd >= cp + width);
}

void Search::hilight(TWindow& out,__filesize_t cfp,tRelCoord minx,tRelCoord maxx,tRelCoord y,const char* data,hl_search flags)
{
    unsigned __len,width;
    int x;
    char attr;
    uint8_t chars[__TVIO_MAXSCREENWIDTH];
    ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
    width = (flags & HL_Use_Double_Width) == HL_Use_Double_Width ? maxx*2 : maxx-minx;
    attr = browser_cset.highline;
    ::memcpy(chars,data,width);
    ::memset(attrs,attr,width);
    x = (int)(FoundTextSt - cfp);
    if((flags & HL_Use_Double_Width) == HL_Use_Double_Width) x *= 2;
    __len = (unsigned)(FoundTextEnd - FoundTextSt);
    if((flags & HL_Use_Double_Width) == HL_Use_Double_Width) __len *= 2;
    if(__len > width - x) __len = width - x;
    if(x < 0) { __len += x; x = 0; }
    if(__len && x + __len <= width) {
	unsigned char end,st;
	st = x;
	end = (__len + x);
	attr = browser_cset.hlight;
	memset(&attrs[st],attr,end-st);
    }
    out.write(minx+1,y+1,chars,attrs,width);
}

void Search::read_ini(Ini_Profile& ini) {
    std::string stmp=bctx.read_profile_string(ini,"Beye","Search","String","");
    ::strcpy((char*)search_buff,stmp.c_str());
    search_len = stmp.length();
    stmp=bctx.read_profile_string(ini,"Beye","Search","Case","off");
    beyeSearchFlg=stricmp(stmp.c_str(),"on") == 0 ? Search::Case_Sens : Search::None;
    stmp=bctx.read_profile_string(ini,"Beye","Search","Word","off");
    if(stricmp(stmp.c_str(),"on") == 0) beyeSearchFlg |= Search::Word_Only;
    stmp=bctx.read_profile_string(ini,"Beye","Search","Backward","off");
    if(stricmp(stmp.c_str(),"on") == 0) beyeSearchFlg |= Search::Reverse;
    stmp=bctx.read_profile_string(ini,"Beye","Search","Template","off");
    if(stricmp(stmp.c_str(),"on") == 0) beyeSearchFlg |= Search::Wild_Cards;
    stmp=bctx.read_profile_string(ini,"Beye","Search","UsePlugin","off");
    if(stricmp(stmp.c_str(),"on") == 0) beyeSearchFlg |= Search::Plugins;
    stmp=bctx.read_profile_string(ini,"Beye","Search","AsHex","off");
    if(stricmp(stmp.c_str(),"on") == 0) beyeSearchFlg |= Search::As_Hex;
}
void Search::save_ini(Ini_Profile& ini) {
    search_buff[search_len] = '\0';
    bctx.write_profile_string(ini,"Beye","Search","String",(char *)search_buff);
    bctx.write_profile_string(ini,"Beye","Search","Case",beyeSearchFlg & Search::Case_Sens ? "on" : "off");
    bctx.write_profile_string(ini,"Beye","Search","Word",beyeSearchFlg & Search::Word_Only ? "on" : "off");
    bctx.write_profile_string(ini,"Beye","Search","Backward",beyeSearchFlg & Search::Reverse ? "on" : "off");
    bctx.write_profile_string(ini,"Beye","Search","Template",beyeSearchFlg & Search::Wild_Cards ? "on" : "off");
    bctx.write_profile_string(ini,"Beye","Search","UsePlugin",beyeSearchFlg & Search::Plugins ? "on" : "off");
    bctx.write_profile_string(ini,"Beye","Search","AsHex",beyeSearchFlg & Search::As_Hex ? "on" : "off");
}

} // namespace	usr
