#ifndef __SYSINFO_HPP_INCLUDED
#define __SYSINFO_HPP_INCLUDED 1
#include "libbeye/libbeye.h"
#include "reg_form.h"

#include <vector>

namespace beye {
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
} // namespace beye
#endif
