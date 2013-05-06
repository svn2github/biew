#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   libbeye
 * @file        libbeye/bbio.c
 * @brief       This file contains implementation of Buffering binary input/ouput routines.
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
 * @todo        Support for preemptive memory allocation and multiple buffers for
 *              one opened stream
**/
#include <algorithm>
#include <iostream>

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "libbeye/bbio.h"

namespace beye {
BBio_File::BBio_File()
	:filepos(0)
	,openmode(0)
	,optimize(0)
	,is_eof(true)
	,founderr(false)
	,_flength(0)
{
}

BBio_File::~BBio_File() { if(!filename().empty()) close(); }

/* notes: all function with prefix=>__ assume, that buffer present */

bool BBio_File::__fill(__fileoff_t pos)
{
    any_t* mbuff;
    __filesize_t remaind;
    bool ret;
    if(pos < 0) pos = 0;
    if((__filesize_t)pos > _flength) {
	pos = _flength;
	vfb.buflen = 0;
	ret = false;
    } else {
	vfb.f_start = pos;
	remaind = _flength - pos;
	vfb.buflen = (__filesize_t)vfb.bufsize < remaind ? vfb.bufsize : (unsigned)remaind;
	mbuff = vfb.buffer;
	BFile::seek(pos,Seek_Set);
	ret = (unsigned)BFile::read(mbuff,vfb.buflen) == vfb.buflen;
    }
    return ret;
}

bool BBio_File::__flush()
{
    any_t* mbuff;
    bool ret;
    ret = true;
    if(!vfb.updated) {
	mbuff = vfb.buffer;
	BFile::seek(vfb.f_start,Seek_Set);
	if(vfb.buflen)	ret = (unsigned)BFile::write(mbuff,vfb.buflen) == vfb.buflen;
	if(ret)		vfb.updated = true;
    }
    return ret;
}

bool BBio_File::__seek(__fileoff_t pos,e_seek origin)
{
    bool ret,rret;
    ret=seek_fptr(pos,origin);
    filepos = pos;
    if(__isOutOfBuffer(pos)) {
	__flush();
	switch(optimize & Opt_DirMask) {
	    default:
	    case Opt_Db:	break;
	    case Opt_Random:	pos -= vfb.bufsize / 2;
				break;
	    case Opt_BackScan:	pos -= (vfb.bufsize - 1);
				break;
	    case Opt_RForward:	pos -= vfb.bufsize / 10;
				break;
	    case Opt_RBackScan:	pos -= (vfb.bufsize/10)*9;
				break;
	}
	if(filepos < _flength) {
	    rret = __fill(pos);
	    if(ret) ret = rret;
	} else {
	    vfb.f_start = filepos;
	    vfb.buflen = 0;
	}
    }
    return ret;
}

unsigned char BBio_File::__getc()
{
    char* buff = vfb.buffer;
    unsigned _buffpos;
    bool ret;
    unsigned char ch;
    ret = true;
    founderr = false;
    if(__isOutOfContents(filepos)) ret = __seek(filepos,Seek_Set);
    ch = -1;
    if(vfb.buflen && ret && filepos <= _flength) {
	_buffpos = (unsigned)(filepos - vfb.f_start);
	if(_buffpos < vfb.buflen)  ch = buff[_buffpos];
	if(filepos < _flength) filepos++;
	__fileoff_t fpos=filepos;
	ret=chk_eof(fpos);
	filepos=fpos;
    } else {
	errno = EACCES;
	ret = false;
    }
    if(ret == false) founderr = true;
    return ch;
}

bool BBio_File::__putc(unsigned char ch)
{
    char *  buff;
    unsigned _buffpos;
    bool ret;
    if(is_writeable(openmode)) {
	ret = true;
	buff = vfb.buffer;
	if(__isOutOfBuffer(filepos)) __seek(filepos,Seek_Set);
	_buffpos = (unsigned)(filepos - vfb.f_start);
	buff[_buffpos++] = ch;
	vfb.updated = false;
	if(vfb.buflen < _buffpos && vfb.buflen < vfb.bufsize) vfb.buflen = _buffpos;
	if(_flength <= vfb.f_start + _buffpos) _flength = vfb.f_start + _buffpos;
	filepos++;
    } else { errno = EACCES; ret = false; }
    return ret;
}

#if 0
static void  __FASTCALL__ dump_BFILE(BFILE*obj)
{
fprintf(stderr,
	"      %p\n"
	"      filepos=%llu _flength=%llu\n"
	"      %s openmode=%u optimize=%i\n"
	"      is_mmf=%i primary_mmf=%i\n"
	"      VFB:\n"
	"        handle = %p FBufStart=%llu MBuffer=%p\n"
	"        MBufLen=%u MBufSize=%u updated=%u\n"
	"      MMB: mmf=%p mmf_addr=%p\n"
	"        handle = %p FBufStart=%llu MBuffer=%p\n"
	"        MBufLen=%u MBufSize=%u updated=%u\n"
	"      is_eof=%i\n"
	,obj
	,obj->filepos,obj->_flength
	,obj->FileName,obj->openmode,obj->optimize
	,obj->is_mmf,obj->primary_mmf
	,obj->handle(),obj->vfb.f_start,obj->vfb.buffer
	,obj->vfb.buflen,obj->vfb.bufsize,obj->vfb.updated
	,obj->is_mmf?obj->mmb->mmf:0,obj->is_mmf?obj->mmb->mmf_addr:0
	,obj->is_eof);
}
#endif

bool BBio_File::__getbuff(char* buff,unsigned cbBuff)
{
    unsigned diffsize;
    unsigned MBufStart,MBufRem;
    int _optimize = optimize;
    bool ret = true;

    optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
    while(cbBuff) {
	MBufStart = (unsigned)(filepos - vfb.f_start);
	MBufRem = vfb.buflen - MBufStart;
	if(!MBufRem || __isOutOfContents(filepos)) {
	    ret = __seek(filepos,Seek_Set);
	    if(!ret) break;
	    if(filepos >= _flength) break;
	    MBufStart = (unsigned)(filepos - vfb.f_start);
	    MBufRem = vfb.buflen - MBufStart;
	}
	if(!MBufRem) {
	    errno = E2BIG; /* fault: internal error */
	    ret = false;
	    break;
	}
	diffsize = std::min(MBufRem,cbBuff);
	::memcpy(buff,&vfb.buffer[MBufStart],diffsize);
	filepos += diffsize;
	if(filepos > _flength) filepos = _flength;
	cbBuff -= diffsize;
	buff += diffsize;
    }
    optimize = _optimize;
    return ret;
}

bool BBio_File::__putbuff(const char* buff,unsigned cbBuff)
{
    unsigned diffsize;
    unsigned MBufStart,MBufRem;
    int _optimize = optimize;
    bool ret = true;
    if(is_writeable(openmode)) {
	optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
	ret = true;
	while(cbBuff) {
	    MBufStart = (unsigned)(filepos - vfb.f_start);
	    MBufRem = vfb.bufsize - MBufStart;
	    if(!MBufRem || __isOutOfBuffer(filepos)) {
		ret = __seek(filepos,Seek_Set);
		if(!ret) break;
		MBufStart = (unsigned)(filepos - vfb.f_start);
		MBufRem = vfb.bufsize - MBufStart;
	    }
	    if(!MBufRem) {
		errno = E2BIG; /* fault: internal error */
		ret = false;
		break;
	    }
	    diffsize = std::min(MBufRem,cbBuff);
	    ::memcpy(&vfb.buffer[MBufStart],buff,diffsize);
	    vfb.updated = false;
	    if(vfb.buflen < MBufStart + diffsize) vfb.buflen = MBufStart + diffsize;
	    if(_flength < filepos + diffsize) _flength = filepos + diffsize;
	    filepos += diffsize;
	    cbBuff -= diffsize;
	    buff += diffsize;
	}
	optimize = _optimize;
    } else { errno = EACCES; ret = false; }
    return ret;
}

bool BBio_File::open(const std::string& _fname,unsigned _openmode,unsigned bSize,unsigned optimization)
{
    unsigned len;
    openmode = _openmode;

    optimization = Opt_Db;
    if(!BFile::open(_fname,openmode)) return false;
    optimize = optimization;
    vfb.f_start = 0L;
    vfb.updated  = true;
    len = bSize;
    if(bSize == UINT_MAX) len = (unsigned)_flength;
    if(len) {
	if((vfb.buffer = new char [len]) != NULL) {
	    vfb.buflen = 0;
	    vfb.bufsize = len;
	} else {
	    BFile::close();
	    return false;
	}
	__fill(0L);
    } else vfb.buffer = NULL;
    is_eof=false;
    return true;
}

bool BBio_File::open(const std::string& _fname,unsigned _openmode,unsigned cache_size)
{
    return open(_fname,_openmode,cache_size,Opt_Db);
}

bool BBio_File::close()
{
    if(!filename().empty()) {
	if(is_writeable(openmode)) __flush();
	/* For compatibility with DOS-32: don't try to close stderr */
	BFile::close();
    }
    if(vfb.buffer) delete vfb.buffer;
    return true;
}

bool BBio_File::seek(__fileoff_t pos,e_seek orig)
{
    bool ret;
    if(is_cache_valid()) ret = __seek(pos,orig);
    else ret = BFile::seek(pos,orig);
    return ret;
}

__filesize_t BBio_File::tell() const
{
    return is_cache_valid() ? filepos : (__filesize_t)BFile::tell();
}

uint8_t BBio_File::read_byte()
{
    return is_cache_valid() ?
	    __getc() :
	    BFile::read_byte();
}

uint16_t BBio_File::read_word()
{
    uint16_t ret;
    is_cache_valid() ?
	__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint16_t)) :
	ret=BFile::read_word();
    return ret;
}

