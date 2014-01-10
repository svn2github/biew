#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include "system.h"
#include "system_interface.h"

namespace	usr {
extern const system_interface_info system_posix_info;
extern const system_interface_info system_null_info;
static const system_interface_info* system_interfaces[] = {
    &system_posix_info,
    &system_null_info
};

System::System()
{
    for(size_t i=0;system_interfaces[i]!=&system_null_info;i++) {
	try {
	    sys = system_interfaces[i]->query_interface();
	    sys_info = system_interfaces[i];
	} catch (const missing_device_exception& e) {
	    delete sys;
	    sys = NULL;
	    continue;
	}
    }
    if(!sys) throw missing_driver_exception();

}
System::~System() { delete sys; }

void System::yield_timeslice() const { sys->yield_timeslice(); }
std::string System::get_home_dir(const std::string& progname) const { return sys->get_home_dir(progname); }
std::string System::get_ini_name(const std::string& progname) const { return sys->get_ini_name(progname); }
std::string System::get_rc_dir(const std::string& progname) const { return sys->get_rc_dir(progname); }
unsigned System::set_timer_callback(unsigned ms,timer_callback *func) const { return sys->set_timer_callback(ms,func); }
void System::restore_timer() const { sys->restore_timer(); }
/* National Language Support */
void System::nls_oem2osdep(unsigned char *str,unsigned size) const { sys->nls_oem2osdep(str,size); }
void System::nls_cmdline2oem(unsigned char *str,unsigned size) const { sys->nls_cmdline2oem(str,size); }
void System::nls_oem2fs(unsigned char *str,unsigned size) const { sys->nls_oem2fs(str,size); }
} // namespace 	usr
