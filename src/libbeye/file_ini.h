/**
 * @namespace   libbeye
 * @file        libbeye/file_ini.h
 * @brief       This file contains prototypes of .ini file services.
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
 * @warning     All internal functions is undocumented
 * @todo        To document internal functions
**/
#ifndef __FILE_INI_RUNTIME_SUPPORT_SYSTEM__
#define __FILE_INI_RUNTIME_SUPPORT_SYSTEM__ 1
#include <map>

#include "libbeye/bbio.h"
using namespace beye;

/**
    List of possible errors that are generic
*/
enum {
    __FI_NOERRORS     = 0, /**< No errors */
    __FI_BADFILENAME  =-1, /**< Can not open file */
    __FI_TOOMANY      =-2, /**< Too many opened files */
    __FI_NOTMEM       =-3, /**< Memory exhausted */
    __FI_OPENCOND     =-4, /**< Opened 'if' (missing '#endif') */
    __FI_IFNOTFOUND   =-5, /**< Missing 'if' for 'endif' statement */
    __FI_ELSESTAT     =-6, /**< Missing 'if' for 'else' statement */
    __FI_UNRECOGN     =-7, /**< Unknown '#' directive */
    __FI_BADCOND      =-8, /**< Syntax error in 'if' statement */
    __FI_OPENSECT     =-9, /**< Expected opened section or subsection or invalid string */
    __FI_BADCHAR      =-10, /**< Bad character on line (possible lost comment) */
    __FI_BADVAR       =-11, /**< Bad variable in 'set' or 'delete' statement */
    __FI_BADVAL       =-12, /**< Bad value of variable in 'set' statement */
    __FI_NOVAR        =-13, /**< Unrecognized name of variable in 'delete' statement */
    __FI_NODEFVAR     =-14, /**< Detected undefined variable (case sensitivity?) */
    __FI_ELIFSTAT     =-15, /**< Missing 'if' for 'elif' statement */
    __FI_OPENVAR      =-16, /**< Opened variable on line (use even number of '%' characters) */
    __FI_NOTEQU       =-17, /**< Lost or mismatch character '=' in assigned expression */
    __FI_USER         =-18, /**< User defined message */
    __FI_FIUSER       =-19  /**< User error */
};
/**
    possible answers to the errors
*/
enum {
    __FI_IGNORE   =0, /**< Ignore error and continue */
    __FI_EXITPROC =1 /**< Terminate the program execution */
};
/**
    return constants for FiSearch
*/
enum {
    __FI_NOTFOUND   =0, /**< Required string is not found */
    __FI_SECTION    =1, /**< Required string is section */
    __FI_SUBSECTION =2, /**< required string is subsection */
    __FI_ITEM       =3  /**< required string is item */
};

/** Contains information about current record in ini file */
typedef struct tagIniInfo
{
  const char * section;      /**< section name */
  const char * subsection;   /**< subsection name */
  const char * item;         /**< item name */
  const char * value;        /**< value of item */
}IniInfo;

enum {
    FI_MAXSTRLEN=255 /**< Specifies maximal length of string, that can be readed from ini file */
};

/********************************************************************\
* Middle Level procedure                                             *
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
* Use only this function.                                            *
* This function calls UserFunc as CALLBACK routine.                  *
* All internal variables will be expanded, and command processor     *
* will be done. Unknown instruction and hotkeys should be ignored.   *
\********************************************************************/
		   /** Pointer to a user supplied function that receive readed record from ini file.
		     * @return                For continue of scaning - false
					      For terminating scaning - true (means: all done)
		     * @param info            pointers to current record from inni file
		    **/
typedef bool      (__FASTCALL__ *FiUserFunc)(IniInfo * info);

		   /** Performs ini-file scanning.
		     * @return                none
		     * @param filename        Specifies name of file to be processed
		     * @param fuser           Specifies user-defined callback function
		     * @note                  Before calling user-defined function
		     *                        all internal variables will be expanded,
		     *                        and command processor will be done.
		     *                        Unknown instruction and hotkeys should
		     *                        be ignored.
		     * @see                   FiUserFunc
		    **/
void          __FASTCALL__ FiProgress(const std::string& filename,FiUserFunc fuser);

/******************************************************************\
* Low level routines                                               *
\******************************************************************/
namespace beye {
    class Ini_io : public Opaque {
	public:
	    Ini_io();
	    virtual ~Ini_io();

