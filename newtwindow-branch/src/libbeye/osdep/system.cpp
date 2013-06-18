#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include "system.h"
#include "__os_dep.h"

namespace	usr {
System::System() { __init_sys(); }
System::~System() { __term_sys(); }

void System::yield_timeslice() const { __OsYield(); }
bool System::get_cbreak() const { return __OsGetCBreak(); }
void System::set_cbreak( bool state ) const { __OsSetCBreak(state); }
std::string System::get_home_dir(const std::string& progname) const { return __get_home_dir(progname.c_str()); }
std::string System::get_ini_name(const std::string& progname) const { return __get_ini_name(progname.c_str()); }
std::string System::get_rc_dir(const std::string& progname) const { return __get_rc_dir(progname.c_str()); }
unsigned System::set_timer_callback(unsigned ms,timer_callback *func) const { return __OsSetTimerCallBack(ms,func); }
void System::restore_timer() const { __OsRestoreTimer(); }
/* National Language Support */
void System::nls_prepare_oem_for_vio(tvioBuff *it,unsigned size) const {
    unsigned i;
    unsigned char ch;
    for(i = 0;i < size;i++) {
	ch = it->chars[i];
	it->oem_pg[i] = NLS_IS_OEMPG(ch) ? ch : 0;
    }
    nls_oem2osdep(it->chars,size);
}
void System::nls_oem2osdep(unsigned char *str,unsigned size) const { __nls_OemToOsdep(str,size); }
void System::nls_cmdline2oem(unsigned char *str,unsigned size) const { __nls_CmdlineToOem(str,size); }
void System::nls_oem2fs(unsigned char *str,unsigned size) const { __nls_OemToFs(str,size); }
} // namespace 	usr
