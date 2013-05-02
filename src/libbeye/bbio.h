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
#ifndef __BBIO_H
#define __BBIO_H 1
#include <string>

#include "libbeye/libbeye.h"
/******************************************************************\
*  Buffered binary file streams input/output section               *
*  Helpful for read/write small size objects from/to file          *
\******************************************************************/

namespace beye {
/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
/** Virtual file buffer structure */
struct vfb {
     bhandle_t     handle;    /**< file handle */
    __filesize_t   FBufStart; /**< logical position of mirror the buffer onto file */
    char*          MBuffer;   /**< NULL - not buffered i/o */
    unsigned       MBufLen;   /**< length data, actually contains in buffer */
    unsigned       MBufSize;  /**< real size of buffer */
    bool           updated;   /**< true if buffer contains data, that not pesent in file */
};

/** Memory mapped buffer structure */
struct mmb {
    mmfHandle       mmf;       /**< If OS support MMF contains handle of memory-mapped file */
    any_t*          mmf_addr;  /**< If OS support MMF contains base address of memory where file is mapped */
};

class BFile;
extern BFile bNull; /**< Stream associated with STDERR */

class BFile : public Opaque {
    private:
	Opaque		opaque;
	__filesize_t	FilePos;   /**< current logical position in file */
	__filesize_t	FLength;   /**< real length of the file */
	char*		FileName;  /**< Real file name of opened file */
	unsigned	openmode;  /**< mode,that OsOpen this file */
	int		optimize;  /**< seek optimization */
	bool		is_mmf;    /**< indicates that 'mmb' is used */
	bool		primary_mmf; /**< If this is set then we have not duplicated handle */
	union {                     /**< cache subsystem */
	    struct vfb	vfb;       /**< buffered file */
	    struct mmb*	mmb;       /**< Pointer to memory mapped file. We must have pointer, but not object!!! It for bioDupEx */
	}b;
	bool		is_eof;    /**< Indicates EOF for buffering streams */
	bool		founderr;
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
	    Opt_DirMask  =0x000F, /**< direction mask */
	    Opt_UseMMF   =0xFFFF, /**< Use mmf instead buffering i/o. This covers all optimizations */
	    Opt_NoCache  =0x8000, /**< disable cache */

	    Seek_Set     =SEEKF_START, /**< specifies reference location from begin of file */
	    Seek_Cur     =SEEKF_CUR,   /**< specifies reference location from current position of file */
	    Seek_End     =SEEKF_END   /**< specifies reference location from end of file */
	};

	BFile();
	virtual ~BFile();
		   /** Opens existed file and buffered it
		     * @return                handle of stream
		     * @param fname           indicates name of file to be open
		     * @param openmode        indicates opening mode flags - BIO_*
		     * @param buffSize        indicates size of buffer. Value UINT_MAX indicates - buffering entire file.
		     * @note                  Returns bNull if opening is fail.
		     * @warning               Carefully use parameter - buffSize.
		     *                        If you created new file with 0 bytes
		     *                        of length, and value of buffSize =
		     *                        UINT_MAX, then i/o will be unbuffered.
		     *                        It better to use values < UINT_MAX.
		     *                        Value UINT_MAX better to use for readonly
		     *                        operations for small files to load those into
		     *                        memory entire.
		     * @see                   bioClose
		    **/
	bool	open(const std::string& fname,unsigned openmode,unsigned buffSize,unsigned optimization);

		   /** Changes size of opened file.
		     * @return                true if operation was succesfully performed
		     * @param newsize         new size of file in bytes
		     * @warning               If file is truncated, the data from
		     *                        the new end of file to the original
		     *                        end of the file are lost.
		    **/
	bool	chsize(__filesize_t newsize);

		   /** Closes opened stream.
		     * @return                true if operation was succesfully performed
		     * @see                   bioOpen
		    **/
	bool	close();

		   /** Determines whether a opened stream has reached the End of File.
		     * @return                true if EOF has reached
		    **/
	bool	eof() const;

		   /** Returns the length (in bytes) of file associated with opened stream.
		     * @return                file length
		    **/
	__filesize_t flength() const;