uint32_t BBio_File::read_dword()
{
    uint32_t ret;
    is_cache_valid() ?
	__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint32_t)) :
	ret=BFile::read_dword();
    return ret;
}

uint64_t BBio_File::read_qword()
{
    uint64_t ret;
    is_cache_valid() ?
	__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint64_t)) :
	ret=BFile::read_qword();
    return ret;
}

bool BBio_File::read(any_t* _buffer,unsigned cbBuffer)
{
    return is_cache_valid() ?
	    __getbuff(reinterpret_cast<char*>(_buffer),cbBuffer) :
	    BFile::read(_buffer,cbBuffer);
}

bool BBio_File::write_byte(uint8_t bVal)
{
    return is_cache_valid() ?
	    __putc(bVal) :
	    BFile::write_byte(bVal);
}

bool BBio_File::write_word(uint16_t wVal)
{
    return is_cache_valid() ?
	    __putbuff(reinterpret_cast<const char*>(&wVal),sizeof(uint16_t)) :
	    BFile::write_word(wVal);
}

bool BBio_File::write_dword(uint32_t dwVal)
{
  return is_cache_valid() ?
	    __putbuff(reinterpret_cast<const char*>(&dwVal),sizeof(uint32_t)) :
	    BFile::write_dword(dwVal);
}

