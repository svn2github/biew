#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
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
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include "libbeye/bstream.h"
#include "libbeye/file_ini.h"

enum {
    INI_MAXSTRLEN=4096 /**< Specifies maximal length of string, that can be readed from ini file */
};

/**
    List of possible errors that are generic
*/
enum {
    __INI_NOERRORS     = 0, /**< No errors */
    __INI_BADFILENAME  =-1, /**< Can not open file */
    __INI_TOOMANY      =-2, /**< Too many opened files */
    __INI_NOTMEM       =-3, /**< Memory exhausted */
    __INI_OPENCOND     =-4, /**< Opened 'if' (missing '#endif') */
    __INI_IFNOTFOUND   =-5, /**< Missing 'if' for 'endif' statement */
    __INI_ELSESTAT     =-6, /**< Missing 'if' for 'else' statement */
    __INI_UNRECOGN     =-7, /**< Unknown '#' directive */
    __INI_BADCOND      =-8, /**< Syntax error in 'if' statement */
    __INI_OPENSECT     =-9, /**< Expected opened section or subsection or invalid string */
    __INI_BADCHAR      =-10, /**< Bad character on line (possible lost comment) */
    __INI_BADVAR       =-11, /**< Bad variable in 'set' or 'delete' statement */
    __INI_BADVAL       =-12, /**< Bad value of variable in 'set' statement */
    __INI_NOVAR        =-13, /**< Unrecognized name of variable in 'delete' statement */
    __INI_NODEFVAR     =-14, /**< Detected undefined variable (case sensitivity?) */
    __INI_ELIFSTAT     =-15, /**< Missing 'if' for 'elif' statement */
    __INI_OPENVAR      =-16, /**< Opened variable on line (use even number of '%' characters) */
    __INI_NOTEQU       =-17, /**< Lost or mismatch character '=' in assigned expression */
    __INI_USER         =-18, /**< User defined message */
    __INI_FIUSER       =-19  /**< User error */
};
/**
    possible answers to the errors
*/
enum {
    __INI_IGNORE   =0, /**< Ignore error and continue */
    __INI_EXITPROC =1 /**< Terminate the program execution */
};
/**
    return constants for FiSearch
*/
enum {
    __INI_NOTFOUND   =0, /**< Required string is not found */
    __INI_SECTION    =1, /**< Required string is section */
    __INI_SUBSECTION =2, /**< required string is subsection */
    __INI_ITEM       =3  /**< required string is item */
};

