#include "processor.h"

#include "libbeye/osdep/system.h"
#include "_hrd_inf.h"

namespace	usr {
Processor::Processor(System& s):sys(s) {}
Processor::~Processor() {}
void Processor::cpu_info(char *buff,unsigned cbBuff,void (*percents_callback)(int)) const { __FillCPUInfo(sys,buff,cbBuff,percents_callback); }
} // namespace	usr