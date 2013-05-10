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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "colorset.h"
#include "bin_util.h"
#include "plugins/disasm.h"
#include "plugins/bin/nlm.h"
#include "codeguid.h"
#include "bmfile.h"
#include "bconsole.h"
#include "reg_form.h"
#include "tstrings.h"
#include "beyeutil.h"
#include "beyehelp.h"
#include "libbeye/libbeye.h"
#include "libbeye/kbd_code.h"

namespace	usr {
static CodeGuider* code_guider;
static Nlm_Internal_Fixed_Header nlm;
static linearArray *PubNames = NULL;

static bool  __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa);
static void __FASTCALL__ nlm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&));
static __filesize_t __FASTCALL__ NLMPA2VA(__filesize_t pa);

static binary_stream* nlm_cache = &bNull;

static __filesize_t __FASTCALL__ ShowNLMHeader()
{
  __filesize_t fpos;
  char modName[NLM_MODULE_NAME_SIZE];
  TWindow * w;
  unsigned keycode;
  fpos = BMGetCurrFilePos();
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

static __filesize_t __FASTCALL__ ShowNewNLM()
{
  __filesize_t fpos,ssize,m,d,sharedEntry,sharedExit;
  char modName[256];
  unsigned char len;
  TWindow *w;
  unsigned keycode;
  sharedEntry = sharedExit = 0;
  fpos = BMGetCurrFilePos();
  w = CrtDlgWndnls(" NetWare Loadable Module ",74,23);
  w->goto_xy(1,1);
  bmSeek(sizeof(Nlm_Internal_Fixed_Header),binary_stream::Seek_Set);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  ssize = bmReadDWord();
  bmSeek(4,binary_stream::Seek_Cur); /** skip reserved */
  w->printf("%s\n"
	   "Stack size                    = %08lXH\n"
	   ,modName
	   ,ssize);
  bmReadBuffer(modName,5);
  modName[5] = 0;
  w->printf("Old thread name               = %s\n",modName);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  w->printf("Screen name                   = %s\n",modName);
  len = bmReadByte();
  bmReadBuffer(modName,len + 1);
  w->printf("Thread name                   = %s",modName);
  while(1)
  {
    bmReadBuffer(modName,9);
    if(bmEOF()) break;
    modName[9] = 0;
    if(memcmp(modName,"VeRsIoN#",8) == 0)
    {
      bmSeek(-1,binary_stream::Seek_Cur);
      ssize = bmReadDWord();
      d = bmReadDWord();
      m = bmReadDWord();
      w->printf("\nVersion ( Revision )          = %lu.%lu ( %08lXH )\n",ssize,d,m);
      ssize = bmReadDWord();
      m     = bmReadDWord();
      d     = bmReadDWord();
      w->printf("Date (DD.MM.YY)               = %lu.%lu.%lu",d,m,ssize);
    }
    else
      if(memcmp(modName,"CoPyRiGhT",9) == 0)
      {
	bmSeek(1,binary_stream::Seek_Cur);
	len = bmReadByte();
	bmReadBuffer(modName,len + 1);
	w->printf("\nCopyright = %s",modName);
      }
      else
	if(memcmp(modName,"MeSsAgEs",8) == 0)
	{
	  bmSeek(-1,binary_stream::Seek_Cur);
	  ssize = bmReadDWord();
	  w->printf("\nLanguage                      = %08lXH\n",ssize);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  d = bmReadDWord();
	  w->printf("Messages (offset/length/count)= %08lXH/%08lXH/%08lXH\n",ssize,m,d);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  d = bmReadDWord();
	  w->printf("Help (offset/length/dataOff)  = %08lXH/%08lXH/%08lXH\n",ssize,m,d);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedCode (offset/length)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedData (offset/length)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedReloc (offset/count)    = %08lXH/%08lXH\n",ssize,m);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedExtRef (offset/count)   = %08lXH/%08lXH\n",ssize,m);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedPublics (offset/count)  = %08lXH/%08lXH\n",ssize,m);
	  ssize = bmReadDWord();
	  m = bmReadDWord();
	  w->printf("SharedDebugRec (offset/count) = %08lXH/%08lXH\n",ssize,m);
	  sharedEntry = bmReadDWord();
	  w->set_color(dialog_cset.entry);
	  w->printf("Shared initialization offset  = %08lXH [Enter]",sharedEntry);
	  w->printf("\n"); w->clreol();
	  sharedExit = bmReadDWord();
	  w->set_color(dialog_cset.altentry);
	  w->printf("Shared exit procedure offset  = %08lXH [Ctrl+Enter | F5]",sharedExit);
	  w->printf("\n"); w->clreol();
	  w->set_color(dialog_cset.main);
	  ssize = bmReadDWord();
	  w->printf("Product ID                    = %08lXH",ssize);
	}
	else
	  if(memcmp(modName,"CuStHeAd",8) == 0)
	  {
	    unsigned long hdr;
	    bmSeek(-1,binary_stream::Seek_Cur);
	    ssize = bmReadDWord();
	    d = bmReadDWord();
	    m = bmReadDWord();
	    bmReadBuffer(modName,8);
	    modName[8] = 0;
	    hdr = bmReadDWord();
	    w->printf("\nCustHead (name/hdrOff/hdrLen/dataOff/dataLen) = %s/%08lXH/%08lXH/%08lXH/%08lHX",modName,hdr,ssize,d,m);
	  }
	  else
	    if(memcmp(modName,"CyGnUsEx",8) == 0)
	    {
	      bmSeek(-1,binary_stream::Seek_Cur);
	      d = bmReadDWord();
	      m = bmReadDWord();
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

static bool __FASTCALL__ __ReadExtRefNamesNLM(binary_stream& handle,memArray * obj,unsigned n)
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

static __filesize_t  __FASTCALL__ CalcEntryNLM(unsigned ord,bool dispmsg)
{
 unsigned char length;
 unsigned i;
 __filesize_t ret,fpos,cpos;
 fpos = BMGetCurrFilePos();
 cpos = nlm.nlm_publicsOffset;
 for(i = 0;i < ord;i++)
 {
   length = bmReadByteEx(cpos,binary_stream::Seek_Set); cpos+=length + 5;
 }
 length = bmReadByteEx(cpos,binary_stream::Seek_Set); cpos+=length + 1;
 ret = bmReadDWordEx(cpos,binary_stream::Seek_Set);
 ret &= 0x00FFFFFFL;
 ret += nlm.nlm_codeImageOffset;
 bmSeek(fpos,binary_stream::Seek_Set);
 if(ret > bmGetFLength())
 {
    ret = fpos;
    if(dispmsg) ErrMessageBox(NO_ENTRY,"");
 }
 return ret;
}

static bool __FASTCALL__ NLMNamesReadItems(binary_stream& handle,memArray * obj,unsigned nnames)
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

static __filesize_t __FASTCALL__ ShowExtRefNLM()
{
  fmtShowList((unsigned)nlm.nlm_numberOfExternalReferences,
	      __ReadExtRefNamesNLM,
	      EXT_REFER,
	      LB_SORTABLE,
	      NULL);
   return BMGetCurrFilePos();
}

static bool __FASTCALL__ __ReadModRefNamesNLM(binary_stream& handle,memArray * obj,unsigned nnames)
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


static __filesize_t __FASTCALL__ ShowModRefNLM()
{
  fmtShowList((unsigned)nlm.nlm_numberOfModuleDependencies,
	      __ReadModRefNamesNLM,
	      MOD_REFER,
	      LB_SORTABLE,
	      NULL);
   return BMGetCurrFilePos();
}

static __filesize_t __FASTCALL__ ShowPubNamNLM()
{
  __filesize_t fpos = BMGetCurrFilePos();
  int ret;
  ret = fmtShowList((unsigned)nlm.nlm_numberOfPublics,NLMNamesReadItems,
		    EXP_TABLE,
		    LB_SELECTIVE,NULL);
  if(ret != -1)
  {
    fpos = CalcEntryNLM(ret,true);
  }
  return fpos;
}

/***************************************************************************/
/************************  FOR NLM  ****************************************/
/***************************************************************************/
static char __codelen;

typedef struct tagRELOC_NLM
{
  unsigned long offset;
  unsigned long nameoff; /** if refnum == -1 then internal */
}RELOC_NLM;

static linearArray *RelocNlm = NULL;

static tCompare __FASTCALL__ nlm_compare_s(const any_t*e1,const any_t*e2)
{
  const RELOC_NLM  *r1, *r2;
  r1 = reinterpret_cast<const RELOC_NLM*>(e1);
  r2 = reinterpret_cast<const RELOC_NLM*>(e2);
  return __CmpLong__(r1->offset,r2->offset);
}

static tCompare __FASTCALL__ nlm_compare_f(const any_t*e1,const any_t*e2)
{
  const RELOC_NLM  *r1, *r2;
  r1 = reinterpret_cast<const RELOC_NLM*>(e1);
  r2 = reinterpret_cast<const RELOC_NLM*>(e2);
  return __CmpLong__(r1->offset,r2->offset);
}

static void  __FASTCALL__ BuildRelocNlm()
{
  unsigned i,j;
  unsigned long val,niter,noff;
  __filesize_t cpos;
  unsigned char len;
  TWindow * w;
  RELOC_NLM rel;
  if(!(RelocNlm = la_Build(0,sizeof(RELOC_NLM),MemOutBox))) return;
  w = CrtDlgWndnls(SYSTEM_BUSY,49,1);
  if(!PubNames) nlm_ReadPubNameList(bmbioHandle(),MemOutBox);
  w->goto_xy(1,1);
  w->puts(BUILD_REFS);
  /** -- for external references */
  cpos = nlm.nlm_externalReferencesOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfExternalReferences;j++)
  {
    bool is_eof;
    noff = cpos;
    is_eof = false;
    len = bmReadByteEx(cpos,binary_stream::Seek_Set); cpos += len + 1;
    niter = bmReadDWordEx(cpos,binary_stream::Seek_Set); cpos += 4;
    for(i = 0;i < niter;i++)
    {
      val = bmReadDWordEx(cpos,binary_stream::Seek_Set); cpos += 4;
      if((is_eof = bmEOF()) != 0) break;
      rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
      rel.nameoff = noff;
      if(!la_AddData(RelocNlm,&rel,MemOutBox)) goto next;
    }
    if(is_eof) break;
  }
  /** -- for internal references */
  cpos = nlm.nlm_relocationFixupOffset;
  for(j = 0;j < (unsigned)nlm.nlm_numberOfRelocationFixups;j++)
  {
    val = bmReadDWord();
    if(bmEOF()) break;
    rel.offset = (val&0x00FFFFFFL) + nlm.nlm_codeImageOffset;
    rel.nameoff = -1;
    if(!la_AddData(RelocNlm,&rel,MemOutBox)) break;
  }
  next:
  la_Sort(RelocNlm,nlm_compare_s);
  delete w;
}

static bool  __FASTCALL__ BuildReferStrNLM(char *str,RELOC_NLM*rne,int flags)
{
  __filesize_t val;
  bool retrf;
  char name[256];
  binary_stream* b_cache;
  unsigned char len;
  b_cache = nlm_cache;
  b_cache->seek(rne->nameoff,binary_stream::Seek_Set);
  retrf = true;
  if(rne->nameoff != 0xFFFFFFFFUL)
  {
    len = b_cache->read(type_byte);
    b_cache->read(name,len);
    name[len] = 0;
    strcat(str,name);
  }
  else
  {
    val = bmReadDWordEx(rne->offset,binary_stream::Seek_Set);
    if(FindPubName(name,sizeof(name),val))
    {
      strcat(str,name);
    }
    else
     if(!(flags & APREF_SAVE_VIRT))
     {
       strcat(str,"(*this)+");
       strcat(str,Get8Digit(val));
       retrf = true;
     }
     else retrf = false;
  }
  return retrf;
}

static bool __FASTCALL__ AppendNLMRef(const DisMode& parent,char *str,__filesize_t ulShift,int flags,int codelen,__filesize_t r_sh)
{
  RELOC_NLM *rnlm,key;
  bool retrf;
  char buff[400];
  if(flags & APREF_TRY_PIC) return false;
  if(!nlm.nlm_numberOfExternalReferences || nlm.nlm_externalReferencesOffset >= bmGetFLength()) retrf = false;
  else
  {
    if(!RelocNlm) BuildRelocNlm();
    key.offset = ulShift;
    __codelen = codelen;
    rnlm = (RELOC_NLM*)la_Find(RelocNlm,&key,nlm_compare_f);
    retrf = rnlm ? BuildReferStrNLM(str,rnlm,flags) : false;
  }
  if(!retrf && (flags & APREF_TRY_LABEL))
  {
     if(!PubNames) nlm_ReadPubNameList(bmbioHandle(),MemOutBox);
     if(FindPubName(buff,sizeof(buff),r_sh))
     {
       strcat(str,buff);
       if(!DumpMode && !EditMode) code_guider->add_go_address(parent,str,r_sh);
       retrf = true;
     }
  }
  return retrf;
}

static bool __FASTCALL__ IsNLM()
{
  char ctrl[NLM_SIGNATURE_SIZE];
  bmReadBufferEx(ctrl,NLM_SIGNATURE_SIZE,0,binary_stream::Seek_Set);
  return memcmp(ctrl,NLM_SIGNATURE,NLM_SIGNATURE_SIZE) == 0;
}

static void __FASTCALL__ NLMinit(CodeGuider& _code_guider)
{
    code_guider=&_code_guider;
  binary_stream& main_handle = bmbioHandle();
  bmReadBufferEx(&nlm,sizeof(Nlm_Internal_Fixed_Header),0,binary_stream::Seek_Set);
  if((nlm_cache = main_handle.dup()) == &bNull) nlm_cache = &main_handle;
}

static void __FASTCALL__ NLMdestroy()
{
  binary_stream& main_handle = bmbioHandle();
  if(nlm_cache != &bNull && nlm_cache != &main_handle) delete nlm_cache;
}

static int __FASTCALL__ NLMbitness(__filesize_t off)
{
  UNUSED(off);
  return DAB_USE32;
}

static bool __FASTCALL__ NLMAddrResolv(char *addr,__filesize_t cfpos)
{
 /* Since this function is used in references resolving of disassembler
    it must be seriously optimized for speed. */
  bool bret = true;
  uint32_t res;
  if(cfpos < sizeof(Nlm_Internal_Fixed_Header))
  {
    strcpy(addr,"nlm32h:");
    strcpy(&addr[7],Get2Digit(cfpos));
  }
  else
    if((res=NLMPA2VA(cfpos)) != 0)
    {
      addr[0] = '.';
      strcpy(&addr[1],Get8Digit(res));
    }
    else bret = false;
  return bret;
}

static __filesize_t __FASTCALL__ HelpNLM()
{
  hlpDisplay(10007);
  return BMGetCurrFilePos();
}

static void __FASTCALL__ nlm_ReadPubName(binary_stream& b_cache,const struct PubName *it,
			    char *buff,unsigned cb_buff)
{
    unsigned char length;
    b_cache.seek(it->nameoff,binary_stream::Seek_Set);
    length = b_cache.read(type_byte);
    length = std::min(unsigned(length),cb_buff);
    b_cache.read(buff,length);
    buff[length] = 0;
}

static bool  __FASTCALL__ FindPubName(char *buff,unsigned cb_buff,__filesize_t pa)
{
  struct PubName *ret,key;
  key.pa = pa;
  if(!PubNames) nlm_ReadPubNameList(*nlm_cache,MemOutBox);
  ret = (PubName*)la_Find(PubNames,&key,fmtComparePubNames);
  if(ret)
  {
    nlm_ReadPubName(*nlm_cache,ret,buff,cb_buff);
    return true;
  }
  return udnFindName(pa,buff,cb_buff);
}

static void __FASTCALL__ nlm_ReadPubNameList(binary_stream& handle,void (__FASTCALL__ *mem_out)(const std::string&))
{
 unsigned char length;
 unsigned i;
 unsigned nnames = (unsigned)nlm.nlm_numberOfPublics;
 if(!PubNames)
   if(!(PubNames = la_Build(0,sizeof(struct PubName),mem_out))) return;
 handle.seek(nlm.nlm_publicsOffset,binary_stream::Seek_Set);
 for(i = 0;i < nnames;i++)
 {
   struct PubName nlm_pn;
   nlm_pn.nameoff = handle.tell();
   length         = handle.read(type_byte);
   handle.seek(length,binary_stream::Seek_Cur);
   nlm_pn.pa      = (handle.read(type_dword) & 0x00FFFFFFL) + nlm.nlm_codeImageOffset;
   nlm_pn.attr    = SC_GLOBAL;
   if(!la_AddData(PubNames,&nlm_pn,mem_out)) break;
   if(handle.eof()) break;
 }
 if(PubNames->nItems) la_Sort(PubNames,fmtComparePubNames);
}

static __filesize_t __FASTCALL__ NLMGetPubSym(char *str,unsigned cb_str,unsigned *func_class,
			   __filesize_t pa,bool as_prev)
{
  if(!PubNames) nlm_ReadPubNameList(*nlm_cache,NULL);
  return fmtGetPubSym(*nlm_cache,str,cb_str,func_class,pa,as_prev,
		      PubNames,
		      nlm_ReadPubName);
}

static unsigned __FASTCALL__ NLMGetObjAttr(__filesize_t pa,char *name,unsigned cb_name,
		      __filesize_t *start,__filesize_t *end,int *_class,int *bitness)
{
  unsigned ret;
  UNUSED(cb_name);
  *start = 0;
  *end = bmGetFLength();
  *_class = OC_NOOBJECT;
  *bitness = NLMbitness(pa);
  name[0] = 0;
  if(pa < nlm.nlm_codeImageOffset)
  {
    *end = nlm.nlm_codeImageOffset;
    ret = 0;
  }
  else
    if(pa >= nlm.nlm_codeImageOffset && pa < nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize)
    {
      *start = nlm.nlm_codeImageOffset;
      *_class = OC_CODE;
      *end = nlm.nlm_codeImageOffset + nlm.nlm_codeImageSize;
      ret = 1;
    }
    else
    if(pa >= nlm.nlm_dataImageOffset && pa < nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize)
    {
      *_class = OC_DATA;
      *start = nlm.nlm_dataImageOffset;
      *end = *start + nlm.nlm_dataImageSize;
      ret = 2;
    }
    else
    {
      *_class = OC_NOOBJECT;
      *start = nlm.nlm_dataImageOffset + nlm.nlm_dataImageSize;
      ret = 3;
    }
  return ret;
}

static __filesize_t __FASTCALL__ NLMVA2PA(__filesize_t va)
{
  return va + std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
}

static __filesize_t __FASTCALL__ NLMPA2VA(__filesize_t pa)
{
  __filesize_t base = std::min(nlm.nlm_codeImageOffset,nlm.nlm_dataImageOffset);
  return pa > base ? pa - base : 0L;
}

static int __FASTCALL__ NLMPlatform() { return DISASM_CPU_IX86; }

extern const REGISTRY_BIN nlm386Table =
{
  "nlm-i386 (Novell Loadable Module)",
  { "NlmHlp", "ModRef", "PubDef", NULL, "ExtNam", NULL, NULL, "NlmHdr", NULL, NULL },
  { HelpNLM, ShowModRefNLM, ShowPubNamNLM, NULL, ShowExtRefNLM, NULL, NULL, ShowNewNLM, NULL, NULL },
  IsNLM, NLMinit, NLMdestroy,
  ShowNLMHeader,
  AppendNLMRef,
  NLMPlatform,
  NLMbitness,
  NULL,
  NLMAddrResolv,
  NLMVA2PA,
  NLMPA2VA,
  NLMGetPubSym,
  NLMGetObjAttr
};
} // namespace	usr