namespace	usr {
static const unsigned __C_EOF=0x1A;
static const char iniOpenComment=';';
static const char iniLegalSet[] = " _0123456789"
			   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			   "abcdefghijklmnopqrstuvwxyz";

static inline bool IS_VALID_NAME(const std::string& name) { return name.find_first_not_of(iniLegalSet) == std::string::npos; }
static inline bool IS_SECT(const std::string& str,char ch) { return str[0] == ch; }
static inline bool is_section(const std::string& str ) { return IS_SECT(str,'['); }
static inline bool is_subsection(const std::string& str ) { return IS_SECT(str,'<'); }
static inline bool is_command(const std::string& str ) { return IS_SECT(str,'#'); }
static inline bool is_item(const std::string& str) { return !(is_section(str) || is_subsection(str) || is_command(str)); }
/**************************************************************\
*                      Low level support                       *
\**************************************************************/
Ini_io::Ini_io(Ini_Parser& _parent):parent(_parent) {}
Ini_io::~Ini_io() { if(fs.is_open()) close(); }
bool Ini_io::open(const std::string& filename)
{
    /* Try to load .ini file entire into memory */
    fs.open(filename.c_str(),std::ios_base::in|std::ios_base::binary);
    /* Note! All OSes except DOS-DOS386 allows opening of empty filenames as /dev/null */
    if(!fs.is_open() && filename[0]) parent.aerror(__INI_BADFILENAME,0,filename);
    parent.file_info.push_back(std::make_pair(0,filename));
    return true;
}

void Ini_io::close()
{
    parent.file_info.pop_back();
    if(fs.is_open()) fs.close();
}

char* Ini_io::get_next_string(char * str,unsigned int size,char *original)
{
  unsigned char *sret;
  unsigned len;
  unsigned char ch;
  str[0] = 0;
  while(!fs.eof())
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
    parent.file_info.back().first++;
    if((sret == NULL && !fs.eof())) parent.aerror(__INI_BADFILENAME,0,str);
    sret = (unsigned char*)strchr(str,iniOpenComment);
    if(sret) *sret = 0;

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
  return str;
}

std::string Ini_io::get_next_string() {
    char tmp[INI_MAXSTRLEN];
    get_next_string(tmp,sizeof(tmp),NULL);
    return std::string(tmp);
}

std::string Ini_io::get_next_string(std::string& original) {
    char tmp[INI_MAXSTRLEN];
    char org[INI_MAXSTRLEN];
    get_next_string(tmp,sizeof(tmp),org);
    original=org;
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
     fs.read(&ch,sizeof(char));
     if(ch == '\n' || ch == '\r')
     {
       *str = ch;  str++;
       fs.read(&ch1,sizeof(char));
       if((ch1 == '\n' || ch1 == '\r') && ch != ch1)
       {
	 *str = ch; str++;
       }
       else
       {
	 if(fs.eof())
	 {
	   if((signed char)ch1 != -1 && ch1 != __C_EOF)
	   {
	     *str = ch1; str++;
	   }
	   break;
	 }
	 fs.seekg(-1,std::ios_base::cur);
       }
       break;
     }
     if(fs.eof())
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

/************* BEGIN of List Var section ********************/

Variable_Set::Variable_Set(Ini_Parser& _parent):parent(_parent) {}
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
	char tmp[INI_MAXSTRLEN+1];
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
	if( npercent%2 ) parent.error_cl(__INI_OPENVAR);
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

/*************************************************************\
*                  Middle level support                       *
\*************************************************************/
/*************** END of List Var Section ***************/
Ini_Parser::Ini_Parser(ini_user_func usr,any_t* data):user_proc(usr),user_data(data),case_sens(2),ifSmarting(true),vars(*this) {}
Ini_Parser::~Ini_Parser() { vars.clear(); }

void Ini_Parser::Ini_Parser::aerror(int nError,int row,const std::string& addinfo)
{
    int eret = 0;
    eret = error(nError,row,addinfo);
    if(eret == __INI_EXITPROC) throw std::invalid_argument("Ini_Parser: user request");
}

static const char* list[] = {
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

std::string Ini_Parser::decode_error(int nError)
{
    std::string ret;
    nError = abs(nError);
    if(nError >= 0 && nError <= abs(__INI_FIUSER)) ret = list[nError];
    else ret = "Unknown Error";
    return ret;
}

int Ini_Parser::error(int ne,int row,const std::string& addinfo)
{
    std::fstream herr;
    std::string what;
    std::ostringstream os;
    herr.open("fi_syserr.$$$",std::ios_base::out);
    herr<<"About : [.Ini] file run-time support library. Written by Nickols_K"<<std::endl<<"Detected ";
    if(ne != __INI_TOOMANY && ~file_info.empty()) {
	herr<<(row ? "fatal" : "")<<" error in : "<<file_info.back().second;
    }
    herr<<std::endl;
    if(row) herr<<"At line : "<<row<<std::endl;
    what = decode_error(ne);
    if(!addinfo.empty()) os<<what<<" "<<addinfo;
    else os<<what;
    herr<<"Message : "<<os.str()<<std::endl;
    if(!user_message.empty()) herr<<"User message : "<<user_message<<std::endl;
    if(!debug_str.empty()) herr<<"Debug info: '"<<debug_str<<"'"<<std::endl;
    herr.close();
    std::cerr<<std::endl<<"Error in .ini file."<<std::endl<<"File fi_syser.$$$ created."<<std::endl;
    return __INI_EXITPROC;
}

std::string Ini_Parser::_get_bracket_string(const std::string& src,char obr,char cbr)
{
    std::string rc;
    size_t ends;
    if(src[0] == obr) {
	unsigned len;
	ends = src.find(cbr,1);
	if(ends==std::string::npos) goto err;
	if(src[ends+1]) Ini_Parser::error_cl(__INI_BADCHAR);
	len = ends-1;
	rc=src.substr(1,len);
    } else {
err:
	Ini_Parser::error_cl(__INI_OPENSECT);
    }
    return rc;
}

std::string Ini_Parser::get_value_of_item(const std::string& src)
{
    std::string rc;
    size_t from;
    from = src.find('=');
    if(from!=std::string::npos) rc=&src.c_str()[++from];
    else     Ini_Parser::error_cl(__INI_NOTEQU);
    return rc;
}

std::string Ini_Parser::get_item_name(const std::string& src)
{
    std::string rc;
    size_t sptr;
    unsigned len;
    sptr = src.find('=');
    if(sptr!=std::string::npos) {
	len = sptr;
	rc=src.substr(0,len);
	if(!IS_VALID_NAME(rc)) Ini_Parser::error_cl(__INI_BADCHAR);
    }
    return rc;
}

std::string Ini_Parser::get_command_string(const std::string& src)
{
    std::string rc;
    unsigned i;
    i = strspn(src.c_str()," #");
    rc=src.substr(i);
    return rc;
}

bool Ini_Parser::get_condition( const std::string& condstr)
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
    if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
    ret = false;
    if(strcmp(cond,"==") == 0)  ret = user_ass==rvar;
    else if(strcmp(cond,"!=") == 0)  ret = user_ass!=rvar;
    else error_cl(__INI_BADCOND);
    return ret;
}

bool Ini_Parser::command_parser( const std::string& cmd )
{
    std::string word,a,v;
    std::string fdeb_save;
    Tokenizer tokenizer(cmd);
    static bool cond_ret = true;
    word = tokenizer.next_legal_word(&iniLegalSet[1]);
    std::transform(word.begin(),word.end(),word.begin(),::tolower);
    if(word=="include") {
	std::string bracket;
	std::string _v;
	char pfile[FILENAME_MAX+1],*pfp,*pfp2;
	bracket = get_bracket_string(tokenizer.data());
	_v=ifSmarting?vars.substitute(bracket):bracket;
	fdeb_save = debug_str;
	/* make path if no path specified */
	strcpy(pfile,file_info.back().second.c_str());
	pfp=strrchr(pfile,'\\');
	pfp2=strrchr(pfile,'/');
	pfp=std::max(pfp,pfp2);
	if(pfp && (_v.find('\\')==std::string::npos || _v.find('/')==std::string::npos)) strcpy(pfp+1,_v.c_str());
	else    strcpy(pfile,_v.c_str());
	file_parser(pfile);
	debug_str = fdeb_save;
	goto Exit_CP;
    } else if(word=="set") {
	v = tokenizer.next_legal_word(&iniLegalSet[1]);
	if(tokenizer.curr_char() != '=') error_cl(__INI_NOTEQU);
	tokenizer.next_char();
	a = tokenizer.next_word(" ");
	if(v[0] == '\0') error_cl(__INI_BADVAR);
	if(a[0] == '\0') error_cl(__INI_BADVAL);
	vars.add(v,a);
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	goto Exit_CP;
    } else if(word=="delete") {
	std::string _a;
	v = tokenizer.next_legal_word(&iniLegalSet[1]);
	_a=ifSmarting?vars.substitute(v):v;
	if(_a[0] == '\0') error_cl(__INI_BADVAR);
	vars.remove(_a);
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	goto Exit_CP;
    } else if(word=="reset") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	vars.clear();
	goto Exit_CP;
     } else if(word=="case") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	case_sens = 2;
	goto Exit_CP;
    } else if(word=="smart") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	ifSmarting = true;
	goto Exit_CP;
    } else if(word=="nosmart") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	ifSmarting = false;
	goto Exit_CP;
    } else if(word=="uppercase") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	case_sens = 1;
	goto Exit_CP;
    } else if(word=="lowercase") {
	if(tokenizer.curr_char()) error_cl(__INI_BADCHAR);
	case_sens = 0;
	goto Exit_CP;
    } else if(word=="error") {
	std::string _a;
	std::string sptr=tokenizer.tail();
	_a=ifSmarting?vars.substitute(sptr):sptr;
	user_message = _a.c_str();
	error_cl(__INI_FIUSER);
    } else if(word=="else") error_cl(__INI_ELSESTAT);
    else if(word=="endif") error_cl(__INI_IFNOTFOUND);
    else if(word=="elif") error_cl(__INI_ELIFSTAT);
    else if(word=="if") {
	std::string sstore;
	unsigned int nsave;
	bool Condition,BeenElse;
	int nLabel;
	nsave = file_info.back().first;
	cond_ret = Condition = get_condition(tokenizer.data());
	nLabel = 1;
	BeenElse = false;
	while(!active_file->eof()) {
	    sstore=active_file->get_next_string();
	    if(sstore[0] == '\0') goto Exit_CP;
	    if(is_command(sstore)) {
		a = get_command_string(sstore);
		if(case_sens == 1) std::transform(a.begin(),a.end(),a.begin(),::toupper);
		if(case_sens == 0) std::transform(a.begin(),a.end(),a.begin(),::tolower);
		Tokenizer c_tokenizer(a);
		v = c_tokenizer.next_word(" ");
		std::transform(v.begin(),v.end(),v.begin(),::tolower);
		if(v=="if" && !Condition) nLabel++;
		if(v=="endif") {
		    nLabel--;
		    if(nLabel == 0) goto Exit_CP;
		    if(nLabel <  0) error_cl(__INI_IFNOTFOUND);
		}
		if(v=="else" && nLabel == 1) {
		    if( BeenElse ) error_cl(__INI_ELSESTAT);
		    if( nLabel == 1 ) cond_ret = Condition = (Condition ? false : true);
		    if( nLabel == 1 ) BeenElse = true;
		    continue;
		}
		if(v=="elif" && nLabel == 1) {
		    if( BeenElse ) error_cl(__INI_ELIFSTAT);
		    if( nLabel == 1 ) cond_ret = Condition = get_condition(c_tokenizer.data());
		    if( nLabel == 1 ) BeenElse = true;
		    continue;
		}
		if(Condition) command_parser(a);
	    } else {
		if(Condition) string_parser(sstore);
	    }
	} // while
	Ini_Parser::aerror(__INI_OPENCOND,nsave,"");
    } // if word==if
    error_cl(__INI_UNRECOGN);
    Exit_CP:
    return cond_ret;
}

