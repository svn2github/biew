#ifndef BEYE_HPP_INCLUDED
#define BEYE_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"

#include <map>
#include <vector>
#include <string>

struct hIniProfile;
namespace beye {
    class addendum;
    class sysinfo;
    class Plugin;
    struct Plugin_Info;
    class CodeGuider;
    struct REGISTRY_BIN;
    class BFile;
    class BeyeContext : public Opaque {
	public:
	    BeyeContext(const std::vector<std::string>& argv, const std::map<std::string,std::string>& envm);
	    virtual ~BeyeContext();

	    bool		new_source();
	    void		parse_cmdline( const std::vector<std::string>& ArgVector );
	    hIniProfile*	load_ini_info();
	    void		save_ini_info() const;
	    bool		LoadInfo();
	    bool		is_valid_ini_args() const;
	    const std::vector<std::string>& list_file() const;
	    const char*		short_name() const { return _shortname; }
	    void		make_shortname();
	    unsigned		read_profile_string(hIniProfile *ini,
						    const char *section,
						    const char *subsection,
						    const char *_item,
						    const char *def_value,
						    char *buffer,
						    unsigned cbBuffer) const;
	    bool		write_profile_string(hIniProfile *ini,
						    const char *section,
						    const char *subsection,
						    const char *item,
						    const char *value) const;
	    void		init_modes( hIniProfile *ini );
	    void		quick_select_mode();
	    bool		select_mode();
	    Plugin*		active_mode() const { return activeMode; }
	    const Plugin_Info*	mode_info() const { return modes[defMainModeSel]; }
	    const REGISTRY_BIN*	active_format() const { return detectedFormat; }
	    void		detect_binfmt();
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

	    BFile&		bm_file() const { return bm_file_handle; }
	    BFile&		sc_bm_file() const { return sc_bm_file_handle; }
	    bool		BMOpen(const std::string& fname);
	    void		BMClose();
	    static BFile*	beyeOpenRO(const std::string& fname,unsigned cache_size);
	    static BFile*	beyeOpenRW(const std::string& fname,unsigned cache_size);

	    std::string ArgVector1;
	    char ini_ver[32];
	    std::string help_name;
	    std::string skin_name;
	    std::string syntax_name;
	    std::string codepage;
	    std::string scheme_name;
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
	    const REGISTRY_BIN*	detectedFormat;
	    const std::vector<std::string>& argv;
	    const std::map<std::string,std::string>& envm;
	    std::vector<std::string> ListFile;
	    char*		LastOpenFileName;
	    char*		_shortname;
	    const char*		ini_name;
	    bool		UseIniFile;
	    size_t		LastMode;
	    unsigned int	beye_mode;
	    unsigned		defMainModeSel;
	    __filesize_t	new_file_size;
	    std::vector<const REGISTRY_BIN*> formats;
	    std::vector<const Plugin_Info*> modes;
	    CodeGuider*		code_guider;
	    addendum*		addons;
	    class sysinfo*	sysinfo;
	    BFile&		bm_file_handle;
	    BFile&		sc_bm_file_handle;
    };
    BeyeContext& beye_context();
} // namespace beye
#endif
