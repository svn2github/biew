#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/nlm386.c
 * @brief       This file contains implementation of NLM-32 (Novell Loadable
 *              Module) file format decoder.
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
#include <algorithm>
#include <set>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "colorset.h"
#include "udn.h"
#include "plugins/disasm.h"
#include "plugins/bin/nlm.h"
#include "codeguid.h"
#include "bconsole.h"
#include "tstrings.h"
#include "beyehelp.h"
#include "listbox.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
    struct RELOC_NLM {
	unsigned long offset;
	unsigned long nameoff; /** if refnum == -1 then internal */

	bool operator<(const RELOC_NLM& rhs) const { return offset<rhs.offset; }
    };
    class NLM_Parser : public Binary_Parser {
	public:
	    NLM_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~NLM_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F5();
	    virtual __filesize_t	action_F8();

	    virtual __filesize_t	show_header() const;
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,Bin_Format::bind_type flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual Bin_Format::bitness	query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va) const;
	    virtual __filesize_t	pa2va(__filesize_t pa) const;
	    virtual Symbol_Info		get_public_symbol(__filesize_t pa,bool as_prev);
	    virtual Object_Info		get_object_attribute(__filesize_t pa);
	private:
	    std::string		nlm_ReadPubName(binary_stream&b_cache,const symbolic_information& it) const;
	    void		nlm_ReadPubNameList(binary_stream& handle);
	    bool		BuildReferStrNLM(std::string& str,const RELOC_NLM& rne,Bin_Format::bind_type flags);
	    void		BuildRelocNlm();
	    std::vector<std::string> __ReadModRefNamesNLM(binary_stream& handle,size_t nnames) const;
	    std::vector<std::string> NLMNamesReadItems(binary_stream& handle,size_t nnames) const;
	    __filesize_t	CalcEntryNLM(unsigned ord,bool dispmsg) const;
	    std::vector<std::string>	__ReadExtRefNamesNLM(binary_stream& handle,size_t n) const;
	    Symbol_Info		FindPubName(__filesize_t pa) const;

	    Nlm_Internal_Fixed_Header nlm;
	    std::set<symbolic_information>	PubNames;
	    binary_stream*	nlm_cache;
	    char		__codelen;
	    std::set<RELOC_NLM>	RelocNlm;

	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    CodeGuider&		code_guider;
	    udn&		_udn;
    };