		   /** Flushes buffer onto disk.
		     * @return                true if operation was succesfully performed
		     * @note                  This operation performs automatically
		     *                        when bioClose is called and
		     *                        openmode == READWRITE || WRITE, or
		     *                        when bioSeek is called and logical
		     *                        file position is out of buffer.
		     * @see                   bioReRead
		    **/
	bool	flush();

		   /** Reads one byte from stream.
		     * @return                Readed byte
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioWriteByte bioReadWord bioReadDWord bioReadBuffer
		    **/
	uint8_t	read_byte();

		   /** Reads two bytes from stream.
		     * @return                Readed word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioWriteWord bioReadByte bioReadDWord bioReadBuffer
		    **/
	uint16_t read_word();

		   /** Reads four bytes from stream.
		     * @return                Readed double word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioWriteDWord bioReadByte bioReadWord bioReadBuffer
		    **/
	uint32_t read_dword();

		   /** Reads 8 bytes from stream.
		     * @return                Readed double word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioWriteDWord bioReadByte bioReadWord bioReadBuffer
		    **/
	uint64_t read_qword();

		   /** Reads specified number of bytes from stream.
		     * @return                true if operation was succesfully performed
		     * @param buffer          specifies buffer, where readed information will be stored
		     * @param cbBuffer        specifies size of buffer
		     * @note                  Function increments logical file
		     *                        position by the number of bytes read.
		     * @see                   bioWriteBuffer bioReadByte bioReadWord bioReadByte
		    **/
	bool	read_buffer(any_t* buffer,unsigned cbBuffer);

		   /** Rereads opened file from disk.
		     * @return                true if operation was succesfully performed
		     * @see                   bioFlush
		    **/
	bool	reread();

		   /** Positions logical file pointer at the specified position.
		     * @return                true if operation was succesfully performed
		     * @param offset          specifies new offset of file pointer
		     * @param origin          specifies reference location from which offset will be computed
		     * @see                   bioTell BIO_SEEK_SET BIO_SEEK_CUR BIO_SEEK_END
		    **/
	bool	seek(__fileoff_t offset,int origin);

		   /** Returns current optimization of buffering.
		     * @return                optimization (BIO_OPT_*)
		     * @see                   bioSetOptimization
		    **/
	unsigned get_optimization() const;

		   /** Sets new optimization of buffering and returns previous.
		     * @return                optimization (BIO_OPT_*)
		     * @see                   bioGetOptimization
		    **/
	unsigned set_optimization(unsigned flags);

		   /** Returns logical file position of opened stream.
		     * @return                offset from begin of file
		     * @see                   bioSeek
		    **/
	__filesize_t tell() const;

		   /** Writes one byte to stream.
		     * @return                true if operation was succesfully performed
		     * @param bVal            Byte to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioReadByte bioWriteWord bioWriteWord bioWriteBuffer
		    **/
	bool	write_byte(uint8_t bVal);

		   /** Writes two bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param wVal            Word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioReadWord bioWriteWord bioWriteWord bioWriteBuffer
		    **/
	bool	write_word(uint16_t wVal);

		   /** Writes four bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param dwVal           Double word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioReadDWord bioWriteWord bioWriteWord bioWriteBuffer
		    **/
	bool	write_dword(uint32_t dwVal);

		   /** Writes 8 bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param dwVal           Double word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		     * @see                   bioReadDWord bioWriteWord bioWriteWord bioWriteBuffer
		    **/
	bool	write_qword(uint64_t dwVal);

		   /** Writes specified number of bytes opened to stream.
		     * @return                true if operation was succesfully performed
		     * @param buffer          specifies buffer to be written
		     * @param cbBuffer        specifies size of buffer
		     * @note                  Function increments logical file
		     *                        position by the number of bytes writed.
		     * @see                   bioReadBuffer bioWriteWord bioWriteWord bioByte
		    **/
	bool	write_buffer(const any_t* buffer,unsigned cbBuffer);

		   /** Returns name of file associated with opened stream.
		     * @return                name of file
		    **/
	const char* filename() const;

