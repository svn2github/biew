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
	    virtual unsigned		get_optimization() const;
	    virtual unsigned		set_optimization(unsigned flags);
	    virtual bool		dup(BBio_File&) const;
	    virtual binary_stream*	dup();
    private:
	    Opaque		opaque;
	    unsigned		buffsize;
	    char*		buffer;
	    int			optimize;  /**< seek optimization */
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