bool BBio_File::write_qword(uint64_t dwVal)
{
    return is_cache_valid() ?
	    __putbuff(reinterpret_cast<const char*>(&dwVal),sizeof(uint64_t)) :
	    BFile::write_qword(dwVal);
}

bool BBio_File::write(const any_t* _buffer,unsigned cbBuffer)
{
    return is_cache_valid() ?
	    __putbuff(reinterpret_cast<const char*>(_buffer),cbBuffer) :
	    BFile::write(_buffer,cbBuffer);
}

bool BBio_File::flush()
{
    return is_cache_valid() ? __flush() : true;
}

bool BBio_File::reread()
{
    __filesize_t fpos;
    fpos = tell();
    BFile::seek(0L,Seek_End);
    return seek(fpos,Seek_Set);
}

bool BBio_File::chsize(__filesize_t newsize)
{
    __filesize_t length;
    bool ret;

    length = _flength;
    ret=BFile::chsize(newsize);
    if(length >= newsize) { /* truncate size */
	__seek(newsize, Seek_Set);
	if(vfb.f_start > _flength) { vfb.f_start = _flength; vfb.buflen = 0; }
	if(vfb.f_start + vfb.buflen > _flength) vfb.buflen = (unsigned)(_flength - vfb.f_start);
    }
    _flength=BFile::flength();
    return ret;
}

unsigned BBio_File::set_optimization(unsigned flags)
{
    unsigned ret;
    ret = optimize;
    optimize = flags;
    return ret;
}

unsigned BBio_File::get_optimization() const
{
    return optimize;
}

any_t* BBio_File::buffer() const
{
    return vfb.buffer;
}

unsigned BBio_File::bufflen() const
{
    return vfb.buflen;
}

unsigned BBio_File::buffpos() const
{
    return vfb.buflen - (unsigned)(filepos - vfb.f_start);
}

bool BBio_File::dup(BBio_File& it,unsigned buff_size) const
{
    bool rc = BFile::dup(it);
    it.openmode = openmode;
    it.optimize = optimize;
    it.is_eof = is_eof;
    it.vfb.updated  = vfb.updated;
    it._flength  = _flength;
    if(buff_size) {
	it.vfb.buffer = new char [buff_size];
	if(is_cache_valid()) {
	    it.vfb.buflen = std::min(buff_size,vfb.buflen);
	    it.vfb.bufsize = buff_size;
	    it.filepos = std::min(filepos,vfb.f_start+(buff_size/2));
	    it.vfb.f_start = vfb.f_start;
	    ::memcpy(it.vfb.buffer,vfb.buffer,it.vfb.buflen);
	} else {
	    it.vfb.buflen = 0;
	    it.vfb.bufsize = 0;
	    it.filepos = 0L;
	    it.vfb.f_start = 0;
	}
    } else it.vfb.buffer = 0;
    return rc;
}

BFile* BBio_File::dup(unsigned buff_size) const
{
    BBio_File* ret = new(zeromem) BBio_File;
    if(!dup(*ret,buff_size)) return &bNull;
    return ret;
}

bool BBio_File::eof() const
{
    bool retval;
    if(is_cache_valid())retval = is_eof;
    else		retval = BFile::eof();
    return retval;
}

__filesize_t BBio_File::flength() const { return _flength; }

} // namespace beye
