#include "processor.h"

#include "processor.h"
#include "_hrd_inf.h"

namespace	usr {
Processor::Processor() {}
Processor::~Processor() {}
void Processor::cpu_info(char *buff,unsigned cbBuff,void (*percents_callback)(int)) const { __FillCPUInfo(buff,cbBuff,percents_callback); }
} // namespace	usr