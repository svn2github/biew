#ifndef __SYSINFO_HPP_INCLUDED
#define __SYSINFO_HPP_INCLUDED 1
#include "libbeye/libbeye.h"
#include "reg_form.h"

#include <vector>

namespace beye {
    class sysinfo : public Opaque {
	public:
	    sysinfo();
	    virtual ~sysinfo();
	
	    void	select();
	private:
	    size_t	defToolSel;
	    std::vector<const REGISTRY_SYSINFO*> list;
    };
} // namespace beye
#endif