static const char* txt[]={ "NlmHlp", "ModRef", "PubDef", "", "ExtNam", "", "", "NlmHdr", "", "" };
const char* NLM_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t NLM_Parser::show_header() const
{
  __filesize_t fpos;
  char modName[NLM_MODULE_NAME_SIZE];
  TWindow * w;
  unsigned keycode;
  fpos = bctx.tell();
  w = CrtDlgWndnls(" NetWare Loadable Module ",59,23);
  w->goto_xy(1,1);
  strncpy(modName,(char *)&nlm.nlm_moduleName[1],(int)nlm.nlm_moduleName[0]);
  modName[(unsigned)nlm.nlm_moduleName[0]] = 0;
  w->printf(
	   "Module (Version)              = %s (%02hu.%02hu)\n"
	   "Code image offset             = %08lXH\n"
	   "Code image size               = %08lXH\n"
	   "Data image offset             = %08lXH\n"
	   "Data image size               = %08lXH\n"
	   "Uninitialized data size       = %08lXH\n"
	   "Custom data offset            = %08lXH\n"
	   "Custom data size              = %08lXH\n"
	   "Module dependency offset      = %08lXH\n"
	   "Number of Module dependencies = %08lXH\n"
	   "Relocation fixup offset       = %08lXH\n"
	   "Number of relocations fixup   = %08lXH\n"
	   "External reference offset     = %08lXH\n"
	   "Number of external references = %08lXH\n"
	   "Public offset                 = %08lXH\n"
	   "Number of public              = %08lXH\n"
	   "Debug info offset             = %08lXH\n"
	   "Number of debug records       = %08lXH\n"
	   ,modName
	   ,(unsigned short)nlm.nlm_version
	   ,(unsigned short)(nlm.nlm_version >> 16)
	   ,nlm.nlm_codeImageOffset
	   ,nlm.nlm_codeImageSize
	   ,nlm.nlm_dataImageOffset
	   ,nlm.nlm_dataImageSize
	   ,nlm.nlm_uninitializedDataSize
	   ,nlm.nlm_customDataOffset
	   ,nlm.nlm_customDataSize
	   ,nlm.nlm_moduleDependencyOffset
	   ,nlm.nlm_numberOfModuleDependencies
	   ,nlm.nlm_relocationFixupOffset
	   ,nlm.nlm_numberOfRelocationFixups
	   ,nlm.nlm_externalReferencesOffset
	   ,nlm.nlm_numberOfExternalReferences
	   ,nlm.nlm_publicsOffset
	   ,nlm.nlm_numberOfPublics
	   ,nlm.nlm_debugInfoOffset
	   ,nlm.nlm_numberOfDebugRecords);
  w->set_color(dialog_cset.entry);
  w->printf("Code start offset             = %08lXH [Enter]",nlm.nlm_codeStartOffset);
  w->clreol();
  w->set_color(dialog_cset.altentry);
  w->printf("\nExit procedure offset         = %08lXH [Ctrl+Enter | F5]",nlm.nlm_exitProcedureOffset);
  w->clreol();
  w->set_color(dialog_cset.main);
  w->printf("\nCheck unload procedure offset = %08lXH\n"
	   "Module type                   = %08lXH\n"
	   "Flags                         = %08lXH"
	   ,nlm.nlm_checkUnloadProcedureOffset
	   ,nlm.nlm_moduleType
	   ,nlm.nlm_flags);
  do
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER)
    {
      fpos = nlm.nlm_codeImageOffset + nlm.nlm_codeStartOffset;
      break;
    }
    else
      if(keycode == KE_CTL_ENTER || keycode == KE_F(5))
      {
	fpos = nlm.nlm_codeImageOffset + nlm.nlm_exitProcedureOffset;
	break;
      }
  }while(!(keycode == KE_ESCAPE || keycode == KE_F(10)));
  delete w;
  return fpos;
}

