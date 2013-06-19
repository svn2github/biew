#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include "tconsole.h"
#include "__os_dep.h"
#include "vio_interface.h"

namespace	usr {
extern const vio_interface_info vio_vcsa_info;
extern const vio_interface_info vio_vt100_info;
extern const vio_interface_info vio_null_info;

static const vio_interface_info* vio_interfaces[] = {
#ifdef TARGET_LINUX
    &vio_vcsa_info,
#endif
    &vio_vt100_info,
    &vio_null_info
};

TConsole::TConsole(const char *user_cp, unsigned long vio_flags) {
    for(size_t i=0;vio_interfaces[i]!=&vio_null_info;i++) {
	try {
	    tvio = vio_interfaces[i]->query_interface(user_cp,vio_flags);
	    vio_info = vio_interfaces[i];
	} catch (const missing_device_exception& e) {
	    delete tvio;
	    tvio = NULL;
	    continue;
	}
    }
    if(!tvio) throw missing_driver_exception();
    __init_keyboard(user_cp);
}
TConsole::~TConsole() {
    __term_keyboard();
    delete tvio;
}

int TConsole::kbd_get_key( unsigned long flg ) const { return __kbdGetKey(flg); }
int TConsole::kbd_test_key( unsigned long flg ) const { return __kbdTestKey(flg); }
int TConsole::kbd_get_shifts() const { return __kbdGetShiftsKey(); }

bool TConsole::mouse_get_state() const { return __MsGetState(); }
void TConsole::mouse_set_state( bool is_visible ) const { __MsSetState(is_visible); }
void TConsole::mouse_get_pos(tAbsCoord& x, tAbsCoord& y ) const { __MsGetPos(&x,&y); }
int  TConsole::mouse_get_buttons() const { return __MsGetBtns(); }

int TConsole::vio_get_cursor_type() const { return tvio->get_cursor_type(); }
void TConsole::vio_set_cursor_type( int c_type ) const { tvio->set_cursor_type(c_type); }
void TConsole::vio_get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const { tvio->get_cursor_pos(x,y); }
void TConsole::vio_set_cursor_pos(tAbsCoord x,tAbsCoord y) const { tvio->set_cursor_pos(x,y); }
tvideo_buffer TConsole::vio_read_buff(tAbsCoord x,tAbsCoord y,size_t len) const {
    return tvio->read_buffer(x,y,len);
}
void TConsole::vio_write_buff(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff) const {
    tvio->write_buffer(x,y,buff);
}

tAbsCoord TConsole::vio_width() const { return tvio->get_width(); }
tAbsCoord TConsole::vio_height() const { return tvio->get_height(); }
unsigned  TConsole::vio_num_colors() const { return tvio->get_num_colors(); }

void TConsole::vio_set_transparent_color(unsigned char value) const { tvio->set_transparent_color(value); }
int TConsole::input_raw_info(char *head, char *text) const { return __inputRawInfo(head,text); }

} // namespace	usr
