#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
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
#include <stdexcept>

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
#include "udn.h"
#include "codeguid.h"
#include "editor.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "search.h"
#include "setup.h"
#include "libbeye/bbio.h"
#include "libbeye/mmfile.h"
#include "libbeye/file_ini.h"
#include "libbeye/kbd_code.h"
#include "libbeye/libbeye.h"
#include "libbeye/osdep/system.h"
#include "libbeye/osdep/tconsole.h"

#include "plugins/plugin.h"

namespace	usr {
static const unsigned SHORT_PATH_LEN=__TVIO_MAXSCREENWIDTH-54;
    struct beye_priv : public Opaque {
	beye_priv(BeyeContext& bctx,const std::vector<std::string>& _argv, const std::map<std::string,std::string>& _envm);
	virtual ~beye_priv();
	Plugin*			activeMode;
	Bin_Format*		_bin_format;
	const std::vector<std::string>& argv;
	const std::map<std::string,std::string>& envm;
	std::vector<std::string> ListFile;
	std::string		LastOpenFileName;
	char*			_shortname;
	std::string		ini_name;
	bool			UseIniFile;
	size_t			LastMode;
	unsigned int		beye_mode;
	unsigned		defMainModeSel;
	__filesize_t		new_file_size;
	std::vector<const Plugin_Info*> modes;
	Search*			search;
	CodeGuider*		code_guider;
	addendum*		addons;
	class sysinfo*		sysinfo;
	binary_stream*		bm_file_handle;
	binary_stream*		sc_bm_file_handle;
	TConsole*		_tconsole;
	LocalPtr<System>	_system;
	TWindow*		ErrorWnd;
	TWindow*		TitleWnd;
	TWindow*		PromptWnd;
	TWindow*		MainWnd;
	class udn		udn;
    };

beye_priv::beye_priv(BeyeContext& bctx,const std::vector<std::string>& _argv, const std::map<std::string,std::string>& _envm)
	:argv(_argv)
	,envm(_envm)
	,UseIniFile(true)
	,beye_mode(UINT_MAX)
	,defMainModeSel(1)
	,new_file_size(FILESIZE_MAX)
	,search(new(zeromem) Search(bctx))
	,code_guider(new(zeromem) CodeGuider(bctx))
	,_system(new(zeromem) System)
	,udn(bctx) {
    addons = new(zeromem) addendum(bctx);
    sysinfo= new(zeromem) class sysinfo(bctx);
    _shortname = new char[SHORT_PATH_LEN + 1];
}
beye_priv::~beye_priv() {
    delete activeMode;
    delete _bin_format;

    delete sysinfo;
    delete addons;
    delete _shortname;
    delete code_guider;
    delete search;

    if(MainWnd) delete MainWnd;
    if(PromptWnd) delete PromptWnd;
    if(TitleWnd) delete TitleWnd;
    if(ErrorWnd) delete ErrorWnd;
}

extern const Plugin_Info binMode;
extern const Plugin_Info textMode;
extern const Plugin_Info hexMode;
extern const Plugin_Info disMode;

static volatile char antiviral_hole1[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

static Opaque		opaque1;
static BeyeContext* BeyeCtx=NULL;
static Opaque		opaque2;

static volatile char antiviral_hole2[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;


static volatile char antiviral_hole3[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

static int malloc_debug=0;

static volatile char antiviral_hole4[__VM_PAGE_SIZE__] __PAGE_ALIGNED__;

BeyeContext& beye_context() { return *BeyeCtx; }

bool BeyeContext::select_mode()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    size_t i,nModes = priv.modes.size();
    const char *modeName[nModes];
    int retval;

    for(i = 0;i < nModes;i++) modeName[i] = priv.modes[i]->name;
    retval = ListBox(modeName,nModes," Select translation mode: ",LB_SELECTIVE|LB_USEACC,priv.defMainModeSel);
    if(retval != -1) {
	priv.defMainModeSel = retval;
	delete priv.activeMode;
	priv.activeMode = priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
	return true;
    }
    return false;
}

void BeyeContext::init_modes( Ini_Profile& ini )
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    if(!priv.activeMode) priv.activeMode = priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
    priv.activeMode->read_ini(ini);
}

void BeyeContext::quick_select_mode()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    size_t nModes = priv.modes.size();
    if(priv.defMainModeSel < nModes - 1) priv.defMainModeSel++;
    else                            priv.defMainModeSel = 0;
    delete priv.activeMode;
    priv.activeMode = priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
}

void BeyeContext::make_shortname()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
  unsigned l;
  unsigned slen = priv.TitleWnd->client_width()-54;
  l = ArgVector1.length();
  if(l <= slen) strcpy(priv._shortname,ArgVector1.c_str());
  else
  {
    strncpy(priv._shortname,ArgVector1.c_str(),slen/2 - 3);
    priv._shortname[slen/2-4] = 0;
    strcat(priv._shortname,"...");
    strcat(priv._shortname,&ArgVector1.c_str()[l - slen/2]);
  }
  priv._system->nls_cmdline2oem((unsigned char *)priv._shortname,strlen(priv._shortname));
}

