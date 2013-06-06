#ifndef __ADDENDUM_HPP_INCLUDED
#define __ADDENDUM_HPP_INCLUDED 1
#include <vector>

#include "libbeye/libbeye.h"

namespace	usr {
    class BeyeContext;
    struct Addon_Info;
    class addendum : public Opaque {
	public:
	    addendum(BeyeContext& bc);
	    virtual ~addendum();
	
	    void	select();
	private:
	    BeyeContext&	bctx;
	    size_t		defToolSel;
	    std::vector<const Addon_Info*> list;
    };
} // namespace	usr
#endif
