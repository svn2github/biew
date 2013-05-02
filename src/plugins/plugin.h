#ifndef __PLUGIN_HPP_INCLUDED
#define __PLUGIN_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

struct hIniProfile;
struct TWindow;
namespace beye {
    class CodeGuider;
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
	    virtual void		read_ini(hIniProfile *) = 0;	/**< reads beye.ini file if need */
	    virtual void		save_ini(hIniProfile *) = 0;	/**< writes to beye.ini if need */
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
} //namespace beye
#endif
