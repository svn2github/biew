#ifndef __LISTBOX_HPP_INCLUDED
#define __LISTBOX_HPP_INCLUDED 1
#include <limits>
#include <vector>

#include <search.h>

namespace	usr {
    class BeyeContext;
    class TWindow;
    class ListBox : public Opaque {
	public:
	    enum flags {
		Simple		=0x00U,
		Selective	=0x01U,
		Sortable	=0x02U,
		UseAcc		=0x04U,
		Ord_Delimiter	=0x7FU
	    };
	    ListBox(BeyeContext&);
	    virtual ~ListBox();

	    virtual int		run(const char** names,unsigned nlist,const std::string& title,
				    flags acc,unsigned defsel=std::numeric_limits<size_t>::max());

	    virtual int		run(std::vector<std::string>& list,const std::string& title,
				    flags acc,unsigned defsel=std::numeric_limits<size_t>::max());
	private:
	    bool		list_compare(const std::string& s1,const std::string& s2) const;
	    bool		_lb_searchtext(const char *str,const char *tmpl,unsigned searchlen,const int *cache, Search::search_flags flg) const;
	    void		PaintLine(TWindow& w,unsigned i,const std::string& name,
					unsigned width,unsigned mord_width,
					bool isOrdinal,
					bool useAcc,bool is_hl) const;
	    void		Paint(TWindow& win,const std::vector<std::string>& names,
					unsigned start,
					unsigned height,unsigned width,
					unsigned mord_width,
					bool isOrdinal,bool useAcc,
					unsigned cursor) const;
	    TWindow*		_CrtMnuWindowDD(const std::string& title,tAbsCoord x1, tAbsCoord y1, tAbsCoord x2,tAbsCoord y2) const;

	    BeyeContext&	bctx;
	    Search&		search;
	    bool byNam;
    };
    inline ListBox::flags operator~(ListBox::flags a) { return static_cast<ListBox::flags>(~static_cast<unsigned>(a)); }
    inline ListBox::flags operator|(ListBox::flags a, ListBox::flags b) { return static_cast<ListBox::flags>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline ListBox::flags operator&(ListBox::flags a, ListBox::flags b) { return static_cast<ListBox::flags>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline ListBox::flags operator^(ListBox::flags a, ListBox::flags b) { return static_cast<ListBox::flags>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline ListBox::flags operator|=(ListBox::flags& a, ListBox::flags b) { return (a=static_cast<ListBox::flags>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline ListBox::flags operator&=(ListBox::flags& a, ListBox::flags b) { return (a=static_cast<ListBox::flags>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline ListBox::flags operator^=(ListBox::flags& a, ListBox::flags b) { return (a=static_cast<ListBox::flags>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace	usr
#endif
