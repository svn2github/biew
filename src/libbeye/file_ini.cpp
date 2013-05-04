#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   libbeye
 * @file        libbeye/file_ini.c
 * @brief       This file contains implementation of .ini files services.
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
 * @bug         Fault if more than one ini file is opened at one time
 * @todo        Reentrance ini library
**/
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "libbeye/bbio.h"
#include "libbeye/file_ini.h"

#if 0
static void dump_BFILE(BFILE *h) {
fprintf(stderr,
	"FilePos=%llu\n"
	"Flength=%llu\n"
	"FileName=%s\n"
	"openmode=%u\n"
	"optimize=%i\n"
	"is_mmf=%i\n"
	"primary_mmf=%u\n"
	"is_eof=%u\n"
	,h->FilePos
	,h->FLength
	,h->FileName
	,h->openmode
	,h->optimize
	,h->is_mmf
	,h->primary_mmf
	,h->is_eof);
}
#endif

static unsigned char CaseSens = 2; /**< 2 - case 1 - upper 0 - lower */
static FiUserFunc proc;
static Ini_io* ActiveFile = NULL;
const char *fi_Debug_Str = NULL;

static int __FASTCALL__ StdError(int ne,int row,const std::string& addinfo);
static void __FASTCALL__ FiFileParserStd(const std::string& filename);
static bool __FASTCALL__ FiStringParserStd(const std::string& curr_str);
static bool __FASTCALL__ FiCommandParserStd( const std::string& cmd );
static bool __FASTCALL__ FiGetConditionStd( const std::string& condstr);

static int (__FASTCALL__ *FiError)(int nError,int row,const std::string& ) = StdError;
static void  (__FASTCALL__ *FiFileParser)(const std::string& fname) = FiFileParserStd;
static bool (__FASTCALL__ *FiStringParser)(const std::string& curr_str) = FiStringParserStd;
static bool (__FASTCALL__ *FiCommandParser)(const std::string& cmd) = FiCommandParserStd;
static bool (__FASTCALL__ *FiGetCondition)( const std::string& condstr) = FiGetConditionStd;

/**************************************************************\
*                      Low level support                       *
\**************************************************************/


static const unsigned __C_EOF=0x1A;

static std::vector<std::pair<size_t,std::string> > file_info;
static const char*  FiUserMessage = NULL;
static const char iniLegalSet[] = " _0123456789"
			   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			   "abcdefghijklmnopqrstuvwxyz";

static inline bool IS_VALID_NAME(const std::string& name) { return name.find_first_not_of(iniLegalSet) == std::string::npos; }
static inline bool IS_SECT(const std::string& str,char ch) { return str[0] == ch; }
static inline bool FiisSection(const std::string& str ) { return IS_SECT(str,'['); }
static inline bool FiisSubSection(const std::string& str ) { return IS_SECT(str,'<'); }
static inline bool FiisCommand(const std::string& str ) { return IS_SECT(str,'#'); }
static inline bool FiisItem(const std::string& str) { return !(FiisSection(str) || FiisSubSection(str) || FiisCommand(str)); }

static void __FASTCALL__ FiAError(int nError,int row,const std::string& addinfo)
{
 int eret = 0;
 if(FiError) eret = (*FiError)(nError,row,addinfo);
 if(eret == __FI_EXITPROC) exit(255);
}

static void __FASTCALL__ FiAErrorCL(int nError) { FiAError(nError,file_info.back().first,""); }

static const char * list[] = {
 "No errors",
 "Can't open file: '%s' (bad '#include' statement?).",
 "Too many open files.",
 "Memory exhausted.",
 "Open 'if' (missing '#endif').",
 "Missing 'if' for 'endif' statement.",
 "Missing 'if' for 'else' statement.",
 "Unknown '#' directive.",
 "Syntax error in 'if' statement.",
 "Expected open section or subsection, or invalid string.",
 "Bad character on line (possible lost comment).",
 "Bad variable in 'set' or 'delete' statement.",
 "Bad value of variable in 'set' statement.",
 "Unrecognized name of variable in 'delete' statement.",
 "Undefined variable detected (case sensitivity?).",
 "Missing 'if' for 'elif' statement.",
 "Open variable on line (use even number of '%' characters).",
 "Lost or mismatch character '=' in assigned expression.",
 "",
 "User error."
};

static const char * __FASTCALL__ FiDecodeError(int nError)
{
 const char *ret;
 nError = abs(nError);
 if(nError >= 0 && nError <= abs(__FI_FIUSER)) ret = list[nError];
 else ret = "Unknown Error";
 return ret;
}

static int __FASTCALL__ StdError(int ne,int row,const std::string& addinfo)
{
    FILE * herr;
    const char * what;
    char sout[4096];
    if((herr = fopen("fi_syserr.$$$","wt")) == NULL) herr = stderr;
    fprintf(herr,"About : [.Ini] file run-time support library. Written by Nickols_K\n"
		 "Detected ");
    if(ne != __FI_TOOMANY && ~file_info.empty())
    {
      fprintf(herr,"%s error in : %s",row ? "fatal" : "",file_info.back().second.c_str());
    }
    fprintf(herr,"\n");
    if(row)  fprintf(herr,"At line : %i\n",row);
    what = FiDecodeError(ne);
    if(!addinfo.empty()) sprintf(sout,what,addinfo.c_str());
    else strncpy(sout,what,sizeof(sout));
    fprintf(herr,"Message : %s\n",sout);
    if(FiUserMessage) fprintf(herr,"User message : %s\n",FiUserMessage);
    if(fi_Debug_Str) if(*fi_Debug_Str) fprintf(herr,"Debug info: '%s'\n",fi_Debug_Str);
    fclose(herr);
    std::cerr<<std::endl<<"Error in .ini file."<<std::endl<<"File fi_syser.$$$ created."<<std::endl;
    return __FI_EXITPROC;
}

