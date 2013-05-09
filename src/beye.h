#ifndef BEYE_HPP_INCLUDED
#define BEYE_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"

#include <map>
#include <vector>
#include <string>

namespace	usr {
    class addendum;
    class sysinfo;
    class Plugin;
    struct Plugin_Info;
    class CodeGuider;
    class Bin_Format;
    class binary_stream;
    class Ini_Profile;
    class System;
    class TConsole;
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
	    const char*		short_name() const { return _shortname; }
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
	    Plugin&		active_mode() const { return *activeMode; }
	    Bin_Format&		bin_format() const { return *_bin_format; }
	    const Plugin_Info*	mode_info() const { return modes[defMainModeSel]; }
	    TConsole&		tconsole() const { return *_tconsole; }
	    System&		system() const { return *_system; }
	    void		show_usage() const;
	    void		main_loop();
		   /** Main search routine
		     * @param is_continue  indicates initialization of search
					   If set then search should be continued
					   search dialog will displayed otherwise
		     * @return             new offset on successful search and
					   current offset otherwise
		    **/
	    __filesize_t	search( bool is_continue );

	    void		select_tool() const;
	    void		select_sysinfo() const;

	    CodeGuider&		codeguider() const { return *code_guider; }

	    void		PaintTitle() const;

	    binary_stream&	bm_file() const { return bm_file_handle; }
	    binary_stream&	sc_bm_file() const { return sc_bm_file_handle; }
	    bool		BMOpen(const std::string& fname);
	    void		BMClose();
	    static binary_stream* beyeOpenRO(const std::string& fname,unsigned cache_size);
	    static binary_stream* beyeOpenRW(const std::string& fname,unsigned cache_size);

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
	    __filesize_t headshift;
	    __filesize_t LastOffset;
	private:
	    void		auto_detect_mode();
	    int			queryKey(const std::string& arg);

	    Opaque		opaque;
	    Plugin*		activeMode;
	    Bin_Format*		_bin_format;
	    const std::vector<std::string>& argv;
	    const std::map<std::string,std::string>& envm;
	    std::vector<std::string> ListFile;
	    std::string		LastOpenFileName;
	    char*		_shortname;
	    const char*		ini_name;
	    bool		UseIniFile;
	    size_t		LastMode;
	    unsigned int	beye_mode;
	    unsigned		defMainModeSel;
	    __filesize_t	new_file_size;
	    std::vector<const Plugin_Info*> modes;
	    CodeGuider*		code_guider;
	    addendum*		addons;
	    class sysinfo*	sysinfo;
	    binary_stream&	bm_file_handle;
	    binary_stream&	sc_bm_file_handle;
	    TConsole*		_tconsole;
	    LocalPtr<System>	_system;
    };
    BeyeContext& beye_context();
} // namespace	usr
#endif