bool Ini_Parser::string_parser(const std::string& curr_str)
{
    std::string item,val;
    if(is_command(curr_str))
    {
      std::string _item;
      _item = get_command_string(curr_str);
      command_parser(_item);
      return false;
    }
    else
    if(is_section(curr_str))
    {
      curr_sect = get_section_name(curr_str);
      return false;
    }
    else
    if(is_subsection(curr_str))
    {
      curr_subsect = get_subsection_name(curr_str);
      return false;
    }
    else
    if(is_item(curr_str))
    {
      std::string buffer;
      bool retval;
      IniInfo info;
      buffer=ifSmarting?vars.substitute(curr_str):curr_str;
      item = get_item_name(buffer);
      retval = false;
      if(item[0])
      {
       val = get_value_of_item(buffer);
       if(!curr_sect.empty()) info.section = curr_sect.c_str();
       else info.section = "";
       if(!curr_subsect.empty()) info.subsection = curr_subsect.c_str();
       else info.subsection = "";
       info.item = item.c_str();
       info.value = val.c_str();
       if(user_proc) retval = (*user_proc)(info,user_data);
      }
      else Ini_Parser::error_cl(__INI_BADCHAR);
      return retval;
    }
  return false;
}

void Ini_Parser::file_parser(const std::string& filename)
{
  std::string work_str, ondelete;
  Ini_io& h = *new(zeromem) Ini_io(*this);
  Ini_io* oldh = active_file;
  bool done;
  h.open(filename);
  active_file = &h;
  debug_str = ondelete = work_str;
  while(!h.eof())
  {
      work_str=h.get_next_string();
      if(!work_str[0]) break;
      if(case_sens == 1) std::transform(work_str.begin(),work_str.end(),work_str.begin(),::toupper);
      if(case_sens == 0) std::transform(work_str.begin(),work_str.end(),work_str.begin(),::tolower);
      done = string_parser(work_str);
      if(done) break;
  }
  h.close();
  active_file = oldh;
}