namespace beye {
Ini_io::Ini_io():_opened(false),handler(bNull) {}
Ini_io::~Ini_io() { if(&handler!=&bNull) close(); }
bool Ini_io::open(const std::string& filename)
{
    handler = *new BFile;
    bool rc;
    /* Try to load .ini file entire into memory */
    rc = handler.open(filename,FO_READONLY | SO_DENYWRITE,UINT_MAX,BFile::Opt_UseMMF);
    /* Note! All OSes except DOS-DOS386 allows opening of empty filenames as /dev/null */
    if(rc == false && filename[0]) FiAError(__FI_BADFILENAME,0,filename.c_str());
    file_info.push_back(std::make_pair(0,filename));
    _opened=rc;
    return rc;
}

void Ini_io::close()
{
    file_info.pop_back();
    if(&handler!=&bNull) {
	delete &handler;
	handler=bNull;
    }
}

static const char FiOpenComment=';';
static bool  FiAllWantInput = false;

char* Ini_io::get_next_string(char * str,unsigned int size,char *original)
{
  unsigned char *sret;
  unsigned len;
  unsigned char ch;
  str[0] = 0;
  while(!handler.eof())
  {
    str[0] = 0;
    sret = (unsigned char*)GETS(str,size);
    len = strlen(str);
    while(len)
    {
      ch = str[len-1];
      if(ch == '\n' || ch == '\r') str[--len] = 0;
      else break;
    }
    if(original) strcpy(original,str);
    file_info.back().first++;
    if((sret == NULL && !handler.eof()))  FiAError(__FI_BADFILENAME,0,str);
    sret = (unsigned char*)strchr(str,FiOpenComment);
    if(sret) *sret = 0;
    if(!FiAllWantInput)
    {
      szTrimTrailingSpace(str);
      szTrimLeadingSpace(str);
      /* kill all spaces around punctuations */
      /*
	 Correct me: spaces will be sqweezed even inside " " brackets.
	 But it is language dependent.
	 For .ini files it is not significant.
      */
      sret = (unsigned char*)str;
      while((ch=*sret) != 0)
      {
	if(ispunct(ch)) sret = (unsigned char*)szKillSpaceAround(str,(char*)sret);
	sret++;
      }
      len = strlen(str);
      sret = (unsigned char*)str;
      while((ch=*sret) != 0)
      {
	if(isspace(ch))
	{
	  if(isspace(*(sret+1))) sret = (unsigned char*)szKillSpaceAround(str,(char*)sret);
	}
	sret++;
      }
      if(strlen(str))  break; /* loop while comment */
    }
  }
  return str;
}

std::string Ini_io::get_next_string() {
    char tmp[FI_MAXSTRLEN];
    get_next_string(tmp,sizeof(tmp),NULL);
    return std::string(tmp);
}

char* Ini_io::GETS(char *str,unsigned num)
{
  char *ret;
  unsigned i;
  char ch,ch1;
  ret = str;
  for(i = 0;i < num;i++)
  {
     ch = handler.read_byte();
     if(ch == '\n' || ch == '\r')
     {
       *str = ch;  str++;
       ch1 = handler.read_byte();
       if((ch1 == '\n' || ch1 == '\r') && ch != ch1)
       {
	 *str = ch; str++;
       }
       else
       {
	 if(handler.eof())
	 {
	   if((signed char)ch1 != -1 && ch1 != __C_EOF)
	   {
	     *str = ch1; str++;
	   }
	   break;
	 }
	 handler.seek(-1,SEEK_CUR);
       }
       break;
     }
     if(handler.eof())
     {
       if((signed char)ch != -1 && ch != __C_EOF)
       {
	 *str = ch; str++;
       }
       break;
     }
     *str = ch; str++;
  }
  *str = 0;
  return ret;
}

} // namespace beye

static size_t  __FASTCALL__ __GetLengthBrStr(const std::string& src,char obr,char cbr)
{
 size_t ends;
 size_t ret = 0;
 if(src[0] == obr)
 {
   ends = src.find(cbr,1);
   if(ends==std::string::npos) goto err;
   if(src[ends+1]) FiAErrorCL(__FI_BADCHAR);
   ret = ends-1;
 }
 else
 {
   err:
   FiAErrorCL(__FI_OPENSECT);
 }
 return ret;
}

static char *  __FASTCALL__ __GetBrStrName(const std::string& src,char * store,char obr,char cbr)
{
 size_t ends;
 if(src[0] == obr)
 {
   unsigned len;
   ends = src.find(cbr,1);
   if(ends==std::string::npos) goto err;
   if(src[ends+1]) FiAErrorCL(__FI_BADCHAR);
   len = ends-1;
   memcpy(store,&src.c_str()[1],len);
   store[len] = 0;
 }
 else
 {
   err:
   FiAErrorCL(__FI_OPENSECT);
 }
 return store;
}

static inline size_t FiGetLengthBracketString(const std::string& str ) { return __GetLengthBrStr(str,'"','"'); }
static inline char*  FiGetBracketString(const std::string& str,char* store) { return __GetBrStrName(str,store,'"','"'); }
static inline char*  FiGetSectionName(const std::string& src,char* store) { return __GetBrStrName(src,store,'[',']'); }
static inline char*  FiGetSubSectionName(const std::string& src,char* store) { return __GetBrStrName(src,store,'<','>'); }
static inline size_t FiGetLengthSection(const std::string& src ) { return __GetLengthBrStr(src,'[',']'); }
static inline size_t FiGetLengthSubSection(const std::string& src ) { return __GetLengthBrStr(src,'<','>'); }

static std::string __FASTCALL__ __GetBrStrName(const std::string& src,char obr,char cbr)
{
    std::string rc;
    size_t ends;
    if(src[0] == obr) {
	unsigned len;
	ends = src.find(cbr,1);
	if(ends==std::string::npos) goto err;
	if(src[ends+1]) FiAErrorCL(__FI_BADCHAR);
	len = ends-1;
	rc=src.substr(1,len);
    } else {
err:
	FiAErrorCL(__FI_OPENSECT);
    }
    return rc;
}

static inline std::string FiGetBracketString(const std::string& str) { return __GetBrStrName(str,'"','"'); }
static inline std::string FiGetSectionName(const std::string& src) { return __GetBrStrName(src,'[',']'); }
static inline std::string FiGetSubSectionName(const std::string& src) { return __GetBrStrName(src,'<','>'); }