void BeyeContext::auto_detect_mode()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    size_t i,n = priv.modes.size();
    Plugin* mode;
    for(i = 0;i < n;i++) {
	mode = priv.modes[i]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
	if(mode->detect()) {
	    priv.defMainModeSel = i;
	    break;
	}
	delete mode; mode = NULL;
    }
    if(mode) delete mode;
    priv.activeMode = priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
    bm_file().seek(0,binary_stream::Seek_Set);
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
    beye_priv& priv = static_cast<beye_priv&>(opaque);
  unsigned i;
  priv.ListFile.clear();
  for(i = 1;i < ArgVector.size();i++)
  {
     int beye_key;
     beye_key = queryKey(ArgVector[i]);
     switch(beye_key)
     {
       case 0: priv.beye_mode = UINT_MAX; break;
       case 1: priv.beye_mode = 1; break;
       case 2: priv.beye_mode = 3; break;
       case 3: priv.beye_mode = 2; break;
       case 4: priv.beye_mode = 0; break;
       case 5:
		priv.new_file_size = strtoull(ArgVector[++i].c_str(),NULL,10);
		break;
       case 6: priv.UseIniFile = false; break;
       case 7: i++; break; // parsed early
       case 8: i++; break; // parsed early
       case 9: priv.ListFile.clear(); return;
       default: priv.ListFile.push_back(ArgVector[i]);
     }
  }
  if(!priv.ListFile.empty()) ArgVector1 = priv.ListFile[0];
}

bool BeyeContext::LoadInfo( )
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    make_shortname();
    if(priv.new_file_size != FILESIZE_MAX) {
	binary_stream* h = new(zeromem) binary_stream;
	if(binary_stream::exists(ArgVector1) == false) h->create(ArgVector1);
	else {
	    if(!h->open(ArgVector1,binary_stream::FO_READWRITE | binary_stream::SO_DENYNONE))
		h->open(ArgVector1,binary_stream::FO_READWRITE | binary_stream::SO_COMPAT);
	}
	h->chsize(priv.new_file_size);
        delete h;
    }
    if(BMOpen(ArgVector1) != true) return false;
    if(priv.beye_mode != UINT_MAX) priv.defMainModeSel = priv.beye_mode;
    else {
	if(priv.LastMode >= priv.modes.size() || !is_valid_ini_args()) auto_detect_mode();
	else priv.defMainModeSel = priv.LastMode;
    }
    return true;
}

void BeyeContext::PaintTitle() const
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv.TitleWnd->freeze();
    priv.TitleWnd->goto_xy(1,1);
    priv.TitleWnd->clreol();
    priv.TitleWnd->printf("File : %s",short_name());
    priv.TitleWnd->goto_xy(priv.TitleWnd->client_width()-43,1);
    priv.TitleWnd->printf("Size : %8llu bytes",flength());
    priv.TitleWnd->refresh();
}

