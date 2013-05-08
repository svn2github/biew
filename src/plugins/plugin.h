#ifndef __PLUGIN_HPP_INCLUDED
#define __PLUGIN_HPP_INCLUDED 1
#include <vector>

#include "libbeye/libbeye.h"

namespace	usr {
    class TWindow;
    class CodeGuider;
    class Ini_Profile;
    class Plugin : public Opaque {
	public:
	    enum e_flag {
		None		=0x0000,
		Text		=0x0001,
		Disasm		=0x0002,
		UseCodeGuide	=0x0004,
		Has_SearchEngine=0x0008,
		Has_ConvertCP	=0x0010
	    };

	    Plugin(CodeGuider& code_guider) { UNUSED(code_guider); }
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
	Plugin* (*query_interface)(CodeGuider& code_guider);
    };

    class DisMode;
    struct REGISTRY_BIN;
    class Bin_Format : public Opaque {
	public:
	    static const __filesize_t Bad_Address = __filesize_t(-1);

	    Bin_Format(CodeGuider& parent);
	    virtual ~Bin_Format();

	    virtual void		detect_format();

	    virtual const char*		name() const;

	    virtual const char*		prompt(unsigned idx) const;	/**< on Alt-Fx selection */
	    virtual __filesize_t	action_F1() const;
	    virtual __filesize_t	action_F2() const;
	    virtual __filesize_t	action_F3() const;
	    virtual __filesize_t	action_F4() const;
	    virtual __filesize_t	action_F5() const;
	    virtual __filesize_t	action_F6() const;
	    virtual __filesize_t	action_F7() const;
	    virtual __filesize_t	action_F8() const;
	    virtual __filesize_t	action_F9() const;
	    virtual __filesize_t	action_F10() const;

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift) const;

	    virtual int			query_platform() const;

			 /** Returns DAB_XXX. Quick version for disassembler */
	    virtual int			query_bitness(__filesize_t off) const;

			 /** Returns DAE_XXX. */
	    virtual int			query_endian(__filesize_t off) const;

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
	    virtual bool		address_resolving(char * str,__filesize_t off) const;

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
	    virtual __filesize_t	get_public_symbol(char *str,unsigned cb_str,unsigned *_class,__filesize_t pa,bool as_prev) const;

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
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,__filesize_t *start,__filesize_t *end,int *_class,int *bitness) const;
	private:
	    std::vector<const REGISTRY_BIN*>	formats;
	    const REGISTRY_BIN*		detectedFormat;
	    const REGISTRY_BIN*		mz_format;
	    CodeGuider&			parent;
    };

} //namespace	usr
#endif