static std::string __FASTCALL__ FiGetValueOfItem(const std::string& src)
{
    std::string rc;
    size_t from;
    from = src.find('=');
    if(from!=std::string::npos) rc=&src.c_str()[++from];
    else     FiAErrorCL(__FI_NOTEQU);
    return rc;
}


static char *    __FASTCALL__ FiGetItemName(const std::string& src,char * store)
{
  size_t sptr;
  unsigned len;
  sptr = src.find('=');
  if(sptr!=std::string::npos) {
    len = sptr;
    memcpy(store,src.c_str(),len);
    store[len] = 0;
    if(!IS_VALID_NAME(store)) FiAErrorCL(__FI_BADCHAR);
  }
  return store;
}

static std::string  __FASTCALL__ FiGetItemName(const std::string& src)
{
    std::string rc;
    size_t sptr;
    unsigned len;
    sptr = src.find('=');
    if(sptr!=std::string::npos) {
	len = sptr;
	rc=src.substr(0,len);
	if(!IS_VALID_NAME(rc)) FiAErrorCL(__FI_BADCHAR);
    }
    return rc;
}

static std::string  __FASTCALL__ FiGetCommandString(const std::string& src)
{
    std::string rc;
    unsigned i;
    i = strspn(src.c_str()," #");
    rc=src.substr(i);
    return rc;
}

/************** BEGIN of Construct section ********************/

/*
   These functions protect library from stack overflow in 16-bit mode,
   because #include statement does recursion.
*/

static char *  __FASTCALL__ __FiCMaxStr()
{
  char* ret;
  ret = new char [FI_MAXSTRLEN + 1];
  if(!ret) FiAError(__FI_NOTMEM,0,NULL);
  return ret;
}

/************* END of Construct section *********************/

/*************************************************************\
*                  Middle level support                       *
\*************************************************************/

/************* BEGIN of List Var section ********************/

namespace beye {
Variable_Set::Variable_Set() {}
Variable_Set::~Variable_Set() {}

void Variable_Set::add(const std::string& var,const std::string& associate) {
    set[var] = associate;
}

void Variable_Set::remove(const std::string& var) {
    std::map<std::string,std::string>::iterator it;
    it = set.find(var);
    set.erase(it);
}

void Variable_Set::clear() { set.clear(); }

std::string Variable_Set::expand(const std::string& var) { return set[var]; }

std::string Variable_Set::substitute(const std::string& src,char delim) {
    std::string rc;
    if(src.find(delim)!=std::string::npos) {
	char tmp[FI_MAXSTRLEN+1];
	char npercent;
	bool isVar;
	unsigned char tmp_ptr;
	unsigned int i,slen;
	npercent = 0;
	tmp_ptr = 0;
	isVar = false;
	slen = src.length();
	for(i = 0;i < slen;i++) {
	    char c;
	    c = src[i];
	    if(c == delim) {
		npercent++;
		isVar = !isVar;
		if(!isVar) {
		    tmp[tmp_ptr] = '\0';
		    rc+=expand(tmp);
		    tmp_ptr = 0;
		}
	    } else {
		if(isVar)	tmp[tmp_ptr++] = c;
		else		rc+=c;
	    }
	}
	if( npercent%2 ) FiAErrorCL(__FI_OPENVAR);
    } else rc=src;
    return rc;
}


Tokenizer::Tokenizer(const std::string& _src):src(_src),iptr(0) {}
Tokenizer::~Tokenizer() {}

size_t Tokenizer::next_length(const std::string& illegal_symbols) const {
    size_t j,i;
    i = strspn(&src[iptr],illegal_symbols.c_str());
    j = strcspn(&src[i+iptr],illegal_symbols.c_str());
    return j;
}

std::string Tokenizer::next_word(const std::string& illegal_symbols) {
    std::string rc;
    size_t j;
    iptr += strspn(&src[iptr],illegal_symbols.c_str());
    j = strcspn(&src[iptr],illegal_symbols.c_str());
    rc=src.substr(iptr,j);
    iptr += j;
    return rc;
}

size_t Tokenizer::next_legal_length(const std::string& legal_symbols) const
{
    unsigned int j,i;
    i = strcspn(&src[iptr],legal_symbols.c_str());
    j = strspn(&src[i+iptr],legal_symbols.c_str());
    return j;
}

std::string Tokenizer::next_legal_word(const std::string& legal_symbols)
{
    std::string rc;
    unsigned int j;
    iptr += strcspn(&src[iptr],legal_symbols.c_str());
    j = strspn(&src[iptr],legal_symbols.c_str());
    rc=src.substr(iptr,j);
    iptr += j;
    return rc;
}
std::string Tokenizer::tail() {
    std::string rc=src.substr(iptr+1);
    return rc;
}
} // namespace beye

/*************** END of List Var Section ***************/
static Variable_Set vars;
static bool ifSmarting = true;

static bool __FASTCALL__ FiGetConditionStd( const std::string& condstr)
{
    std::string var;
    std::string user_ass;
    std::string real_ass;
    std::string rvar;
    Tokenizer tokenizer(condstr);
    bool ret;
    char cond[3];

    var = tokenizer.next_word(" !=");
    real_ass=ifSmarting?vars.substitute(var):var;
    rvar = vars.expand(real_ass);
    cond[0] = tokenizer.next_char();
    cond[1] = tokenizer.next_char();
    cond[2] = '\0';

    var = tokenizer.next_word(" ");
    user_ass=ifSmarting?vars.substitute(var):var;
    if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
    ret = false;
    if(strcmp(cond,"==") == 0)  ret = user_ass==rvar;
    else if(strcmp(cond,"!=") == 0)  ret = user_ass!=rvar;
    else FiAErrorCL(__FI_BADCOND);
    return ret;
}