static void MyAtExit()
{
  delete BeyeCtx;
  mp_uninit_malloc(malloc_debug?1:0);
}

bool BeyeContext::is_valid_ini_args( ) const
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
  return iniSettingsAnywhere ? true :
	 !ArgVector1.empty() ?
	 ArgVector1==priv.LastOpenFileName ?
	 priv.beye_mode != UINT_MAX && priv.beye_mode != priv.LastMode ?
	 false : true : false : false;
}

Ini_Profile& BeyeContext::load_ini_info()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    Search& s = *priv.search;
    char buf[20];
    std::string tmp,stmp;
    Ini_Profile& ini = *new(zeromem) Ini_Profile;
    const char* iname;

    iname = ::getenv("BEYE_INI");
    if(iname) priv.ini_name=iname;
    if(priv.ini_name.empty()) priv.ini_name = priv._system->get_ini_name("beye");
    if(priv.UseIniFile) ini.open(priv.ini_name);
    help_name=read_profile_string(ini,"Beye","Setup","HelpName","");
    skin_name=read_profile_string(ini,"Beye","Setup","SkinName","");
    syntax_name=read_profile_string(ini,"Beye","Setup","SyntaxName","");

    s.read_ini(ini);

    priv.LastOpenFileName=read_profile_string(ini,"Beye","Browser","LastOpen","");
    sprintf(buf,"%u",priv.LastMode); /* [dBorca] so that src and dst won't overlap for strncpy */
    tmp=read_profile_string(ini,"Beye","Browser","LastMode",buf);
    priv.LastMode = (size_t)strtoul(tmp.c_str(),NULL,10);
    tmp=read_profile_string(ini,"Beye","Browser","Offset","0");
    LastOffset = atoll(tmp.c_str());
    ini_ver=read_profile_string(ini,"Beye","Setup","Version","");
    tmp=read_profile_string(ini,"Beye","Setup","DirectConsole","yes");
    if(stricmp(tmp.c_str(),"yes") == 0) vioIniFlags = __TVIO_FLG_DIRECT_CONSOLE_ACCESS;
    tmp=read_profile_string(ini,"Beye","Setup","ForceMono","no");
    if(stricmp(tmp.c_str(),"yes") == 0) twinIniFlags = TWIF_FORCEMONO;
    tmp=read_profile_string(ini,"Beye","Setup","Force7Bit","no");
    if(stricmp(tmp.c_str(),"yes") == 0) vioIniFlags |= __TVIO_FLG_USE_7BIT;
    tmp=read_profile_string(ini,"Beye","Setup","MouseSens","yes");
    if(stricmp(tmp.c_str(),"yes") == 0) kbdFlags = KBD_NONSTOP_ON_MOUSE_PRESS;
    tmp=read_profile_string(ini,"Beye","Setup","IniSettingsAnywhere","no");
    if(stricmp(tmp.c_str(),"yes") == 0) iniSettingsAnywhere = true;
    tmp=read_profile_string(ini,"Beye","Setup","FioUseMMF","no");
    if(stricmp(tmp.c_str(),"yes") == 0) fioUseMMF = true;
    if(!MMFile::has_mmio) fioUseMMF = false;
    tmp=read_profile_string(ini,"Beye","Setup","PreserveTimeStamp","no");
    if(stricmp(tmp.c_str(),"yes") == 0) iniPreserveTime = true;
    tmp=read_profile_string(ini,"Beye","Setup","UseExternalProgs","no");
    if(stricmp(tmp.c_str(),"yes") == 0) iniUseExtProgs = true;
    codepage=read_profile_string(ini,"Beye","Setup","Codepage","CP866");
    priv.udn.read_ini(ini);
    return ini;
}

