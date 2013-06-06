#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include <algorithm>
#include <functional>
#include <string>

#include <errno.h>

#include "beye.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "colorset.h"
#include "listbox.h"
#include "tstrings.h"
#include "libbeye/twindow.h"
#include "libbeye/osdep/tconsole.h"

namespace	usr {
TWindow* ListBox::_CrtMnuWindowDD(const std::string& title,tAbsCoord x1, tAbsCoord y1, tAbsCoord x2,tAbsCoord y2) const
{
    TWindow *win;
    TWindow::twc_flag tflags = TWindow::Flag_Has_Frame|TWindow::Flag_NLS;
    win = new(zeromem) TWindow(x1,y1,x2-x1+2,y2-y1+2,tflags);
    if(!x1 && !y1) win->into_center();
    win->set_color(menu_cset.main);
    win->clear();
    win->set_frame(TWindow::DOUBLE_FRAME,menu_cset.border);
    if(!title.empty()) win->set_title(title,TWindow::TMode_Center,menu_cset.title);
    win->show();
    return win;
}

void ListBox::PaintLine(TWindow& w,unsigned i,const std::string& name,
					unsigned width,unsigned mord_width,
					bool isOrdinal,
					bool useAcc,bool is_hl) const
{
    size_t namelen;
    uint8_t buffer[__TVIO_MAXSCREENWIDTH + 1];
    ::memset(buffer,TWC_DEF_FILLER,sizeof(buffer));
    buffer[__TVIO_MAXSCREENWIDTH] = 0; /* [dBorca] play it safe for strchr below */
    namelen = name.length();
    if(isOrdinal) {
	size_t endptr;
	endptr = name.rfind(Ord_Delimiter);
	if(endptr!=std::string::npos) {
	    unsigned len, rlen;
	    // write name
	    len = endptr;
	    rlen = len;
	    if(len > width - mord_width-1) rlen = width - mord_width-3;
	    ::memcpy(buffer,name.c_str(),rlen);
	    if(len > rlen) memcpy(buffer+rlen,"..", 2);           // using 2 dots now -XF
	    // write ordinal. it's left aligned now -XF
	    buffer[width - mord_width - 1] = '@';
	    len = rlen = namelen - (len+1);
	    if(rlen > mord_width) rlen = mord_width - 2;
	    ::memcpy(&buffer[width - mord_width], &name.c_str()[endptr+1], rlen);
	    if(len > rlen) memcpy(buffer+width-mord_width+rlen,"..", 2);
	}
    } else if(!name.empty()) ::memcpy(buffer,name.c_str(),std::min(namelen,size_t(width)));
    if(useAcc) {
	const uint8_t *st,*ends,*ptr;
	char ch;
	w.goto_xy(3,i+1);
	st = buffer;
	ends = buffer+width;
	while(1) {
	    ptr = (const uint8_t*)strchr((char*)st,'~');
	    if(ptr) {
		unsigned outlen;
		outlen = ptr-st;
		w.write(w.where_x(),w.where_y(),st,outlen);
		w.goto_xy(w.where_x()+outlen,w.where_y());
		st = ptr;
		ch = *(++st);
		if(ch != '~') {
		    ColorAttr ca;
		    ca = w.get_color();
		    w.set_color(is_hl ? menu_cset.hotkey.focused : menu_cset.hotkey.active);
		    w.putch(ch);
		    w.set_color(ca);
		}
		st++;
	    } else {
		w.write(w.where_x(),w.where_y(),st,(unsigned)(ends-st));
		break;
	    }
	}
    }
    else  w.write(3,i+1,buffer,width);
}

void ListBox::Paint(TWindow& win,const std::vector<std::string>& names,
		    unsigned start,
		    unsigned height,unsigned width,
		    unsigned mord_width,
		    bool isOrdinal,bool useAcc,
		    unsigned cursor) const
{
    unsigned i, pos = 0;
    size_t nlist=names.size();
    win.freeze();
    width -= 3;
    if (height>2 && height<nlist) pos = 1 + (start+cursor)*(height-2)/nlist;
    for(i = 0;i < height;i++) {
	win.set_color(menu_cset.main);
	win.goto_xy(1,i + 1);
	if (i == 0)
	    win.putch(start ? TWC_UP_ARROW : TWC_DEF_FILLER);
	else if(i == height-1)
	    win.putch(start + height < nlist ? TWC_DN_ARROW : TWC_DEF_FILLER);
	else if (i == pos)
	    win.putch(TWC_THUMB);
	else win.putch(TWC_DEF_FILLER);
	win.goto_xy(2,i + 1);
	win.putch(TWC_SV);
	win.set_color(menu_cset.item.active);
	PaintLine(win,i,names[i + start],width,mord_width,isOrdinal,useAcc,cursor == i);
    }
    win.refresh();
}

bool ListBox::_lb_searchtext(const char *str,const char *tmpl,unsigned slen,const int *cache, Search::search_flags flg) const
{
    return search.strFind(str, strlen(str), tmpl, slen, cache, flg) ? true : false;
}

template<class Base, class T>
struct adaptor_t {
    typedef bool(Base::*method_t)(const T&,const T&) const;
    adaptor_t(Base* b,method_t m):base(b),method(m) {}
    adaptor_t(const adaptor_t& copy):base(copy.base),method(copy.method) {}
    bool operator()(const T& c1,const T& c2) const { return (base->*method)(c1,c2); }