		   /** Causes opened stream to be duplicated.
		     * @return                handle of duplicted stream
		     * @note                  function duplicates OS handle
		     *                        of stream and buffer with all
		     *                        characteristics.
		     * @see                   bioDupEx
		    **/
	BFile*	dup() const;

		   /** Causes opened stream to be duplicated.
		     * @return                handle of duplicted stream
		     * @param bioFile         handle of opened stream
		     * @param buffSize        specifies new size of buffer to be used with duplicated stream
		     * @note                  function duplicates OS handle
		     *                        of stream and buffer with all
		     *                        possible characteristics.
		     * @see                   bioDup
		    **/
	BFile*	dup_ex(unsigned buffSize) const;

		   /** Returns low-level OS handle of opened stream.
		     * @return                OS handle of opened stream
		    **/
	bhandle_t handle() const;

		   /** Returns pointer to buffer of opened stream.
		     * @return                pointer to buffer
		     * @note                  This function allowes direct
		     *                        access to file cache.
		     * @see                   bioBuffLen bioBuffPos
		    **/
	any_t*	buffer() const;

		   /** Returns length of opened stream buffer.
		     * @return                length of buffer
		     * @note                  This function allowes direct
		     *                        access to file cache.
		     * @see                   bioBuff bioBuffPos
		    **/
	unsigned bufflen() const;

		   /** Returns logical buffer position.
		     * @return                length of buffer
		     * @note                  This function allowes direct
		     *                        access to file cache.
		     * @warning               Logical buffer position is not
		     *                        logical file position.
		     * @see                   bioBuff bioBuffLen
		    **/
	unsigned buffpos() const;
    private:
	bool is_cache_valid() const { return (b.vfb.MBuffer && !(optimize & Opt_NoCache)); }
	bool is_writeable(unsigned _openmode) const { return ((_openmode & FO_READWRITE) || (_openmode & FO_WRITEONLY)); }
	bool __isOutOfBuffer(__filesize_t pos) const { return (pos < b.vfb.FBufStart || pos >= b.vfb.FBufStart + b.vfb.MBufSize) && !is_mmf; }
	bool __isOutOfContents(__filesize_t pos) const { return ((pos < b.vfb.FBufStart || pos >= b.vfb.FBufStart + b.vfb.MBufLen) && !is_mmf); }
	bool __fill(__fileoff_t pos);
	bool __flush();
	bool __seek(__fileoff_t pos,int origin);
	unsigned char __getc();
	bool __putc(unsigned char ch);
	bool __getbuff(char * buff,unsigned cbBuff);
	bool __putbuff(const char * buff,unsigned cbBuff);
	bool chk_eof(__fileoff_t& pos) { is_eof = false;
		/* Accessing memory after mmf[FLength-1] causes GPF in MMF mode */
		/* so we must add special checks for it but no for read-write mode */
		/* Special case: FLength == 0. When file is being created pos == FLength.*/
		    if(FLength && !is_writeable(openmode) && pos >= (__fileoff_t)FLength) {
			pos = FLength-1;
			is_eof = true;
			return false;
		    }
		return true;
	    }

	bool seek_fptr(__fileoff_t& pos,int origin) {
		switch((int)origin) {
		    case Seek_Set: break;
		    case Seek_Cur: pos += FilePos; break;
		    default:           pos += FLength;
		}
		return chk_eof(pos);
	    }
    };
    inline BFile::opt operator~(BFile::opt a) { return static_cast<BFile::opt>(~static_cast<unsigned>(a)); }
    inline BFile::opt operator|(BFile::opt a, BFile::opt b) { return static_cast<BFile::opt>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline BFile::opt operator&(BFile::opt a, BFile::opt b) { return static_cast<BFile::opt>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline BFile::opt operator^(BFile::opt a, BFile::opt b) { return static_cast<BFile::opt>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline BFile::opt operator|=(BFile::opt& a, BFile::opt b) { return (a=static_cast<BFile::opt>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline BFile::opt operator&=(BFile::opt& a, BFile::opt b) { return (a=static_cast<BFile::opt>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline BFile::opt operator^=(BFile::opt& a, BFile::opt b) { return (a=static_cast<BFile::opt>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace beye
#endif