static bool __FASTCALL__ FiCommandParserStd( const std::string& cmd )
{
    std::string word,a,v;
    const char *fdeb_save;
    Tokenizer tokenizer(cmd);
    static bool cond_ret = true;
    word = tokenizer.next_legal_word(&iniLegalSet[1]);
    std::transform(word.begin(),word.end(),word.begin(),::tolower);
    if(word=="include") {
	std::string bracket;
	std::string _v;
	char pfile[FILENAME_MAX+1],*pfp,*pfp2;
	bracket = FiGetBracketString(tokenizer.data());
	_v=ifSmarting?vars.substitute(bracket):bracket;
	fdeb_save = fi_Debug_Str;
	/* make path if no path specified */
	strcpy(pfile,file_info.back().second.c_str());
	pfp=strrchr(pfile,'\\');
	pfp2=strrchr(pfile,'/');
	pfp=std::max(pfp,pfp2);
	if(pfp && (_v.find('\\')==std::string::npos || _v.find('/')==std::string::npos)) strcpy(pfp+1,_v.c_str());
	else    strcpy(pfile,_v.c_str());
	(*FiFileParser)(pfile);
	fi_Debug_Str = fdeb_save;
	goto Exit_CP;
    } else if(word=="set") {
	v = tokenizer.next_legal_word(&iniLegalSet[1]);
	if(tokenizer.curr_char() != '=') FiAErrorCL(__FI_NOTEQU);
	tokenizer.next_char();
	a = tokenizer.next_word(" ");
	if(v[0] == '\0') FiAErrorCL(__FI_BADVAR);
	if(a[0] == '\0') FiAErrorCL(__FI_BADVAL);
	vars.add(v,a);
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	goto Exit_CP;
    } else if(word=="delete") {
	std::string _a;
	v = tokenizer.next_legal_word(&iniLegalSet[1]);
	_a=ifSmarting?vars.substitute(v):v;
	if(_a[0] == '\0') FiAErrorCL(__FI_BADVAR);
	vars.remove(_a);
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	goto Exit_CP;
    } else if(word=="reset") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	vars.clear();
	goto Exit_CP;
     } else if(word=="case") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	CaseSens = 2;
	goto Exit_CP;
    } else if(word=="smart") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	ifSmarting = true;
	goto Exit_CP;
    } else if(word=="nosmart") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	ifSmarting = false;
	goto Exit_CP;
    } else if(word=="uppercase") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	CaseSens = 1;
	goto Exit_CP;
    } else if(word=="lowercase") {
	if(tokenizer.curr_char()) FiAErrorCL(__FI_BADCHAR);
	CaseSens = 0;
	goto Exit_CP;
    } else if(word=="error") {
	std::string _a;
	std::string sptr=tokenizer.tail();
	_a=ifSmarting?vars.substitute(sptr):sptr;
	FiUserMessage = _a.c_str();
	FiAErrorCL(__FI_FIUSER);
    } else if(word=="else") FiAErrorCL(__FI_ELSESTAT);
    else if(word=="endif") FiAErrorCL(__FI_IFNOTFOUND);
    else if(word=="elif") FiAErrorCL(__FI_ELIFSTAT);
    else if(word=="if") {
	std::string sstore;
	unsigned int nsave;
	bool Condition,BeenElse;
	int nLabel;
	nsave = file_info.back().first;
	cond_ret = Condition = (*FiGetCondition)(tokenizer.data());
	nLabel = 1;
	BeenElse = false;
	while(!ActiveFile->eof()) {
	    sstore=ActiveFile->get_next_string();
	    if(sstore[0] == '\0') goto Exit_CP;
	    if(FiisCommand(sstore)) {
		a = FiGetCommandString(sstore);
		if(CaseSens == 1) std::transform(a.begin(),a.end(),a.begin(),::toupper);
		if(CaseSens == 0) std::transform(a.begin(),a.end(),a.begin(),::tolower);
		Tokenizer c_tokenizer(a);
		v = c_tokenizer.next_word(" ");
		std::transform(v.begin(),v.end(),v.begin(),::tolower);
		if(v=="if" && !Condition) nLabel++;
		if(v=="endif") {
		    nLabel--;
		    if(nLabel == 0) goto Exit_CP;
		    if(nLabel <  0) FiAErrorCL(__FI_IFNOTFOUND);
		}
		if(v=="else" && nLabel == 1) {
		    if( BeenElse ) FiAErrorCL(__FI_ELSESTAT);
		    if( nLabel == 1 ) cond_ret = Condition = (Condition ? false : true);
		    if( nLabel == 1 ) BeenElse = true;
		    continue;
		}
		if(v=="elif" && nLabel == 1) {
		    if( BeenElse ) FiAErrorCL(__FI_ELIFSTAT);
		    if( nLabel == 1 ) cond_ret = Condition = (*FiGetCondition)(c_tokenizer.data());
		    if( nLabel == 1 ) BeenElse = true;
		    continue;
		}
		if(Condition) (*FiCommandParser)(a);
	    } else {
		if(Condition) (*FiStringParser)(sstore);
	    }
	} // while
	FiAError(__FI_OPENCOND,nsave,NULL);
    } // if word==if
    FiAErrorCL(__FI_UNRECOGN);
    Exit_CP:
    return cond_ret;
}

static std::string curr_sect;
static std::string curr_subsect;
static bool __FASTCALL__ FiStringParserStd(const std::string& curr_str)
{
    std::string item,val;
    if(FiisCommand(curr_str))
    {
      std::string _item;
      _item = FiGetCommandString(curr_str);
      (*FiCommandParser)(_item);
      return false;
    }
    else
    if(FiisSection(curr_str))
    {
      curr_sect = FiGetSectionName(curr_str);
      return false;
    }
    else
    if(FiisSubSection(curr_str))
    {
      curr_subsect = FiGetSubSectionName(curr_str);
      return false;
    }
    else
    if(FiisItem(curr_str))
    {
      std::string buffer;
      bool retval;
      IniInfo info;
      buffer=ifSmarting?vars.substitute(curr_str):curr_str;
      item = FiGetItemName(buffer);
      retval = false;
      if(item[0])
      {
       val = FiGetValueOfItem(buffer);
       if(!curr_sect.empty()) info.section = curr_sect.c_str();
       else info.section = "";
       if(!curr_subsect.empty()) info.subsection = curr_subsect.c_str();
       else info.subsection = "";
       info.item = item.c_str();
       info.value = val.c_str();
       retval = (*proc)(&info);
      }
      else FiAErrorCL(__FI_BADCHAR);
      return retval;
    }
  return false;
}

