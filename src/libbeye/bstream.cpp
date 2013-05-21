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
{
}

binary_stream::~binary_stream() { if(!fname.empty()) close(); }

bool binary_stream::open(const std::string& _fname,unsigned _openmode)
{
    mode=std::ios_base::binary;
    if(_openmode&(FO_WRITEONLY|FO_READWRITE)) mode|=std::ios_base::out;
    else mode|=std::ios_base::in;
    if(!fname.empty()) return false; // prevent open without close
    fs.open(_fname.c_str(),mode);
    if(!fs.good()) return false;
    fname=_fname;
    update_length();
    return true;
}

bool binary_stream::create(const std::string& name)
{
    return binary_stream::open(name,O_RDWR | O_CREAT | O_TRUNC);
}

bool binary_stream::close()
{
    fs.close();
    fname.clear();
    return fs.good();
}

bool binary_stream::seek(__fileoff_t offset,e_seek origin) {
    std::ios_base::seekdir dir;
    switch(origin) {
	default:
	case Seek_Set: dir=std::ios_base::beg; break;
	case Seek_Cur: dir=std::ios_base::cur; break;
	case Seek_End: dir=std::ios_base::end; break;
    }
    if(mode&std::ios_base::in) {
	fs.seekg(offset,dir);
	return fs.good();
    } else {
	fs.seekp(offset,dir);
	return fs.good();
    }
}

__filesize_t binary_stream::tell()
{
    __filesize_t p1=0,p2=0;
    if(mode& std::ios_base::in) {
	p1=fs.tellg();
	return p1;
    }
    p2=fs.tellp();
    return p2;
}

uint8_t binary_stream::read(const data_type_qualifier__byte_t&)
{
    return fs.get();
}

uint16_t binary_stream::read(const data_type_qualifier__word_t&)
{
    uint16_t ret;
    fs.read((char*)&ret,sizeof(uint16_t));
    return ret;
}

uint32_t binary_stream::read(const data_type_qualifier_dword_t&)
{
    uint32_t ret;
    fs.read((char*)&ret,sizeof(uint32_t));
    return ret;
}

uint64_t binary_stream::read(const data_type_qualifier_qword_t&)
{
    uint64_t ret;
    fs.read((char*)&ret,sizeof(uint64_t));
    return ret;
}

bool binary_stream::read(any_t* _buffer,unsigned cbBuffer)
{
    fs.read((char*)_buffer,cbBuffer);
    return fs.good();
}

bool binary_stream::write(uint8_t bVal)
{
    binary_stream::write(&bVal,sizeof(uint8_t));
    return fs.good();
}

bool binary_stream::write(uint16_t wVal)
{
    binary_stream::write(&wVal,sizeof(uint16_t));
    return fs.good();
}

bool binary_stream::write(uint32_t dwVal)
{
    binary_stream::write(&dwVal,sizeof(uint32_t));
    return fs.good();
}

bool binary_stream::write(uint64_t qwVal)
{
    binary_stream::write(&qwVal,sizeof(uint64_t));
    return fs.good();
}

bool binary_stream::write(const any_t* _buffer,unsigned cbBuffer)
{
    fs.write((const char*)_buffer,cbBuffer);
    update_length();
    return fs.good();
}

bool binary_stream::flush()
{
    fs.flush();
    return fs.good();
}

bool binary_stream::chsize(__filesize_t newsize)
{
    __filesize_t length, fillsize;
    char * buf;
    unsigned  bufsize, numtowrite;
    bool ret;

    length = flength();
    if(length >= newsize) { /* truncate size */
	fs.close();
	binary_stream::truncate(fname,newsize);
	fs.open(fname.c_str(),mode);
	ret = fs.good();
	fsize=newsize;
    } else {
	fillsize=newsize-length;  /* increase size */
	ret = false;
	bufsize = 8192;
	if((buf = new char [bufsize]) != NULL) {
	    ret = true;
	    ::memset(buf, 0, bufsize);   /* write zeros to pad file */
	    binary_stream::seek(0L,Seek_End);
	    do {
		numtowrite = (unsigned)std::min(__filesize_t(bufsize),fillsize);
		if(!binary_stream::write(buf, numtowrite)) { ret = false; break; }
		fillsize-=numtowrite;
	    } while(fillsize);
	    delete buf;
	    fsize=newsize;
	}
    }
    return ret;
}

bool binary_stream::dup(binary_stream& it) const
{
    if(fname.empty()) return false;
    it.fs.open(fname.c_str(),mode);
    if(!it.fs.good()) return false;
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

#if 0
int binary_stream::truncate(__filesize_t newsize)
{
    int rc;
    rc=ftruncate(_handle,newsize);
    update_length();
    return rc;
}
#endif
void binary_stream::update_length() {
    __filesize_t curr_pos=binary_stream::tell();
    binary_stream::seek(0L,Seek_End);
    fsize=binary_stream::tell();
    binary_stream::seek(curr_pos,Seek_Set);
}

bool binary_stream::reread() { fs.sync(); return fs.good(); }

unsigned binary_stream::set_optimization(unsigned flags) { UNUSED(flags); return 0; }
unsigned binary_stream::get_optimization() const { return 0; }

/*  static */

int binary_stream::truncate(const std::string& name,__filesize_t newsize) { return ::truncate(name.c_str(),newsize); }
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
