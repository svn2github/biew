#ifndef __ADDON_HPP_INCLUDED
#define __ADDON_HPP_INCLUDED 1

#include "libbeye/libbeye.h"

namespace beye {
    class Addon : public Opaque {
	public:
	    Addon() {}
	    virtual ~Addon() {}

	    virtual void	run() = 0;
    };

    struct Addon_Info {
	const char* name;
	Addon* (*query_interface)();
    };

} // namespace beye

#endif