static void __FASTCALL__ FiFileParserStd(const std::string& filename)
{
  char * work_str, * ondelete;
  Ini_io& h = *new(zeromem) Ini_io;
  Ini_io* oldh = ActiveFile;
  bool done;
  h.open(filename);
  ActiveFile = &h;
  work_str = __FiCMaxStr();
  fi_Debug_Str = ondelete = work_str;
  while(!h.eof())
  {
      h.get_next_string(work_str,FI_MAXSTRLEN,NULL);
      if(!work_str || !work_str[0]) break;
      if(CaseSens == 1) strupr(work_str);
      if(CaseSens == 0) strlwr(work_str);
      done = (*FiStringParser)(work_str);
      if(done) break;
  }
  h.close();
  if(work_str) PFREE(work_str)
  else PFREE(ondelete)
  ActiveFile = oldh;
}

void __FASTCALL__  FiProgress(const std::string& filename,FiUserFunc usrproc)
{
  if(!usrproc) return;
  proc = usrproc;

  (*FiFileParser)(filename);

  proc = NULL;
  vars.clear();
}

static void __FASTCALL__ hlFiFileParserStd(hIniProfile *ini)
{
  char * work_str, * ondelete;
  Ini_io* oldh = ActiveFile;
  bool done;
  ActiveFile = ini->handler;
  work_str = __FiCMaxStr();
  fi_Debug_Str = ondelete = work_str;
  ini->handler->rewind_ini();
  while(!ini->handler->eof())
  {
      ini->handler->get_next_string(work_str,FI_MAXSTRLEN,NULL);
      if(!work_str || !work_str[0]) break;
      if(CaseSens == 1) strupr(work_str);
      if(CaseSens == 0) strlwr(work_str);
      done = (*FiStringParser)(work_str);
      if(done) break;
  }
  if(work_str) PFREE(work_str)
  else PFREE(ondelete)
  ActiveFile = oldh;
}

static void __FASTCALL__ hlFiProgress(hIniProfile *ini,FiUserFunc usrproc)
{

  if(!usrproc) return;
  proc = usrproc;

  hlFiFileParserStd(ini);

  proc = NULL;
  vars.clear();
}

/*****************************************************************\
*                      High level support                          *
\******************************************************************/

static hIniProfile *opened_ini;

static const unsigned IC_STRING=0x0001;

typedef struct tag_ini_Cache
{
  unsigned flags;
  char *   item;
  union
  {
    linearArray *leaf;
    char        *value;
  }v;
}ini_cache;

static tCompare __FASTCALL__ __full_compare_cache(const any_t*v1,const any_t*v2)
{
  const ini_cache  *c1,  *c2;
  int iflg;
  tCompare i_ret;
  c1 = (const ini_cache  *)v1;
  c2 = (const ini_cache  *)v2;
  iflg = __CmpLong__(c1->flags,c2->flags);
  i_ret = strcmp(c1->item,c2->item);
  return i_ret ? i_ret : iflg;
}

inline tCompare __compare_cache(const any_t*v1,const any_t*v2) { return __full_compare_cache(v1,v2); }

static bool  __FASTCALL__ __addCache(const std::string& section,const std::string& subsection,
			       const std::string& item,const std::string& value)
{
  any_t*found;
  ini_cache  *it;
  ini_cache ic;
  ic.item = const_cast<char*>(section.c_str());
  ic.flags = 0;
  if(!(found =la_Find((linearArray *)opened_ini->cache,&ic,__full_compare_cache)))
  {
      ic.item = new char [section.size()+1];
      if(!ic.item) { opened_ini->flags &= ~HINI_FULLCACHED; return true; }
      strcpy(ic.item,section.c_str());
      if(!(ic.v.leaf = la_Build(0,sizeof(ini_cache),0)))
      {
	 delete ic.item;
	 opened_ini->flags &= ~HINI_FULLCACHED;
	 return true;
      }
      ic.flags = 0;
      if(!(la_AddData((linearArray *)opened_ini->cache,&ic,NULL)))
      {
	delete ic.item;
	la_Destroy(ic.v.leaf);
	opened_ini->flags &= ~HINI_FULLCACHED;
	return true;
      }
      opened_ini->flags |= HINI_UPDATED;
      la_Sort((linearArray *)opened_ini->cache,__compare_cache);
      found = la_Find((linearArray *)opened_ini->cache,&ic,__full_compare_cache);
      goto do_subsect;
  }
  else
  {
    do_subsect:
      it = (ini_cache  *)found;
      ic.item = const_cast<char*>(subsection.c_str());
      if(!(found=la_Find(it->v.leaf,&ic,__full_compare_cache)))
      {
	ic.item = new char [subsection.size()+1];
	if(!ic.item) { opened_ini->flags &= ~HINI_FULLCACHED; return true; }
	strcpy(ic.item,subsection.c_str());
	if(!(ic.v.leaf = la_Build(0,sizeof(ini_cache),0)))
	{
	   delete ic.item;
	   opened_ini->flags &= ~HINI_FULLCACHED;
	   return true;
	}
	ic.flags = 0;
	if(!(la_AddData(it->v.leaf,&ic,NULL)))
	{
	  delete ic.item;
	  la_Destroy(ic.v.leaf);
	  opened_ini->flags &= ~HINI_FULLCACHED;
	  return true;
	}
	opened_ini->flags |= HINI_UPDATED;
	la_Sort(it->v.leaf,__compare_cache);
	found = la_Find(it->v.leaf,&ic,__full_compare_cache);
	goto do_item;
      }
      else
      {
	do_item:
	it = (ini_cache  *)found;
	ic.item = const_cast<char*>(item.c_str());
	ic.flags = IC_STRING;
	if(!(found=la_Find(it->v.leaf,&ic,__full_compare_cache)))
	{
	  ic.item = new char [item.size()+1];
	  if(!ic.item) { opened_ini->flags &= ~HINI_FULLCACHED; return true; }
	  strcpy(ic.item,item.c_str());
	  ic.v.value = new char[value.size()+1];
	  if(!ic.v.value) { delete ic.item; opened_ini->flags &= ~HINI_FULLCACHED; return true; }
	  strcpy(ic.v.value,value.c_str());
	  if(!(la_AddData(it->v.leaf,&ic,NULL)))
	  {
	    delete ic.item;
	    delete ic.v.value;
	    opened_ini->flags &= ~HINI_FULLCACHED;
	    return true;
	  }
	  opened_ini->flags |= HINI_UPDATED;
	  la_Sort(it->v.leaf,__compare_cache);
	}
	else
	{
	  /* item already exists. Try replace it */
	  it = (ini_cache  *)found;
	  if(value!=it->v.value)
	  {
	     char *newval,*oldval;
	     newval = new char[value.size()+1];
	     if(!newval)
	     {
	       opened_ini->flags &= ~HINI_FULLCACHED;
	       return true;
	     }
	     strcpy(newval,value.c_str());
	     oldval = it->v.value;
	     it->v.value = newval;
	     delete oldval;
	     opened_ini->flags |= HINI_UPDATED;
	  }
	}
      }
  }
  return false;
}

