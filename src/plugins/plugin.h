#ifndef __PLUGIN_HPP_INCLUDED
#define __PLUGIN_HPP_INCLUDED 1
#include <vector>
#include <exception>

#include "libbeye/libbeye.h"
#include "beyeutil.h"

namespace	usr {
    class BeyeContext;
    class TWindow;
    class Search;
    class CodeGuider;
    class Ini_Profile;
    class DisMode;
    class Bin_Format;
    class udn;
    struct Symbol_Info;
    struct Object_Info;

    class bad_format_exception : public std::exception {
	public:
	    bad_format_exception() throw();
	    virtual ~bad_format_exception() throw();

	    virtual const char*	what() const throw();
    };

    struct plugin_position {
	__filesize_t	lastbyte;
	int		textshift;
    };

    class Plugin : public Opaque {
	public:
	    static const __filesize_t Bad_Address = __filesize_t(-1);
	    enum e_flag {
		None		=0x0000,
		Text		=0x0001,
		Disasm		=0x0002,
		UseCodeGuide	=0x0004,
		Has_SearchEngine=0x0008,
		Has_ConvertCP	=0x0010
	    };

	    Plugin(BeyeContext&,const Bin_Format&,binary_stream& h,TWindow& main_wnd,CodeGuider& code_guider,udn& _udn,Search& s) { UNUSED(h); UNUSED(main_wnd); UNUSED(code_guider); UNUSED(_udn); UNUSED(s); }
	    virtual ~Plugin() {}

	    virtual const char*		prompt(unsigned idx) const = 0;	/**< on Ctrl-Fx selection */
	    virtual bool		action_F1() { return false; }
	    virtual bool		action_F2() { return false; }
	    virtual bool		action_F3() { return false; }
	    virtual bool		action_F4() { return false; }
	    virtual bool		action_F5() { return false; }
	    virtual bool		action_F6() { return false; }
	    virtual bool		action_F7() { return false; }
	    virtual bool		action_F8() { return false; }
	    virtual bool		action_F9() { return false; }
	    virtual bool		action_F10() { return false; }

	    virtual bool		detect() = 0;			/**< detects possibility to assign this mode as default mode for openned file. */
	    virtual e_flag		flags() const = 0;
			/** Paints the file on the screen.
			  * @param keycode   indicates keyboard code which caused repainting
			  * @param textshift indicates shift of text. Useful only for text mode.
			  * return           end position of plugin
			**/
	    virtual plugin_position	paint(unsigned keycode,unsigned textshift) = 0;

