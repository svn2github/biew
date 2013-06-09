#ifndef __MMFILE_HPP_INCLUDED
#define __MMFILE_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

#include <fcntl.h>

#include "bstream.h"

namespace	usr {
    class MMFile : public binary_stream {
	public:
	    MMFile();
	    virtual ~MMFile();

	    static const bool		has_mmio;

	    virtual bool		open(const std::string& fname,unsigned openmode);
	    virtual bool		close();
	    virtual bool		eof() const;
	    virtual bool		flush();
	    virtual uint8_t		read(const data_type_qualifier__byte_t&);
	    virtual uint16_t		read(const data_type_qualifier__word_t&);
	    virtual uint32_t		read(const data_type_qualifier_dword_t&);
	    virtual uint64_t		read(const data_type_qualifier_qword_t&);
	    virtual binary_packet	read(size_t cbBuffer);
	    virtual bool		seek(__fileoff_t offset,e_seek origin);
	    virtual __filesize_t	tell() const;
	    virtual bool		write(uint8_t bVal);
	    virtual bool		write(uint16_t wVal);
	    virtual bool		write(uint32_t dwVal);
	    virtual bool		write(uint64_t dwVal);
	    virtual bool		write(const binary_packet& buff);
	    virtual bool		chsize(__filesize_t newsize);
	    virtual bool		dup(MMFile&) const;
	    virtual binary_stream*	dup();
	    virtual bool		reread();
	    virtual any_t*		buffer() const;
	protected:
	    static int			mk_prot(int mode);
	    static int			mk_flags(int mode);
	    bool			is_writeable(unsigned _openmode) const { return ((_openmode & O_RDWR) || (_openmode & O_WRONLY)); }

	    bool chk_eof() const {
		if(flength() && !is_writeable(mode) && filepos < flength()) return false;
		return true;
	    }

	    bool seek_fptr(__fileoff_t pos,e_seek origin) {
		switch(origin) {
		    case Seek_Set: filepos=pos; break;
		    case Seek_Cur: filepos+=pos; break;
		    default:       filepos+=flength()+pos;
		}
		return chk_eof();
	    }
	private:
	    any_t*			addr;
	    int				mode;
	    __filesize_t		filepos;
	    bool			primary;
    };
} // namespace	usr
#endif