void Ini_Parser::scan(const std::string& filename,ini_user_func usrproc,any_t* data)
{
    Ini_Parser* parser = new(zeromem) Ini_Parser(usrproc,data);
    parser->file_parser(filename);
    delete parser;
}

void Ini_Profile::file_scaning()
{
    std::string work_str, ondelete;
    Ini_io* oldh = active_file;
    bool done;
    active_file = handler;
    handler->rewind_ini();
    while(!handler->eof()) {
	work_str=handler->get_next_string();
	if(!work_str[0]) break;
	if(case_sens == 1) std::transform(work_str.begin(),work_str.end(),work_str.begin(),::toupper);
	if(case_sens == 0) std::transform(work_str.begin(),work_str.end(),work_str.begin(),::tolower);
	done = string_parser(work_str);
	if(done) break;
    }
    active_file = oldh;
}

/*****************************************************************\
*                      High level support                          *
\******************************************************************/
bool Ini_Profile::__addCache(const std::string& section,const std::string& subsection,
			const std::string& item,const std::string& value)
{
    std::map<std::string,std::map<std::string,std::string> >& inner = cache[section];
    std::map<std::string,std::string>& in_inner = inner[subsection];
    std::map<std::string,std::string>::iterator iter=in_inner.find(item);
    if(iter==in_inner.end()) in_inner.insert(std::make_pair(item,value));
    else (*iter).second = value;
    return false;
}