void BeyeContext::save_ini_info() const
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    Search& s = *priv.search;
    char tmp[20];
    unsigned char search_buff[MAX_SEARCH_SIZE];
    unsigned char search_len = s.length();
    ::memcpy(search_buff,s.buff(),search_len);
    search_buff[search_len] = 0;
    Ini_Profile& ini = *new(zeromem) Ini_Profile;
    ini.open(priv.ini_name);
    write_profile_string(ini,"Beye","Setup","HelpName",help_name.c_str());
    write_profile_string(ini,"Beye","Setup","SkinName",skin_name.c_str());
    write_profile_string(ini,"Beye","Setup","SyntaxName",syntax_name.c_str());
    write_profile_string(ini,"Beye","Setup","Version",BEYE_VERSION);
    write_profile_string(ini,"Beye","Browser","LastOpen",ArgVector1.c_str());
    sprintf(tmp,"%u",priv.defMainModeSel);
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

    s.save_ini(ini);

    priv.activeMode->save_ini(ini);
    priv.udn.save_ini(ini);
    delete &ini;
}

static binary_stream::ftime ftim;
static bool ftim_ok = false;

void BeyeContext::show_usage() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    unsigned evt,i,nln,h,y;
    TWindow *win;
    nln = sizeof(beyeArg)/sizeof(struct tagbeyeArg);
    h = nln+4;
    y = priv._tconsole->vio_height()/2-h/2;
    win = new(zeromem) TWindow(2,y,priv._tconsole->vio_width()-2,h+1,TWindow::Flag_None | TWindow::Flag_NLS);
    if(!win) goto done;
    win->set_title(BEYE_VER_MSG,TWindow::TMode_Center,error_cset.border);
    win->into_center();
    win->set_color(error_cset.main);
    win->set_frame(TWindow::DOUBLE_FRAME,error_cset.border);
    win->set_footer(" Press [ ESC ] to quit ",TWindow::TMode_Right,error_cset.border);
    win->clear();
    win->goto_xy(1,1);
    win->puts(" Usage: beye [OPTIONS] file...");
    for(i = 0;i < nln;i++)
    {
	win->goto_xy(1,4+i);
	win->printf("  %s     %s\n",beyeArg[i].key,beyeArg[i].prompt);
    }
    win->show();
    do {
	evt = GetEvent(NULL,NULL,priv.ErrorWnd);
    }while(!(evt == KE_ESCAPE || evt == KE_F(10) || evt == KE_ENTER));
    delete win;
    done:
    termBConsole();
}

void	BeyeContext::create_windows() {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv.ErrorWnd = new(zeromem) TWindow(1,1,51,17,TWindow::Flag_None | TWindow::Flag_NLS);
    if(priv.ErrorWnd) priv.ErrorWnd->set_title(" Error ",TWindow::TMode_Center,error_cset.border);
    else throw std::runtime_error("fatal error: can't create window");
    priv.ErrorWnd->into_center();
    priv.ErrorWnd->set_color(error_cset.main);
    priv.ErrorWnd->set_frame(TWindow::DOUBLE_FRAME,error_cset.border);
    priv.PromptWnd = new(zeromem) TWindow(1,priv._tconsole->vio_height(),priv._tconsole->vio_width()-1,1,TWindow::Flag_NLS);
    priv.PromptWnd->set_color(prompt_cset.digit);
    priv.PromptWnd->clear();
    priv.PromptWnd->show();
    if(BeyeCtx->ini_ver!=BEYE_VERSION) {
	class Setup* setup = new class Setup(*this);
	setup->run();
	delete setup;
    }
    priv.TitleWnd = new(zeromem) TWindow(1,1,priv._tconsole->vio_width(),1,TWindow::Flag_None);
    priv.TitleWnd->set_color(title_cset.main);
    priv.TitleWnd->clear();
    priv.TitleWnd->show();
    priv.MainWnd = new(zeromem) TWindow(1,2,priv._tconsole->vio_width(),priv._tconsole->vio_height()-2,TWindow::Flag_None);
    priv.MainWnd->set_color(browser_cset.main);
    priv.MainWnd->clear();
}

