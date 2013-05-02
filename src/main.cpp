#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        main.cpp
 * @brief       This file contains entry point of BEYE project.
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
 * @author      Kostya Nosov <k-nosov@yandex.ru>
 * @date        27.11.2000
 * @note        Changing technology recognition of new-exe files
**/
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_RESOURCE
#include <sys/resource.h>
#endif
#include <stdlib.h>
#include <errno.h>

#include "addons/addendum.h"
#include "addons/sysinfo.h"
#include "beye.h"
#include "bconsole.h"
#include "colorset.h"
#include "bmfile.h"
#include "bin_util.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "reg_form.h"
#include "beyeutil.h"
#include "search.h"
#include "setup.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"

#include "plugins/plugin.h"

namespace beye {
extern const REGISTRY_BIN binTable;
extern const REGISTRY_BIN rmTable;
extern const REGISTRY_BIN movTable;
extern const REGISTRY_BIN mp3Table;
extern const REGISTRY_BIN mpegTable;
extern const REGISTRY_BIN jpegTable;
extern const REGISTRY_BIN wavTable;
extern const REGISTRY_BIN aviTable;
extern const REGISTRY_BIN asfTable;
extern const REGISTRY_BIN bmpTable;
extern const REGISTRY_BIN neTable;
extern const REGISTRY_BIN peTable;
extern const REGISTRY_BIN leTable;
extern const REGISTRY_BIN lxTable;
extern const REGISTRY_BIN nlm386Table;
extern const REGISTRY_BIN elf386Table;
extern const REGISTRY_BIN jvmTable;
extern const REGISTRY_BIN coff386Table;
extern const REGISTRY_BIN archTable;
extern const REGISTRY_BIN aoutTable;
extern const REGISTRY_BIN OldPharLapTable;
extern const REGISTRY_BIN PharLapTable;
extern const REGISTRY_BIN rdoffTable;
extern const REGISTRY_BIN rdoff2Table;
extern const REGISTRY_BIN sisTable;
extern const REGISTRY_BIN sisxTable;
extern const REGISTRY_BIN lmfTable;
extern const REGISTRY_BIN mzTable;
extern const REGISTRY_BIN dossysTable;

extern const Plugin_Info binMode;
extern const Plugin_Info textMode;
extern const Plugin_Info hexMode;
extern const Plugin_Info disMode;

extern char last_skin_error[];
static volatile char antiviral_hole1[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

static BeyeContext* BeyeCtx=NULL;

static volatile char antiviral_hole2[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

TWindow * MainWnd = 0,*HelpWnd = 0,*TitleWnd = 0,*ErrorWnd = 0;

static const unsigned SHORT_PATH_LEN=__TVIO_MAXSCREENWIDTH-54;

static volatile char antiviral_hole3[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

static int malloc_debug=0;

static volatile char antiviral_hole4[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

BeyeContext& beye_context() { return *BeyeCtx; }

bool BeyeContext::select_mode()
{
    size_t i,nModes = modes.size();
    const char *modeName[nModes];
    int retval;

    for(i = 0;i < nModes;i++) modeName[i] = modes[i]->name;
    retval = SelBoxA(const_cast<char**>(modeName),nModes," Select translation mode: ",defMainModeSel);
    if(retval != -1) {
	defMainModeSel = retval;
	delete activeMode;
	activeMode = modes[defMainModeSel]->query_interface(*code_guider);
	return true;
    }
    return false;
}

void BeyeContext::init_modes( hIniProfile *ini )
{
    if(!activeMode) activeMode = modes[defMainModeSel]->query_interface(*code_guider);
    activeMode->read_ini(ini);
}

void BeyeContext::quick_select_mode()
{
    size_t nModes = modes.size();
    if(defMainModeSel < nModes - 1) defMainModeSel++;
    else                            defMainModeSel = 0;
    delete activeMode;
    activeMode = modes[defMainModeSel]->query_interface(*code_guider);
}

void BeyeContext::make_shortname()
{
  unsigned l;
  unsigned slen = twGetClientWidth(TitleWnd)-54;
  l = ArgVector1.length();
  if(l <= slen) strcpy(_shortname,ArgVector1.c_str());
  else
  {
    strncpy(_shortname,ArgVector1.c_str(),slen/2 - 3);
    _shortname[slen/2-4] = 0;
    strcat(_shortname,"...");
    strcat(_shortname,&ArgVector1.c_str()[l - slen/2]);
  }
  __nls_CmdlineToOem((unsigned char *)_shortname,strlen(_shortname));
}

__filesize_t IsNewExe()
{
  __filesize_t ret;
  char id[2];
  bmReadBufferEx(id,sizeof(id),0,BM_SEEK_SET);
#if 0
   /*
      It is well documented technology, but it correctly working
      only with normal stubs, i.e. when New EXE header is located at
      offset > 0x40. However, in PC world exists files with wrong
      stubs, which are normal for Host OS. Hence beye must recognize
      them as normal New EXE files, despite the fact that DOS can
      not execute ones.
      Fixed by Kostya Nosov <k-nosov@yandex.ru>.
   */
   if(!( id[0] == 'M' && id[1] == 'Z' &&
	bmReadWordEx(0x18,BM_SEEK_SET) >= 0x40 &&
	(ret=bmReadDWordEx(0x3C,BM_SEEK_SET)) > 0x40L)) ret = 0;
#endif
   if(!( id[0] == 'M' && id[1] == 'Z' &&
	(ret=bmReadDWordEx(0x3C,BM_SEEK_SET)) > 0x02L)) ret = 0;
   return (__filesize_t)ret;
}

void BeyeContext::auto_detect_mode()
{
    size_t i,n = modes.size();
    Plugin* mode;
    for(i = 0;i < n;i++) {
	mode = modes[i]->query_interface(*code_guider);
	if(mode->detect()) {
	    defMainModeSel = i;
	    break;
	}
	delete mode; mode = NULL;
    }
    if(mode) delete mode;
    activeMode = modes[defMainModeSel]->query_interface(*code_guider);
    BMSeek(0,BM_SEEK_SET);
}

static const struct tagbeyeArg {
    const char key[4];
    const char *prompt;
}beyeArg[] = {
  { "-a", "autodetect mode (default)" },
  { "-b", "view file in binary mode" },
  { "-d", "view file in disassembler mode" },
  { "-h", "view file in hexadecimal mode" },
  { "-t", "view file in text mode" },
  { "-s", "change size of file to NNN bytes (create, if file does not exist)" },
  { "-i", "ignore .ini file (create new)" },
  { "-m", "debug mp_malloc: (1 - bounds; 2 - prebounds; 3-leaks)" },
  { "-k", "test anti-viral protection" },
  { "-?", "display this screen" }
};

int BeyeContext::queryKey(const std::string& arg)
{
  int ret = -1;
  size_t i;
  for(i = 0;i < sizeof(beyeArg)/sizeof(struct tagbeyeArg);i++)
  {
    if(arg==beyeArg[i].key) { ret = i; break; }
  }
  return ret;
}

void BeyeContext::parse_cmdline( const std::vector<std::string>& ArgVector )
{
  unsigned i;
  ListFile.clear();
  for(i = 1;i < ArgVector.size();i++)
  {
     int beye_key;
     beye_key = queryKey(ArgVector[i]);
     switch(beye_key)
     {
       case 0: beye_mode = UINT_MAX; break;
       case 1: beye_mode = 1; break;
       case 2: beye_mode = 3; break;
       case 3: beye_mode = 2; break;
       case 4: beye_mode = 0; break;
       case 5:
		new_file_size = strtoull(ArgVector[++i].c_str(),NULL,10);
		break;
       case 6: UseIniFile = false; break;
       case 7: i++; break; // parsed early
       case 8: i++; break; // parsed early
       case 9: ListFile.clear(); return;
       default: ListFile.push_back(ArgVector[i]);
     }
  }
  if(!ListFile.empty()) ArgVector1 = ListFile[0];
}

bool BeyeContext::LoadInfo( )
{
    make_shortname();
    if(new_file_size != FILESIZE_MAX) {
	bhandle_t handle;
	if(__IsFileExists(beye_context().ArgVector1.c_str()) == false) handle = __OsCreate(beye_context().ArgVector1.c_str());
	else {
	    handle = __OsOpen(beye_context().ArgVector1.c_str(),FO_READWRITE | SO_DENYNONE);
	    if(handle == NULL_HANDLE) handle = __OsOpen(ArgVector1.c_str(),FO_READWRITE | SO_COMPAT);
	}
	if(handle != NULL_HANDLE) {
	    __OsChSize(handle,new_file_size);
	    __OsClose(handle);
	} else {
	    errnoMessageBox(OPEN_FAIL,NULL,errno);
	    return false;
        }
    }
    if(BMOpen(ArgVector1) != true) return false;
    if(beye_mode != UINT_MAX) defMainModeSel = beye_mode;
    else {
	if(LastMode >= modes.size() || !is_valid_ini_args()) auto_detect_mode();
	else defMainModeSel = LastMode;
    }
    return true;
}

void BeyeContext::detect_binfmt()
{
 size_t i,sz=formats.size();
 if(!bmGetFLength())
 {
   detectedFormat = &binTable;
   return;
 }
 for(i = 0;i < sz;i++)
 {
   if(formats[i]->check_format())
   {
     detectedFormat = formats[i];
     if(detectedFormat->init) detectedFormat->init(*code_guider);
     break;
   }
 }
}

void BeyeContext::PaintTitle() const
{
 twUseWin(TitleWnd);
 twFreezeWin(TitleWnd);
 twGotoXY(1,1);
 twClrEOL();
 twPrintF("File : %s",beye_context().short_name());
 twGotoXY(twGetClientWidth(TitleWnd)-43,1);
 twPrintF("Size : %8llu bytes",BMGetFLength());
 twRefreshWin(TitleWnd);
}

static void MyAtExit( void )
{
  if(MainWnd) CloseWnd(MainWnd);
  if(HelpWnd) CloseWnd(HelpWnd);
  if(TitleWnd) CloseWnd(TitleWnd);
  if(ErrorWnd) CloseWnd(ErrorWnd);
  termBConsole();
  __term_sys();
  delete BeyeCtx;
  mp_uninit_malloc(malloc_debug?1:0);
}

bool BeyeContext::is_valid_ini_args( ) const
{
  return iniSettingsAnywhere ? true :
	 !ArgVector1.empty() ?
	 ArgVector1==LastOpenFileName ?
	 beye_mode != UINT_MAX && beye_mode != LastMode ?
	 false : true : false : false;
}

hIniProfile* BeyeContext::load_ini_info()
{
  char tmp[20], buf[20],stmp[4096];
  hIniProfile *ini;
  ini_name = getenv("BEYE_INI");
  if(!ini_name) ini_name = __get_ini_name("beye");
  ini = UseIniFile ? iniOpenFile(ini_name,NULL) : NULL;
  read_profile_string(ini,"Beye","Setup","HelpName","",stmp,sizeof(stmp));
  help_name=stmp;
  read_profile_string(ini,"Beye","Setup","SkinName","",stmp,sizeof(stmp));
  skin_name=stmp;
  read_profile_string(ini,"Beye","Setup","SyntaxName","",stmp,sizeof(stmp));
  syntax_name=stmp;
  read_profile_string(ini,"Beye","Search","String","",(char *)search_buff,sizeof(search_buff));
  search_len = strlen((char *)search_buff);
  read_profile_string(ini,"Beye","Search","Case","off",tmp,sizeof(tmp));
  beyeSearchFlg = stricmp(tmp,"on") == 0 ? SF_CASESENS : SF_NONE;
  read_profile_string(ini,"Beye","Search","Word","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) beyeSearchFlg |= SF_WORDONLY;
  read_profile_string(ini,"Beye","Search","Backward","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) beyeSearchFlg |= SF_REVERSE;
  read_profile_string(ini,"Beye","Search","Template","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) beyeSearchFlg |= SF_WILDCARDS;
  read_profile_string(ini,"Beye","Search","UsePlugin","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) beyeSearchFlg |= SF_PLUGINS;
  read_profile_string(ini,"Beye","Search","AsHex","off",tmp,sizeof(tmp));
  if(stricmp(tmp,"on") == 0) beyeSearchFlg |= SF_ASHEX;
  read_profile_string(ini,"Beye","Browser","LastOpen","",LastOpenFileName,4096);
  sprintf(buf,"%u",LastMode); /* [dBorca] so that src and dst won't overlap for strncpy */
  read_profile_string(ini,"Beye","Browser","LastMode",buf,tmp,sizeof(tmp));
  LastMode = (size_t)strtoul(tmp,NULL,10);
  read_profile_string(ini,"Beye","Browser","Offset","0",tmp,sizeof(tmp));
  LastOffset = atoll(tmp);
  read_profile_string(ini,"Beye","Setup","Version","",ini_ver,sizeof(ini_ver));
  read_profile_string(ini,"Beye","Setup","DirectConsole","yes",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) vioIniFlags = __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
  read_profile_string(ini,"Beye","Setup","ForceMono","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) twinIniFlags = TWIF_FORCEMONO;
  read_profile_string(ini,"Beye","Setup","Force7Bit","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) vioIniFlags |= __TVIO_FLG_USE_7BIT;
  read_profile_string(ini,"Beye","Setup","MouseSens","yes",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) kbdFlags = KBD_NONSTOP_ON_MOUSE_PRESS;
  read_profile_string(ini,"Beye","Setup","IniSettingsAnywhere","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) iniSettingsAnywhere = true;
  read_profile_string(ini,"Beye","Setup","FioUseMMF","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) fioUseMMF = true;
  if(!__mmfIsWorkable()) fioUseMMF = false;
  read_profile_string(ini,"Beye","Setup","PreserveTimeStamp","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) iniPreserveTime = true;
  read_profile_string(ini,"Beye","Setup","UseExternalProgs","no",tmp,sizeof(tmp));
  if(stricmp(tmp,"yes") == 0) iniUseExtProgs = true;
  read_profile_string(ini,"Beye","Setup","Codepage","CP866",stmp,sizeof(stmp));
  codepage=stmp;
  return ini;
}

void BeyeContext::save_ini_info() const
{
  char tmp[20];
  hIniProfile *ini;
  search_buff[search_len] = 0;
  ini = iniOpenFile(ini_name,NULL);
  write_profile_string(ini,"Beye","Setup","HelpName",help_name.c_str());
  write_profile_string(ini,"Beye","Setup","SkinName",skin_name.c_str());
  write_profile_string(ini,"Beye","Setup","SyntaxName",syntax_name.c_str());
  write_profile_string(ini,"Beye","Setup","Version",BEYE_VERSION);
  write_profile_string(ini,"Beye","Search","String",(char *)search_buff);
  write_profile_string(ini,"Beye","Search","Case",beyeSearchFlg & SF_CASESENS ? "on" : "off");
  write_profile_string(ini,"Beye","Search","Word",beyeSearchFlg & SF_WORDONLY ? "on" : "off");
  write_profile_string(ini,"Beye","Search","Backward",beyeSearchFlg & SF_REVERSE ? "on" : "off");
  write_profile_string(ini,"Beye","Search","Template",beyeSearchFlg & SF_WILDCARDS ? "on" : "off");
  write_profile_string(ini,"Beye","Search","UsePlugin",beyeSearchFlg & SF_PLUGINS ? "on" : "off");
  write_profile_string(ini,"Beye","Search","AsHex",beyeSearchFlg & SF_ASHEX ? "on" : "off");
  write_profile_string(ini,"Beye","Browser","LastOpen",ArgVector1.c_str());
  sprintf(tmp,"%u",defMainModeSel);
  write_profile_string(ini,"Beye","Browser","LastMode",tmp);
  sprintf(tmp,"%llu",LastOffset);
  write_profile_string(ini,"Beye","Browser","Offset",tmp);
  strcpy(tmp,vioIniFlags & __TVIO_FLG_DIRECT_CONSOLE_ACCESS ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","DirectConsole",tmp);
  strcpy(tmp,twinIniFlags & TWIF_FORCEMONO ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","ForceMono",tmp);
  strcpy(tmp,(vioIniFlags & __TVIO_FLG_USE_7BIT) == __TVIO_FLG_USE_7BIT ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","Force7Bit",tmp);
  strcpy(tmp,kbdFlags & KBD_NONSTOP_ON_MOUSE_PRESS ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","MouseSens",tmp);
  strcpy(tmp,iniSettingsAnywhere ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","IniSettingsAnywhere",tmp);
  strcpy(tmp,fioUseMMF ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","FioUseMMF",tmp);
  strcpy(tmp,iniPreserveTime ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","PreserveTimeStamp",tmp);
  strcpy(tmp,iniUseExtProgs ? "yes" : "no");
  write_profile_string(ini,"Beye","Setup","UseExternalProgs",tmp);
  write_profile_string(ini,"Beye","Setup","Codepage",codepage.c_str());
  activeMode->save_ini(ini);
  udnTerm(ini);
  iniCloseFile(ini);
}

static FTime ftim;
static bool ftim_ok = false;

void BeyeContext::show_usage() const {
    unsigned evt,i,nln,h,y;
    TWindow *win;
    nln = sizeof(beyeArg)/sizeof(struct tagbeyeArg);
    h = nln+4;
    y = tvioHeight/2-h/2;
    win = WindowOpen(2,y,tvioWidth-1,y+h,TWS_NONE | TWS_NLSOEM);
    if(!win) goto done;
    twSetTitleAttr(win,BEYE_VER_MSG,TW_TMODE_CENTER,error_cset.border);
    twCentredWin(win,NULL);
    twSetColorAttr(error_cset.main);
    twSetFrameAttr(win,TW_DOUBLE_FRAME,error_cset.border);
    twSetFooterAttr(win," Press [ ESC ] to quit ",TW_TMODE_RIGHT,error_cset.border);
    twClearWin();
    twGotoXY(1,1);
    twPutS(" Usage: beye [OPTIONS] file...");
    for(i = 0;i < nln;i++)
    {
	twGotoXY(1,4+i);
	twPrintF("  %s     %s\n",beyeArg[i].key,beyeArg[i].prompt);
    }
    twShowWin(win);
    do {
	evt = GetEvent(NULL,NULL,ErrorWnd);
    }while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_ENTER));
    twDestroyWin(win);
    done:
    termBConsole();
}

int Beye(const std::vector<std::string>& argv, const std::map<std::string,std::string>& envm)
{
 hIniProfile *ini;
 bool skin_err;
 int retval;
#ifndef NDEBUG
#ifdef RLIMIT_CORE
  {
    /* on many systems default coresize is 0.
       Enable any coresize here. */
    struct rlimit rl;
    getrlimit(RLIMIT_CORE,&rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_CORE,&rl);
  }
#endif
#endif
  BeyeCtx=new(zeromem) BeyeContext(argv,envm);
/*
    flg=MPA_FLG_RANDOMIZER;
    flg=MPA_FLG_BOUNDS_CHECK;
    flg=MPA_FLG_BEFORE_CHECK;
    flg=MPA_FLG_BACKTRACE;
*/
 __init_sys();
 ini=BeyeCtx->load_ini_info();
 skin_err = csetReadIniFile(BeyeCtx->skin_name.c_str());
 initBConsole(BeyeCtx->vioIniFlags,BeyeCtx->twinIniFlags);
 if(argv.size() < 2) goto show_usage;
 BeyeCtx->parse_cmdline(argv);
 if(BeyeCtx->list_file().empty())
 {
   /** print usage message */
    size_t i;
    show_usage:
    BeyeCtx->show_usage();
    std::cerr<<BEYE_VER_MSG<<std::endl;
    std::cerr<<" Usage: beye [OPTIONS] file...<<"<<std::endl<<std::endl;
    for(i = 0;i < sizeof(beyeArg)/sizeof(struct tagbeyeArg);i++) {
	std::cerr<<beyeArg[i].key<<" "<<beyeArg[i].prompt<<std::endl;
    }
    std::cerr<<std::endl;
    return EXIT_FAILURE;
 }
 udnInit(ini);
 ErrorWnd = WindowOpen(1,1,50,16,TWS_NONE | TWS_NLSOEM);
 if(ErrorWnd) twSetTitleAttr(ErrorWnd," Error ",TW_TMODE_CENTER,error_cset.border);
 else { std::cerr<<"fatal error: can't create window"<<std::endl; return EXIT_FAILURE; }
 twCentredWin(ErrorWnd,NULL);
 twSetColorAttr(error_cset.main);
 twSetFrameAttr(ErrorWnd,TW_DOUBLE_FRAME,error_cset.border);
 HelpWnd = WindowOpen(1,tvioHeight,tvioWidth,tvioHeight,TWS_NLSOEM);
 twSetColorAttr(prompt_cset.digit);
 twClearWin();
 twShowWin(HelpWnd);
 if(strcmp(BeyeCtx->ini_ver,BEYE_VERSION) != 0) Setup();
 TitleWnd = WindowOpen(1,1,tvioWidth,1,TWS_NONE);
 twSetColorAttr(title_cset.main);
 twClearWin();
 twShowWin(TitleWnd);
 atexit(MyAtExit);
 retval = EXIT_SUCCESS;
 if(skin_err)
 {
   char sout[256];
   sprintf(sout,"Error in skin file detected: '%s'",last_skin_error);
   ErrMessageBox(sout,NULL);
 }
 /* We must do it before opening a file because of some RTL has bug
    when are trying to open already open file with no sharing access */
 ftim_ok = __OsGetFTime(BeyeCtx->ArgVector1.c_str(),&ftim);
 if(!BeyeCtx->LoadInfo())
 {
   if(ini) iniCloseFile(ini);
   retval = EXIT_FAILURE;
   goto Bye;
 }
 BeyeCtx->detect_binfmt();
 BeyeCtx->init_modes(ini);
 if(ini) iniCloseFile(ini);
 MainWnd = WindowOpen(1,2,tvioWidth,tvioHeight-1,TWS_NONE);
 twSetColorAttr(browser_cset.main);
 twClearWin();
 BeyeCtx->PaintTitle();
 if(!BeyeCtx->is_valid_ini_args() || BeyeCtx->LastOffset > BMGetFLength()) BeyeCtx->LastOffset = 0;
 twShowWin(MainWnd);
 BeyeCtx->main_loop();
 BeyeCtx->LastOffset = BMGetCurrFilePos();
 BeyeCtx->save_ini_info();
 if(BeyeCtx->iniPreserveTime && ftim_ok) __OsSetFTime(BeyeCtx->ArgVector1.c_str(),&ftim);
 Bye:
 return retval;
}

bool BeyeContext::new_source()
{
  int i;
    bool ret;
    unsigned j,freq;
    static int prev_file;
    char ** nlsListFile;
    size_t sz=ListFile.size();
    nlsListFile = new char*[sz];
    if(nlsListFile) {
	for(j = 0;j < sz;j++) {
	    nlsListFile[j] = new char [ListFile[j].length()+1];
	    if(!nlsListFile[j]) break;
	}
    } else { MemOutBox("Initializing List of File\n"); return 0; }
    for(freq = 0;freq < j;freq++) {
	unsigned ls;
	ls = ListFile[freq].length();
	::memcpy(nlsListFile[freq],ListFile[freq].c_str(),ls+1);
	__nls_CmdlineToOem((unsigned char *)nlsListFile[freq],ls);
    }
    i = SelBoxA(nlsListFile,j," Select new file: ",prev_file);
    ret = 0;
    for(freq = 0;freq < j;freq++) delete nlsListFile[freq];
    delete nlsListFile;
    if(i != -1) {
	if(iniPreserveTime && ftim_ok) __OsSetFTime(ArgVector1.c_str(),&ftim);
	BMClose();
	ftim_ok = __OsGetFTime(ListFile[i].c_str(),&ftim);
	if(BMOpen(ListFile[i]) == true) {
	    ArgVector1 = ListFile[i];
	    if(detectedFormat->destroy) detectedFormat->destroy();
	    delete activeMode;
	    make_shortname();
	    detect_binfmt();
	    activeMode=modes[defMainModeSel]->query_interface(*code_guider);
	    ret = true;
	} else {
	    if(BMOpen(ArgVector1) != true) ::exit(EXIT_FAILURE);
	    if(detectedFormat->destroy) detectedFormat->destroy();
	    delete activeMode;
	    make_shortname();
	    detect_binfmt();
	    activeMode=modes[defMainModeSel]->query_interface(*code_guider);
	    ret = false;
	}
    }
    return ret;
}

unsigned BeyeContext::read_profile_string(hIniProfile *ini,
			       const char *section,
			       const char *subsection,
			       const char *_item,
			       const char *def_value,
			       char *buffer,
			       unsigned cbBuffer) const
{
  return UseIniFile ? iniReadProfileString(ini,section,subsection,
					   _item,def_value,buffer,cbBuffer)
		    : 1;
}

bool BeyeContext::write_profile_string(hIniProfile *ini,
					  const char *section,
					  const char *subsection,
					  const char *item,
					  const char *value) const
{
  return iniWriteProfileString(ini,section,subsection,item,value);
}


BeyeContext::BeyeContext(const std::vector<std::string>& _argv, const std::map<std::string,std::string>& _envm)
	    :vioIniFlags(0L),
	    twinIniFlags(0L),
	    kbdFlags(0L),
	    iniSettingsAnywhere(false),
	    fioUseMMF(false),
	    iniPreserveTime(false),
	    iniUseExtProgs(false),
	    headshift(0L),
	    LastOffset(0L),
	    detectedFormat(0),
	    argv(_argv),
	    envm(_envm),
	    UseIniFile(true),
	    beye_mode(UINT_MAX),
	    defMainModeSel(1),
	    new_file_size(FILESIZE_MAX),
	    code_guider(new(zeromem) CodeGuider),
	    bm_file_handle(bNull),
	    sc_bm_file_handle(bNull)
{
    addons = new(zeromem) addendum;
    sysinfo= new(zeromem) class sysinfo;

    modes.push_back(&textMode);
    modes.push_back(&binMode);
    modes.push_back(&hexMode);
    modes.push_back(&disMode);

    formats.push_back(&neTable);
    formats.push_back(&peTable);
    formats.push_back(&leTable);
    formats.push_back(&lxTable);
    formats.push_back(&nlm386Table);
    formats.push_back(&elf386Table);
    formats.push_back(&jvmTable);
    formats.push_back(&coff386Table);
    formats.push_back(&archTable);
    formats.push_back(&aoutTable);
    formats.push_back(&OldPharLapTable);
    formats.push_back(&PharLapTable);
    formats.push_back(&rdoffTable);
    formats.push_back(&rdoff2Table);
    formats.push_back(&lmfTable);
    formats.push_back(&sisTable);
    formats.push_back(&sisxTable);
    formats.push_back(&aviTable);
    formats.push_back(&asfTable);
    formats.push_back(&bmpTable);
    formats.push_back(&mpegTable);
    formats.push_back(&jpegTable);
    formats.push_back(&wavTable);
    formats.push_back(&movTable);
    formats.push_back(&rmTable);
    formats.push_back(&mp3Table);
    formats.push_back(&mzTable);
    formats.push_back(&dossysTable);
    formats.push_back(&binTable);

    codepage="CP866";
    scheme_name="Built-in";
    if(argv.size()>1) ArgVector1 = argv[1];
    LastOpenFileName = new char[4096];
    _shortname = new char[SHORT_PATH_LEN + 1];
    LastMode = modes.size()+10;
}

BeyeContext::~BeyeContext() {
    delete activeMode;
    if(active_format()->destroy) active_format()->destroy();

    delete sysinfo;
    delete addons;
    delete LastOpenFileName;
    delete _shortname;
    delete code_guider;
    BMClose();
}
const std::vector<std::string>& BeyeContext::list_file() const { return ListFile; }

void BeyeContext::select_tool() const { addons->select(); }
void BeyeContext::select_sysinfo() const { sysinfo->select(); }

BFile* BeyeContext::beyeOpenRO(const std::string& fname,unsigned cache_size)
{
  BFile* fret;
  fret=new BFile;
  bool rc;
  rc = fret->open(fname,FO_READONLY | SO_DENYNONE,cache_size,beye_context().fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
  if(rc == false)
    rc = fret->open(fname,FO_READONLY | SO_COMPAT,cache_size,beye_context().fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
  if(rc==false) { delete fret; fret=&bNull; }
  return fret;
}

BFile* BeyeContext:: beyeOpenRW(const std::string& fname,unsigned cache_size)
{
  BFile* fret;
  fret=new BFile;
  bool rc;
  rc = fret->open(fname,FO_READWRITE | SO_DENYNONE,cache_size,beye_context().fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
  if(rc == false)
    rc = fret->open(fname,FO_READWRITE | SO_COMPAT,cache_size,beye_context().fioUseMMF ? BFile::Opt_UseMMF : BFile::Opt_Db);
  if(rc==false) { delete fret; fret=&bNull; }
  return fret;
}

bool BeyeContext::BMOpen(const std::string& fname)
{
  BFile *bm,*sc;
  bm = beyeOpenRO(fname,BBIO_CACHE_SIZE);
  if(bm == &bNull)
  {
    errnoMessageBox(OPEN_FAIL,NULL,errno);
    return false;
  }
  if(&bm_file_handle != &bNull) delete &bm_file_handle;
  bm_file_handle = *bm;
  sc = bm_file_handle.dup_ex(BBIO_SMALL_CACHE_SIZE);
  if(sc == &bNull)
  {
    errnoMessageBox(DUP_FAIL,NULL,errno);
    return false;
  }
  if(&sc_bm_file_handle != &bNull) delete &sc_bm_file_handle;
  sc_bm_file_handle = *sc;
  bm_file_handle.set_optimization(BFile::Opt_Random);
  sc_bm_file_handle.set_optimization(BFile::Opt_Random);
  return true;
}

void BeyeContext::BMClose()
{
  if(&bm_file_handle != &bNull) delete &bm_file_handle;
  bm_file_handle = bNull;
  if(&sc_bm_file_handle != &bNull) delete &sc_bm_file_handle;
  sc_bm_file_handle = bNull;
}

static bool test_antiviral_protection(int* verbose)
{
    if(*verbose) std::cerr<<"Your've specified test-av option!\nRight now Beye should make coredump!"<<std::endl;
    *verbose=antiviral_hole1[0]|antiviral_hole2[0]|antiviral_hole3[0]|antiviral_hole4[0];
    std::cerr<<"Antiviral protection of Beye doesn't work!"<<std::endl;
    return false;
}
} // namespace beye

int main(int argc,char* args[], char *envp[])
{
    try {
	/* init malloc */
	int do_av_test=0;
	size_t i;
	mp_malloc_e flg=MPA_FLG_RANDOMIZER;
	malloc_debug=0;
	for(i=0;i<size_t(argc);i++) {
	    if(strcmp(args[i],"-m")==0) {
		i++;
		malloc_debug=::atoi(args[i]);
		switch(malloc_debug) {
		    default:
		    case 0: flg=MPA_FLG_RANDOMIZER; break;
		    case 1: flg=MPA_FLG_BOUNDS_CHECK; break;
		    case 2: flg=MPA_FLG_BEFORE_CHECK; break;
		    case 3: flg=MPA_FLG_BACKTRACE; break;
		}
		break;
	    }
	    if(strcmp(args[i],"-k")==0) do_av_test=1;
	}
	mp_init_malloc(args[0],1000,10,flg);
	/* init vectors */
	std::vector<std::string> ArgVector;
	std::string str,stmp;
	for(i=0;i<size_t(argc);i++) {
	    str=args[i];
	    ArgVector.push_back(str);
	}
	args[argc] = (char*)make_false_pointer((any_t*)antiviral_hole1);
	std::map<std::string,std::string> envm;
	unsigned j=0;
	size_t pos;
	while(envp[j]) {
	    str=envp[j++];
	    pos=str.find('=');
	    if(pos==std::string::npos) throw "Broken environment variable: "+str;
	    stmp=str.substr(pos+1);
	    str=str.substr(0,pos);
	    envm[str]=stmp;
	}
//	envp[j+1] = NULL;
#ifdef HAVE_MPROTECT
	/* init antiviral protection */
	int rc;
	rc=mp_mprotect((any_t*)antiviral_hole1,sizeof(antiviral_hole1),MP_DENY_ALL);
	rc|=mp_mprotect((any_t*)antiviral_hole2,sizeof(antiviral_hole2),MP_DENY_ALL);
	rc|=mp_mprotect((any_t*)antiviral_hole3,sizeof(antiviral_hole3),MP_DENY_ALL);
	rc|=mp_mprotect((any_t*)antiviral_hole4,sizeof(antiviral_hole4),MP_DENY_ALL);
	if(rc) {
		std::cerr<<"*** Error! Cannot initialize antiviral protection: '"<<strerror(errno)<<"' ***!"<<std::endl;
		return EXIT_FAILURE;
	}
#else
	std::cerr<<"*** Project was compiled without antiviral protection!"<<std::endl;
#endif
	if(do_av_test) {
	    test_antiviral_protection(&do_av_test);
	    std::cerr<<"*** Test of antiviral protection failed! ***"<<std::endl;
	    return EXIT_FAILURE;
	}
	/* call program */
	return Beye(ArgVector,envm);
    } catch(const std::string& what) {
	std::cout<<"[main_module] Exception '"<<what<<"'caught in module: MPlayerXP"<<std::endl;
    }
    return EXIT_FAILURE;
}
