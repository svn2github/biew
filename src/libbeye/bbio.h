/**
 * @namespace   libbeye
 * @file        libbeye/bbio.h
 * @brief       This file contains prototypes of BBIO technology functions.
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
#ifndef __BBIO_HPP_INCLUDED
#define __BBIO_HPP_INCLUDED 1
#include <string>

#include "libbeye/libbeye.h"
#include "bstream.h"
/******************************************************************\
*  Buffered binary file streams input/output section               *
*  Helpful for read/write small size objects from/to file          *
\******************************************************************/

namespace	usr {
    class BBio_File : public binary_stream {
	public:
	    enum opt {
			 /** FORWARD: default (forward scan)
			     reposition of cache as 100% forward
			     from current file position */
		Opt_Forward  =0x0000,
		Opt_Db       =Opt_Forward,
			 /** RANDOM: middle scan
			     reposition of cache as 50% forward & 50%
			     backward from current file position */
		Opt_Random   =0x0001,
			 /** BACKWARD: backward scan
			     reposition of cache as 100% backward
			     from current file position */
		Opt_BackScan =0x0002,
			 /** RANDOM FORWARD: reposition of cache as 90% forward
			     & 10% backward from current file position */
		Opt_RForward =0x0003,
			 /** RANDOM BACKWARD: reposition of cache as 10% forward
			     & 90% backward from current file position */
		Opt_RBackScan=0x0004,
		Opt_DirMask  =0x000F /**< direction mask */
	    };

	    BBio_File(unsigned buffSize,unsigned optimization);
	    virtual ~BBio_File();
	    virtual bool		open(const std::string& fname,unsigned openmode);
	    virtual bool		chsize(__filesize_t newsize);
	    virtual bool		close();
	    virtual bool		eof() const;
	    virtual __filesize_t	flength() const;
	    virtual bool		flush();
	    virtual uint8_t		read(const data_type_qualifier__byte_t&);
	    virtual uint16_t		read(const data_type_qualifier__word_t&);
	    virtual uint32_t		read(const data_type_qualifier_dword_t&);
	    virtual uint64_t		read(const data_type_qualifier_qword_t&);
	    virtual bool		read(any_t* buffer,unsigned cbBuffer);
	    virtual bool		reread();
	    virtual bool		seek(__fileoff_t offset,e_seek origin);
	    virtual unsigned		get_optimization() const;
	    virtual unsigned		set_optimization(unsigned flags);
	    virtual __filesize_t	tell() const;
	    virtual bool		write(uint8_t bVal);
	    virtual bool		write(uint16_t wVal);
	    virtual bool		write(uint32_t dwVal);
	    virtual bool		write(uint64_t dwVal);
	    virtual bool		write(const any_t* buffer,unsigned cbBuffer);
	    virtual bool		dup(BBio_File&) const;
	    virtual binary_stream*	dup() const;
    private:
	    /** Virtual file buffer structure */
	    class binary_cache {
		public:
		    binary_cache(BBio_File& _parent,unsigned size);
		    virtual ~binary_cache();

		    unsigned 		buffsize() const { return bufsize; }
		    __filesize_t	fstart() const { return f_start; }
		    binary_cache&	operator=(const binary_cache& it);
		    bool		is_out_of_buffer(__filesize_t pos) const { return pos < f_start || pos >= f_start + bufsize; }
		    bool		is_out_of_contents(__filesize_t pos) const { return pos < f_start || pos >= f_start + buflen; }
		    bool		fill(__fileoff_t pos);
		    bool		flush();
		    bool		seek(__fileoff_t pos,e_seek origin);
		    unsigned char	read();
		    bool		write(unsigned char ch);
		    bool		read(char* buff,unsigned cbBuff);
		    bool		write(const char* buff,unsigned cbBuff);
		    void		chsize(__filesize_t newsize);

		    unsigned		buflen;  /**< length data, actually contains in buffer */
		    bool		updated; /**< true if buffer contains data, that not pesent in file */
		private:
		    __filesize_t	f_start; /**< logical position of mirror the buffer onto file */
		    unsigned		bufsize; /**< real size of buffer */
		    LocalPtr<char>	buffer;  /**< NULL - is illegal case */
		    BBio_File&	parent;
	    };

	    bool is_writeable(unsigned _openmode) const { return (_openmode & O_RDWR) || (_openmode & O_WRONLY); }
	    bool chk_eof(__fileoff_t& pos) { is_eof = false;
		/* Accessing memory after mmf[flength-1] causes GPF in MMF mode */
		/* so we must add special checks for it but no for read-write mode */
		/* Special case: flength() == 0. When file is being created pos == flength().*/
		    if(flength() && !is_writeable(openmode) && pos >= (__fileoff_t)flength()) {
			pos = flength()-1;
			is_eof = true;
			return false;
		    }
		return true;
	    }

	    bool seek_fptr(__fileoff_t& pos,int origin) {
		switch((int)origin) {
		    case Seek_Set: break;
		    case Seek_Cur: pos += filepos; break;
		    default:       pos += flength();
		}
		return chk_eof(pos);
	    }

	    Opaque		opaque;
	    __filesize_t	filepos;   /**< current logical position in file */
	    unsigned		openmode;  /**< mode,that OsOpen this file */
	    int			optimize;  /**< seek optimization */
	    binary_cache	vfb;       /**< buffered file */
	    bool		is_eof;    /**< Indicates EOF for buffering streams */
	    bool		founderr;
	    __filesize_t	_flength;
    };
    inline BBio_File::opt operator~(BBio_File::opt a) { return static_cast<BBio_File::opt>(~static_cast<unsigned>(a)); }
    inline BBio_File::opt operator|(BBio_File::opt a, BBio_File::opt b) { return static_cast<BBio_File::opt>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline BBio_File::opt operator&(BBio_File::opt a, BBio_File::opt b) { return static_cast<BBio_File::opt>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline BBio_File::opt operator^(BBio_File::opt a, BBio_File::opt b) { return static_cast<BBio_File::opt>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline BBio_File::opt operator|=(BBio_File::opt& a, BBio_File::opt b) { return (a=static_cast<BBio_File::opt>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline BBio_File::opt operator&=(BBio_File::opt& a, BBio_File::opt b) { return (a=static_cast<BBio_File::opt>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline BBio_File::opt operator^=(BBio_File::opt& a, BBio_File::opt b) { return (a=static_cast<BBio_File::opt>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace	usr
#endif

