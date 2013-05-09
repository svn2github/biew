#ifndef __BSTREAM_HPP_INCLUDED
#define __BSTREAM_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

#include <fcntl.h>
#include <unistd.h>

namespace	usr {
    class binary_stream : public Opaque {
	public:
	    enum {
		FO_READONLY	=0x0000, /**< Defines flag of file opening, that indicates opening in read-only mode */
		FO_WRITEONLY	=0x0001, /**< Defines flag of file opening, that indicates opening in write-only mode */
		FO_READWRITE	=0x0002 /**< Defines flag of file opening, that indicates opening in read-write mode */
	    };
	    enum {
		SO_COMPAT	=0x0000, /**< Defines flag of file sharing, that indicates opening in compatibility mode */
		SO_DENYALL	=0x0010, /**< Defines flag of file sharing, that indicates opening in mode, which denies any access to file by other processes */
		SO_DENYWRITE	=0x0020, /**< Defines flag of file sharing, that indicates opening in mode, which denies write access to file by other processes */
		SO_DENYREAD	=0x0030, /**< Defines flag of file sharing, that indicates opening in mode, which denies read access to file by other processes */
		SO_DENYNONE	=0x0040, /**< Defines flag of file sharing, that indicates opening in mode, which allows any access to file by other processes */
		SO_PRIVATE	=0x0080  /**< Defines flag of file sharing, that indicates opening in mode, which denies any access to file by child processes */
	    };

	    enum e_seek {
		Seek_Set	=SEEK_SET, /**< specifies reference location from begin of file */
		Seek_Cur	=SEEK_CUR, /**< specifies reference location from current position of file */
		Seek_End	=SEEK_END  /**< specifies reference location from end of file */
	    };

	    binary_stream();
	    virtual ~binary_stream();
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
		     * @see                   close
		    **/
	    virtual bool		open(const std::string& fname,unsigned openmode);

	    virtual bool		create(const std::string& fname);

		   /** Closes opened stream.
		     * @return                true if operation was succesfully performed
		     * @note                  This method is automaticzally called in destructor
		     * @see                   open
		    **/
	    virtual bool		close();

		   /** Determines whether a opened stream has reached the End of File.
		     * @return                true if EOF has reached
		    **/
	    virtual bool		eof() const;

		   /** Returns the length (in bytes) of file associated with opened stream.
		     * @return                file length
		    **/
	    virtual __filesize_t	flength() const;

		   /** Flushes buffer onto disk.
		     * @return                true if operation was succesfully performed
		     * @note                  This operation performs automatically
		     *                        when close is called.
		    **/
	    virtual bool		flush();

		   /** Reads one byte from stream.
		     * @return                Readed byte
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual uint8_t		read(const data_type_qualifier__byte_t&);

		   /** Reads two bytes from stream.
		     * @return                Readed word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual uint16_t		read(const data_type_qualifier__word_t&);

		   /** Reads four bytes from stream.
		     * @return                Readed double word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual uint32_t		read(const data_type_qualifier_dword_t&);

		   /** Reads 8 bytes from stream.
		     * @return                Readed double word
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual uint64_t		read(const data_type_qualifier_qword_t&);

		   /** Reads specified number of bytes from stream.
		     * @return                true if operation was succesfully performed
		     * @param buffer          specifies buffer, where readed information will be stored
		     * @param cbBuffer        specifies size of buffer
		     * @note                  Function increments logical file
		     *                        position by the number of bytes read.
		    **/
	    virtual bool		read(any_t* buffer,unsigned cbBuffer);

		   /** Positions logical file pointer at the specified position.
		     * @return                true if operation was succesfully performed
		     * @param offset          specifies new offset of file pointer
		     * @param origin          specifies reference location from which offset will be computed
		    **/
	    virtual bool		seek(__fileoff_t offset,e_seek origin);

		   /** Returns logical file position of opened stream.
		     * @return                offset from begin of file
		    **/
	    virtual __filesize_t	tell() const;

		   /** Writes one byte to stream.
		     * @return                true if operation was succesfully performed
		     * @param bVal            Byte to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual bool		write(uint8_t bVal);

		   /** Writes two bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param wVal            Word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual bool		write(uint16_t wVal);

		   /** Writes four bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param dwVal           Double word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual bool		write(uint32_t dwVal);

		   /** Writes 8 bytes to stream.
		     * @return                true if operation was succesfully performed
		     * @param dwVal           Double word to be written
		     * @note                  Logical file position is
		     *                        incremented after operation.
		    **/
	    virtual bool		write(uint64_t dwVal);

		   /** Writes specified number of bytes opened to stream.
		     * @return                true if operation was succesfully performed
		     * @param buffer          specifies buffer to be written
		     * @param cbBuffer        specifies size of buffer
		     * @note                  Function increments logical file
		     *                        position by the number of bytes writed.
		    **/
	    virtual bool		write(const any_t* buffer,unsigned cbBuffer);

		   /** Returns name of file associated with opened stream.
		     * @return                name of file
		    **/
	    virtual std::string		filename() const;

		   /** Changes size of opened file.
		     * @return                true if operation was succesfully performed
		     * @param newsize         new size of file in bytes
		     * @warning               If file is truncated, the data from
		     *                        the new end of file to the original
		     *                        end of the file are lost.
		    **/
	    virtual bool		chsize(__filesize_t newsize);

		   /** Causes opened stream to be duplicated.
		     * @return                handle of duplicted stream
		     * @note                  function duplicates OS handle
		     *                        of stream and buffer with all
		     *                        characteristics.
		    **/
	    virtual bool		dup(binary_stream&) const;
	    virtual binary_stream*		dup() const;

		   /** Returns low-level OS handle of opened stream.
		     * @return                OS handle of opened stream
		    **/
	    virtual int			handle() const;
		   /** Rereads opened file from disk.
		     * @return                true if operation was succesfully performed
		    **/
	    virtual bool		reread();

	    virtual unsigned		get_optimization() const;
	    virtual unsigned		set_optimization(unsigned flags);

	    virtual int			truncate(__filesize_t newsize);
/* Static member for standalone usage */

/** Structure for storing and setting file time information */
	    struct ftime {
		unsigned long acctime; /**< contains last access time */
		unsigned long modtime; /**< constains modification time */
	    };
	    static int			truncate(const std::string& fname,__filesize_t newsize);
	    static int			exists(const std::string& fname);
	    static int			rename(const std::string& oldname,const std::string& newname);
	    static int			unlink(const std::string& name);
	    static bool			get_ftime(const std::string& name,ftime& data);
	    static bool			set_ftime(const std::string& name,const ftime& data);
	protected:
	    bool			is_writeable(unsigned _openmode) const { return ((_openmode & O_RDWR) || (_openmode & O_WRONLY)); }
	private:
	    void			update_length();
	    __filesize_t		_tell() const;
	    int				_handle;
	    std::string			fname;
	    __filesize_t		fsize;
    };
    extern binary_stream bNull; /**< Stream associated with STDERR */
} // namespace	usr

#endif
