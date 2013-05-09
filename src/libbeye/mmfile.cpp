#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include <stdlib.h>
#include <string.h>

#include "mmfile.h"

namespace	usr {
MMFile::MMFile()
	:addr(NULL)
	,mode(0)
	,filepos(0)
	,primary(false)
{
}

MMFile::~MMFile() {}

bool MMFile::write(uint8_t bVal) { UNUSED(bVal); return false; }
bool MMFile::write(uint16_t wVal) { UNUSED(wVal); return false; }
bool MMFile::write(uint32_t dwVal) { UNUSED(dwVal); return false; }
bool MMFile::write(uint64_t qwVal) { UNUSED(qwVal); return false; }
bool MMFile::write(const any_t* _buffer,unsigned cbBuffer)
{
    UNUSED(_buffer);
    UNUSED(cbBuffer);
    return false;
}

any_t* MMFile::buffer() const
{
    return addr;
}

bool MMFile::dup(MMFile& it) const
{
    bool rc;
    if(!(rc=binary_stream::dup(it))) return rc;
    it.addr=addr;
    it.mode=mode;
    it.filepos=filepos;
    it.primary=false;
    return true;
}

binary_stream* MMFile::dup() const
{
    MMFile* ret = new(zeromem) MMFile;
    if(!dup(*ret)) return &bNull;
    return ret;
}

bool MMFile::seek(__fileoff_t pos,e_seek orig)
{
    return seek_fptr(pos,orig);
}

__filesize_t MMFile::tell() const
{
    return filepos;
}

bool MMFile::eof() const { return chk_eof(); }

uint8_t MMFile::read(const data_type_qualifier__byte_t&)
{
    uint8_t rval;
    rval = ((uint8_t*)addr)[filepos++];
    if(chk_eof()) return -1;
    return rval;
}

uint16_t MMFile::read(const data_type_qualifier__word_t&)
{
    uint16_t rval=-1;
    read(&rval,sizeof(uint16_t));
    if(chk_eof()) return -1;
    return rval;
}

uint32_t MMFile::read(const data_type_qualifier_dword_t&)
{
    uint32_t rval=-1;
    read(&rval,sizeof(uint32_t));
    if(chk_eof()) return -1;
    return rval;
}

uint64_t MMFile::read(const data_type_qualifier_qword_t&)
{
    uint64_t rval=-1;
    read(&rval,sizeof(uint64_t));
    if(chk_eof()) return -1;
    return rval;
}

bool MMFile::read(any_t* _buffer,unsigned cbBuffer)
{
    bool ret;
    size_t size = std::min(__filesize_t(cbBuffer),flength() - filepos);
    ret=chk_eof();
    if(size && !ret)  {
	::memcpy(_buffer,((uint8_t *)addr) + filepos,size);
	filepos += size;
    }
    return ret;
}

bool MMFile::reread()
{
    return flush();
}

#ifdef HAVE_MMAP
#include <sys/mman.h>
int MMFile::mk_prot(int mode)
{
    int pflg;
    pflg = PROT_READ;
    if(mode & O_WRONLY) pflg = PROT_WRITE;
    else if(mode & O_RDWR) pflg |= PROT_WRITE;
    return pflg;
}

int MMFile::mk_flags(int mode)
{
    int pflg;
    pflg = 0;
#ifdef MAP_SHARED
    if((mode & SO_DENYREAD) ||
	(mode & SO_DENYWRITE) ||
	(mode & SO_DENYNONE)) pflg |= MAP_SHARED;
#endif
#ifdef MAP_PRIVATE
    if(mode & SO_PRIVATE) pflg |= MAP_PRIVATE;
#endif
    return pflg;
}

bool MMFile::open(const std::string& _fname,unsigned _openmode)
{
    mode=_openmode;
    if(!is_writeable(mode)) {
	if(binary_stream::open(_fname,_openmode)==true) {
	    /* Attempt open as MMF */
	    if(flength() <= __filesize_t(std::numeric_limits<long>::max())) {
		addr = ::mmap(NULL,flength(),mk_prot(_openmode),mk_flags(_openmode),handle(),0L);
		primary=true;
	        if(addr!=(any_t*)-1) return true;
	    }
	}
    }
    return false;
}

bool MMFile::close()
{
    if(is_writeable(mode)) flush();
    if(primary) {
	::munmap(addr,flength());
	return binary_stream::close();
    }
    return true;
}

bool MMFile::flush()
{
    return ::msync(addr,flength(),MS_SYNC) ? false : true;
}

bool MMFile::chsize(__filesize_t newsize)
{
    __filesize_t oldsize=flength();
    any_t* new_addr;
    bool can_continue = false;
    if(newsize < oldsize) { /* truncate */
	if((new_addr = ::mremap(addr,oldsize,newsize,MREMAP_MAYMOVE)) != (any_t*)-1) can_continue = true;
	if(can_continue) can_continue = binary_stream::chsize(newsize);
    } else { /* expand */
	can_continue=binary_stream::chsize(newsize);
	if(can_continue) can_continue = ((new_addr = ::mremap(addr,oldsize,newsize,MREMAP_MAYMOVE)) != (any_t*)-1);
    }
    if(can_continue) {
	addr = new_addr;
	return true;
    } else /* Attempt to unroll transaction back */
	binary_stream::chsize(oldsize);
    return false;
}
const bool MMFile::has_mmio=true;
#else // HAVE_MMAP
int MMFile::mk_prot(int mode) { UNUSED(mode); return 0; }
int MMFile::mk_flags(int mode) { UNUSED(mode); return 0; }
bool MMFile::open(const std::string& _fname,unsigned _openmode,unsigned cache_size) { UNUSED(_fname); UNUSED(_openmode); UNUSED(cache_size); return false; }
bool MMFile::close() { return false; }
bool MMFile::flush() { return false; }
bool MMFile::chsize(__filesize_t newsize) { UNUSED(newsize); return false; }
const bool MMFile::has_mmio=false;
#endif // HAVE_MMAP
} // namespace	usr
