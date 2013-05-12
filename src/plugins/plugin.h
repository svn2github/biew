#ifndef __PLUGIN_HPP_INCLUDED
#define __PLUGIN_HPP_INCLUDED 1
#include <vector>

#include "libbeye/libbeye.h"
#include "bmfile.h"
#include "beyeutil.h"
#include "reg_form.h"

namespace	usr {
    class TWindow;
    class CodeGuider;
    class Ini_Profile;
    class DisMode;
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

	    Plugin(TWindow& main_wnd,CodeGuider& code_guider) { UNUSED(main_wnd); UNUSED(code_guider); }
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
			  * return           new shift of text
			**/
	    virtual unsigned		paint(unsigned keycode,unsigned textshift) = 0;

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
	Plugin* (*query_interface)(TWindow&,CodeGuider&);
    };

    class Binary_Parser : public Opaque {
	public:
	    Binary_Parser(CodeGuider&) {}
	    virtual ~Binary_Parser() {}

	    virtual const char*		prompt(unsigned idx) const = 0;   /**< on ALT-Fx selection */

	    virtual __filesize_t	action_F1() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F2() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F3() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F4() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F5() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F6() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F7() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F8() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F9() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F10(){ return Plugin::Bad_Address; }

	    virtual __filesize_t	show_header() { return Plugin::Bad_Address; }  /**< if not an MZ style format */
			/**
			  Binss disassembler reference to string.
			   * @param str          string buffer for append to
			   * @param shift        physical address of field, that required of binding
			   * @param flags        see above
			   * @param codelen      length of field, that required binding
			   * @param r_shift      used only if APPREF_TRY_LABEL mode is set, contains real value of field, that required binding
			   * @return             true if reference was appended
			*/
	    virtual bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift) { UNUSED(_parent); UNUSED(str); UNUSED(shift); UNUSED(flg); UNUSED(codelen); UNUSED(r_shift); return false; }

			 /** Returns CPU platform, that required by format.
			   * @note           Full list of platform please see in
			   *                 plugins/disasm.h file. If this
			   *                 function return -1 then platform is
			   *                 undefined.
			  **/
	    virtual int			query_platform() const = 0;

			 /** Returns DAB_XXX. Quick version for disassembler */
	    virtual int			query_bitness(__filesize_t) const { return DAB_USE16; }

			 /** Returns DAE_XXX. */
	    virtual int			query_endian(__filesize_t) const { return DAE_LITTLE; }

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
	    virtual bool		address_resolving(char *,__filesize_t) { return false; }

			 /** Converts virtual address to physical (means file offset).
			   * @param va       indicates virtual address to be converted
			   * @return         0 if operation meaningless
			  **/
	    virtual __filesize_t	va2pa(__filesize_t) { return Plugin::Bad_Address; }

			 /** Converts physical address to virtual.
			   * @param pa       indicates physical address to be converted
			   * @note           seg pointer can be NULL
			  **/
	    virtual __filesize_t	pa2va(__filesize_t) { return Plugin::Bad_Address; }

			 /** Fills the string with public symbol
			   * @param str       pointer to the string to be filled
			   * @param cb_str    indicates maximal length of string
			   * @param _class    pointer to the memory where can be stored class of symbol (See SC_* conatnts)
			   * @param pa        indicates physical offset within file
			   * @param as_prev   indicates direction of symbol searching from given physical offset
			   * @return          0 - if no symbol name available
			   *                  in given direction (as_prev)
			   *                  physical address of public symbol
			   *                  which is found in given direction
			  **/
	    virtual __filesize_t	get_public_symbol(char* str,unsigned cb_str,unsigned *_class,
							    __filesize_t pa,bool as_prev) { UNUSED(str); UNUSED(cb_str); UNUSED(_class); UNUSED(pa); UNUSED(as_prev); return Plugin::Bad_Address; }

			 /** Determines attributes of object at given physical file address.
			   * @param pa        indicates physical file offset of object
			   * @param name      pointer to the string which is to be filled with object name
			   * @param cb_name   indicates maximal length of string
			   * @param start     pointer to the memory where must be stored start of given object, as file offset.
			   * @param end       pointer to the memory where must be stored end of given object, as file offset.
			   * @param _class    pointer to the memory where must be stored _class of object (See OC_* constants).
			   * @param bitness   pointer to the memory where must be stored bitness of object (See DAB_* constants).
			   * @return          logical number of object or 0 if at given offset is no object.
			   * @note            all arguments exclude name of object
			   *                  must be filled.
			   * @remark          For example: if exe-format - new
			   *                  exe i.e. contains MZ and NEW
			   *                  header and given file offset
			   *                  points to old exe stub then start
			   *                  = 0, end = begin of first data or
			   *                  code object).
			  **/
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
							__filesize_t *start,__filesize_t *end,int *_class,int *bitness) { UNUSED(pa); UNUSED(name); UNUSED(cb_name); UNUSED(start); UNUSED(end); UNUSED(_class); UNUSED(bitness); return 0; }
    };

    struct Binary_Parser_Info {
	const char*	name;
	bool		(*probe)(); /**< Checks format */
	Binary_Parser* (*query_interface)(CodeGuider&);
    };

    class Bin_Format : public Opaque {
	public:
	    Bin_Format(CodeGuider& parent);
	    virtual ~Bin_Format();

	    void		detect_format();

	    const char*		name() const { return formats[active_format]->name; }

	    const char*		prompt(unsigned idx) const { return detectedFormat->prompt(idx); }	/**< on Alt-Fx selection */
	    __filesize_t	action_F1()  const { return detectedFormat->action_F1(); }
	    __filesize_t	action_F2()  const { return detectedFormat->action_F2(); }
	    __filesize_t	action_F3()  const { return detectedFormat->action_F3(); }
	    __filesize_t	action_F4()  const { return detectedFormat->action_F4(); }
	    __filesize_t	action_F5()  const { return detectedFormat->action_F5(); }
	    __filesize_t	action_F6()  const { return detectedFormat->action_F6(); }
	    __filesize_t	action_F7()  const { return detectedFormat->action_F7(); }
	    __filesize_t	action_F8()  const { return detectedFormat->action_F8(); }
	    __filesize_t	action_F9()  const { return detectedFormat->action_F9(); }
	    __filesize_t	action_F10() const { return detectedFormat->action_F10(); }

	    __filesize_t	show_header() const { return detectedFormat->show_header(); }
	    bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift) const { return detectedFormat->bind(_parent,str,shift,flg,codelen,r_shift); }

	    int			query_platform() const;

			 /** Returns DAB_XXX. Quick version for disassembler */
	    int			query_bitness(__filesize_t off) const;

			 /** Returns DAE_XXX. */
	    int			query_endian(__filesize_t off) const;

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
	    bool		address_resolving(char * str,__filesize_t off) const { return detectedFormat->address_resolving(str,off); }

			 /** Converts virtual address to physical (means file offset).
			   * @param va       indicates virtual address to be converted
			   * @return         Bad_Address if operation is meaningless
			  **/
	    __filesize_t	va2pa(__filesize_t va) const {
						__filesize_t rc=detectedFormat->va2pa(va);
						if(!rc) rc=Plugin::Bad_Address;
						return rc;
					    }
			 /** Converts physical address to virtual.
			   * @param pa       indicates physical address to be converted
			   * @note           seg pointer can be NULL
			   * @return         Bad_Address if operation is meaningless
			  **/
	    __filesize_t	pa2va(__filesize_t pa) const {
						__filesize_t rc=detectedFormat->pa2va(pa);
						if(!rc) rc=Plugin::Bad_Address;
						return rc;
					    }

	    /*-- Below placed functions for 'put structures' method of save as dialog --*/

			 /** Fills the string with public symbol
			   * @param str       pointer to the string to be filled
			   * @param cb_str    indicates maximal length of string
			   * @param _class    pointer to the memory where can be stored class of symbol (See SC_* conatnts)
			   * @param pa        indicates physical offset within file
			   * @param as_prev   indicates direction of symbol searching from given physical offset
			   * @return          Bad_Address - if no symbol name available
			   *                  in given direction (as_prev)
			   *                  physical address of public symbol
			   *                  which is found in given direction
			  **/
	    __filesize_t	get_public_symbol(char *str,unsigned cb_str,unsigned *_class,__filesize_t pa,bool as_prev) const {
					    __filesize_t rc=detectedFormat->get_public_symbol(str,cb_str,_class,pa,as_prev);
					    if(!rc) rc = Plugin::Bad_Address;
					    return rc;
					}

			 /** Determines attributes of object at given physical file address.
			   * @param pa        indicates physical file offset of object
			   * @param name      pointer to the string which is to be filled with object name
			   * @param cb_name   indicates maximal length of string
			   * @param start     pointer to the memory where must be stored start of given object, as file offset.
			   * @param end       pointer to the memory where must be stored end of given object, as file offset.
			   * @param _class    pointer to the memory where must be stored _class of object (See OC_* constants).
			   * @param bitness   pointer to the memory where must be stored bitness of object (See DAB_* constants).
			   * @return          logical number of object or Bad_Address if at given offset is no object.
			   * @note            all arguments exclude name of object
			   *                  must be filled.
			   * @remark          For example: if exe-format - new
			   *                  exe i.e. contains MZ and NEW
			   *                  header and given file offset
			   *                  points to old exe stub then start
			   *                  = 0, end = begin of first data or
			   *                  code object).
			  **/
	    unsigned		get_object_attribute(__filesize_t pa,char *_name,unsigned cb_name,__filesize_t *start,__filesize_t *end,int *_class,int *bitness) const { return detectedFormat->get_object_attribute(pa,_name,cb_name,start,end,_class,bitness); }
	private:
	    std::vector<const Binary_Parser_Info*>	formats;
	    size_t			active_format;
	    Binary_Parser*		detectedFormat;
	    CodeGuider&			parent;
    };
} //namespace	usr
#endif