__filesize_t NLM_Parser::action_F8()
{
  __filesize_t fpos,ssize,m,d,sharedEntry,sharedExit;
  char modName[256];
  unsigned char len;
  TWindow *w;
  unsigned keycode;
  sharedEntry = sharedExit = 0;
  fpos = bctx.tell();
  w = CrtDlgWndnls(" NetWare Loadable Module ",74,23);
  w->goto_xy(1,1);
  main_handle.seek(sizeof(Nlm_Internal_Fixed_Header),binary_stream::Seek_Set);
  len = main_handle.read(type_byte);
  main_handle.read(modName,len + 1);
  ssize = main_handle.read(type_dword);
  main_handle.seek(4,binary_stream::Seek_Cur); /** skip reserved */
  w->printf("%s\n"
	   "Stack size                    = %08lXH\n"
	   ,modName
	   ,ssize);
  main_handle.read(modName,5);
  modName[5] = 0;
  w->printf("Old thread name               = %s\n",modName);
  len = main_handle.read(type_byte);
  main_handle.read(modName,len + 1);
  w->printf("Screen name                   = %s\n",modName);
  len = main_handle.read(type_byte);
  main_handle.read(modName,len + 1);
  w->printf("Thread name                   = %s",modName);
  while(1)
  {
    main_handle.read(modName,9);
    if(main_handle.eof()) break;
    modName[9] = 0;
    if(memcmp(modName,"VeRsIoN#",8) == 0)
    {
      main_handle.seek(-1,binary_stream::Seek_Cur);
      ssize = main_handle.read(type_dword);
      d = main_handle.read(type_dword);
      m = main_handle.read(type_dword);
      w->printf("\nVersion ( Revision )          = %lu.%lu ( %08lXH )\n",ssize,d,m);
      ssize = main_handle.read(type_dword);
      m     = main_handle.read(type_dword);
      d     = main_handle.read(type_dword);
      w->printf("Date (DD.MM.YY)               = %lu.%lu.%lu",d,m,ssize);
    }
    else
      if(memcmp(modName,"CoPyRiGhT",9) == 0)
      {
	main_handle.seek(1,binary_stream::Seek_Cur);
	len = main_handle.read(type_byte);
	main_handle.read(modName,len + 1);
	w->printf("\nCopyright = %s",modName);
      }
      else
	if(memcmp(modName,"MeSsAgEs",8) == 0)
	{
	  main_handle.seek(-1,binary_stream::Seek_Cur);
	  ssize = main_handle.read(type_dword);
	  w->printf("\nLanguage                      = %08lXH\n",ssize);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  d = main_handle.read(type_dword);
	  w->printf("Messages (offset/length/count)= %08lXH/%08lXH/%08lXH\n",ssize,m,d);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  d = main_handle.read(type_dword);
	  w->printf("Help (offset/length/dataOff)  = %08lXH/%08lXH/%08lXH\n",ssize,m,d);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedCode (offset/length)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedData (offset/length)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedReloc (offset/count)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedExtRef (offset/count)   = %08lXH/%08lXH\n",ssize,m);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedPublics (offset/count)  = %08lXH/%08lXH\n",ssize,m);
	  ssize = main_handle.read(type_dword);
	  m = main_handle.read(type_dword);
	  w->printf("SharedDebugRec (offset/count) = %08lXH/%08lXH\n",ssize,m);
	  sharedEntry = main_handle.read(type_dword);
	  w->set_color(dialog_cset.entry);
	  w->printf("Shared initialization offset  = %08lXH [Enter]",sharedEntry);
	  w->printf("\n"); w->clreol();
	  sharedExit = main_handle.read(type_dword);
	  w->set_color(dialog_cset.altentry);
	  w->printf("Shared exit procedure offset  = %08lXH [Ctrl+Enter | F5]",sharedExit);
	  w->printf("\n"); w->clreol();
	  w->set_color(dialog_cset.main);
	  ssize = main_handle.read(type_dword);
	  w->printf("Product ID                    = %08lXH",ssize);
	}
	else
	  if(memcmp(modName,"CuStHeAd",8) == 0)
	  {
	    unsigned long hdr;
	    main_handle.seek(-1,binary_stream::Seek_Cur);
	    ssize = main_handle.read(type_dword);
	    d = main_handle.read(type_dword);
	    m = main_handle.read(type_dword);
	    main_handle.read(modName,8);
	    modName[8] = 0;
	    hdr = main_handle.read(type_dword);
	    w->printf("\nCustHead (name/hdrOff/hdrLen/dataOff/dataLen) = %s/%08lXH/%08lXH/%08lXH/%08lHX",modName,hdr,ssize,d,m);
	  }
	  else
	    if(memcmp(modName,"CyGnUsEx",8) == 0)
	    {
	      main_handle.seek(-1,binary_stream::Seek_Cur);
	      d = main_handle.read(type_dword);
	      m = main_handle.read(type_dword);
	      w->printf("\nCygnus (offset/length) = %08lXH/%08lXH",d,m);
	    }
	    else break;
  }
  do
  {
    keycode = GetEvent(drawEmptyPrompt,NULL,w);
    if(keycode == KE_ENTER)
    {
       if(sharedEntry) fpos = sharedEntry + nlm.nlm_codeImageOffset;
       break;
    }
    else
      if((keycode == KE_CTL_ENTER || keycode == KE_F(5)) && sharedExit)
      {
	fpos = sharedExit + nlm.nlm_codeImageOffset;
	break;
      }
  }while(!(keycode == KE_ESCAPE || keycode == KE_F(10)));
  delete w;
  return fpos;
}

std::vector<std::string> NLM_Parser::__ReadExtRefNamesNLM(binary_stream& handle,size_t n) const
{
    std::vector<std::string> rc;
    size_t i;
    handle.seek(nlm.nlm_externalReferencesOffset,binary_stream::Seek_Set);
    for(i = 0;i < n;i++) {
	char stmp[256];
	unsigned char length;
	unsigned long nrefs;
	length = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	handle.read(stmp,length);
	stmp[length] = 0;
	nrefs = handle.read(type_dword);
	handle.seek(nrefs*4,binary_stream::Seek_Cur);
	rc.push_back(stmp);
    }
    return rc;
}

