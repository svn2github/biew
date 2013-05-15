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
#include "bin_util.h"
#include "plugins/disasm.h"
#include "plugins/bin/nlm.h"
#include "codeguid.h"
#include "bconsole.h"
#include "reg_form.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "beyehelp.h"
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
	    NLM_Parser(binary_stream&,CodeGuider&);
	    virtual ~NLM_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F1();
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();
	    virtual __filesize_t	action_F5();
	    virtual __filesize_t	action_F8();

	    virtual __filesize_t	show_header();
	    virtual bool		bind(const DisMode& _parent,std::string& str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift);
	    virtual int			query_platform() const;
	    virtual int			query_bitness(__filesize_t) const;
	    virtual bool		address_resolving(std::string&,__filesize_t);
	    virtual __filesize_t	va2pa(__filesize_t va);
	    virtual __filesize_t	pa2va(__filesize_t pa);
	    virtual __filesize_t	get_public_symbol(std::string& str,unsigned& _class,
							    __filesize_t pa,bool as_prev);
	    virtual unsigned		get_object_attribute(__filesize_t pa,std::string& name,
							__filesize_t& start,__filesize_t& end,int& _class,int& bitness);
	private:
	    std::string		nlm_ReadPubName(binary_stream&b_cache,const symbolic_information& it);
	    void		nlm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
	    int			NLMbitness(__filesize_t off);
	    bool		BuildReferStrNLM(std::string& str,const RELOC_NLM& rne,int flags);
	    void		BuildRelocNlm();
	    static tCompare	nlm_compare_f(const any_t *e1,const any_t *e2);
	    static tCompare	nlm_compare_s(const any_t *e1,const any_t *e2);
	    bool		__ReadModRefNamesNLM(binary_stream&handle,memArray *obj,unsigned nnames);
	    bool		NLMNamesReadItems(binary_stream&handle,memArray *obj,unsigned nnames);
	    __filesize_t	CalcEntryNLM(unsigned ord,bool dispmsg);
	    bool		__ReadExtRefNamesNLM(binary_stream&handle,memArray *obj,unsigned n);
	    bool		FindPubName(std::string& buff,__filesize_t pa);

	    Nlm_Internal_Fixed_Header nlm;
	    std::set<symbolic_information>	PubNames;
	    binary_stream*	nlm_cache;
	    char		__codelen;
	    std::set<RELOC_NLM>	RelocNlm;

	    binary_stream&	main_handle;
	    CodeGuider&		code_guider;
    };
static const char* txt[]={ "NlmHlp", "ModRef", "PubDef", "", "ExtNam", "", "", "NlmHdr", "", "" };
const char* NLM_Parser::prompt(unsigned idx) const { return txt[idx]; }

