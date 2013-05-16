#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        bin_util.c
 * @brief       This file contains common functions of plugins/bin of BEYE project.
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
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "beye.h"
#include "bin_util.h"
#include "reg_form.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "plugins/disasm.h"

namespace	usr {
/* User Defined names (UDN) */

struct udn {
    char		name[256];
    __filesize_t	offset;

    bool operator<(const udn& rhs) const { return offset<rhs.offset; }
};

static std::set<udn> udn_list;
static bool udn_modified=false;
static std::string udn_fname;

static bool __FASTCALL__ udnAddItem() {
    __filesize_t off;
    udn item;
    std::set<udn>::const_iterator prev=udn_list.end();
    char ud_name[256],prompt[256];
    off = beye_context().tell();
    sprintf(prompt," Name for %08X offset: ",off);
    ud_name[0]='\0';
    if(!udn_list.empty()) {
	item.name[255]='\0';
	item.offset=off;
	prev = udn_list.find(item);
	if(prev!=udn_list.end()) strcpy(ud_name,(*prev).name);
    }
    if(GetStringDlg(ud_name,prompt," [ENTER] - Proceed ",NAME_MSG))
    {
	if(prev!=udn_list.end()) udn_list.erase(prev);
	if(!udn_list.empty()) {
	    strcpy(item.name,ud_name);
	    item.offset=off;
	    udn_list.insert(item);
//	    la_Sort(udn_list,udn_compare);
	}
	udn_modified=true;
	return true;
    }
    return false;
}

static unsigned __FASTCALL__ udnGetNumItems(binary_stream& handle) {
    UNUSED(handle);
    return udn_list.size();
}

static bool    __FASTCALL__ udnReadItems(binary_stream& handle,memArray * names,unsigned nnames)
{
    char stmp[256];
    unsigned i;
    UNUSED(handle);
    std::set<udn>::iterator it=udn_list.begin();
    for(i=0;i<nnames;i++) {
	sprintf(stmp,"%-40s %08lX"
		,(*it).name
		,(*it).offset);
	it++;
	if(!ma_AddString(names,stmp,true)) break;
    }
    return true;
}

static bool __FASTCALL__ udnDeleteItem() {
    int ret=-1;
    if(udn_list.empty()) { beye_context().ErrMessageBox("UDN list is empty!",""); return false; }
    std::string title = " User-defined Names (aka bookmarks) ";
    ssize_t nnames = udnGetNumItems(beye_context().sc_bm_file());
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = udnReadItems(beye_context().sc_bm_file(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret!=-1) {
	int i=0;
	std::set<udn>::iterator it;
	for(it=udn_list.begin();it!=udn_list.end();it++) { if(i==ret) break; i++; }
	udn_list.erase(it);
	udn_modified=true;
    }
    return ret==-1?false:true;
}

bool __FASTCALL__ udnSelectName(__filesize_t *off) {
    int ret=-1;
    if(udn_list.empty()) { beye_context().ErrMessageBox("UDN list is empty!",""); return false; }
    std::string title = " User-defined Names (aka bookmarks) ";
    ssize_t nnames = udnGetNumItems(beye_context().sc_bm_file());
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = udnReadItems(beye_context().sc_bm_file(),obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret!=-1) {
	int i=0;
	std::set<udn>::const_iterator it;
	for(it=udn_list.begin();it!=udn_list.end();it++) { if(i==ret) break; i++; }
	*off = (*it).offset;
    }
    return ret==-1?false:true;
}

bool __FASTCALL__ udnFindName(__filesize_t pa,char *buff, unsigned cb_buff) {
    std::set<udn>::const_iterator it;
    udn key;
    if(!udn_list.empty()) {
	key.name[0]='\0';
	key.offset = pa;
	it=udn_list.find(key);
	if(it!=udn_list.end()) {
	    strncpy(buff,(*it).name,cb_buff);
	    buff[cb_buff-1]='\0';
	    return true;
	}
    }
    return false;
}

bool __FASTCALL__ udnFindName(__filesize_t pa,std::string& buff) {
    std::set<udn>::const_iterator it;
    udn key;
    if(!udn_list.empty()) {
	key.name[0]='\0';
	key.offset = pa;
	it=udn_list.find(key);
	if(it!=udn_list.end()) {
	    buff.assign((*it).name);
	    return true;
	}
    }
    return false;
}

bool __FASTCALL__ __udnSaveList()
{
    if(!udn_list.empty()) {
	std::ofstream out;
	out.open(udn_fname.c_str(),std::ios_base::out);
	if(out.is_open()) {
	    out<<"; This is an automatically generated list of user-defined names"<<std::endl;
	    out<<"; for: "<<beye_context().bm_file().filename()<<std::endl;
	    out<<"; by Beye-"<<BEYE_VERSION<<std::endl;
	    for(std::set<udn>::const_iterator it=udn_list.begin();it!=udn_list.end();it++)
		out<<std::hex<<std::setfill('0')<<std::setw(16)
		    <<(*it).offset<<":"<<(*it).name<<std::endl;
	    out.close();
	    udn_modified=false;
	    return true;
	}
	else {
	    char stmp[256];
	    sprintf(stmp,"Can't open file: %s\n",strerror(errno));
	    beye_context().ErrMessageBox(udn_fname,stmp);
	}
    }
    return false;
}


bool __FASTCALL__ udnSaveList() {
    char tmps[4096];
    if(GetStringDlg(tmps," Please enter file name: "," [ENTER] - Proceed ",NAME_MSG))
    {
	udn_fname=tmps;
	if(!udn_list.empty())	return __udnSaveList();
	else		beye_context().ErrMessageBox("UDN list is empty!","");
    }
    return false;
}

bool __FASTCALL__  __udnLoadList() {
    unsigned i;
    udn item;
    std::ifstream in;
    in.open(udn_fname.c_str(),std::ios_base::in);
    if(in.is_open()) {
	    char buff[4096],*brk;
	    unsigned blen;
	    i = 0;
	    while(!in.eof()) {
		buff[0]='\0';
		in.getline(buff,sizeof(buff));
		i++;
		if(buff[0]==';'||buff[0]=='\0') continue;
		brk=strchr(buff,':');
		if(!brk) {
		    char stmp[256];
		    sprintf(stmp,"Can't recognize line: %u",i);
		    beye_context().ErrMessageBox(stmp,"");
		    return true;
		}
		*brk='\0';
		sscanf(buff,"%016llX",&item.offset);
		strncpy(item.name,brk+1,sizeof(item.name));
		item.name[sizeof(item.name)-1]='\0';
		blen = strlen(item.name);
		while(item.name[blen-1]==10||item.name[blen-1]==13) {
		    item.name[blen-1]='\0'; blen--;
		}
		udn_list.insert(item);
	    }
	    in.close();
//	    if(udn_list) la_Sort(udn_list,udn_compare);
	    return true;
	}
	else {
	    char stmp[256];
	    sprintf(stmp,"Can't open file: %s\n",strerror(errno));
	    beye_context().ErrMessageBox(udn_fname,stmp);
    }
    return false;
}

bool __FASTCALL__ udnLoadList() {
    char tmps[4096];
    if(GetStringDlg(tmps," Please enter file name: "," [ENTER] - Proceed ",NAME_MSG))
    {
	udn_fname=tmps;
	if(!udn_list.empty())	return __udnLoadList();
	else		beye_context().ErrMessageBox("UDN list is empty!","");
    }
    return false;
}

static const char *udn_operations[] =
{
    "~Add item",
    "~Delete item",
    "~Load list from file",
    "~Save list to file"
};
typedef bool (__FASTCALL__ *udnFunc)();

static udnFunc udn_funcs[] =
{
    udnAddItem,
    udnDeleteItem,
    udnLoadList,
    udnSaveList
};

bool __FASTCALL__ udnUserNames() {
  unsigned nModes;
  int i;
  nModes = sizeof(udn_operations)/sizeof(char *);
  i = 0;
  i = ListBox(udn_operations,nModes," Select operation: ",LB_SELECTIVE|LB_USEACC,i);
  if(i != -1)
  {
     int ret;
     TWindow * w;
     w = PleaseWaitWnd();
     ret = (*udn_funcs[i])();
     delete w;
     return ret;
  }
  return false;
}

void __FASTCALL__ udnInit( Ini_Profile& ini ) {
  if(beye_context().is_valid_ini_args())
  {
    udn_fname=beye_context().read_profile_string(ini,"Beye","Browser","udn_list","");
    if(!udn_fname.empty()) __udnLoadList();
  }
}

void __FASTCALL__ udnTerm( Ini_Profile& ini ) {
  if(!udn_list.empty()) {
    if(udn_modified) {
	beye_context().WarnMessageBox("User-defined list of names was not saved",NULL);
	udnSaveList();
    }
    udn_list.clear();
  }
  beye_context().write_profile_string(ini,"Beye","Browser","udn_list",udn_fname);
}
} // namespace	usr
