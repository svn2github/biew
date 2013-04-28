#ifndef __PLUGIN_HPP_INCLUDED
#define __PLUGIN_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

namespace beye {
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

	    Plugin() {}
	    virtual ~Plugin() {}

	    virtual const char*		prompt(unsigned idx) const = 0;
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

	    virtual bool		detect() = 0;
	    virtual e_flag		flags() const = 0;
	    virtual unsigned		paint(unsigned keycode,unsigned textshift) = 0;

	    virtual unsigned		get_symbol_size() const = 0;
	    virtual unsigned		get_max_symbol_size() const { return get_symbol_size(); }
	    virtual unsigned		get_max_line_length() const = 0;
	    virtual const char*		misckey_name() const = 0;
	    virtual void		misckey_action() = 0;
	    virtual unsigned long	prev_page_size() const = 0;
	    virtual unsigned long	curr_page_size() const = 0;
	    virtual unsigned long	prev_line_width() const = 0;
	    virtual unsigned long	curr_line_width() const = 0;
	    virtual void		help() const = 0;
	    virtual void		read_ini(hIniProfile *) = 0;
	    virtual void		save_ini(hIniProfile *) = 0;
	    virtual unsigned		convert_cp(char *str,unsigned len, bool use_fs_nls) { UNUSED(str); UNUSED(len); UNUSED(use_fs_nls); return 0; }
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
	Plugin* (*query_interface)();
    };
} //namespace beye
#endif
