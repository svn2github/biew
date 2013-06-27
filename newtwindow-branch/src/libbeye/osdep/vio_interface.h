#ifndef __VIO_INTERFACE_HPP_INCLUDED
#define __VIO_INTERFACE_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

#include <string>

namespace	usr {
    class tvideo_buffer;
    class vio_interface : public Opaque {
	public:
	    vio_interface(const std::string& user_cp,unsigned long flags) { UNUSED(user_cp); UNUSED(flags); }
	    virtual ~vio_interface() {}

	    virtual void		set_transparent_color(uint8_t) = 0;
	    virtual void		write_buffer(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff) = 0;
	    virtual tvideo_buffer	read_buffer(tAbsCoord x,tAbsCoord y,size_t len) = 0;
	    virtual void		get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const = 0;
	    virtual void		set_cursor_pos(tAbsCoord x,tAbsCoord y) = 0;
	    virtual int			get_cursor_type() const = 0;
	    virtual int			set_cursor_type(int) = 0;

	    virtual tAbsCoord		get_width() const = 0;
	    virtual tAbsCoord		get_height() const = 0;
	    virtual unsigned		get_num_colors() const = 0;
	protected:
    };

    struct vio_interface_info {
	const char*	name;
	vio_interface*	(*query_interface)(const std::string& user_cp,unsigned long flags);
    };
} // namespace	usr
#endif