static bool __FASTCALL__ __buildCache(IniInfo *ini)
{
  return __addCache(ini->section,ini->subsection,
		    ini->item,ini->value);
}

static void __FASTCALL__ __iter_destroy(any_t*it);

static void __destroyCache(linearArray* it) { la_IterDestroy(it,__iter_destroy); }

static void __FASTCALL__ __iter_destroy(any_t*it)
{
  ini_cache  *ic;
  ic = (ini_cache  *)it;
  if(ic->flags & IC_STRING)
  {
    delete ic->item;
    delete ic->v.value;
  }
  else
  {
    delete ic->item;
    la_IterDestroy(ic->v.leaf,__iter_destroy);
  }
}

static unsigned ret;
static unsigned buf_len;
static char *buf_ptr;
static std::string sect,subsect,item;

static bhandle_t  __FASTCALL__ make_temp(const std::string& path,char *name_ptr)
{
  char *fullname, *nptr;
  unsigned i,len;
  bhandle_t handle;
  fullname = new char [(path.length()+1)*2];
  if(!fullname) return NULL_HANDLE;
  strcpy(fullname,path.c_str());
  len = strlen(fullname);
  if(fullname[len-4] == '.') nptr = &fullname[len-4];
  else                       nptr = &fullname[len];
  handle = NULL_HANDLE;
  for(i = 0;i < 100;i++)
  {
  /*
     in this loop we are change only extension:
     file.tmp file.t1 ... file.t99
     (it's only for compatibilities between different OS FS)
  */
    sprintf(nptr,".t%i",i);
    handle = __OsOpen(fullname,FO_READONLY | SO_DENYNONE);
    if(handle == NULL_HANDLE) handle = __OsOpen(fullname,FO_READONLY | SO_COMPAT);
    if(handle == NULL_HANDLE)
    {
      handle = __OsCreate(fullname);
      if(handle != NULL_HANDLE)
      {
	strcpy(name_ptr,fullname);
	goto bye;
      }
    }
  }
  bye:
  return handle;
}

static bool __FASTCALL__ MyCallback(IniInfo * ini)
{
  bool cond;
  cond = false;
  if(!sect.empty()) cond = sect==ini->section;
  if(!subsect.empty()) cond &= subsect==ini->subsection;
  if(!item.empty()) cond &= item==ini->item;
  if(cond)
  {
    ret = std::min(strlen(ini->value),size_t(buf_len));
    strncpy(buf_ptr,ini->value,buf_len);
    return true;
  }
  return false;
}

static int  __FASTCALL__ out_sect(FILE * handle,const std::string& section)
{
   fprintf(handle,"[ %s ]\n",section.c_str());
   return 2;
}

static int  __FASTCALL__ out_subsect(FILE * handle,const std::string& subsection)
{
   fprintf(handle,"  < %s >\n",subsection.c_str());
   return 2;
}

static void  __FASTCALL__ out_item(FILE * handle,unsigned nled,const std::string& _item,const std::string& value)
{
  char *sm_char;
  unsigned i;
  sm_char = (char*)strchr(value.c_str(),'%');
  if(sm_char && ifSmarting)
  {
    fprintf(handle,"#nosmart\n");
  }
  for(i = 0;i < nled;i++)
  {
    fprintf(handle," ");
  }
  fprintf(handle,"%s = ",_item.c_str());
  fwrite(value.c_str(),value.length(),1,handle);
  fprintf(handle,"\n");
  if(sm_char && ifSmarting)
  {
    fprintf(handle,"#smart\n");
  }
}

hIniProfile * __FASTCALL__ iniOpenFile(const std::string& fname,bool *has_error)
{
    bool rc=false;
    hIniProfile *_ret = new(zeromem) hIniProfile;
    if(has_error) *has_error = true;
    _ret->handler=new(zeromem) Ini_io;
    _ret->fname=fname;
    if(__IsFileExists(_ret->fname.c_str())) rc=_ret->handler->open(fname);

    opened_ini = _ret;
    _ret->flags |= HINI_FULLCACHED;
    _ret->cache = (any_t*)la_Build(0,sizeof(ini_cache),NULL);
    if(_ret->cache && rc) {
	hlFiProgress(_ret,__buildCache);
	_ret->flags &= ~HINI_UPDATED;
    }
    else          _ret->flags = 0;
    if(has_error) *has_error = !rc;
    return _ret;
}

static bool  __FASTCALL__ __flushCache(hIniProfile *ini);

void __FASTCALL__ iniCloseFile(hIniProfile *ini)
{
  if(ini)
  {
    if(ini->cache)
    {
      __flushCache(ini);
      __destroyCache((linearArray *)ini->cache);
    }
    delete ini->handler;
    delete ini;
  }
}

