#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
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

namespace	usr {
BBio_File::BBio_File(unsigned bSize,unsigned optimization)
	:filepos(0)
	,openmode(0)
	,optimize(optimization)
	,vfb(*this,bSize)
	,is_eof(true)
	,founderr(false)
	,_flength(0)
{
}

BBio_File::~BBio_File() { if(!filename().empty()) close(); }


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

bool BBio_File::open(const std::string& _fname,unsigned _openmode)
{
    openmode = _openmode;

    if(!binary_stream::open(_fname,openmode)) return false;
    vfb.fill(0L);
    is_eof=false;
    return true;
}

bool BBio_File::close()
{
    if(!filename().empty()) {
	if(is_writeable(openmode)) vfb.flush();
	/* For compatibility with DOS-32: don't try to close stderr */
	binary_stream::close();
    }
    return true;
}

bool BBio_File::seek(__fileoff_t pos,e_seek orig)
{
    bool ret;
    ret = vfb.seek(pos,orig);
    return ret;
}

__filesize_t BBio_File::tell() const
{
    return filepos;
}

uint8_t BBio_File::read(const beye_aka_binary_eye_project_data_type_qualifier__byte_t&)
{
    return vfb.read();
}

uint16_t BBio_File::read(const beye_aka_binary_eye_project_data_type_qualifier__word_t&)
{
    uint16_t ret;
    binary_packet bp=vfb.read(sizeof(uint16_t));
    memcpy(&ret,bp.data(),bp.size());
    return ret;
}

uint32_t BBio_File::read(const beye_aka_binary_eye_project_data_type_qualifier_dword_t&)
{
    uint32_t ret;
    binary_packet bp=vfb.read(sizeof(uint32_t));
    memcpy(&ret,bp.data(),bp.size());
    return ret;
}

uint64_t BBio_File::read(const beye_aka_binary_eye_project_data_type_qualifier_qword_t&)
{
    uint64_t ret;
    binary_packet bp=vfb.read(sizeof(uint64_t));
    memcpy(&ret,bp.data(),bp.size());
    return ret;
}

binary_packet BBio_File::read(size_t cbBuffer)
{
    return vfb.read(cbBuffer);
}

bool BBio_File::write(uint8_t bVal)
{
    return vfb.write(bVal);
}

bool BBio_File::write(uint16_t wVal)
{
    binary_packet bp(&wVal,sizeof(uint16_t));
    return vfb.write(bp);
}

bool BBio_File::write(uint32_t dwVal)
{
    binary_packet bp(&dwVal,sizeof(uint32_t));
    return vfb.write(bp);
}

bool BBio_File::write(uint64_t dwVal)
{
    binary_packet bp(&dwVal,sizeof(uint64_t));
    return vfb.write(bp);
}

bool BBio_File::write(const binary_packet& _buffer)
{
    return vfb.write(_buffer);
}

bool BBio_File::flush()
{
    return vfb.flush();
}

bool BBio_File::reread()
{
    __filesize_t fpos;
    fpos = tell();
    binary_stream::seek(0L,Seek_End);
    return seek(fpos,Seek_Set);
}

bool BBio_File::chsize(__filesize_t newsize)
{
    __filesize_t length;
    bool ret;

    length = _flength;
    ret=binary_stream::chsize(newsize);
    if(length >= newsize) vfb.chsize(newsize);
    _flength=binary_stream::flength();
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

bool BBio_File::dup(BBio_File& it) const
{
    bool rc = binary_stream::dup(it);
    it.openmode = openmode;
    it.optimize = optimize;
    it.is_eof = is_eof;
    it._flength  = _flength;
    it.filepos = std::min(filepos,vfb.fstart()+(vfb.buffsize()/2));
    it.vfb = vfb;
    return rc;
}

binary_stream* BBio_File::dup()
{
    BBio_File* ret = new(zeromem) BBio_File(vfb.buffsize(),Opt_Db);
    if(!dup(*ret)) return this;
    return ret;
}

bool BBio_File::eof() const
{
    return is_eof;
}

__filesize_t BBio_File::flength() const { return _flength; }

BBio_File::binary_cache::binary_cache(BBio_File& _parent,unsigned size)
	    :buffer(new char[size])
	    ,parent(_parent)
{
    f_start = 0L;
    updated  = true;
}
BBio_File::binary_cache::~binary_cache() {}
BBio_File::binary_cache& BBio_File::binary_cache::operator=(const BBio_File::binary_cache& it) {
    updated = it.updated;
    buflen  = std::min(it.bufsize,it.buflen);
    bufsize = it.bufsize;
    f_start = it.f_start;
    char& dptr = *buffer;
    const char& sptr = *it.buffer;
    ::memcpy(&dptr,&sptr,buflen);
    return *this;
}

bool BBio_File::binary_cache::fill(__fileoff_t pos)
{
    char& ptr = *buffer;
    any_t* mbuff = &ptr;
    __filesize_t remaind;
    bool ret;
    if(pos < 0) pos = 0;
    if((__filesize_t)pos > parent._flength) {
	pos = parent._flength;
	buflen = 0;
	ret = false;
    } else {
	f_start = pos;
	remaind = parent._flength - pos;
	buflen = (__filesize_t)bufsize < remaind ? bufsize : (unsigned)remaind;
	parent.seek(pos,Seek_Set);
	binary_packet bp = parent.read(buflen); memcpy(mbuff,bp.data(),bp.size());
	ret = bp.size() == buflen;
    }
    return ret;
}

bool BBio_File::binary_cache::flush()
{
    char& ptr = *buffer;
    any_t* mbuff = &ptr;
    bool ret;
    ret = true;
    if(!updated) {
	parent.seek(f_start,Seek_Set);
	if(buflen) {
	    binary_packet bp(mbuff,buflen);
	    ret = (unsigned)parent.write(bp) == buflen;
	}
	if(ret)		updated = true;
    }
    return ret;
}

bool BBio_File::binary_cache::seek(__fileoff_t pos,e_seek origin)
{
    bool ret,rret;
    ret=parent.seek_fptr(pos,origin);
    parent.filepos = pos;
    if(is_out_of_buffer(pos)) {
	flush();
	switch(parent.optimize & Opt_DirMask) {
	    default:
	    case Opt_Db:	break;
	    case Opt_Random:	pos -= bufsize / 2;
				break;
	    case Opt_BackScan:	pos -= (bufsize - 1);
				break;
	    case Opt_RForward:	pos -= bufsize / 10;
				break;
	    case Opt_RBackScan:	pos -= (bufsize/10)*9;
				break;
	}
	if(parent.filepos < parent._flength) {
	    rret = fill(pos);
	    if(ret) ret = rret;
	} else {
	    f_start = parent.filepos;
	    buflen = 0;
	}
    }
    return ret;
}

unsigned char BBio_File::binary_cache::read()
{
    char& ptr = *buffer;
    char* buff = &ptr;
    unsigned _buffpos;
    bool ret;
    unsigned char ch;
    ret = true;
    parent.founderr = false;
    if(is_out_of_contents(parent.filepos)) ret = seek(parent.filepos,Seek_Set);
    ch = -1;
    if(buflen && ret && parent.filepos <= parent._flength) {
	_buffpos = (unsigned)(parent.filepos - f_start);
	if(_buffpos < buflen)  ch = buff[_buffpos];
	if(parent.filepos < parent._flength) parent.filepos++;
	__fileoff_t fpos=parent.filepos;
	ret=parent.chk_eof(fpos);
	parent.filepos=fpos;
    } else {
	errno = EACCES;
	ret = false;
    }
    if(ret == false) parent.founderr = true;
    return ch;
}

bool BBio_File::binary_cache::write(unsigned char ch)
{
    char* buff;
    unsigned _buffpos;
    bool ret;
    if(parent.is_writeable(parent.openmode)) {
	ret = true;
	char& ptr = *buffer;
	buff = &ptr;
	if(is_out_of_buffer(parent.filepos)) seek(parent.filepos,Seek_Set);
	_buffpos = (unsigned)(parent.filepos - f_start);
	buff[_buffpos++] = ch;
	updated = false;
	if(buflen < _buffpos && buflen < bufsize) buflen = _buffpos;
	if(parent._flength <= f_start + _buffpos) parent._flength = f_start + _buffpos;
	parent.filepos++;
    } else { errno = EACCES; ret = false; }
    return ret;
}

binary_packet BBio_File::binary_cache::read(size_t cbBuff)
{
    binary_packet rc(cbBuff);
    size_t diffsize;
    size_t MBufStart,MBufRem;
    int _optimize = parent.optimize;
    bool ret = true;
    uint8_t* buff = rc.cdata();

    parent.optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
    while(cbBuff) {
	MBufStart = (unsigned)(parent.filepos - f_start);
	MBufRem = buflen - MBufStart;
	if(!MBufRem || is_out_of_contents(parent.filepos)) {
	    ret = seek(parent.filepos,Seek_Set);
	    if(!ret) break;
	    if(parent.filepos >= parent._flength) break;
	    MBufStart = (unsigned)(parent.filepos - f_start);
	    MBufRem = buflen - MBufStart;
	}
	if(!MBufRem) {
	    errno = E2BIG; /* fault: internal error */
	    ret = false;
	    break;
	}
	diffsize = std::min(MBufRem,cbBuff);
	char& ptr = *buffer;
	char* pptr = &ptr;
	::memcpy(buff,&pptr[MBufStart],diffsize);
	parent.filepos += diffsize;
	if(parent.filepos > parent._flength) parent.filepos = parent._flength;
	cbBuff -= diffsize;
	buff += diffsize;
    }
    parent.optimize = _optimize;
    return ret;
}

bool BBio_File::binary_cache::write(const binary_packet& bp)
{
    size_t diffsize;
    size_t MBufStart,MBufRem;
    int _optimize = parent.optimize;
    bool ret = true;
    const uint8_t* buff = bp.cdata();
    size_t cbBuff = bp.size();
    if(parent.is_writeable(parent.openmode)) {
	parent.optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
	ret = true;
	while(cbBuff) {
	    MBufStart = (unsigned)(parent.filepos - f_start);
	    MBufRem = bufsize - MBufStart;
	    if(!MBufRem || is_out_of_buffer(parent.filepos)) {
		ret = seek(parent.filepos,Seek_Set);
		if(!ret) break;
		MBufStart = (unsigned)(parent.filepos - f_start);
		MBufRem = bufsize - MBufStart;
	    }
	    if(!MBufRem) {
		errno = E2BIG; /* fault: internal error */
		ret = false;
		break;
	    }
	    diffsize = std::min(MBufRem,cbBuff);
	    char& ptr = *buffer;
	    char* pptr = &ptr;
	    ::memcpy(&pptr[MBufStart],buff,diffsize);
	    updated = false;
	    if(buflen < MBufStart + diffsize) buflen = MBufStart + diffsize;
	    if(parent._flength < parent.filepos + diffsize) parent._flength = parent.filepos + diffsize;
	    parent.filepos += diffsize;
	    cbBuff -= diffsize;
	    buff += diffsize;
	}
	parent.optimize = _optimize;
    } else { errno = EACCES; ret = false; }
    return ret;
}

void BBio_File::binary_cache::chsize(__filesize_t newsize) {
    seek(newsize, Seek_Set);
    if(f_start > parent._flength) { f_start = parent._flength; buflen = 0; }
    if(f_start + buflen > parent._flength) buflen = (unsigned)(parent._flength - f_start);
}
} // namespace	usr
