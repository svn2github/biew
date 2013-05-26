#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include "tconsole.h"
#include "__os_dep.h"

namespace	usr {
TConsole::TConsole(const char *user_cp, unsigned long vio_flags) {
    __init_vio(user_cp,vio_flags);
    __init_keyboard(user_cp);
}
TConsole::~TConsole() {
    __term_keyboard();
    __term_vio();
}

int TConsole::kbd_get_key( unsigned long flg ) const { return __kbdGetKey(flg); }
int TConsole::kbd_test_key( unsigned long flg ) const { return __kbdTestKey(flg); }
int TConsole::kbd_get_shifts() const { return __kbdGetShiftsKey(); }

bool TConsole::mouse_get_state() const { return __MsGetState(); }
void TConsole::mouse_set_state( bool is_visible ) const { __MsSetState(is_visible); }
void TConsole::mouse_get_pos(tAbsCoord& x, tAbsCoord& y ) const { __MsGetPos(&x,&y); }
int  TConsole::mouse_get_buttons() const { return __MsGetBtns(); }

int TConsole::vio_get_cursor_type() const { return __vioGetCursorType(); }
void TConsole::vio_set_cursor_type( int c_type ) const { __vioSetCursorType(c_type); }
void TConsole::vio_get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const { __vioGetCursorPos(&x,&y); }
void TConsole::vio_set_cursor_pos(tAbsCoord x,tAbsCoord y) const { __vioSetCursorPos(x,y); }
tvideo_buffer TConsole::vio_read_buff(tAbsCoord x,tAbsCoord y,size_t len) const {
    tvioBuff tmp;
    tmp.chars = new t_vchar[len];
    tmp.oem_pg= new t_vchar[len];
    tmp.attrs = new ColorAttr[len];
    __vioReadBuff(x,y,&tmp,len);
    tvideo_buffer rc(tmp.chars,tmp.oem_pg,tmp.attrs,len);
    delete tmp.chars;
    delete tmp.oem_pg;
    delete tmp.attrs;
    return rc;
}
void TConsole::vio_write_buff(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff) const {
    size_t len=buff.length();
    tvioBuff tmp;
    tmp.chars = new t_vchar[len];
    tmp.oem_pg= new t_vchar[len];
    tmp.attrs = new ColorAttr[len];
    ::memcpy(tmp.chars,buff.get_chars(),len);
    ::memcpy(tmp.oem_pg,buff.get_oempg(),len);
    ::memcpy(tmp.attrs,buff.get_attrs(),len);
    __vioWriteBuff(x,y,&tmp,len);
    delete tmp.chars;
    delete tmp.oem_pg;
    delete tmp.attrs;
}

tAbsCoord TConsole::vio_width() const { return tvioWidth; }
tAbsCoord TConsole::vio_height() const { return tvioHeight; }
unsigned  TConsole::vio_num_colors() const { return tvioNumColors; }

void TConsole::vio_set_transparent_color(unsigned char value) const { __vioSetTransparentColor(value); }
int TConsole::input_raw_info(char *head, char *text) const { return __inputRawInfo(head,text); }

} // namespace	usr
