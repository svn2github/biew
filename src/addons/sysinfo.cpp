#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
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

#include "sysinfo.h"
#include "bconsole.h"
#include "beyeutil.h"

namespace beye {
extern REGISTRY_SYSINFO AsciiTable;
extern REGISTRY_SYSINFO CPUPerformance;
extern REGISTRY_SYSINFO InputViewer;
extern REGISTRY_SYSINFO ConsoleInfo;

void sysinfo::select()
{
    size_t i,nTools=list.size();
    const char *toolName[nTools];
    int retval;

    nTools = list.size();
    for(i = 0;i < nTools;i++) toolName[i] = list[i]->name;
    retval = SelBoxA(const_cast<char**>(toolName),nTools," Select tool: ",defToolSel);
    if(retval != -1) {
	list[retval]->sysinfo();
	defToolSel = retval;
    }
}

sysinfo::sysinfo()
	:defToolSel(0)
{
    list.push_back(&AsciiTable);
    list.push_back(&CPUPerformance);
    list.push_back(&InputViewer);
    list.push_back(&ConsoleInfo);
    size_t i,sz=list.size();
    for(i = 0;i < sz;i++) {
	if(list[i]->read_ini) list[i]->read_ini();
    }
}

sysinfo::~sysinfo()
{
    size_t i,sz=list.size();
    for(i = 0;i < sz;i++) {
	if(list[i]->save_ini) list[i]->save_ini();
    }
}
} // namespace beye

