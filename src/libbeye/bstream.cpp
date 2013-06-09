#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include <time.h>
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bstream.h"

namespace	usr {
binary_stream::binary_stream()
	:_handle(-1)
{
}

binary_stream::~binary_stream() { if(!fname.empty()) close(); }

bool binary_stream::open(const std::string& _fname,unsigned _openmode)
{
    int h;
    if(!fname.empty()) return false; // prevent open without close
    h = ::open(_fname.c_str(),_openmode);
    if(h==-1) return false;
    _handle = h ^ reinterpret_cast<long>(this);
    fname=_fname;
    update_length();
    return true;
}

bool binary_stream::create(const std::string& name)
{
    return open(name,O_RDWR | O_CREAT | O_TRUNC);
}

bool binary_stream::close()
{
    int h = _handle ^ reinterpret_cast<long>(this);
    if(h > 2) ::close(h);
    fname.clear();
    return true;
}

bool binary_stream::seek(__fileoff_t offset,e_seek origin) {
    int h = _handle ^ reinterpret_cast<long>(this);
    return ::lseek(h,offset,origin)>0;
}

__filesize_t binary_stream::tell() const
{
    return _tell();
}

__filesize_t binary_stream::_tell() const
{
    int h = _handle ^ reinterpret_cast<long>(this);
    return ::lseek(h,0L,SEEK_CUR);
}

uint8_t binary_stream::read(const data_type_qualifier__byte_t&)
{
    uint8_t ret;
    int h = _handle ^ reinterpret_cast<long>(this);
    if(::read(h,&ret,sizeof(uint8_t))!=sizeof(uint8_t)) ret=-1;
    return ret;
}

uint16_t binary_stream::read(const data_type_qualifier__word_t&)
{
    uint16_t ret;
    int h = _handle ^ reinterpret_cast<long>(this);
    if(::read(h,&ret,sizeof(uint16_t))!=sizeof(uint16_t)) ret=-1;
    return ret;
}

uint32_t binary_stream::read(const data_type_qualifier_dword_t&)
{
    uint32_t ret;
    int h = _handle ^ reinterpret_cast<long>(this);
    if(::read(h,&ret,sizeof(uint32_t))!=sizeof(uint32_t)) ret=-1;
    return ret;
}

uint64_t binary_stream::read(const data_type_qualifier_qword_t&)
{
    uint64_t ret;
    int h = _handle ^ reinterpret_cast<long>(this);
    if(::read(h,&ret,sizeof(uint64_t))!=sizeof(uint64_t)) ret=-1;
    return ret;
}

binary_packet binary_stream::read(size_t cbBuffer)
{
    int cb;
    int h = _handle ^ reinterpret_cast<long>(this);
    binary_packet rc(cbBuffer);
    cb=::read(h,rc.data(),cbBuffer);
    if(cb>0 && size_t(cb)!=cbBuffer) rc.resize(cb);
    return rc;
}

bool binary_stream::write(uint8_t bVal)
{
    bool rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::write(h,&bVal,sizeof(uint8_t))==sizeof(uint8_t);
    update_length();
    return rc;
}

bool binary_stream::write(uint16_t wVal)
{
    bool rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::write(h,&wVal,sizeof(uint16_t))==sizeof(uint16_t);
    update_length();
    return rc;
}

bool binary_stream::write(uint32_t dwVal)
{
    bool rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::write(h,&dwVal,sizeof(uint32_t))==sizeof(uint32_t);
    update_length();
    return rc;
}

bool binary_stream::write(uint64_t qwVal)
{
    bool rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::write(h,&qwVal,sizeof(uint64_t))==sizeof(uint64_t);
    update_length();
    return rc;
}

bool binary_stream::write(const binary_packet& _buffer)
{
    bool rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::write(h,_buffer.data(),_buffer.size())==int(_buffer.size());
    update_length();
    return rc;
}

bool binary_stream::flush()
{
    return true;
}

__filesize_t binary_stream::flength() const
{
    return fsize;
}

bool binary_stream::chsize(__filesize_t newsize)
{
    __filesize_t length, fillsize;
    char * buf;
    unsigned  bufsize, numtowrite;
    bool ret;
    int h = _handle ^ reinterpret_cast<long>(this);

    length = flength();
    if(length >= newsize) { /* truncate size */
	::lseek(h,newsize, SEEK_SET);
	length = newsize;
	ret = truncate(length) == 0;
    } else {
	fillsize=newsize-length;  /* increase size */
	ret = false;
	bufsize = 8192;
	if((buf = new char [bufsize]) != NULL) {
	    ret = true;
	    ::memset(buf, 0, bufsize);   /* write zeros to pad file */
	    ::lseek(h,0L,SEEK_END);
	    do {
		numtowrite = (unsigned)std::min(__filesize_t(bufsize),fillsize);
		if(!::write(h,buf, numtowrite)) { ret = false; break; }
		fillsize-=numtowrite;
	    } while(fillsize);
	    delete buf;
	    fsize=newsize;
	}
    }
    return ret;
}

int binary_stream::handle() const
{
    return _handle;
}

std::string binary_stream::filename() const
{
    return fname;
}

bool binary_stream::dup(binary_stream& it) const
{
    int h;
    if(fname.empty()) return false;
    if((h=::dup(_handle))==-1) {
	return false;
    }
    it._handle = h ^ reinterpret_cast<long>(&it);
    it.fname=fname;
    it.fsize=fsize;
    return true;
}

binary_stream* binary_stream::dup()
{
    binary_stream* ret = new(zeromem) binary_stream;
    if(!dup(*ret)) return this;
    return ret;
}

bool binary_stream::eof() const
{
    return _tell() >= fsize;
}

int binary_stream::truncate(__filesize_t newsize)
{
    int rc;
    int h = _handle ^ reinterpret_cast<long>(this);
    rc=::ftruncate(h,newsize);
    update_length();
    return rc;
}

void binary_stream::update_length() {
    int h = _handle ^ reinterpret_cast<long>(this);
    __filesize_t curr_pos=_tell();
    ::lseek(h,0L,SEEK_END);
    fsize=_tell();
    ::lseek(h,curr_pos,SEEK_SET);
}

bool binary_stream::reread() { return true; }

unsigned binary_stream::set_optimization(unsigned flags) { UNUSED(flags); return 0; }
unsigned binary_stream::get_optimization() const { return 0; }

/*  static */
int binary_stream::exists(const std::string& name) { return ::access(name.c_str(),F_OK) == 0; }
int binary_stream::unlink(const std::string& name) {  return ::unlink(name.c_str()); }
int binary_stream::rename(const std::string& oldname,const std::string& newname) {  return ::rename(oldname.c_str(),newname.c_str()); }
bool binary_stream::get_ftime(const std::string& name,ftime& data) {
    bool ret = false;
    struct stat statbuf;
    if(!::stat(name.c_str(),&statbuf)) {
	data.acctime = statbuf.st_atime;
	data.modtime = statbuf.st_mtime;
	ret = true;
    }
    return ret;
}

bool binary_stream::set_ftime(const std::string& name,const ftime& data)
{
    struct utimbuf ubuf;
    ubuf.actime = data.acctime;
    ubuf.modtime= data.modtime;
    return ::utime(name.c_str(),&ubuf) ? false : true;
}

} // namespace	usr
