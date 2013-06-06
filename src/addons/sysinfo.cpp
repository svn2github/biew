#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        sysinfo.c
 * @brief       This file contains system information utility and tools.
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
**/
#include <stddef.h>

#include "addon.h"
#include "sysinfo.h"
#include "bconsole.h"
#include "beyeutil.h"

namespace	usr {
extern const Addon_Info AsciiTable;
extern const Addon_Info CPUPerformance;
extern const Addon_Info InputViewer;
extern const Addon_Info ConsoleInfo;

void sysinfo::select()
{
    size_t i,nTools=list.size();
    const char *toolName[nTools];
    int retval;

    nTools = list.size();
    for(i = 0;i < nTools;i++) toolName[i] = list[i]->name;
    retval = ListBox(toolName,nTools," Select tool: ",LB_SELECTIVE|LB_USEACC,defToolSel);
    if(retval != -1) {
	const Addon_Info* addon_info = list[retval];
	Addon* addon = addon_info->query_interface(bctx);
	addon->run();
	delete addon;
	defToolSel = retval;
    }
}

sysinfo::sysinfo(BeyeContext& bc)
	:bctx(bc)
	,defToolSel(0)
{
    list.push_back(&AsciiTable);
    list.push_back(&CPUPerformance);
    list.push_back(&InputViewer);
    list.push_back(&ConsoleInfo);
}

sysinfo::~sysinfo() {}
} // namespace	usr