int Beye(const std::vector<std::string>& argv, const std::map<std::string,std::string>& envm)
{
    bool skin_err;
    int retval;
#ifndef NDEBUG
#ifdef RLIMIT_CORE
    /* on many systems default coresize is 0.
       Enable any coresize here. */
    struct rlimit rl;
    getrlimit(RLIMIT_CORE,&rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_CORE,&rl);
#endif
#endif
    BeyeCtx=new(zeromem) BeyeContext(argv,envm);
/*
    flg=MPA_FLG_RANDOMIZER;
    flg=MPA_FLG_BOUNDS_CHECK;
    flg=MPA_FLG_BEFORE_CHECK;
    flg=MPA_FLG_BACKTRACE;
*/
    Ini_Profile& ini=BeyeCtx->load_ini_info();
    skin_err = csetReadIniFile(BeyeCtx->skin_name.c_str());
    initBConsole(BeyeCtx->vioIniFlags,BeyeCtx->twinIniFlags);
    if(argv.size() < 2) goto show_usage;
    BeyeCtx->parse_cmdline(argv);
    if(BeyeCtx->list_file().empty()) {
	/** print usage message */
	size_t i;
show_usage:
	BeyeCtx->show_usage();
	std::cerr<<BEYE_VER_MSG<<std::endl;
	std::cerr<<" Usage: beye [OPTIONS] file...<<"<<std::endl<<std::endl;
	for(i = 0;i < sizeof(beyeArg)/sizeof(struct tagbeyeArg);i++)
	    std::cerr<<beyeArg[i].key<<" "<<beyeArg[i].prompt<<std::endl;
	std::cerr<<std::endl;
	return EXIT_FAILURE;
    }
    BeyeCtx->create_windows();
    atexit(MyAtExit);
    retval = EXIT_SUCCESS;
    if(skin_err) {
	char sout[256];
	sprintf(sout,"Error in skin file detected: '%s'",BeyeCtx->last_skin_error.c_str());
	BeyeCtx->ErrMessageBox(sout,"");
    }
    /* We must do it before opening a file because of some RTL has bug
	when are trying to open already open file with no sharing access */
    ftim_ok = binary_stream::get_ftime(BeyeCtx->ArgVector1,ftim);
    if(!BeyeCtx->LoadInfo()) {
	delete &ini;
	retval = EXIT_FAILURE;
	goto Bye;
    }
    BeyeCtx->detect_format(BeyeCtx->sc_bm_file());
    BeyeCtx->init_modes(ini);
    delete &ini;
    BeyeCtx->PaintTitle();
    if(!BeyeCtx->is_valid_ini_args() || BeyeCtx->LastOffset > BeyeCtx->flength()) BeyeCtx->LastOffset = 0;
    BeyeCtx->main_wnd().show();
    BeyeCtx->main_loop();
    BeyeCtx->LastOffset = BeyeCtx->tell();
    BeyeCtx->save_ini_info();
    if(BeyeCtx->iniPreserveTime && ftim_ok) binary_stream::set_ftime(BeyeCtx->ArgVector1,ftim);
Bye:
    return retval;
}