bool Ini_Profile::__buildCache(const IniInfo& ini,any_t* data)
{
    Ini_Profile* self = reinterpret_cast<Ini_Profile*>(data);
    return self->__addCache(ini.section,ini.subsection,ini.item,ini.value);
}

int Ini_Profile::out_sect(std::fstream& fs,const std::string& section,unsigned nled)
{
    for(unsigned i = 0;i < nled;i++) fs<<" ";
    fs<<"[ "<<section<<" ]"<<std::endl;
    return 2;
}

int Ini_Profile::out_subsect(std::fstream& fs,const std::string& subsection,unsigned nled)
{
    for(unsigned i = 0;i < nled;i++) fs<<" ";
    fs<<"< "<<subsection<<" >"<<std::endl;
    return 2;
}

void Ini_Profile::out_item(std::fstream& fs,unsigned nled,const std::string& _item,const std::string& value)
{
    char *sm_char;
    unsigned i;
    sm_char = (char*)strchr(value.c_str(),'%');
    if(sm_char && ifSmarting) fs<<"#nosmart"<<std::endl;
    for(i = 0;i < nled;i++) fs<<" ";
    fs<<_item<<" = ";
    fs.write(value.c_str(),value.length());
    fs<<std::endl;
    if(sm_char && ifSmarting) fs<<"#smart"<<std::endl;
}

