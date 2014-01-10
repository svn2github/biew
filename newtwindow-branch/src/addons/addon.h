#ifndef __ADDON_HPP_INCLUDED
#define __ADDON_HPP_INCLUDED 1

#include "libbeye/libbeye.h"

namespace	usr {
    class BeyeContext;
    class Addon : public Opaque {
	public:
	    Addon(BeyeContext&) {}
	    virtual ~Addon() {}

	    virtual void	run() = 0;
    };

    struct Addon_Info {
	const char* name;
	Addon* (*query_interface)(BeyeContext& bc);
    };

} // namespace	usr

#endif