__filesize_t NLM_Parser::CalcEntryNLM(unsigned ord,bool dispmsg) const
{
 unsigned char length;
 unsigned i;
 __filesize_t ret,fpos,cpos;
 fpos = bctx.tell();
 cpos = nlm.nlm_publicsOffset;
 for(i = 0;i < ord;i++)
 {
    main_handle.seek(cpos,binary_stream::Seek_Set);
    length = main_handle.read(type_byte); cpos+=length + 5;
 }
 main_handle.seek(cpos,binary_stream::Seek_Set);
 length = main_handle.read(type_byte);
 cpos+=length + 1;
 main_handle.seek(cpos,binary_stream::Seek_Set);
 ret = main_handle.read(type_dword);
 ret &= 0x00FFFFFFL;
 ret += nlm.nlm_codeImageOffset;
 main_handle.seek(fpos,binary_stream::Seek_Set);
 if(ret > main_handle.flength())
 {
    ret = fpos;
    if(dispmsg) bctx.ErrMessageBox(NO_ENTRY,"");
 }
 return ret;
}

std::vector<std::string> NLM_Parser::NLMNamesReadItems(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned char length;
    unsigned i;
    handle.seek(nlm.nlm_publicsOffset,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	char stmp[256];
	length = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	if(length > 66) {
	    handle.read(stmp,66);
	    handle.seek(length - 66,binary_stream::Seek_Cur);
	    strcat(stmp,">>>");
	} else { handle.read(stmp,length); stmp[length] = 0; }
	handle.seek(4L,binary_stream::Seek_Cur);
	rc.push_back(stmp);
    }
    return rc;
}

__filesize_t NLM_Parser::action_F5()
{
    std::string title = EXT_REFER;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfExternalReferences;
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadExtRefNamesNLM(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return bctx.tell();
}

std::vector<std::string> NLM_Parser::__ReadModRefNamesNLM(binary_stream& handle,size_t nnames) const
{
    std::vector<std::string> rc;
    unsigned char length;
    unsigned i;
    handle.seek(nlm.nlm_moduleDependencyOffset,binary_stream::Seek_Set);
    for(i = 0;i < nnames;i++) {
	char stmp[256];
	length = handle.read(type_byte);
	if(IsKbdTerminate() || handle.eof()) break;
	if(length > 66) {
	    handle.read(stmp,66);
	    handle.seek(length - 66,binary_stream::Seek_Cur);
	    strcat(stmp,">>>");
	}
	else { handle.read(stmp,length); stmp[length] = 0; }
	rc.push_back(stmp);
    }
    return rc;
}

__filesize_t NLM_Parser::action_F2()
{
    std::string title = MOD_REFER;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfModuleDependencies;
    ListBox::flags flags = ListBox::Sortable;
    TWindow* w = PleaseWaitWnd();
    std::vector<std::string> objs = __ReadModRefNamesNLM(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    lb.run(objs,title,flags,-1);
exit:
    return bctx.tell();
}

__filesize_t NLM_Parser::action_F3()
{
    __filesize_t fpos = bctx.tell();
    int ret;
    std::string title = EXP_TABLE;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfPublics;
    ListBox::flags flags = ListBox::Selective;
    TWindow* w;
    ret = -1;
    w = PleaseWaitWnd();
    std::vector<std::string> objs = NLMNamesReadItems(main_handle,nnames);
    delete w;
    ListBox lb(bctx);
    if(objs.empty()) { bctx.NotifyBox(NOT_ENTRY,title); goto exit; }
    ret = lb.run(objs,title,flags,-1);
exit:
    if(ret != -1) fpos = CalcEntryNLM(ret,true);
    return fpos;
}

/***************************************************************************/
/************************  FOR NLM  ****************************************/
/***************************************************************************/
void NLM_Parser::BuildRelocNlm()
{
  unsigned i,j;
  unsigned long val,niter,noff;
  __filesize_t cpos;
  unsigned char len;
  TWindow * w;
  RELOC_NLM rel;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(PubNames.empty()) nlm_ReadPubNameList(main_handle);
  w->goto_xy(1,1);
  w->puts(BUILD_REFS);
  /** -- for external references */
  cpos = nlm.nlm_externalReferencesOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfExternalReferences;j++)
  {
    bool is_eof;
    noff = cpos;
    is_eof = false;
    main_handle.seek(cpos,binary_stream::Seek_Set);
    len = main_handle.read(type_byte);
    cpos += len + 1;
    main_handle.seek(cpos,binary_stream::Seek_Set);
    niter = main_handle.read(type_dword);
    for(i = 0;i < niter;i++)
    {
      val = main_handle.read(type_dword);
      if((is_eof = main_handle.eof()) != 0) break;
      rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
      rel.nameoff = noff;
      RelocNlm.insert(rel);
    }
    if(is_eof) break;
  }
  /** -- for internal references */
  cpos = nlm.nlm_relocationFixupOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfRelocationFixups;j++)
  {
    val = main_handle.read(type_dword);
    if(main_handle.eof()) break;
    rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
    rel.nameoff = -1;
    RelocNlm.insert(rel);
  }
//  la_Sort(RelocNlm,nlm_compare_s);
  delete w;
}

