#ifndef __ADDENDUM_HPP_INCLUDED
#define __ADDENDUM_HPP_INCLUDED 1
#include "libbeye/libbeye.h"
#include "reg_form.h"

#include <vector>

namespace beye {
    class addendum : public Opaque {
	public:
	    addendum();
	    virtual ~addendum();
	
	    void	select();
	private:
	    size_t	defToolSel;
	    std::vector<REGISTRY_TOOL*> list;
    };
} // namespace beye
#endif
