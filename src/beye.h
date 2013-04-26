#ifndef BEYE_HPP_INCLUDED
#define BEYE_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"
#include "libbeye/file_ini.h"

#include <map>
#include <vector>
#include <string>

#include <stdio.h>

namespace beye {
    class BeyeContext : public Opaque {
	public:
	    BeyeContext(const std::vector<std::string>& argv, const std::map<std::string,std::string>& envm);
	    virtual ~BeyeContext();

	    bool		new_source();
	    void		parse_cmdline( const std::vector<std::string>& ArgVector );
	    hIniProfile*	load_ini_info();
	    void		save_ini_info() const;
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

	    std::string ArgVector1;
	    char ini_ver[32];
	    char help_name[FILENAME_MAX+1];
	    char skin_name[FILENAME_MAX+1];
	    char syntax_name[FILENAME_MAX+1];
	    char codepage[256];
	    char scheme_name[256];
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
	    const std::vector<std::string>& argv;
	    const std::map<std::string,std::string>& envm;
	    std::vector<std::string> ListFile;
	    char* LastOpenFileName;
	    char* _shortname;
	    const char *ini_name;
	    bool UseIniFile;
    };
    BeyeContext& beye_context();
} // namespace beye
#endif