bool NLM_Parser::BuildReferStrNLM(std::string& str,const RELOC_NLM& rne,Bin_Format::bind_type flags)
{
  __filesize_t val;
  bool retrf;
  binary_stream* b_cache;
  unsigned char len;
  b_cache = nlm_cache;
  b_cache->seek(rne.nameoff,binary_stream::Seek_Set);
  retrf = true;
  if(PubNames.empty()) nlm_ReadPubNameList(*nlm_cache);
  if(rne.nameoff != 0xFFFFFFFFUL)
  {
    len = b_cache->read(type_byte);
    char stmp[len+1];
    b_cache->read(stmp,len);
    stmp[len] = 0;
    str+=stmp;
  }
  else
  {
    main_handle.seek(rne.offset,binary_stream::Seek_Set);
    val = main_handle.read(type_dword);
    Symbol_Info rc = FindPubName(val);
    if(rc.pa!=Plugin::Bad_Address) str+=rc.name;
    else if(!(flags & Bin_Format::Save_Virt)) {
       str+="(*this)+";
       str+=Get8Digit(val);
       retrf = true;
    }
    else retrf = false;
  }
  return retrf;
}

bool NLM_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,Bin_Format::bind_type flags,int codelen,__filesize_t r_sh)
{
  RELOC_NLM key;
  std::set<RELOC_NLM>::const_iterator rnlm;
  bool retrf;
  if(flags & Bin_Format::Try_Pic) return false;
  if(!nlm.nlm_numberOfExternalReferences || nlm.nlm_externalReferencesOffset >= main_handle.flength()) retrf = false;
  else
  {
    if(RelocNlm.empty()) BuildRelocNlm();
    key.offset = ulShift;
    __codelen = codelen;
    rnlm = RelocNlm.find(key);
    retrf = (rnlm!=RelocNlm.end()) ? BuildReferStrNLM(str,*rnlm,flags) : false;
  }
  if(!retrf && (flags & Bin_Format::Try_Label))
  {
     if(PubNames.empty()) nlm_ReadPubNameList(main_handle);
     Symbol_Info rc = FindPubName(r_sh);
     if(rc.pa!=Plugin::Bad_Address) {
       str+=rc.name;
       if(!DumpMode && !EditMode) code_guider.add_go_address(parent,str,r_sh);
       retrf = true;
     }
  }
  return retrf;
}

NLM_Parser::NLM_Parser(BeyeContext& b,binary_stream& h,CodeGuider& _code_guider,udn& u)
	    :Binary_Parser(b,h,_code_guider,u)
	    ,nlm_cache(&h)
	    ,bctx(b)
	    ,main_handle(h)
	    ,code_guider(_code_guider)
	    ,_udn(u)
{
    char ctrl[NLM_SIGNATURE_SIZE];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(ctrl,NLM_SIGNATURE_SIZE);
    if(memcmp(ctrl,NLM_SIGNATURE,NLM_SIGNATURE_SIZE) != 0) throw bad_format_exception();

    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&nlm,sizeof(Nlm_Internal_Fixed_Header));
    nlm_cache = main_handle.dup();
}

NLM_Parser::~NLM_Parser()
{
    if(nlm_cache != &main_handle) delete nlm_cache;
}

Bin_Format::bitness NLM_Parser::query_bitness(__filesize_t off) const
{
    UNUSED(off);
    return Bin_Format::Use32;
}

