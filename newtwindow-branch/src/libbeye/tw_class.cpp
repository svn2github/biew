#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/tw_class.c
 * @brief       This file contains implementation of classes for Text Window manager.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include "libbeye/twindow.h"

static std::set<TwClass> class_set;

bool __FASTCALL__ twcRegisterClass(const std::string& name, unsigned flags, twClassFunc method)
{
    TwClass newest;
    const TwClass* exists = twcFindClass(name);
    if(!exists) {
	newest.name = name;
	newest.flags = flags;
	newest.method= method;
	class_set.insert(newest);
	return true;
    }
    return false;
}

void __FASTCALL__ twcDestroyClassSet()
{
  class_set.clear();
}

const TwClass* __FASTCALL__ twcFindClass(const std::string& name)
{
    TwClass key;
    const TwClass* rc;
    key.name = name;
    rc=&(*class_set.find(key));
    return rc;
}