	    virtual bool		open( const std::string& filename );
	    virtual void		close();
	    virtual char*		get_next_string(char* store, unsigned int len, char *original );
	    virtual std::string		get_next_string();

	    virtual int			eof() const { return handler.eof(); }
	    virtual bool		seek(__fileoff_t off,int origin) const { return handler.seek(off,origin); }
	    inline void			rewind_ini() const { handler.seek(0L,BFile::Seek_Set); }
	    inline bool			opened() const { return _opened; }
	private:
	    char*			GETS(char *str,unsigned num);

	    bool		_opened;
	    BFile&		handler;
    };

    class Tokenizer : public Opaque {
	public:
	    Tokenizer(const std::string& _src);
	    virtual ~Tokenizer();

	    virtual size_t	next_length(const std::string& illegal_symbols) const;
	    virtual std::string	next_word(const std::string& illegal_symbols);
	    virtual size_t	next_legal_length(const std::string& legal_symbols) const;
	    virtual std::string	next_legal_word(const std::string& legal_symbols);
	    virtual char	next_char() { return src[iptr++]; }
	    virtual char	curr_char() const { return src[iptr]; }
	    virtual std::string	tail();
	    virtual const char*	data() const { return &src.c_str()[iptr]; }
	private:
	    std::string src;
	    size_t iptr;
    };

/**
    variables set
*/
    class Variable_Set : public Opaque {
	public:
	    Variable_Set();
	    virtual ~Variable_Set();

	    virtual void	add(const std::string& var,const std::string& asociate);
	    virtual void	remove(const std::string& var);
	    virtual void	clear();
	    virtual std::string	expand(const std::string& var);
	    virtual std::string	substitute(const std::string& src,char delim='%');
	private:
	    std::map<std::string,std::string> set;
    };
}// namespace beye

/******************************************************************\
* High level routines (similar to MS WIN SDK)                      *
\******************************************************************/
enum {
    HINI_FULLCACHED=0x0001,
    HINI_UPDATED   =0x0002
};

struct hIniProfile
{
   Ini_io*	handler;
   std::string	fname;
   any_t*	cache;
   unsigned	flags;
};

		   /** Opens ini file for using with iniReadProfileString and iniWriteProfileString functions.
		     * @return                handle of opened stream
		     * @param filename        Specifies name of file to be open
		     * @param has_error       Pointer to the memory where will be stored error if occured
		     * @warning               You must not call any other function
		     *                        If error occured and has_error assigned
		     *                        non NULL value.
		     * @see                   iniCloseFile
		    **/
extern hIniProfile*    __FASTCALL__ iniOpenFile(const std::string& fname,bool *has_error);

		   /** Closes ini file stream.
		     * @return                none
		     * @param ini             handle of opened stream
		     * @see                   iniOpenFile
		    **/
extern void            __FASTCALL__ iniCloseFile(hIniProfile *ini);

		   /** Performs search of given item in ini file and reads it value if found.
		     * @return                length of readed value
		     * @param ini             handle of opened stream
		     * @param section         specifies section name
		     * @param subsection      specifies subsection name
		     * @param _item           specifies item name
		     * @param def_value       specifies default return value
		     * @param buffer          specifies buffer where will be stored readed value
		     * @param cbBuffer        specifies size of buffer.
		     * @note                  if given item is not present in
		     *                        ini file, then default value will
		     *                        returned.
		     * @see                   iniWriteProfileString
		    **/
extern unsigned __FASTCALL__ iniReadProfileString(hIniProfile *ini,
				     const std::string& section,
				     const std::string& subsection,
				     const std::string& _item,
				     const std::string& def_value,
				     char *buffer,
				     unsigned cbBuffer);

		   /** Writes given item to ini file.
		     * @return                true if operation performed successfully
		     * @param ini             handle of opened stream
		     * @param section         specifies section name
		     * @param subsection      specifies subsection name
		     * @param item            specifies item name
		     * @param value           specifies value of item
		     * @see                   iniReadProfileString
		    **/
extern bool __FASTCALL__ iniWriteProfileString(hIniProfile *ini,
				     const std::string& section,
				     const std::string& subsection,
				     const std::string& item,
				     const std::string& value);

#endif
