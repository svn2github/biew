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
#include <vector>

using namespace	usr;

namespace	usr {
/******************************************************************\
* Low level routines                                               *
\******************************************************************/
    class Ini_Parser;
    class Ini_io : public Opaque {
	public:
	    Ini_io(Ini_Parser&);
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
	    Ini_Parser&			parent;
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
	    Variable_Set(Ini_Parser& parent);
	    virtual ~Variable_Set();

	    virtual void	add(const std::string& var,const std::string& asociate);
	    virtual void	remove(const std::string& var);
	    virtual void	clear();
	    virtual std::string	expand(const std::string& var);
	    virtual std::string	substitute(const std::string& src,char delim='%');
	private:
	    std::map<std::string,std::string> set;
	    Ini_Parser&		parent;
    };
/********************************************************************\
* Middle Level procedure                                             *
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
* Use only this function.                                            *
* This function calls UserFunc as CALLBACK routine.                  *
* All internal variables will be expanded, and command processor     *
* will be done. Unknown instruction and hotkeys should be ignored.   *
\********************************************************************/
    /** Contains information about current record in ini file */
    struct IniInfo {
	const char* section;	/**< section name */
	const char* subsection;	/**< subsection name */
	const char* item;	/**< item name */
	const char* value;	/**< value of item */
    };
		   /** Pointer to a user supplied function that receive readed record from ini file.
		     * @return                For continue of scaning - false
					      For terminating scaning - true (means: all done)
		     * @param info            pointers to current record from inni file
		     * @param data            pointers to user's data
		    **/
    typedef bool      (__FASTCALL__ *ini_user_func)(const IniInfo& info,any_t* data);

    class Ini_Parser : public Opaque {
	public:
	    Ini_Parser(ini_user_func usr,any_t* usr_data);
	    virtual ~Ini_Parser();

	    virtual void		file_parser(const std::string& filename);
	    virtual bool		string_parser(const std::string& curr_str);
	    virtual bool		command_parser(const std::string& cmd);
	    virtual bool		get_condition(const std::string& condstr);
	    virtual int			error(int ne,int row,const std::string& addinfo);
	    virtual void		aerror(int nError,int row,const std::string& addinfo="");
	    void			error_cl(int nError) { aerror(nError,file_info.back().first); }

	    virtual std::string		_get_bracket_string(const std::string& src,char obr,char cbr);
	    inline std::string		get_bracket_string(const std::string& str) { return _get_bracket_string(str,'"','"'); }
	    inline std::string		get_section_name(const std::string& src) { return _get_bracket_string(src,'[',']'); }
	    inline std::string		get_subsection_name(const std::string& src) { return _get_bracket_string(src,'<','>'); }

	    virtual std::string		get_value_of_item(const std::string& src);
	    virtual std::string		get_item_name(const std::string& src);
	    virtual std::string		get_command_string(const std::string& src);
		   /** Performs ini-file scanning.
		     * @return                none
		     * @param filename        Specifies name of file to be processed
		     * @param fuser           Specifies user-defined callback function
		     * @param data           Specifies user-defined data for callback function
		     * @note                  Before calling user-defined function
		     *                        all internal variables will be expanded,
		     *                        and command processor will be done.
		     *                        Unknown instruction and hotkeys should
		     *                        be ignored.
		    **/
	    static void			scan(const std::string& filename,ini_user_func fuser,any_t* data);
	protected:
	    friend Ini_io;
	    std::vector<std::pair<size_t,std::string> > file_info;
	    ini_user_func		user_proc;
	    any_t*			user_data;
	    Ini_io*			active_file;
	    unsigned char		case_sens; /**< 2 - case 1 - upper 0 - lower */
	    bool			ifSmarting;
	private:
	    static std::string		decode_error(int nError);

	    std::string			debug_str;
	    std::string			user_message;
	    Variable_Set		vars;
	    std::string			curr_sect;
	    std::string			curr_subsect;
    };

/******************************************************************\
* High level routines (similar to MS WIN SDK)                      *
\******************************************************************/
    class Ini_Profile : public Ini_Parser {
	public:
	    Ini_Profile();
	    virtual ~Ini_Profile();
		   /** Opens ini file for using with iniReadProfileString and iniWriteProfileString functions.
		     * @return                true on success
		     * @param filename        Specifies name of file to be open
		     * @param has_error       Pointer to the memory where will be stored error if occured
		     * @warning               You must not call any other function
		     *                        If error occured and has_error assigned
		     *                        non NULL value.
		     * @see                   close
		    **/
		virtual bool		open(const std::string& fname);

		   /** Closes ini file stream.
		     * @return                none
		     * @see                   open
		    **/
		virtual void		close();

		   /** Performs search of given item in ini file and reads it value if found.
		     * @return                length of readed value
		     * @param section         specifies section name
		     * @param subsection      specifies subsection name
		     * @param _item           specifies item name
		     * @param def_value       specifies default return value
		     * @param buffer          specifies buffer where will be stored readed value
		     * @param cbBuffer        specifies size of buffer.
		     * @note                  if given item is not present in
		     *                        ini file, then default value will
		     *                        returned.
		     * @see                   write
		    **/
		virtual std::string	read(const std::string& section,
					    const std::string& subsection,
					    const std::string& _item,
					    const std::string& def_value) const;

		   /** Writes given item to ini file.
		     * @return                true if operation performed successfully
		     * @param section         specifies section name
		     * @param subsection      specifies subsection name
		     * @param item            specifies item name
		     * @param value           specifies value of item
		     * @see                   read
		    **/
		virtual bool		write(const std::string& section,
					    const std::string& subsection,
					    const std::string& item,
					    const std::string& value);
	private:
	    void		file_scaning();
	    bool		__addCache(const std::string& section,const std::string& subsection,
					    const std::string& item,const std::string& value);
	    static bool	__FASTCALL__ __buildCache(const IniInfo& ini,any_t* data);
	    int			out_sect(std::fstream& fs,const std::string& section,unsigned nled);
	    int			out_subsect(std::fstream& fs,const std::string& subsection,unsigned nled);
	    void		out_item(std::fstream& fs,unsigned nled,const std::string& _item,const std::string& value);
	    bool		__makeIni(std::fstream& hout);
	    void		flush_item(std::fstream& flush_handler,const std::map<std::string,std::string>& inner);
	    void		flush_subsect(std::fstream& flush_handler,const std::map<std::string,std::map<std::string,std::string > >& inner);
	    void		flush_sect(std::fstream& flush_handler,const std::map<std::string,std::map<std::string,std::map<std::string,std::string> > >& _cache);
	    bool		__flushCache();

	    Ini_io*		handler;
	    std::string		fname;
	    std::map <std::string,std::map<std::string,std::map<std::string,std::string > > > cache;
	    bool		updated;
	    int			__nled;
    };
}// namespace	usr
#endif