__filesize_t NLM_Parser::show_header()
{
  __filesize_t fpos;
  char modName[NLM_MODULE_NAME_SIZE];
  TWindow * w;
  unsigned keycode;
  fpos = beye_context().tell();
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
  fpos = beye_context().tell();
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

bool NLM_Parser::__ReadExtRefNamesNLM(binary_stream& handle,memArray * obj,unsigned n)
{
 unsigned i;
 handle.seek(nlm.nlm_externalReferencesOffset,binary_stream::Seek_Set);
 for(i = 0;i < n;i++)
 {
   char stmp[256];
   unsigned char length;
   unsigned long nrefs;
   length = handle.read(type_byte);
   if(IsKbdTerminate() || handle.eof()) break;
   handle.read(stmp,length);
   stmp[length] = 0;
   nrefs = handle.read(type_dword);
   handle.seek(nrefs*4,binary_stream::Seek_Cur);
   if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

__filesize_t NLM_Parser::CalcEntryNLM(unsigned ord,bool dispmsg)
{
 unsigned char length;
 unsigned i;
 __filesize_t ret,fpos,cpos;
 fpos = beye_context().tell();
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
    if(dispmsg) beye_context().ErrMessageBox(NO_ENTRY,"");
 }
 return ret;
}

bool NLM_Parser::NLMNamesReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
{
 unsigned char length;
 unsigned i;
 handle.seek(nlm.nlm_publicsOffset,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   char stmp[256];
   length = handle.read(type_byte);
   if(IsKbdTerminate() || handle.eof()) break;
   if(length > 66)
   {
     handle.read(stmp,66);
     handle.seek(length - 66,binary_stream::Seek_Cur);
     strcat(stmp,">>>");
   }
   else { handle.read(stmp,length); stmp[length] = 0; }
   handle.seek(4L,binary_stream::Seek_Cur);
   if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

__filesize_t NLM_Parser::action_F5()
{
    std::string title = EXT_REFER;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfExternalReferences;
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadExtRefNamesNLM(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return beye_context().tell();
}

bool NLM_Parser::__ReadModRefNamesNLM(binary_stream& handle,memArray * obj,unsigned nnames)
{
 unsigned char length;
 unsigned i;
 handle.seek(nlm.nlm_moduleDependencyOffset,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   char stmp[256];
   length = handle.read(type_byte);
   if(IsKbdTerminate() || handle.eof()) break;
   if(length > 66)
   {
     handle.read(stmp,66);
     handle.seek(length - 66,binary_stream::Seek_Cur);
     strcat(stmp,">>>");
   }
   else { handle.read(stmp,length); stmp[length] = 0; }
   if(!ma_AddString(obj,stmp,true)) break;
 }
 return true;
}

__filesize_t NLM_Parser::action_F2()
{
    std::string title = MOD_REFER;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfModuleDependencies;
    int flags = LB_SORTABLE;
    bool bval;
    memArray* obj;
    TWindow* w;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = __ReadModRefNamesNLM(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    return beye_context().tell();
}

__filesize_t NLM_Parser::action_F3()
{
    __filesize_t fpos = beye_context().tell();
    int ret;
    std::string title = EXP_TABLE;
    ssize_t nnames = (unsigned)nlm.nlm_numberOfPublics;
    int flags = LB_SELECTIVE;
    bool bval;
    memArray* obj;
    TWindow* w;
    ret = -1;
    if(!(obj = ma_Build(nnames,true))) goto exit;
    w = PleaseWaitWnd();
    bval = NLMNamesReadItems(main_handle,obj,nnames);
    delete w;
    if(bval) {
	if(!obj->nItems) { beye_context().NotifyBox(NOT_ENTRY,title); goto exit; }
	ret = ma_Display(obj,title,flags,-1);
    }
    ma_Destroy(obj);
    exit:
    if(ret != -1) fpos = CalcEntryNLM(ret,true);
    return fpos;
}

/***************************************************************************/
/************************  FOR NLM  ****************************************/
/***************************************************************************/
tCompare NLM_Parser::nlm_compare_s(const any_t*e1,const any_t*e2)
{
  const RELOC_NLM  *r1, *r2;
  r1 = reinterpret_cast<const RELOC_NLM*>(e1);
  r2 = reinterpret_cast<const RELOC_NLM*>(e2);
  return __CmpLong__(r1->offset,r2->offset);
}

tCompare NLM_Parser::nlm_compare_f(const any_t*e1,const any_t*e2)
{
  const RELOC_NLM  *r1, *r2;
  r1 = reinterpret_cast<const RELOC_NLM*>(e1);
  r2 = reinterpret_cast<const RELOC_NLM*>(e2);
  return __CmpLong__(r1->offset,r2->offset);
}

void NLM_Parser::BuildRelocNlm()
{
  unsigned i,j;
  unsigned long val,niter,noff;
  __filesize_t cpos;
  unsigned char len;
  TWindow * w;
  RELOC_NLM rel;
//  if(!(RelocNlm = la_Build(0,sizeof(RELOC_NLM),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(PubNames.empty()) nlm_ReadPubNameList(main_handle,MemOutBox);
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
  next:
//  la_Sort(RelocNlm,nlm_compare_s);
  delete w;
}

bool NLM_Parser::BuildReferStrNLM(std::string& str,const RELOC_NLM& rne,int flags)
{
  __filesize_t val;
  bool retrf;
  std::string name;
  binary_stream* b_cache;
  unsigned char len;
  b_cache = nlm_cache;
  b_cache->seek(rne.nameoff,binary_stream::Seek_Set);
  retrf = true;
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
    if(FindPubName(name,val))
    {
      str+=name;
    }
    else
     if(!(flags & APREF_SAVE_VIRT))
     {
       str+="(*this)+";
       str+=Get8Digit(val);
       retrf = true;
     }
     else retrf = false;
  }
  return retrf;
}

bool NLM_Parser::bind(const DisMode& parent,std::string& str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_NLM key;
  std::set<RELOC_NLM>::const_iterator rnlm;
  bool retrf;
  std::string buff;
  if(flags & APREF_TRY_PIC) return false;
  if(!nlm.nlm_numberOfExternalReferences || nlm.nlm_externalReferencesOffset >= main_handle.flength()) retrf = false;
  else
  {
    if(RelocNlm.empty()) BuildRelocNlm();
    key.offset = ulShift;
    __codelen = codelen;
    rnlm = RelocNlm.find(key);
    retrf = (rnlm!=RelocNlm.end()) ? BuildReferStrNLM(str,*rnlm,flags) : false;
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(PubNames.empty()) nlm_ReadPubNameList(main_handle,MemOutBox);
     if(FindPubName(buff,r_sh))
     {
       str+=buff;
       if(!DumpMode && !EditMode) code_guider.add_go_address(parent,str,r_sh);
       retrf = true;
     }
  }
  return retrf;
}

NLM_Parser::NLM_Parser(binary_stream& h,CodeGuider& _code_guider)
	    :Binary_Parser(h,_code_guider)
	    ,nlm_cache(&bNull)
	    ,main_handle(h)
	    ,code_guider(_code_guider)
{
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(&nlm,sizeof(Nlm_Internal_Fixed_Header));
    if((nlm_cache = main_handle.dup()) == &bNull) nlm_cache = &main_handle;
}

NLM_Parser::~NLM_Parser()
{
    if(nlm_cache != &bNull && nlm_cache != &main_handle) delete nlm_cache;
}

int NLM_Parser::query_bitness(__filesize_t off) const
{
    UNUSED(off);
    return DAB_USE32;
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
  hlpDisplay(10007);
  return beye_context().tell();
}

std::string NLM_Parser::nlm_ReadPubName(binary_stream& b_cache,const symbolic_information& it)
{
    unsigned char length;
    b_cache.seek(it.nameoff,binary_stream::Seek_Set);
    length = b_cache.read(type_byte);
    char buff[length+1];
    b_cache.read(buff,length);
    buff[length] = 0;
    return buff;
}

bool NLM_Parser::FindPubName(std::string& buff,__filesize_t pa)
{
  symbolic_information key;
  std::set<symbolic_information>::const_iterator ret;
  key.pa = pa;
  if(PubNames.empty()) nlm_ReadPubNameList(*nlm_cache,MemOutBox);
  ret = PubNames.find(key);
  if(ret!=PubNames.end()) {
    buff=nlm_ReadPubName(*nlm_cache,*ret);
    return true;
  }
  return udnFindName(pa,buff);
}

void NLM_Parser::nlm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
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
   nlm_pn.attr    = SC_GLOBAL;
   PubNames.insert(nlm_pn);
   if(handle.eof()) break;
 }
// if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

__filesize_t NLM_Parser::get_public_symbol(std::string& str,unsigned& func_class,
			   __filesize_t pa,bool as_prev)
{
    __filesize_t fpos;
    if(PubNames.empty()) nlm_ReadPubNameList(*nlm_cache,NULL);
    std::set<symbolic_information>::const_iterator idx;
    symbolic_information key;
    key.pa=pa;
    fpos=find_symbolic_information(PubNames,func_class,key,as_prev,idx);
    if(idx!=PubNames.end()) {
	str=nlm_ReadPubName(*nlm_cache,*idx);
    }
    return fpos;
}

unsigned NLM_Parser::get_object_attribute(__filesize_t pa,std::string& name,
		      __filesize_t& start,__filesize_t& end,int& _class,int& bitness)
{
  unsigned ret;
  start = 0;
  end = main_handle.flength();
  _class = OC_NOOBJECT;
  bitness = query_bitness(pa);
  name[0] = 0;
  if(pa < nlm.nlm_codeImageOffset)
  {
    end = nlm.nlm_codeImageOffset;
    ret = 0;
  }
  else
    if(pa >= nlm.nlm_codeImageOffset && pa < nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize)
    {
      start = nlm.nlm_codeImageOffset;
      _class = OC_CODE;
      end = nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize;
      ret = 1;
    }
    else
    if(pa >= nlm.nlm_dataImageOffset && pa < nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize)
    {
      _class = OC_DATA;
      start = nlm.nlm_dataImageOffset;
      end = start + nlm.nlm_dataImageSize;
      ret = 2;
    }
    else
    {
      _class = OC_NOOBJECT;
      start = nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize;
      ret = 3;
    }
  return ret;
}

__filesize_t NLM_Parser::va2pa(__filesize_t va)
{
  return va + std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
}

__filesize_t NLM_Parser::pa2va(__filesize_t pa)
{
  __filesize_t base = std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
  return pa > base ? pa - base : 0L;
}

int NLM_Parser::query_platform() const { return DISASM_CPU_IX86; }

static bool probe(binary_stream& main_handle) {
  char ctrl[NLM_SIGNATURE_SIZE];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(ctrl,NLM_SIGNATURE_SIZE);
  return memcmp(ctrl,NLM_SIGNATURE,NLM_SIGNATURE_SIZE) == 0;
}

static Binary_Parser* query_interface(binary_stream& h,CodeGuider& _parent) { return new(zeromem) NLM_Parser(h,_parent); }
extern const Binary_Parser_Info nlm_info = {
    "nlm-i386 (Novell Loadable Module)",	/**< plugin name */
    probe,
    query_interface
};
} // namespace	usr
