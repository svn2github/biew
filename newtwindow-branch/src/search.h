/**
 * @namespace	usr
 * @file        search.h
 * @brief       This file contains search function prototypes.
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
#ifndef __SEARCH__H
#define __SEARCH__H

namespace	usr {
    enum {
	MAX_SEARCH_SIZE=76
    };

    typedef unsigned tRelCoord;
    class TWindow;
    class PercentWindow;
    class BeyeContext;
    class Ini_Profile;
    class Search : public Opaque {
	public:
	    enum dialog_flags {
		Simple=0x0000,   /**< indicates simple search dialog to be displayed */
		All_Features=0x0001    /**< indicates fully featured search dialog to be displayed */
	    };
	    enum search_flags {
		None		=0x0000,   /**< indicates no flags */
		Case_Sens	=0x0001,   /**< indicates case sensitivity search engine */
		Word_Only	=0x0002,   /**< indicates word only to be found */
		Reverse		=0x0004,   /**< indicates reverse search */
		Wild_Cards	=0x0008,   /**< indicates wildcard can be used */
		Plugins		=0x0010,   /**< indicates using plugin's output */
		As_Hex		=0x0020    /**< indicates hex mode of input sequence */
	    };
	    enum hl_search {
		HL_Normal=0x0000,
		HL_Use_Double_Width=0x0001
	    };
	    Search(BeyeContext&);
	    virtual ~Search();

	    virtual bool	dialog(dialog_flags dlg_flags,
					char *searchbuff,
					unsigned char* searchlen,
					search_flags& search_flags) const;

		   /** Performs seacrh of given sequence in the string
		     * @param str          indicates string where search must be performed
		     * @param str_len      indicates length string where search must be performed
		     * @param sbuff        indicates searching sequence
		     * @param sbuflen      indicates length of sequence to be found
		     * @param cache        indicates Boyer-Moore cache
		     * @param flags        indicates SF_* flags
		     * @return             address of first occurence of
					   of searching sequence in the
					   string or NULL if not found.
		    **/
	    virtual char*	strFind(const char *str, unsigned str_len,
					const any_t* sbuff, unsigned sbuflen,
					const int *cache, search_flags flags);

		   /** Fills cache for Boyer-Moore search
		     * @param cache        indicates cache to be filled
					   Should have size of 256 elements.
		     * @param pattern      indicates search pattern
		     * @param pattern_len  indicates length of search pattern
		     * @param case_sens    indicates case sensitivity of search
		     * @return             none
		    **/
	    virtual void 	fillBoyerMooreCache(int *cache,
					const char *pattern,
					unsigned pattern_len,
					bool case_sens) const;
		   /** Main search routine
		     * @param is_continue  indicates initialization of search
					   If set then search should be continued
					   search dialog will displayed otherwise
		     * @return             new offset on successful search and
					   current offset otherwise
		    **/
	    virtual __filesize_t	run( bool is_continue );

	    virtual int			is_inline(__filesize_t cp,int width) const;

	    virtual void		hilight(TWindow& out,__filesize_t cfp,tRelCoord minx,
						tRelCoord maxx,tRelCoord y,const char* buff,hl_search flags) const;

	    virtual void		set_flags(search_flags);
	    virtual search_flags	get_flags() const;
	    virtual const unsigned char*buff() const { return search_buff; }
	    virtual unsigned char	length() const { return search_len; }

	    virtual __filesize_t	found_start() const { return FoundTextSt; }
	    virtual __filesize_t	found_end() const { return FoundTextEnd; }
	    virtual void		set_found(__filesize_t start,__filesize_t end);
	    virtual void		reset();

	    virtual void		read_ini(Ini_Profile&);	/**< reads beye.ini file if need */
	    virtual void		save_ini(Ini_Profile&);	/**< writes to beye.ini if need */
	private:
	    __filesize_t	lfind(const char *sfrom,
					unsigned slen,
					unsigned flags,
					__filesize_t start,
					const int *scache,
					const char *pattern,
					unsigned pattern_size,
					search_flags beyeFlg);
	    __filesize_t	adv_find(const char *sfrom,
					unsigned sfromlen,
					__filesize_t start,
					__filesize_t *slen,
					const int *scache,
					const char *pattern,
					unsigned pattern_size,
					search_flags beyeFlg);
	    __filesize_t	adv_find(__filesize_t start,__filesize_t* slen);

	    void		SearchPaint(TWindow& wdlg,dialog_flags flags,search_flags sf_flags) const;
	    void		SearchUpdate(TWindow& wdlg,dialog_flags _flags,search_flags sf_flags) const;

	    unsigned char	search_buff[MAX_SEARCH_SIZE];
	    unsigned char	search_len;
	    search_flags	beyeSearchFlg;

	    __filesize_t	FoundTextSt; /**< Indicates start of found text */
	    __filesize_t	FoundTextEnd;/**< Indicates end of found text */
	    bool		__found;

	    PercentWindow*	prcntswnd;
	    BeyeContext&	bctx;
    };
    inline Search::search_flags operator~(Search::search_flags a) { return static_cast<Search::search_flags>(~static_cast<unsigned>(a)); }
    inline Search::search_flags operator|(Search::search_flags a, Search::search_flags b) { return static_cast<Search::search_flags>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline Search::search_flags operator&(Search::search_flags a, Search::search_flags b) { return static_cast<Search::search_flags>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline Search::search_flags operator^(Search::search_flags a, Search::search_flags b) { return static_cast<Search::search_flags>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline Search::search_flags operator|=(Search::search_flags& a, Search::search_flags b) { return (a=static_cast<Search::search_flags>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline Search::search_flags operator&=(Search::search_flags& a, Search::search_flags b) { return (a=static_cast<Search::search_flags>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline Search::search_flags operator^=(Search::search_flags& a, Search::search_flags b) { return (a=static_cast<Search::search_flags>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace	usr
#endif