bool BeyeContext::new_source()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    int i;
    bool ret;
    unsigned j,freq;
    static int prev_file;
    char ** nlsListFile;
    size_t sz=priv.ListFile.size();
    nlsListFile = new char*[sz];
    for(j = 0;j < sz;j++) nlsListFile[j] = new char [priv.ListFile[j].length()+1];
    for(freq = 0;freq < j;freq++) {
	unsigned ls;
	ls = priv.ListFile[freq].length();
	::memcpy(nlsListFile[freq],priv.ListFile[freq].c_str(),ls+1);
	priv._system->nls_cmdline2oem((unsigned char *)nlsListFile[freq],ls);
    }
    i = ListBox(const_cast<const char**>(nlsListFile),j," Select new file: ",LB_SELECTIVE|LB_USEACC,prev_file);
    ret = 0;
    for(freq = 0;freq < j;freq++) delete nlsListFile[freq];
    delete nlsListFile;
    if(i != -1) {
	if(iniPreserveTime && ftim_ok) binary_stream::set_ftime(ArgVector1,ftim);
	BMClose();
	ftim_ok = binary_stream::get_ftime(priv.ListFile[i],ftim);
	if(BMOpen(priv.ListFile[i]) == true) {
	    ArgVector1 = priv.ListFile[i];
	    delete priv._bin_format;
	    delete priv.activeMode;
	    make_shortname();
	    priv._bin_format = new(zeromem) Bin_Format(*this,*priv.code_guider,priv.udn);
	    priv._bin_format->detect_format(*priv.sc_bm_file_handle);
	    priv.activeMode=priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
	    ret = true;
	} else {
	    if(BMOpen(ArgVector1) != true) throw std::runtime_error("Can't open :"+ArgVector1);
	    delete priv._bin_format;
	    delete priv.activeMode;
	    make_shortname();
	    priv._bin_format = new(zeromem) Bin_Format(*this,*priv.code_guider,priv.udn);
	    priv._bin_format->detect_format(*priv.sc_bm_file_handle);
	    priv.activeMode=priv.modes[priv.defMainModeSel]->query_interface(*this,bin_format(),*priv.bm_file_handle,*priv.MainWnd,*priv.code_guider,priv.udn,*priv.search);
	    ret = false;
	}
    }
    return ret;
}

std::string BeyeContext::read_profile_string(Ini_Profile& ini,
			       const std::string& section,
			       const std::string& subsection,
			       const std::string& _item,
			       const std::string& def_value) const
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.UseIniFile ? ini.read(section,subsection,_item,def_value)
			: def_value;
}

bool BeyeContext::write_profile_string(Ini_Profile& ini,
					  const std::string& section,
					  const std::string& subsection,
					  const std::string& item,
					  const std::string& value) const
{
    return ini.write(section,subsection,item,value);
}

BeyeContext::BeyeContext(const std::vector<std::string>& _argv, const std::map<std::string,std::string>& _envm)
	    :vioIniFlags(0L)
	    ,twinIniFlags(0L)
	    ,kbdFlags(0L)
	    ,iniSettingsAnywhere(false)
	    ,fioUseMMF(false)
	    ,iniPreserveTime(false)
	    ,iniUseExtProgs(false)
	    ,LastOffset(0L)
	    ,opaque(*new(zeromem) beye_priv(*this,_argv,_envm))
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv.modes.push_back(&textMode);
    priv.modes.push_back(&binMode);
    priv.modes.push_back(&hexMode);
    priv.modes.push_back(&disMode);

    priv._bin_format = new(zeromem) Bin_Format(*this,*priv.code_guider,priv.udn);

    codepage="CP866";
    scheme_name="Built-in";
    if(priv.argv.size()>1) ArgVector1 = priv.argv[1];
    priv.LastMode = priv.modes.size()+10;
}

BeyeContext::~BeyeContext() {
    BMClose();
    termBConsole();
}
const std::vector<std::string>& BeyeContext::list_file() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.ListFile;
}
void BeyeContext::select_tool() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv.addons->select();
}
void BeyeContext::select_sysinfo() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv.sysinfo->select();
}

binary_stream* BeyeContext::beyeOpenRO(const std::string& fname,unsigned cache_size)
{
    BeyeContext& bctx = beye_context();
    binary_stream* fret;
    if(!binary_stream::exists(fname)) return NULL;
    if(bctx.fioUseMMF) fret= new(zeromem) MMFile;
//    else			fret= new(zeromem) BBio_File(cache_size,BBio_File::Opt_Db);
    else			fret= new(zeromem) binary_stream;
    bool rc;
    rc = fret->open(fname,binary_stream::FO_READONLY | binary_stream::SO_DENYNONE);
    if(rc == false)
	rc = fret->open(fname,binary_stream::FO_READONLY | binary_stream::SO_COMPAT);
    if(rc==false) { delete fret; fret=NULL; }
    return fret;
}

