#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;

#include <time.h>
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bfile.h"

namespace beye {
BFile::BFile()
	:_handle(-1)
{
}

BFile::~BFile() { if(!fname.empty()) close(); }

bool BFile::open(const std::string& _fname,unsigned _openmode,unsigned cache_size)
{
    UNUSED(cache_size);
    if(!fname.empty()) return false; // prevent open without close
    _handle = ::open(_fname.c_str(),_openmode);
    if(_handle==-1) return false;
    fname=_fname;
    update_length();
    return true;
}

bool BFile::create(const std::string& name)
{
    return open(name,O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
}

bool BFile::close()
{
    if(_handle > 2) ::close(_handle);
    fname.clear();
    return true;
}

bool BFile::seek(__fileoff_t offset,e_seek origin) {
    return ::lseek(_handle,offset,origin)>0;
}

__filesize_t BFile::tell() const
{
    return _tell();
}

__filesize_t BFile::_tell() const
{
    return ::lseek(_handle,0L,SEEK_CUR);
}

uint8_t BFile::read_byte()
{
    uint8_t ret;
    if(::read(_handle,&ret,sizeof(uint8_t))!=sizeof(uint8_t)) ret=-1;
    return ret;
}

uint16_t BFile::read_word()
{
    uint16_t ret;
    if(::read(_handle,&ret,sizeof(uint16_t))!=sizeof(uint16_t)) ret=-1;
    return ret;
}

uint32_t BFile::read_dword()
{
    uint32_t ret;
    if(::read(_handle,&ret,sizeof(uint32_t))!=sizeof(uint32_t)) ret=-1;
    return ret;
}

uint64_t BFile::read_qword()
{
    uint64_t ret;
    if(::read(_handle,&ret,sizeof(uint64_t))!=sizeof(uint64_t)) ret=-1;
    return ret;
}

bool BFile::read(any_t* _buffer,unsigned cbBuffer)
{
     return ::read(_handle,_buffer,cbBuffer)==cbBuffer;
}

bool BFile::write_byte(uint8_t bVal)
{
    bool rc;
    rc=::write(_handle,&bVal,sizeof(uint8_t))==sizeof(uint8_t);
    update_length();
    return rc;
}

bool BFile::write_word(uint16_t wVal)
{
    bool rc;
    rc=::write(_handle,&wVal,sizeof(uint16_t))==sizeof(uint16_t);
    update_length();
    return rc;
}

bool BFile::write_dword(uint32_t dwVal)
{
    bool rc;
    rc=::write(_handle,&dwVal,sizeof(uint32_t))==sizeof(uint32_t);
    update_length();
    return rc;
}

bool BFile::write_qword(uint64_t qwVal)
{
    bool rc;
    rc=::write(_handle,&qwVal,sizeof(uint64_t))==sizeof(uint64_t);
    update_length();
    return rc;
}

bool BFile::write(const any_t* _buffer,unsigned cbBuffer)
{
    bool rc;
    rc=::write(_handle,_buffer,cbBuffer)==cbBuffer;
    update_length();
    return rc;
}

bool BFile::flush()
{
    return true;
}

__filesize_t BFile::flength() const
{
    return fsize;
}

bool BFile::chsize(__filesize_t newsize)
{
    __filesize_t length, fillsize;
    char * buf;
    unsigned  bufsize, numtowrite;
    bool ret;

    length = flength();
    if(length >= newsize) { /* truncate size */
	::lseek(_handle,newsize, SEEK_SET);
	length = newsize;
	ret = truncate(length) == 0;
    } else {
	fillsize=newsize-length;  /* increase size */
	ret = false;
	bufsize = 8192;
	if((buf = new char [bufsize]) != NULL) {
	    ret = true;
	    ::memset(buf, 0, bufsize);   /* write zeros to pad file */
	    ::lseek(_handle,0L,SEEK_END);
	    do {
		numtowrite = (unsigned)std::min(__filesize_t(bufsize),fillsize);
		if(!::write(_handle,buf, numtowrite)) { ret = false; break; }
		fillsize-=numtowrite;
	    } while(fillsize);
	    delete buf;
	    fsize=newsize;
	}
    }
    return ret;
}

int BFile::handle() const
{
    return _handle;
}

std::string BFile::filename() const
{
    return fname;
}

bool BFile::dup(BFile& it,unsigned info) const
{
    UNUSED(info);
    if(fname.empty()) return false;
    if((it._handle=::dup(_handle))==-1) {
	return false;
    }
    it.fname=fname;
    it.fsize=fsize;
    return true;
}

BFile* BFile::dup(unsigned info) const
{
    BFile* ret = new(zeromem) BFile;
    if(!dup(*ret,info)) return &bNull;
    return ret;
}

bool BFile::eof() const
{
    return _tell() >= fsize;
}

int BFile::truncate(__filesize_t newsize)
{
    int rc;
    rc=ftruncate(_handle,newsize);
    update_length();
    return rc;
}

void BFile::update_length() {
    __filesize_t curr_pos=_tell();
    ::lseek(_handle,0L,SEEK_END);
    fsize=_tell();
    ::lseek(_handle,curr_pos,SEEK_SET);
}

bool BFile::reread() { return true; }

unsigned BFile::set_optimization(unsigned flags) { UNUSED(flags); return 0; }
unsigned BFile::get_optimization() const { return 0; }

/*  static */

int BFile::truncate(const std::string& name,__filesize_t newsize) { return ::truncate(name.c_str(),newsize); }
int BFile::exists(const std::string& name) { return ::access(name.c_str(),F_OK) == 0; }
int BFile::unlink(const std::string& name) {  return ::unlink(name.c_str()); }
int BFile::rename(const std::string& oldname,const std::string& newname) {  return ::rename(oldname.c_str(),newname.c_str()); }
bool BFile::get_ftime(const std::string& name,ftime& data) {
    bool ret = false;
    struct stat statbuf;
    if(!::stat(name.c_str(),&statbuf)) {
	data.acctime = statbuf.st_atime;
	data.modtime = statbuf.st_mtime;
	ret = true;
    }
    return ret;
}

bool BFile::set_ftime(const std::string& name,const ftime& data)
{
    struct utimbuf ubuf;
    ubuf.actime = data.acctime;
    ubuf.modtime= data.modtime;
    return ::utime(name.c_str(),&ubuf) ? false : true;
}

BFile bNull; /**< Stream associated with STDERR */
} // namespace beye