bool NLM_Parser::address_resolving(std::string& addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos < sizeof(Nlm_Internal_Fixed_Header))
  {
    addr="nlm32h:";
    addr+=Get2Digit(cfpos);
  }
  else
    if((res=pa2va(cfpos)) != 0)
    {
      addr = ".";
      addr+=Get8Digit(res);
    }
    else bret = false;
  return bret;
}

__filesize_t NLM_Parser::action_F1()
{
    Beye_Help bhelp(bctx);
    if(bhelp.open(true)) {
	bhelp.run(10007);
	bhelp.close();
    }
    return bctx.tell();
}

std::string NLM_Parser::nlm_ReadPubName(binary_stream& b_cache,const symbolic_information& it) const
{
    unsigned char length;
    b_cache.seek(it.nameoff,binary_stream::Seek_Set);
    length = b_cache.read(type_byte);
    char buff[length+1];
    b_cache.read(buff,length);
    buff[length] = 0;
    return buff;
}

Symbol_Info NLM_Parser::FindPubName(__filesize_t pa) const
{
    Symbol_Info rc;
    symbolic_information key;
    std::set<symbolic_information>::const_iterator it;
    key.pa = pa;
    it = PubNames.find(key);
    if(it!=PubNames.end()) {
	rc.pa=pa;
	rc.name=nlm_ReadPubName(*nlm_cache,*it);
	return rc;
    }
    return _udn.find(pa);
}

void NLM_Parser::nlm_ReadPubNameList(binary_stream& handle)
{
 unsigned char length;
 unsigned i;
 unsigned nnames = (unsigned)nlm.nlm_numberOfPublics;
// if(!PubNames)
 handle.seek(nlm.nlm_publicsOffset,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   symbolic_information nlm_pn;
   nlm_pn.nameoff = handle.tell();
   length         = handle.read(type_byte);
   handle.seek(length,binary_stream::Seek_Cur);
   nlm_pn.pa      = (handle.read(type_dword) & 0x00FFFFFFL) + nlm.nlm_codeImageOffset;
   nlm_pn.attr    = Symbol_Info::Global;
   PubNames.insert(nlm_pn);
   if(handle.eof()) break;
 }
}

Symbol_Info NLM_Parser::get_public_symbol(__filesize_t pa,bool as_prev)
{
    Symbol_Info rc;
    if(PubNames.empty()) nlm_ReadPubNameList(*nlm_cache);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    rc=find_symbolic_information(PubNames,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	rc.name=nlm_ReadPubName(*nlm_cache,*idx);
    }
    return rc;
}

Object_Info NLM_Parser::get_object_attribute(__filesize_t pa)
{
    Object_Info rc;
    rc.start = 0;
    rc.end = main_handle.flength();
    rc._class = Object_Info::NoObject;
    rc.bitness = query_bitness(pa);
    rc.number = 0;
    if(pa < nlm.nlm_codeImageOffset) {
	rc.end = nlm.nlm_codeImageOffset;
	rc.number = 0;
    } else if(pa >= nlm.nlm_codeImageOffset && pa < nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize) {
	rc.start = nlm.nlm_codeImageOffset;
	rc._class = Object_Info::Code;
	rc.end = nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize;
	rc.number = 1;
    } else if(pa >= nlm.nlm_dataImageOffset && pa < nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize) {
	rc._class = Object_Info::Data;
	rc.start = nlm.nlm_dataImageOffset;
	rc.end = rc.start + nlm.nlm_dataImageSize;
	rc.number = 2;
    } else {
	rc._class = Object_Info::NoObject;
	rc.start = nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize;
	rc.number = 3;
    }
    return rc;
}

__filesize_t NLM_Parser::va2pa(__filesize_t va) const
{
    return va + std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
}

__filesize_t NLM_Parser::pa2va(__filesize_t pa) const
{
    __filesize_t base = std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
    return pa > base ? pa - base : 0L;
}

int NLM_Parser::query_platform() const { return DISASM_CPU_IX86; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) NLM_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info nlm_info = {
    "nlm-i386 (Novell Loadable Module)",	/**< plugin name */
    query_interface
};
} // namespace	usr
