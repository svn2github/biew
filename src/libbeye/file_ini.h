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
#include <fstream>
#include <map>

using namespace beye;

namespace beye {
    /** Contains information about current record in ini file */
    struct IniInfo {
	const char* section;	/**< section name */
	const char* subsection;	/**< subsection name */
	const char* item;	/**< item name */
	const char* value;	/**< value of item */
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
    class Ini_io : public Opaque {
	public:
	    Ini_io();
	    virtual ~Ini_io();

	    virtual bool		open( const std::string& filename );
	    virtual void		close();
	    virtual char*		get_next_string(char* store, unsigned int len, char *original );
	    virtual std::string		get_next_string();
	    virtual std::string		get_next_string(std::string& original);

	    virtual int			eof() const { return fs.eof(); }
	    virtual bool		seek(__fileoff_t off,std::ios_base::seekdir origin) { return fs.seekg(off,origin); }
	    inline void			rewind_ini() { fs.seekg(0L,std::ios_base::beg); }
	    inline bool			opened() const { return fs.is_open(); }
	private:
	    char*			GETS(char *str,unsigned num);

	    std::fstream		fs;
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

/******************************************************************\
* High level routines (similar to MS WIN SDK)                      *
\******************************************************************/
    struct hIniProfile {
	Ini_io*		handler;
	std::string	fname;
	std::map <std::string,std::map<std::string,std::map<std::string,std::string > > > cache;
	bool		updated;
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
}// namespace beye
#endif