unsigned __FASTCALL__ iniReadProfileString(hIniProfile *ini,const std::string& section,const std::string& subsection,
			   const std::string& _item,const std::string& def_value,char *buffer,
			   unsigned cbBuffer)
{
   if(!buffer) return 0;
   ret = 1;
   buf_len = cbBuffer;
   buf_ptr = buffer;
   sect = section;
   subsect = subsection;
   item = _item;
   if(cbBuffer) strncpy(buffer,def_value.c_str(),cbBuffer);
   else         buffer[0] = 0;
   if(ini)
   {
     if(ini->handler->opened())
     {
       bool v_found;
       v_found = false;
       if(ini->cache)
       {
	  ini_cache ic;
	  any_t*found, *foundi, *foundv;
	  ini_cache  *fi;
	  ic.item = const_cast<char*>(section.c_str());
	  ic.flags = 0;
	  if((found=la_Find((linearArray*)ini->cache,&ic,__full_compare_cache))!=NULL)
	  {
	    ic.item=const_cast<char*>(subsection.c_str());
	    fi = (ini_cache  *)found;
	    if((foundi=la_Find(fi->v.leaf,&ic,__full_compare_cache))!=NULL)
	    {
	       ic.item = const_cast<char*>(_item.c_str());
	       ic.flags = IC_STRING;
	       fi = (ini_cache  *)foundi;
	       if((foundv=la_Find(fi->v.leaf,&ic,__full_compare_cache))!=NULL)
	       {
		  fi = (ini_cache  *)foundv;
		  strncpy(buffer,fi->v.value,cbBuffer);
		  ret = std::min(strlen(fi->v.value),size_t(cbBuffer));
		  v_found = true;
	       }
	    }
	  }
       }
       if(!v_found && (ini->flags & HINI_FULLCACHED) != HINI_FULLCACHED) hlFiProgress(ini,MyCallback);
     }
   }
   buffer[cbBuffer-1] = 0;
   return ret;
}

static const char* HINI_HEADER="; This file was generated automatically by BEYELIB.\n; WARNING: Any changes made by hands may be lost the next time you run the program.\n";

static FILE *  __FASTCALL__ __makeIni(hIniProfile *ini)
{
  FILE *hout;
  hout = fopen(ini->fname.c_str(),"wt");
  if(hout != NULL)
  {
     fprintf(hout,HINI_HEADER);
  }
  return hout;
}

static bool  __FASTCALL__ __createIni(hIniProfile *ini,
				const std::string& _section,
				const std::string& _subsection,
				const std::string& _item,
				const std::string& _value)
{
  FILE *hout;
  unsigned nled;
  bool _ret;
  hout = __makeIni(ini);
  _ret = true;
  if(hout == NULL) _ret = false;
  else
  {
     nled = 0;
     if(!_section.empty())
     {
       nled += out_sect(hout,_section);
     }
     if(!_subsection.empty())
     {
       nled += out_subsect(hout,_subsection);
     }
     if(!_item.empty())
     {
       out_item(hout,nled,_item,_value);
     }
     fclose(hout);
  }
  return _ret;
}

static bool  __FASTCALL__ __directWriteProfileString(hIniProfile *ini,
					       const std::string& _section,
					       const std::string& _subsection,
					       const std::string& _item,
					       const std::string& _value)
{
   FILE * tmphandle;
   char *tmpname, *prev_val;
   char * workstr, *wstr2;
   char *original;
   unsigned nled,prev_val_size;
   bhandle_t hsrc;
   bool _ret,need_write,s_ok,ss_ok,i_ok,done,sb_ok,ssb_ok,written,Cond,if_on;
   /* test for no change of value */
   prev_val_size = _value.length()+2;
   prev_val = new char [prev_val_size];
   need_write = true;
   if(!ini) return false;
   if(prev_val && ini->handler->opened())
   {
     const char *def_val;
     if(_value=="y") def_val = "n";
     else            def_val = "y";
     iniReadProfileString(ini,_section,_subsection,_item,def_val,prev_val,prev_val_size);
     if(_value==_value) need_write = false;
     delete prev_val;
   }
   if(!need_write) return true;
   tmpname = new char [(ini->fname.length()+1)*2];
   if(!tmpname) return false;
   if(!ini->handler->opened()) /* if file does not exist make it. */
   {
     _ret = __createIni(ini,_section,_subsection,_item,_value);
     if(_ret) ini->handler->open(ini->fname);
     goto Exit_WS;
   }
   ini->handler->seek(0L,BFile::Seek_Set);
   ActiveFile = ini->handler;
   hsrc = make_temp(ini->fname.c_str(),tmpname);
   if(hsrc == NULL_HANDLE) { _ret = false; goto Exit_WS; }
   __OsClose(hsrc);
   tmphandle = fopen(tmpname,"wt");
   if(tmphandle == NULL) { _ret = false; goto Exit_WS; }
   fprintf(tmphandle,HINI_HEADER);
   workstr = __FiCMaxStr();
   wstr2 = __FiCMaxStr();
   original = __FiCMaxStr();
   if(!workstr || !wstr2 || !original)
   {
     fclose(tmphandle);
     __OsDelete(tmpname);
     if(workstr) PFREE(workstr);
     if(wstr2) PFREE(wstr2);
     if(original) PFREE(original);
     _ret = false;
     goto Exit_WS;
   }
   sb_ok = ssb_ok = s_ok = ss_ok = i_ok = done = false;
   nled = 0;
   if(_section.empty()) { s_ok = sb_ok = true; if(_subsection.empty()) ss_ok = ssb_ok = true; }
   wstr2[0] = 1;
   Cond = true;
   if_on = false;
   while(1)
   {
     written = false;
     ini->handler->get_next_string(workstr,FI_MAXSTRLEN,original);
     if(workstr[0] == 0) break;
     if(FiisCommand(workstr))
     {
       std::string cstr;
       cstr = FiGetCommandString(workstr);
       if(cstr.substr(0,2)=="if" ||
	  cstr.substr(0,4)=="elif" ||
	  cstr.substr(0,4)=="else")
       if_on = true;
       if(cstr.substr(0,5)=="endif") if_on = false;
     }
     if(Cond)
     {
       if(FiisSection(workstr))
       {
	  if(!_section.empty())
	  {
	    FiGetSectionName(workstr,wstr2);
	    if(_section==wstr2)
	    {
	      s_ok = sb_ok = true;
	      nled = 2;
	      ss_ok = ssb_ok = _subsection.empty();
	    }
	    else s_ok = false;
	  }
	  else s_ok = false;
       }
       else
       if(FiisSubSection(workstr))
       {
	  if(!_subsection.empty())
	  {
	    FiGetSubSectionName(workstr,wstr2);
	    if(_subsection==wstr2)
	    {
	      ss_ok = ssb_ok = true;
	      nled = 4;
	    }
	    else ss_ok = false;
	  }
	  else ss_ok = false;
       }
       else
       if(FiisItem(workstr))
       {
	  FiGetItemName(workstr,wstr2);
	  if(_item==wstr2) i_ok = true;
	  if(i_ok && s_ok && ss_ok)
	  {
	    if(!done || if_on)
	    {
	       out_item(tmphandle,nled,_item,_value);
	       written = true;
	       done = true;
	    }
	  }
	  else i_ok = false;
      }
      /**********************************************/
      if(sb_ok && !s_ok)
      {
	  if(!done)
	  {
	    if(!ssb_ok && !ss_ok && !_subsection.empty()) { nled += 2; out_subsect(tmphandle,_subsection); }
	    out_item(tmphandle,nled,_item,_value);
	    written = false;
	    done = true;
	  }
      }
      if(s_ok && ssb_ok && !ss_ok)
      {
	 if(!done)
	 {
	     out_item(tmphandle,nled,_item,_value);
	     written = false;
	     done = true;
	 }
      }
    }
    if(!written) fprintf(tmphandle,"%s\n",original);
   }
   /************ this is end of loop *************/
   if(!done)
   {
     if(!sb_ok && !_section.empty())
     {
	nled = out_sect(tmphandle,_section);
	if(!_subsection.empty())
	{
	  ssb_ok = true;
	  nled+=out_subsect(tmphandle,_subsection);
	}
     }
     if(!ssb_ok && !_subsection.empty())
     {
	nled+=out_subsect(tmphandle,_subsection);
     }
     out_item(tmphandle,nled,_item,_value);
     done = true;
   }
   fclose(tmphandle);
   ini->handler->close();
   __OsDelete(ini->fname.c_str());
   __OsRename(tmpname,ini->fname.c_str());
   ini->handler->open(ini->fname);
   PFREE(workstr);
   PFREE(wstr2);
   PFREE(original);
   _ret = true;
   Exit_WS:
   PFREE(tmpname);
   return _ret;
}