	    virtual unsigned		get_symbol_size() const = 0;	/**< Returns symbol size in bytes for selected NLS codepage */
	    virtual unsigned		get_max_symbol_size() const { return get_symbol_size(); }
	    virtual unsigned		get_max_line_length() const = 0;
	    virtual const char*		misckey_name() const = 0;	/**< F4 key name */
	    virtual void		misckey_action() = 0;		/**< F4 action */
	    virtual unsigned long	prev_page_size() const = 0;	/**< Get previous page size */
	    virtual unsigned long	curr_page_size() const = 0;	/**< Get current page size */
	    virtual unsigned long	prev_line_width() const = 0;	/**< Get previous line width */
	    virtual unsigned long	curr_line_width() const = 0;	/**< Get current line width */
	    virtual void		help() const = 0;		/**< display help about mode */
	    virtual void		read_ini(Ini_Profile& ) = 0;	/**< reads beye.ini file if need */
	    virtual void		save_ini(Ini_Profile& ) = 0;	/**< writes to beye.ini if need */
			/** Converts buffer with using selected NLS as xlat table.
			  * @param str      string to be converted
			  * @param len      length of string
			  * @param use_fs_nls specifies usage of nls of file system but not screen.
			  * @return         new size of blocks after conversation
			 **/
	    virtual unsigned		convert_cp(char *str,unsigned len, bool use_fs_nls) { UNUSED(str); UNUSED(len); UNUSED(use_fs_nls); return 0; }
			/** Performs search in plugin's output
			  * @param pwnd      indicates handle of Percent window with progress indicator
			  * @param start     indicates start offset within file where search must be performed
			  * @param slen      on output contains length of found sequence
			  * @param flags     indicates flags (SF_* family) of search.
			  * @param is_continue indicates initialization of search
			  *                  If set then search should be continued
			  * @param is_found  on output must contain true if result is really found
			  * @return          offset of found sequence or ULONG(LONG)_MAX if not found
			**/
	    virtual __filesize_t	search_engine(TWindow *pwnd, __filesize_t start, __filesize_t *slen, unsigned flg, bool is_continue, bool *is_found) { UNUSED(pwnd); UNUSED(start); UNUSED(slen); UNUSED(flg); UNUSED(is_continue); UNUSED(is_found); return 0; }
    };
    inline Plugin::e_flag operator~(Plugin::e_flag a) { return static_cast<Plugin::e_flag>(~static_cast<unsigned>(a)); }
    inline Plugin::e_flag operator|(Plugin::e_flag a, Plugin::e_flag b) { return static_cast<Plugin::e_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline Plugin::e_flag operator&(Plugin::e_flag a, Plugin::e_flag b) { return static_cast<Plugin::e_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline Plugin::e_flag operator^(Plugin::e_flag a, Plugin::e_flag b) { return static_cast<Plugin::e_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline Plugin::e_flag operator|=(Plugin::e_flag& a, Plugin::e_flag b) { return (a=static_cast<Plugin::e_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline Plugin::e_flag operator&=(Plugin::e_flag& a, Plugin::e_flag b) { return (a=static_cast<Plugin::e_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline Plugin::e_flag operator^=(Plugin::e_flag& a, Plugin::e_flag b) { return (a=static_cast<Plugin::e_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }

    struct Plugin_Info {
	const char* name;
	Plugin* (*query_interface)(BeyeContext&,const Bin_Format&,binary_stream&,TWindow&,CodeGuider&,udn&,Search&);
    };

    class Binary_Parser;
    struct Binary_Parser_Info;
    class Bin_Format : public Opaque {
	public:
	    enum bind_type {
		Normal	=0x0000, /**< Append references in short form if it really present in binary */
		Use_Type=0x0001, /**< Append references in detail form if it really present in binary */
		Try_Label=0x0002, /**< Append references in short form even if it not present in binary (smart method) */
		Save_Virt=0x0004, /**< Notifies plugin about preserving of virtual address, if binding is local */
		Try_Pic	=0x0008  /**< Append references in short form assuming that shift is offset in .GOT table where references are binded */
	    };

	    /** List of DisAssembler Bitness */
	    enum bitness {
		Use16	=0,
		Use32	=1,
		Use64	=2,
		Use128	=3,
		Use256	=4,
		Auto	=0xFFFFU /**< never must return! Only for selection */
	    };

	    /** List of DisAssembler Endian */
	    enum endian {
		Little	=0,
		Big	=1
	    };

	    Bin_Format(BeyeContext&,CodeGuider& parent,udn& _udn);
	    virtual ~Bin_Format();

	    virtual void		detect_format(binary_stream& handle);

	    virtual const char*		name() const;

	    virtual const char*		prompt(unsigned idx) const;	/**< on Alt-Fx selection */
	    virtual __filesize_t	action_F1()  const;
	    virtual __filesize_t	action_F2()  const;
	    virtual __filesize_t	action_F3()  const;
	    virtual __filesize_t	action_F4()  const;
	    virtual __filesize_t	action_F5()  const;
	    virtual __filesize_t	action_F6()  const;
	    virtual __filesize_t	action_F7()  const;
	    virtual __filesize_t	action_F8()  const;
	    virtual __filesize_t	action_F9()  const;
	    virtual __filesize_t	action_F10() const;

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,bind_type flg,int codelen,__filesize_t r_shift) const;

	    virtual int			query_platform() const;
	    virtual bitness		query_bitness(__filesize_t off) const;
	    virtual endian		query_endian(__filesize_t off) const;

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
	    virtual bool		address_resolving(std::string& str,__filesize_t off) const;

			 /** Converts virtual address to physical (means file offset).
			   * @param va       indicates virtual address to be converted
			   * @return         Bad_Address if operation is meaningless
			  **/
	    virtual __filesize_t	va2pa(__filesize_t va) const;
			 /** Converts physical address to virtual.
			   * @param pa       indicates physical address to be converted
			   * @note           seg pointer can be NULL
			   * @return         Bad_Address if operation is meaningless
			  **/
	    virtual __filesize_t	pa2va(__filesize_t pa) const;

	    /*-- Below placed functions for 'put structures' method of save as dialog --*/

			 /** Fills the string with public symbol
			   * @param pa        indicates physical offset within file
			   * @param as_prev   indicates direction of symbol searching from given physical offset
			  **/
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev) const;

			 /** Determines attributes of object at given physical file address.
			   * @param pa        indicates physical file offset of object
			   * @remark          For example: if exe-format - new
			   *                  exe i.e. contains MZ and NEW
			   *                  header and given file offset
			   *                  points to old exe stub then start
			   *                  = 0, end = begin of first data or
			   *                  code object).
			  **/
	    virtual Object_Info		get_object_attribute(__filesize_t pa) const;
	private:
	    std::vector<const Binary_Parser_Info*>	formats;
	    size_t			active_format;
	    Binary_Parser*		detectedFormat;
	    BeyeContext&		bctx;
	    CodeGuider&			parent;
	    udn&			_udn;
    };
    inline Bin_Format::bind_type operator~(Bin_Format::bind_type a) { return static_cast<Bin_Format::bind_type>(~static_cast<unsigned>(a)); }
    inline Bin_Format::bind_type operator|(Bin_Format::bind_type a, Bin_Format::bind_type b) { return static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline Bin_Format::bind_type operator&(Bin_Format::bind_type a, Bin_Format::bind_type b) { return static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline Bin_Format::bind_type operator^(Bin_Format::bind_type a, Bin_Format::bind_type b) { return static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline Bin_Format::bind_type operator|=(Bin_Format::bind_type& a, Bin_Format::bind_type b) { return (a=static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline Bin_Format::bind_type operator&=(Bin_Format::bind_type& a, Bin_Format::bind_type b) { return (a=static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline Bin_Format::bind_type operator^=(Bin_Format::bind_type& a, Bin_Format::bind_type b) { return (a=static_cast<Bin_Format::bind_type>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }

    struct Symbol_Info {
	Symbol_Info();
	/** Public symbols classes */
	enum symbol_class {
	    Local=0, /**< means: present as entry but not exported */
	    Global=1  /**< means: exported entry point */
	};
	std::string	name;	// name of public symbol
	symbol_class	_class;	// class of symbol
	__filesize_t	pa;	// physical address of public symbol (Bad_Address if no symbol)
    };

    struct Object_Info {
	/** object classes */
	enum obj_class {
	    Code=0, /**< for code objects */
	    Data=1, /**< for any data objects */
	    NoObject=-1 /**< for non objects (means: relocs, resources, tables ...) */
	};
	unsigned	number;	// logical number of object (0 if it's no object).
	std::string	name;	// object name
	__filesize_t	start;	// file offset of object's start
	__filesize_t	end;	// file offset of object's end
	obj_class	_class;	// _class of object
	Bin_Format::bitness	bitness;// bitness of object.
    };
} //namespace	usr
#endif
