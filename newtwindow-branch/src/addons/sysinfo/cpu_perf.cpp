#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_addons
 * @file        addons/sys/cpu_perf.c
 * @brief       This file contains cpu performance utility and tools.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
 * @author      Andrew Golovnya <andrew_golovnia at users dot sourceforge dot net>
 * @note        CPUInfo display window changed to scrollable window
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "addons/addon.h"

#include "bconsole.h"
#include "beyeutil.h"
#include "listbox.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"
#include "libbeye/sysdep/processor.h"

namespace	usr {
    class CPUPerformance_Addon : public Addon {
	public:
	    CPUPerformance_Addon(BeyeContext& bc);
	    virtual ~CPUPerformance_Addon();
	
	    virtual void	run();
	private:
	    BeyeContext&	bctx;
	    std::vector<std::string>	cpuPointStrings(char* data,size_t data_size) const;
    };

static TWindow*	pwnd;

CPUPerformance_Addon::CPUPerformance_Addon(BeyeContext& bc):Addon(bc),bctx(bc) {}
CPUPerformance_Addon::~CPUPerformance_Addon() {}

static void paint_prcnt(int n_prcnt)
{
  if(n_prcnt > 100) n_prcnt = 100;
  ShowPercentInWnd(pwnd,n_prcnt);
}

std::vector<std::string> CPUPerformance_Addon::cpuPointStrings(char* data,size_t data_size) const
{
    std::vector<std::string> rc;
    size_t i;
    char ch,ch1;
    char *p = data;
    for(i = 0;i < data_size;i++) {
	ch = data[i];
	if(ch == '\n' || ch == '\r') {
	    data[i] = 0;
	    ch1 = data[i+1];
	    if((ch1 == '\n' || ch1 == '\r') && ch != ch1) ++i;
	    rc.push_back(p);
	    p=&data[i+1];
	}
    }
    return rc;
}

void CPUPerformance_Addon::run()
{
    char* cpu_info;
    std::vector<std::string> strs;
    unsigned long data_size;

    cpu_info = new char [4096];
    pwnd = PercentWnd("Analyze:","");
    Processor cpu;
    cpu.cpu_info(cpu_info,4096,paint_prcnt);
    delete pwnd;
    data_size = strlen(cpu_info);
    strs = cpuPointStrings(cpu_info,data_size);
    ListBox lb(bctx);
    lb.run(strs," CPU information ",ListBox::Simple);
    delete cpu_info;
}

static Addon* query_interface(BeyeContext& bc) { return new(zeromem) CPUPerformance_Addon(bc); }
extern const Addon_Info CPUPerformance = {
    "CPU ~performance",
    query_interface
};
} // namespace	usr