static char *__partSect,*__partSubSect;
static hIniProfile *part_ini_profile;

static void __FASTCALL__ part_flush_item(any_t*data)
{
  ini_cache  *it;
  it = (ini_cache  *)data;
  __directWriteProfileString(part_ini_profile,
			     __partSect,
			     __partSubSect,
			     it->item,
			     it->v.value);
}

static void __FASTCALL__ part_flush_subsect(any_t*data)
{
  ini_cache  *it;
  it = (ini_cache  *)data;
  __partSubSect = it->item;
  la_ForEach(it->v.leaf,part_flush_item);
}

static void __FASTCALL__ part_flush_sect(any_t*data)
{
  ini_cache  *it;
  it = (ini_cache  *)data;
  __partSect = it->item;
  la_ForEach(it->v.leaf,part_flush_subsect);
}

static void  __FASTCALL__ __flushPartialCache(hIniProfile *ini)
{
  part_ini_profile = ini;
  la_ForEach((linearArray *)ini->cache,part_flush_sect);
}


bool __FASTCALL__ iniWriteProfileString(hIniProfile *ini,
					 const std::string& _section,
					 const std::string& _subsection,
					 const std::string& _item,
					 const std::string& _value)
{
   bool _ret;
   opened_ini = ini;
   _ret = false;
   if(ini->cache)
   {
     if((ini->flags & HINI_FULLCACHED) != HINI_FULLCACHED) goto flush_it;
     if(__addCache(_section,_subsection,_item,_value) != 0)
     {
       flush_it:
       __flushPartialCache(ini);
       __destroyCache((linearArray *)ini->cache);
       ini->cache = 0;
       goto do_def;
     }
   }
   else
   {
     do_def:
     _ret = __directWriteProfileString(ini,_section,_subsection,_item,_value);
   }
   return _ret;
}

static int __nled;
static FILE * flush_handler = NULL;

static void __FASTCALL__ flush_item(any_t*data)
{
  ini_cache  *it;
  it = (ini_cache  *)data;
  out_item(flush_handler,__nled,it->item,it->v.value);
}

static void __FASTCALL__ flush_subsect(any_t*data)
{
  ini_cache  *it;
  int _has_led;
  it = (ini_cache  *)data;
  _has_led = __nled;
  if(strlen(it->item)) __nled += out_subsect(flush_handler,it->item);
  la_ForEach(it->v.leaf,flush_item);
  __nled = _has_led;
}

static void __FASTCALL__ flush_sect(any_t*data)
{
  ini_cache  *it;
  it = (ini_cache  *)data;
  __nled = 0;
  if(strlen(it->item)) __nled += out_sect(flush_handler,it->item);
  la_ForEach(it->v.leaf,flush_subsect);
}

static bool  __FASTCALL__ __flushCache(hIniProfile *ini)
{
  if((ini->flags & HINI_UPDATED) == HINI_UPDATED && ini->cache)
  {
    if(ini->handler->opened()) ini->handler->close();
    flush_handler = __makeIni(ini);
    if(!flush_handler) return true;
    la_ForEach((linearArray *)ini->cache,flush_sect);
    fclose(flush_handler);
    ini->handler->open(ini->fname);
  }
  return false;
}