binary_stream* BeyeContext::beyeOpenRW(const std::string& fname,unsigned cache_size)
{
    BeyeContext& bctx = beye_context();
    binary_stream* fret;
    if(!binary_stream::exists(fname)) return NULL;
    fret= bctx.fioUseMMF? new(zeromem) MMFile :
//				    new(zeromem) BBio_File(cache_size,BBio_File::Opt_Db);
				    new(zeromem) binary_stream;
    bool rc;
    rc = fret->open(fname,binary_stream::FO_READWRITE | binary_stream::SO_DENYNONE);
    if(rc == false)
	rc = fret->open(fname,binary_stream::FO_READWRITE | binary_stream::SO_COMPAT);
    if(rc==false) { delete fret; fret=NULL; }
    return fret;
}

bool BeyeContext::BMOpen(const std::string& fname)
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
  binary_stream *bm,*sc;
  bm = beyeOpenRO(fname,BBIO_CACHE_SIZE);
  if(bm == NULL) {
    errnoMessageBox(OPEN_FAIL,"",errno);
    return false;
  }
  if(priv.bm_file_handle != NULL) delete priv.bm_file_handle;
  priv.bm_file_handle = bm;
  sc = priv.bm_file_handle->dup();
  if(sc == NULL) {
    errnoMessageBox(DUP_FAIL,"",errno);
    return false;
  }
  if(priv.sc_bm_file_handle != NULL) delete priv.sc_bm_file_handle;
  priv.sc_bm_file_handle = sc;
  return true;
}

void BeyeContext::BMClose()
{
    beye_priv& priv = static_cast<beye_priv&>(opaque);
  if(priv.bm_file_handle != NULL) delete priv.bm_file_handle;
  if(priv.sc_bm_file_handle != NULL && priv.sc_bm_file_handle!=priv.bm_file_handle) delete priv.sc_bm_file_handle;
}

__filesize_t BeyeContext::flength() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.bm_file_handle->flength();
}
__filesize_t BeyeContext::tell() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.bm_file_handle->tell();
}
bool BeyeContext::is_file64() const {
    return flength() > std::numeric_limits<uint32_t>::max();
}

const char* BeyeContext::short_name() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv._shortname;
}

Plugin& BeyeContext::active_mode() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.activeMode;
}
const Bin_Format& BeyeContext::bin_format() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv._bin_format;
}
const Plugin_Info* BeyeContext::mode_info() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.modes[priv.defMainModeSel];
}

TConsole& BeyeContext::tconsole() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv._tconsole;
}
System& BeyeContext::system() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv._system;
}

Search& BeyeContext::search() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.search;
}

CodeGuider& BeyeContext::codeguider() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.code_guider;
}

TWindow& BeyeContext::main_wnd() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.MainWnd;
}

TWindow& BeyeContext::error_wnd() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.ErrorWnd;
}

TWindow& BeyeContext::title_wnd() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.TitleWnd;
}

TWindow& BeyeContext::prompt_wnd() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.PromptWnd;
}

binary_stream& BeyeContext::bm_file() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.bm_file_handle;
}
binary_stream& BeyeContext::sc_bm_file() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return *priv.sc_bm_file_handle;
}

void BeyeContext::detect_format(binary_stream& handle) {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    priv._bin_format->detect_format(handle);
}

udn& BeyeContext::_udn() const {
    beye_priv& priv = static_cast<beye_priv&>(opaque);
    return priv.udn;
}

static bool test_antiviral_protection(int* verbose)
{
    if(*verbose) std::cerr<<"Your've specified test-av option!\nRight now Beye should make coredump!"<<std::endl;
    *verbose=antiviral_hole1[0]|antiviral_hole2[0]|antiviral_hole3[0]|antiviral_hole4[0];
    std::cerr<<"Antiviral protection of Beye doesn't work!"<<std::endl;
    return false;
}
} // namespace	usr

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
