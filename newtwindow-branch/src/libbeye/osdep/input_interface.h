#ifndef INPUT_INTERFACE_HPP_INCLUDED
#define INPUT_INTERFACE_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

namespace	usr {
    class input_interface : public Opaque {
	public:
	    input_interface(const std::string&) {}
	    virtual ~input_interface() {}

	    virtual int			get_key( unsigned long flg) = 0;
	    virtual int			test_key( unsigned long flg ) = 0;
	    virtual int			get_shifts() = 0;
	    virtual int			raw_info(char *head, char *text) = 0;

	    virtual bool		ms_get_state() const = 0;
	    virtual void		ms_set_state(bool is_visible) = 0;
	    virtual void		ms_get_pos(tAbsCoord& x, tAbsCoord& y) const = 0;
	    virtual int			ms_get_btns() = 0;
    };

    struct input_interface_info {
	const char*	name;
	input_interface* (*query_interface)(const std::string& user_cp);
    };
} // namespace	usr
#endif
