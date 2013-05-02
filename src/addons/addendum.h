#ifndef __ADDENDUM_HPP_INCLUDED
#define __ADDENDUM_HPP_INCLUDED 1
#include <vector>

#include "libbeye/libbeye.h"

namespace beye {
    struct Addon_Info;
    class addendum : public Opaque {
	public:
	    addendum();
	    virtual ~addendum();
	
	    void	select();
	private:
	    size_t	defToolSel;
	    std::vector<const Addon_Info*> list;
    };
} // namespace beye
#endif