bool Ini_Profile::open(const std::string& _fname)
{
    bool rc=false;
    bool has_error=true;
    handler=new(zeromem) Ini_io(*this);
    fname=_fname;
    updated=false;
    if(binary_stream::exists(_fname)) rc=handler->open(fname);

    if(rc) file_scaning();
    has_error = !rc;
    return has_error;
}

void Ini_Profile::close()
{
    __flushCache();
    cache.clear();
}

std::string Ini_Profile::read(const std::string& section,const std::string& subsection,
				const std::string& _item,const std::string& def_value) const
{
    std::string value=def_value;
    if(handler->opened()) {
	std::map<std::string,std::map<std::string,std::map<std::string,std::string> > >::const_iterator inner = cache.find(section);
	if(inner!=cache.end()) {
	    std::map<std::string,std::map<std::string,std::string> >::const_iterator in = (*inner).second.find(subsection);
	    if(in!=(*inner).second.end()) {
		for(std::map<std::string,std::string>::const_iterator it=(*in).second.begin();it!=(*in).second.end();++it) {
		    if((*it).first==_item) {
			value=(*it).second;
			break;
		    }
		}
	    }
	}
    }
    return value;
}

static const char* HINI_HEADER[]={
"; This file was generated automatically by BEYELIB.",
"; WARNING: Any changes made by hands may be lost the next time you run the program."
};

bool Ini_Profile::__makeIni(std::fstream& hout)
{
    if(binary_stream::exists(fname)) binary_stream::unlink(fname);
    hout.open(fname.c_str(),std::ios_base::out);
    if(hout.is_open()) for(unsigned i=0;i<2;i++) hout<<HINI_HEADER[i]<<std::endl;
    return true;
}

bool Ini_Profile::write(const std::string& _section,
			const std::string& _subsection,
			const std::string& _item,
			const std::string& _value)
{
    updated=true;
    __addCache(_section,_subsection,_item,_value);
    return false;
}

void Ini_Profile::flush_item(std::fstream& flush_handler,const std::map<std::string,std::string>& inner)
{
    for(std::map<std::string,std::string>::const_iterator it=inner.begin();it!=inner.end();++it) {
	out_item(flush_handler,__nled,(*it).first,(*it).second);
    }
}

void Ini_Profile::flush_subsect(std::fstream& flush_handler,const std::map<std::string,std::map<std::string,std::string > >& inner)
{
    int _has_led = __nled;
    for(std::map<std::string,std::map<std::string,std::string> >::const_iterator it=inner.begin();it!=inner.end();++it) {
	__nled += out_subsect(flush_handler,(*it).first,__nled);
	flush_item(flush_handler,(*it).second);
	__nled=_has_led;
    }
}

void Ini_Profile::flush_sect(std::fstream& flush_handler,const std::map<std::string,std::map<std::string,std::map<std::string,std::string> > >& _cache)
{
    __nled = 0;
    int _has_led = __nled;
    for(std::map<std::string,std::map<std::string,std::map<std::string,std::string> > >::const_iterator it=_cache.begin();it!=_cache.end();++it) {
	__nled += out_sect(flush_handler,(*it).first,__nled);
	flush_subsect(flush_handler,(*it).second);
	__nled=_has_led;
    }
}

bool Ini_Profile::__flushCache()
{
    if(updated) {
	if(handler->opened()) handler->close();
	std::fstream flush_fs;
	__makeIni(flush_fs);
	if(!flush_fs.is_open()) return true;
	flush_sect(flush_fs,cache);
	flush_fs.close();
	handler->open(fname);
    }
    return false;
}
Ini_Profile::Ini_Profile():Ini_Parser(Ini_Profile::__buildCache,this),__nled(0) {}
Ini_Profile::~Ini_Profile() { close(); delete handler; }
} // namespace	usr
