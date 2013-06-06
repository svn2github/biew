#ifndef BEYE_HPP_INCLUDED
#define BEYE_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"

#include <limits>
#include <map>
#include <vector>
#include <string>

namespace	usr {
    enum {
	BBIO_CACHE_SIZE        =0xFFFF,  /* 64k */
	BBIO_SMALL_CACHE_SIZE  =0x4000  /* 16k */
    };

    class addendum;
    class sysinfo;
    class Plugin;
    struct Plugin_Info;
    class Search;
    class CodeGuider;
    class Bin_Format;
    class binary_stream;
    class Ini_Profile;
    class System;
    class TConsole;
    class TWindow;
    class Editor;
    class udn;
    class BeyeContext : public Opaque {
	public:
	    BeyeContext(const std::vector<std::string>& argv, const std::map<std::string,std::string>& envm);
	    virtual ~BeyeContext();

	    bool		new_source();
	    void		parse_cmdline( const std::vector<std::string>& ArgVector );
	    Ini_Profile&	load_ini_info();
	    void		save_ini_info() const;
	    bool		LoadInfo();
	    bool		is_valid_ini_args() const;
	    const std::vector<std::string>& list_file() const;
	    const char*		short_name() const;
	    void		make_shortname();
	    std::string		read_profile_string(Ini_Profile& ini,
						    const std::string& section,
						    const std::string& subsection,
						    const std::string& _item,
						    const std::string& def_value) const;
	    bool		write_profile_string(Ini_Profile& ini,
						    const std::string& section,
						    const std::string& subsection,
						    const std::string& item,
						    const std::string& value) const;
	    void		init_modes(Ini_Profile& ini);
	    void		quick_select_mode();
	    bool		select_mode();
	    Plugin&		active_mode() const;
	    const Bin_Format&	bin_format() const;
	    const Plugin_Info*	mode_info() const;
	    TConsole&		tconsole() const;
	    System&		system() const;
	    void		show_usage() const;
	    void		main_loop();

	    void		select_tool() const;
	    void		select_sysinfo() const;

	    CodeGuider&		codeguider() const;
	    Search&		search() const;
	    TWindow&		main_wnd() const;

	    void		PaintTitle() const;

	    void		create_windows();
	    void		detect_format(binary_stream&);

	    binary_stream&	bm_file() const;
	    binary_stream&	sc_bm_file() const;
	    bool		BMOpen(const std::string& fname);
	    void		BMClose();
	    static binary_stream* beyeOpenRO(const std::string& fname,unsigned cache_size);
	    static binary_stream* beyeOpenRW(const std::string& fname,unsigned cache_size);
	    __filesize_t	flength() const;
	    __filesize_t	tell() const;
	    bool		is_file64() const;

	    udn&		_udn() const;

	    void		TMessageBox(const std::string& text,const std::string& title) const;
	    void		NotifyBox(const std::string& text,const std::string& title) const;
	    void		ErrMessageBox(const std::string& text,const std::string& title) const;
	    void		WarnMessageBox(const std::string& text,const std::string& title) const;
	    void		errnoMessageBox(const std::string& text,const std::string& title,int __errno__) const;
	    void		draw_multi_prompt(const char * const norm[], const char *const shift[], const char * const alt[], const char * const ctrl[]) const;

	    std::string ArgVector1;
	    std::string ini_ver;
	    std::string help_name;
	    std::string skin_name;
	    std::string syntax_name;
	    std::string codepage;
	    std::string scheme_name;
	    std::string last_skin_error;
	    unsigned long vioIniFlags;
	    unsigned long twinIniFlags;
	    unsigned long kbdFlags;
	    bool iniSettingsAnywhere;
	    bool fioUseMMF;
	    bool iniPreserveTime;
	    bool iniUseExtProgs;
	    __filesize_t LastOffset;
	protected:
	    friend class Editor;
	    TWindow&		error_wnd() const;
	    TWindow&		title_wnd() const;
	    TWindow&		prompt_wnd() const;
	private:
	    void		auto_detect_mode();
	    int			queryKey(const std::string& arg);
	    void		message_box(const std::string& text,const std::string& title,
					    ColorAttr base,ColorAttr frame) const;
	    void		draw_title(__filesize_t lastbyte) const;

	    Opaque&		opaque;
    };
    BeyeContext& beye_context();

    inline unsigned	HA_LEN() { return beye_context().is_file64()?18:10; }
} // namespace	usr
#endif