    Base* base;
    method_t method;
};

bool ListBox::list_compare(const std::string& s1,const std::string& s2) const
{
    bool ret;
    if(byNam)  ret = s1 < s2;
    else {
	size_t o1,o2;

	o1 = s1.rfind(Ord_Delimiter);
	o2 = s2.rfind(Ord_Delimiter);
	if(o1!=std::string::npos && o2!=std::string::npos) {
	    unsigned long ord1,ord2;
	    std::string buff1, buff2;
	    buff1=s1.substr(o1+1,6);
	    buff2=s2.substr(o2+1,6);
	    ord1 = atol(buff1.c_str());
	    ord2 = atol(buff2.c_str());
	    ret = ord1<ord2;
	}
	else  ret = s1<s2;
    }
    return ret;
}

int ListBox::run(std::vector<std::string>& names,const std::string& title,flags assel,unsigned defsel)
{
    TWindow * wlist;
    char *acctable = 0;
    unsigned i,j,nlist,width,height,mwidth = title.length();
    unsigned mordstr_width, mord_width;
    int ret,start,ostart,cursor,ocursor,scursor;
    bool isOrdinal,sf;
    nlist=names.size();
    if(!nlist) return -1;
    isOrdinal = true;
    scursor = -1;
    i = 0;
    if((assel & UseAcc) == UseAcc) {
	acctable = new char [names.size()];
	memset(acctable,0,nlist*sizeof(char));

	for(i = 0;i < nlist;i++) {
	    unsigned len;
	    len = names[i].length();
	    for(j = 0;j < len;j++) {
		if(names[i][j] == '~' && names[i][j+1] != '~') {
		    acctable[i] = toupper(names[i][j+1]);
		    break;
		}
	    }
	}
    }
    if(!names[0].empty()) if(names[0].rfind(Ord_Delimiter)==std::string::npos) isOrdinal = false;
    mordstr_width = mord_width = 0;
    if(!isOrdinal) for(i = 0;i < nlist;i++) {
	j = !names[i].empty()?names[i].length():0;
	if(j > mwidth) mwidth = j;
    } else {
	const char *ord_delimiter;
	for(i = 0;i < nlist;i++) {
	    ord_delimiter = !names[i].empty()?strrchr(names[i].c_str(), Ord_Delimiter):NULL;
	    if(ord_delimiter) {
		j = ord_delimiter - names[i].c_str();
		if(j > mordstr_width) mordstr_width = j;
		j = &names[i][names[i].length()] - ord_delimiter;
		if(j > mord_width) mord_width = j;
	    }
	}
	// name now has higher priority than ordinal -XF
	if(mordstr_width > (unsigned)(bctx.tconsole().vio_width()-10))
	    mordstr_width = (unsigned)(bctx.tconsole().vio_width()-10);
	if(mord_width > (unsigned)(bctx.tconsole().vio_width()-4)-mordstr_width-1)
	    mord_width = (unsigned)(bctx.tconsole().vio_width()-4)-mordstr_width-1;
	    mwidth = mordstr_width+mord_width+1;
    }
    mwidth += 4;
    if(mwidth > (unsigned)(bctx.tconsole().vio_width()-1)) mwidth = bctx.tconsole().vio_width()-1;         // maximal width increased to bctx.tconsole().vio_width()-1 -XF
    height = nlist < (unsigned)(bctx.tconsole().vio_height() - 4) ? nlist : bctx.tconsole().vio_height() - 4;
    wlist = _CrtMnuWindowDD(title,0,0,mwidth-1,height);
    if((assel & Selective) == Selective) wlist->set_footer(" [ENTER] - Go ",TWindow::TMode_Right,dialog_cset.selfooter);
restart:
    ostart = start = cursor = ocursor = 0;
    if(defsel != std::numeric_limits<size_t>::max() && defsel < nlist) {
	cursor = defsel;
	while((unsigned)cursor > height) { start += height; cursor -= height; }
	ostart = start;
	ocursor = cursor;
    }
    Paint(*wlist,names,(unsigned)start,height,mwidth,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,(unsigned)cursor);
    width = mwidth - 3;
    if((assel & Selective) == Selective) {
	wlist->set_color(menu_cset.item.focused);
	PaintLine(*wlist,(unsigned)cursor,names[cursor + start],width,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,true);
    }
    sf = false;
    for(;;) {
	unsigned ch;
	ch = GetEvent(isOrdinal ? drawOrdListPrompt : (assel & Sortable) ? drawListPrompt : drawSearchListPrompt,NULL,wlist);
	if(ch == KE_ESCAPE || ch == KE_F(10)) { ret = -1; break; }
	if(ch == KE_ENTER)                    { ret = start + cursor; break; }
	if(ch!=KE_F(7) && ch!=KE_SHIFT_F(7))  scursor = -1;
	switch(ch) {
	    case KE_F(2):
	    case KE_F(3):
		if(isOrdinal || (assel & Sortable)) {
		    byNam = ch == KE_F(2);
		    if(!isOrdinal && !byNam) break;
		    std::sort(names.begin(),names.end(),adaptor_t<ListBox,std::string>(this,&ListBox::list_compare));
		    goto restart;
	        }
	        break;
	    case KE_F(4): { /** save content to disk */
		char ofname[256];
		ofname[0] = 0;
		if(GetStringDlg(ofname," Save info to file : "," [ENTER] - Proceed ",NAME_MSG)) {
		    std::ofstream out;
		    out.open(ofname,std::ios_base::out);
		    if(out.is_open()) {
			strncpy(ofname,title.c_str(),sizeof(ofname));
			ofname[sizeof(ofname)-1] = '\0';
			if(GetStringDlg(ofname," User comments : "," [ENTER] - Proceed "," Description : ")) out<<ofname<<std::endl<<std::endl;
			for(i = 0;i < nlist;i++) {
			    char *p;
			    char stmp[4096];
			    stmp[0]='\0';
			    if(!names[i].empty()) strcpy(stmp,names[i].c_str());
			    p = !names[i].empty()?strchr(stmp,Ord_Delimiter):NULL;
			    if(p) {
				*p = 0;
				out<<stmp;
				for(j = p - stmp;j < 50;j++) out<<" ";
				out<<" @"<<(p+1);
				if(p) *p = Ord_Delimiter;
			    }
			    else out<<names[i];
			    out<<std::endl;
			}
			out.close();
		    }
		    else bctx.errnoMessageBox(WRITE_FAIL,"",errno);
		}
	    }
	    break;
	    case KE_F(7): /** perform binary search in list */
	    case KE_SHIFT_F(7): {
		if (!(ch==KE_SHIFT_F(7) && searchlen) &&
		    !search.dialog(Search::Simple,searchtxt,&searchlen,sflg)) break;

		int direct, cache[UCHAR_MAX+1];
		bool found;
		int ii,endsearch,startsearch;
		searchtxt[searchlen] = 0;
		endsearch = sflg & Search::Reverse ? -1 : (int)nlist;
		direct = sflg & Search::Reverse ? -1 : 1;
		startsearch = (assel & Selective) == Selective ?
				cursor + start :
				scursor != -1 ?
				scursor :
				start;
		if(startsearch > (int)(nlist-1)) startsearch = nlist-1;
		if(startsearch < 0) startsearch = 0;
		if((assel & Selective) == Selective || scursor != -1) sflg & Search::Reverse ? startsearch-- : startsearch++;
		found = false;
		search.fillBoyerMooreCache(cache, searchtxt, searchlen, sflg & Search::Case_Sens);
		for(ii = startsearch;ii != endsearch;ii+=direct) {
		    if(!names[ii].empty()) {
			if(_lb_searchtext(names[ii].c_str(),searchtxt,searchlen,cache,sflg)) {
			    start = scursor = ii;
			    if((unsigned)start > nlist - height) start = nlist - height;
			    ostart = start - 1;
			    if((assel & Selective) == Selective) cursor = scursor - start;
			    found = true;
			    break;
			}
		    }
		}
		if(!found) scursor = -1;
		if(scursor == -1) bctx.ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
	    }
	    break;
	    case KE_DOWNARROW : if((assel & Selective) == Selective) cursor ++; else start ++; break;
	    case KE_UPARROW   : if((assel & Selective) == Selective) cursor --; else start --; break;
	    case KE_PGDN   : start += height; break;
	    case KE_PGUP   : start -= height; break;
	    case KE_CTL_PGDN  : start = nlist - height; cursor = height; break;
	    case KE_CTL_PGUP  : start = cursor = 0; break;
	    default :
		/** Try accelerate choose */
		if((assel & UseAcc) == UseAcc) {
		    if((unsigned char)(ch & 0x00FF) > 31) {
			ch = toupper(ch & 0x00FF);
			for(i = 0;i < nlist;i++)
			    if(ch == (unsigned)acctable[i]) { ret = i; goto Done; }
		    }
		}
	}
	if((assel & Selective) == Selective) {
	    if(cursor < 0) { cursor = 0; start--; }
	    if((unsigned)cursor > height - 1) { cursor = height - 1; start++; }
	}
	if(start < 0) start = 0;
	if((unsigned)start > nlist - height) start = nlist - height;
	if(start != ostart) {
	    ostart = start;
	    Paint(*wlist,names,(unsigned)start,height,mwidth,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,(unsigned)cursor);
	    sf = true;
	}
	if((cursor != ocursor || sf) && (assel & Selective) == Selective) {
	    wlist->set_color(menu_cset.item.active);
	    PaintLine(*wlist,(unsigned)ocursor,names[ocursor + start],width,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,false);
	    wlist->set_color(menu_cset.item.focused);
	    PaintLine(*wlist,(unsigned)cursor,names[cursor + start],width,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,true);
	    ocursor = cursor;
	    sf = false;
	}
	if(scursor != -1) {
	    wlist->set_color(menu_cset.highlight);
	    if(scursor >= start && (unsigned)scursor < start + height)
		PaintLine(*wlist,(unsigned)(scursor - start),names[scursor],width,mord_width,isOrdinal,(assel & UseAcc) == UseAcc,true);
	}
    }
Done:
    delete wlist;
    if(acctable) delete acctable;
    return ret;
}

int ListBox::run(const char** names,unsigned nlist,const std::string& title,flags assel,unsigned defsel)
{
    std::vector<std::string> v;
    for(size_t i=0;i<nlist;i++) v.push_back(names[i]);
    return run(v,title,assel,defsel);
}

ListBox::ListBox(BeyeContext& bc)
	:bctx(bc)
	,search(*new(zeromem) Search(bc))
	,searchlen(0)
	,sflg(Search::None)
{
    searchtxt[0]='\0';
}
ListBox::~ListBox() { delete &search; }
} // namespace	usr
