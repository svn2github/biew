#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        addendum.c
 * @brief       This file contains interface to Addons functions.
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
#include "addendum.h"
#include "bconsole.h"
#include "beyeutil.h"

namespace	usr {
extern const Addon_Info DigitalConvertor;
extern const Addon_Info Calculator;

void addendum::select()
{
    size_t i,nTools=list.size();
    const char *toolName[nTools];
    int retval;

    nTools = list.size();
    for(i = 0;i < nTools;i++) toolName[i] = list[i]->name;
    retval = SelBoxA(toolName,nTools," Select tool: ",defToolSel);
    if(retval != -1) {
	const Addon_Info* addon_info = list[retval];
	Addon* addon = addon_info->query_interface();
	addon->run();
	delete addon;
	defToolSel = retval;
    }
}

addendum::addendum()
	:defToolSel(0)
{
    list.push_back(&DigitalConvertor);
    list.push_back(&Calculator);
}

addendum::~addendum() {}
} // namespace	usr

