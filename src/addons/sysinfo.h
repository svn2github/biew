#ifndef __SYSINFO_HPP_INCLUDED
#define __SYSINFO_HPP_INCLUDED 1
#include <vector>

#include "libbeye/libbeye.h"

namespace	usr {
    struct Addon_Info;
    class sysinfo : public Opaque {
	public:
	    sysinfo();
	    virtual ~sysinfo();
	
	    void	select();
	private:
	    size_t	defToolSel;
	    std::vector<const Addon_Info*> list;
    };
} // namespace	usr
#endif
