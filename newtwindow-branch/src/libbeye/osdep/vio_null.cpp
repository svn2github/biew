#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include <iostream>

#ifndef lint
static const char rcs_id[] = "$Id: vio.c,v 1.16 2009/09/20 14:39:37 nickols_k Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "vio_interface.h"
#include "tconsole.h"
#include "unix/console.h"
#include "libbeye/tvideo_buffer.h"

namespace	usr {

    class vio_null : public vio_interface {
	public:
	    vio_null(System& s,const std::string& user_cp,unsigned long flags);
	    virtual ~vio_null();

	    virtual void		set_transparent_color(uint8_t);
	    virtual void		write_buffer(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff);
	    virtual tvideo_buffer	read_buffer(tAbsCoord x,tAbsCoord y,size_t len);
	    virtual void		get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const;
	    virtual void		set_cursor_pos(tAbsCoord x,tAbsCoord y);
	    virtual int			get_cursor_type() const;
	    virtual int			set_cursor_type(int);

	    virtual tAbsCoord		get_width() const;
	    virtual tAbsCoord		get_height() const;
	    virtual unsigned		get_num_colors() const;
	private:
	    System&			sys;
    };

int vio_null::get_cursor_type() const { return 0; }
int vio_null::set_cursor_type(int type) { UNUSED(type); return 0; }
void vio_null::get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const { x = 0; y = 0; }
void vio_null::set_cursor_pos(tAbsCoord x,tAbsCoord y) { UNUSED(x); UNUSED(y); }
tvideo_buffer vio_null::read_buffer(tAbsCoord x, tAbsCoord y, size_t len)
{
    UNUSED(x);
    UNUSED(y);
    UNUSED(len);
    tvideo_buffer rc(1);
    throw std::logic_error("vio_null::read_buffer");
    return rc;
}

void vio_null::write_buffer(tAbsCoord x, tAbsCoord y, const tvideo_buffer& buff)
{
    UNUSED(x);
    UNUSED(y);
    UNUSED(buff);
    throw std::logic_error("vio_null::write_buffer");
}

vio_null::vio_null(System& s,const std::string& user_cp,unsigned long flags)
	    :vio_interface(s,user_cp,flags)
	    ,sys(s)
{
    throw missing_device_exception();
}

vio_null::~vio_null()
{
}

void vio_null::set_transparent_color(uint8_t value) { UNUSED(value); }
tAbsCoord vio_null::get_width() const { return 0; }
tAbsCoord vio_null::get_height() const { return 0; }
unsigned vio_null::get_num_colors() const { return 0; }

static vio_interface* query_interface(System& s,const std::string& user_cp,unsigned long flags) { return new(zeromem) vio_null(s,user_cp,flags); }

extern const vio_interface_info vio_null_info = {
    "Null video interface",
    query_interface
};
} // namespace	usr