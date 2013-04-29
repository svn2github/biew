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

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "libbeye/bbio.h"

namespace beye {
BFile bNull;

BFile::BFile()
	:FilePos(0),
	FLength(0),
	FileName(NULL),
	openmode(0),
	optimize(0),
	is_mmf(false),
	primary_mmf(false),
	is_eof(true),
	founderr(false)
{
    b.vfb.handle=NULL_HANDLE;
}

BFile::~BFile() { if(FileName) close(); }

/* notes: all function with prefix=>__ assume, that buffer present */

bool BFile::__fill(__fileoff_t pos)
{
  any_t* mbuff;
  __filesize_t remaind;
  bool ret;
  if(pos < 0) pos = 0;
  if((__filesize_t)pos > FLength)
  {
     pos = FLength;
     b.vfb.MBufLen = 0;
     ret = false;
  }
  else
  {
    b.vfb.FBufStart = pos;
    remaind = FLength - pos;
    b.vfb.MBufLen = (__filesize_t)b.vfb.MBufSize < remaind ?
				     b.vfb.MBufSize : (unsigned)remaind;
    mbuff = b.vfb.MBuffer;
    __OsSeek(b.vfb.handle,pos,SEEKF_START);
    ret = (unsigned)__OsRead(b.vfb.handle,mbuff,b.vfb.MBufLen) == b.vfb.MBufLen;
  }
  return ret;
}

bool BFile::__flush()
{
  any_t* mbuff;
  bool ret;
  ret = true;
  if(!b.vfb.updated)
  {
    mbuff = b.vfb.MBuffer;
    __OsSeek(b.vfb.handle,b.vfb.FBufStart,SEEKF_START);
    if(b.vfb.MBufLen) ret = (unsigned)__OsWrite(b.vfb.handle,mbuff,b.vfb.MBufLen) == b.vfb.MBufLen;
    if(ret)           b.vfb.updated = true;
  }
  return ret;
}

bool BFile::__seek(__fileoff_t pos,int origin)
{
 bool ret,rret;
 ret=seek_fptr(pos,origin);
 FilePos = pos;
 if(__isOutOfBuffer(pos))
 {
    __flush();
    switch(optimize & Opt_DirMask)
    {
      default:
      case Opt_Db:         break;
      case Opt_Random:     pos -= b.vfb.MBufSize / 2;
			   break;
      case Opt_BackScan:   pos -= (b.vfb.MBufSize - 1);
			   break;
      case Opt_RForward:   pos -= b.vfb.MBufSize / 10;
			   break;
      case Opt_RBackScan:  pos -= (b.vfb.MBufSize/10)*9;
			   break;
    }
    if(FilePos < FLength)
    {
      rret = __fill(pos);
      if(ret) ret = rret;
    }
    else
    {
      b.vfb.FBufStart = FilePos;
      b.vfb.MBufLen = 0;
    }
 }
 return ret;
}

unsigned char BFile::__getc()
{
  char* buff = b.vfb.MBuffer;
  unsigned _buffpos;
  bool ret;
  unsigned char ch;
  ret = true;
  founderr = false;
  if(__isOutOfContents(FilePos)) ret = __seek(FilePos,SEEKF_START);
  ch = -1;
  if(b.vfb.MBufLen && ret && FilePos <= FLength)
  {
    _buffpos = (unsigned)(FilePos - b.vfb.FBufStart);
    if(_buffpos < b.vfb.MBufLen)  ch = buff[_buffpos];
    if(FilePos < FLength) FilePos++;
    __fileoff_t fpos=FilePos;
    ret=chk_eof(fpos);
    FilePos=fpos;
  }
  else
  {
    errno = EACCES;
    ret = false;
  }
  if(ret == false) founderr = true;
  return ch;
}

bool BFile::__putc(unsigned char ch)
{
  char *  buff;
  unsigned _buffpos;
  bool ret;
  if(is_writeable(openmode))
  {
    ret = true;
    buff = b.vfb.MBuffer;
    if(__isOutOfBuffer(FilePos)) __seek(FilePos,SEEKF_START);
    _buffpos = (unsigned)(FilePos - b.vfb.FBufStart);
    buff[_buffpos++] = ch;
    b.vfb.updated = false;
    if(b.vfb.MBufLen < _buffpos)
      if(b.vfb.MBufLen < b.vfb.MBufSize) b.vfb.MBufLen = _buffpos;
    if(FLength <= b.vfb.FBufStart + _buffpos)
      FLength = b.vfb.FBufStart + _buffpos;
    FilePos++;
  }
  else { errno = EACCES; ret = false; }
  return ret;
}

#if 0
static void  __FASTCALL__ dump_BFILE(BFILE*obj)
{
fprintf(stderr,
	"      %p\n"
	"      FilePos=%llu FLength=%llu\n"
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
	,obj->FilePos,obj->FLength
	,obj->FileName,obj->openmode,obj->optimize
	,obj->is_mmf,obj->primary_mmf
	,obj->b.vfb.handle,obj->b.vfb.FBufStart,obj->b.vfb.MBuffer
	,obj->b.vfb.MBufLen,obj->b.vfb.MBufSize,obj->b.vfb.updated
	,obj->is_mmf?obj->b.mmb->mmf:0,obj->is_mmf?obj->b.mmb->mmf_addr:0
	,obj->is_eof);
}
#endif

bool BFile::__getbuff(char* buff,unsigned cbBuff)
{
  unsigned diffsize;
  unsigned MBufStart,MBufRem;
  int _optimize = optimize;
  bool ret = true;
  if(is_mmf)
  {
    uint32_t size = std::min(__filesize_t(cbBuff),FLength - FilePos);
    __fileoff_t fpos=FilePos;
    ret=chk_eof(fpos);
    FilePos=fpos;
    if(size && ret)
    {
      memcpy(buff,((uint8_t *)b.mmb->mmf_addr) + FilePos,size);
      FilePos += size;
    }
  }
  else
  {
    optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
    while(cbBuff)
    {
      MBufStart = (unsigned)(FilePos - b.vfb.FBufStart);
      MBufRem = b.vfb.MBufLen - MBufStart;
      if(!MBufRem || __isOutOfContents(FilePos))
      {
       ret = __seek(FilePos,SEEKF_START);
       if(!ret) break;
       if(FilePos >= FLength) break;
       MBufStart = (unsigned)(FilePos - b.vfb.FBufStart);
       MBufRem = b.vfb.MBufLen - MBufStart;
      }
      if(!MBufRem)
      {
	 errno = E2BIG; /* fault: internal error */
	 ret = false;
	 break;
      }
      diffsize = std::min(MBufRem,cbBuff);
      memcpy(buff,&b.vfb.MBuffer[MBufStart],diffsize);
      FilePos += diffsize;
      if(FilePos > FLength) FilePos = FLength;
      cbBuff -= diffsize;
      buff += diffsize;
    }
    optimize = _optimize;
  }
  return ret;
}

bool BFile::__putbuff(const char* buff,unsigned cbBuff)
{
  unsigned diffsize;
  unsigned MBufStart,MBufRem;
  int _optimize = optimize;
  bool ret = true;
  if(is_writeable(openmode))
  {
    if(is_mmf)
    {
      uint32_t size = std::min(__filesize_t(cbBuff),FLength - FilePos);
      if(size)
      {
	memcpy(((uint8_t *)b.mmb->mmf_addr) + FilePos,buff,size);
	FilePos += size;
      }
    }
    else
    {
      optimize = (_optimize & ~Opt_DirMask) | Opt_Db;
      ret = true;
      while(cbBuff)
      {
	MBufStart = (unsigned)(FilePos - b.vfb.FBufStart);
	MBufRem = b.vfb.MBufSize - MBufStart;
	if(!MBufRem || __isOutOfBuffer(FilePos))
	{
	 ret = __seek(FilePos,SEEKF_START);
	 if(!ret) break;
	 MBufStart = (unsigned)(FilePos - b.vfb.FBufStart);
	 MBufRem = b.vfb.MBufSize - MBufStart;
	}
	if(!MBufRem)
	{
	   errno = E2BIG; /* fault: internal error */
	   ret = false;
	   break;
	}
	diffsize = std::min(MBufRem,cbBuff);
	memcpy(&b.vfb.MBuffer[MBufStart],buff,diffsize);
	b.vfb.updated = false;
	if(b.vfb.MBufLen < MBufStart + diffsize) b.vfb.MBufLen = MBufStart + diffsize;
	if(FLength < FilePos + diffsize) FLength = FilePos + diffsize;
	FilePos += diffsize;
	cbBuff -= diffsize;
	buff += diffsize;
      }
      optimize = _optimize;
    }
  }
  else { errno = EACCES; ret = false; }
  return ret;
}

bool BFile::open(const std::string& fname,unsigned _openmode,unsigned bSize,unsigned optimization)
{
    unsigned len;
    openmode = _openmode;
    FileName = new char [fname.length()+1];
    strcpy(FileName,fname.c_str());
    /* Attempt open as MMF */
    if(!is_writeable(openmode) && optimization == Opt_UseMMF)
    {
      if((b.mmb = new mmb))
      {
	if((b.mmb->mmf = __mmfOpen(FileName,openmode)) != NULL)
	{
	  b.mmb->mmf_addr = __mmfAddress(b.mmb->mmf);
	  FLength = __mmfSize(b.mmb->mmf);
	  primary_mmf = true;
	  is_mmf = true;
	  b.vfb.MBuffer = NULL; /* [dBorca] be consistent with is_cache_valid */
	}
	else delete b.mmb;
      }
   }
   if(!is_mmf)
   {
     bhandle_t _handle = __OsOpen(FileName,openmode);
     optimization = Opt_Db;
     if(_handle == NULL_HANDLE) return false;
     b.vfb.handle = _handle;
     optimize = optimization;
     b.vfb.FBufStart = 0L;
     b.vfb.updated  = true;
     is_mmf = false;
     FLength = __FileLength(b.vfb.handle);
     len = bSize;
     if(bSize == UINT_MAX) len = (unsigned)FLength;
     if(len)
     {
       if((b.vfb.MBuffer = new char [len]) != NULL)
       {
	 b.vfb.MBufLen = 0;
	 b.vfb.MBufSize = len;
       }
       else
       {
	 __OsClose(b.vfb.handle);
	 return false;
       }
       __fill(0L);
     }
     else b.vfb.MBuffer = NULL;
   }
   is_eof=false;
   return true;
}

bool BFile::close()
{
  if(is_mmf)
  {
    if(primary_mmf)
    {
      __mmfClose(b.mmb->mmf);
      delete b.mmb;
    }
  }
  else
  {
    if(b.vfb.handle != NULL_HANDLE)
    {
      if(is_writeable(openmode)) __flush();
      /* For compatibility with DOS-32: don't try to close stderr */
      if(b.vfb.handle != (bhandle_t)2) __OsClose(b.vfb.handle);
    }
    if(b.vfb.MBuffer) delete b.vfb.MBuffer;
  }
  delete FileName;
  FileName=NULL;
  return true;
}

bool BFile::seek(__fileoff_t pos,int orig)
{
 bool ret;
 if(is_cache_valid()) ret = __seek(pos,orig);
 else
 {
   if(is_mmf)
   {
     ret=seek_fptr(pos,orig);
     FilePos = pos;
   }
   else
   {
     ret = __OsSeek(b.vfb.handle,pos,orig) != 0;
     if(pos == 0L && orig == SEEKF_START) ret = true;
   }
 }
 return ret;
}

__filesize_t BFile::tell() const
{
  return (is_cache_valid() || is_mmf) ? FilePos : (__filesize_t)__OsTell(b.vfb.handle);
}

uint8_t BFile::read_byte()
{
  uint8_t ret;
  if(is_cache_valid() && !is_mmf) {
    ret = __getc();
  }
  else {
    if(is_mmf)
    {
      uint8_t rval;
      rval = ret = ((uint8_t *)b.mmb->mmf_addr)[FilePos];
      if(FilePos < FLength) FilePos++;
      __fileoff_t fpos=FilePos;
      ret=chk_eof(fpos);
      FilePos=fpos;
      ret = rval;
    }
    else {
      if(__OsRead(b.vfb.handle,&ret,sizeof(uint8_t)) != sizeof(uint8_t)) ret = -1;
    }
  }
  return ret;
}

uint16_t BFile::read_word()
{
  uint16_t ret;
  if(is_cache_valid() || is_mmf)
  {
    if(!__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint16_t))) ret = -1;
  }
  else
    if(__OsRead(b.vfb.handle,&ret,sizeof(uint16_t)) != sizeof(uint16_t)) ret = -1;
  return ret;
}

uint32_t BFile::read_dword()
{
  uint32_t ret;
  if(is_cache_valid() || is_mmf)
  {
    if(!__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint32_t))) ret = -1;
  }
  else
    if(__OsRead(b.vfb.handle,&ret,sizeof(uint32_t)) != sizeof(uint32_t)) ret = -1;
  return ret;
}

uint64_t BFile::read_qword()
{
  uint64_t ret;
  if(is_cache_valid() || is_mmf)
  {
    if(!__getbuff(reinterpret_cast<char*>(&ret),sizeof(uint64_t))) ret = -1;
  }
  else
    if(__OsRead(b.vfb.handle,&ret,sizeof(uint64_t)) != sizeof(uint64_t)) ret = -1;
  return ret;
}

bool BFile::read_buffer(any_t* _buffer,unsigned cbBuffer)
{
  return is_cache_valid() || is_mmf ?
	    __getbuff(reinterpret_cast<char*>(_buffer),cbBuffer) :
	    ((unsigned)__OsRead(b.vfb.handle,_buffer,cbBuffer)) == cbBuffer;
}

bool BFile::write_byte(uint8_t bVal)
{
  bool ret = true;
  if(is_cache_valid()) ret = __putc(bVal);
  else
    ret = __OsWrite(b.vfb.handle,&bVal,sizeof(uint8_t)) == sizeof(uint8_t);
  return ret;
}

bool BFile::write_word(uint16_t wVal)
{
  return is_cache_valid() ?
	     __putbuff(reinterpret_cast<const char*>(&wVal),sizeof(uint16_t)) :
	     __OsWrite(b.vfb.handle,&wVal,sizeof(uint16_t)) == sizeof(uint16_t);
}

bool BFile::write_dword(uint32_t dwVal)
{
  return is_cache_valid() ?
	     __putbuff(reinterpret_cast<const char*>(&dwVal),sizeof(uint32_t)) :
	     __OsWrite(b.vfb.handle,&dwVal,sizeof(uint32_t)) == sizeof(uint32_t);
}

bool BFile::write_qword(uint64_t dwVal)
{
  return is_cache_valid() ?
	     __putbuff(reinterpret_cast<const char*>(&dwVal),sizeof(uint64_t)) :
	     __OsWrite(b.vfb.handle,&dwVal,sizeof(uint64_t)) == sizeof(uint64_t);
}

bool BFile::write_buffer(const any_t* _buffer,unsigned cbBuffer)
{
  return is_cache_valid() ?
	     __putbuff(reinterpret_cast<const char*>(_buffer),cbBuffer) :
	     ((unsigned)__OsWrite(b.vfb.handle,_buffer,cbBuffer)) == cbBuffer;
}

bool BFile::flush()
{
  return is_cache_valid() ? __flush() : is_mmf ? __mmfFlush(b.mmb->mmf) : true;
}

bool BFile::reread()
{
  __filesize_t fpos;
  bool ret = true;
  if(is_mmf) {
    b.mmb->mmf = __mmfSync(b.mmb->mmf);
    if(b.mmb->mmf)
    {
      b.mmb->mmf_addr = __mmfAddress(b.mmb->mmf);
      FLength  = __mmfSize(b.mmb->mmf);
    }
    /** @todo Most reliable code */
    else { printm("Internal error occured in __mmfSync\n"); exit(EXIT_FAILURE); }
  }
  else
  {
    fpos = __OsTell(b.vfb.handle);
    __OsSeek(b.vfb.handle,0L,SEEKF_END);
    FLength = __OsTell(b.vfb.handle);
    __OsSeek(b.vfb.handle,fpos,SEEKF_START);
    ret = __fill(b.vfb.FBufStart);
  }
  return ret;
}

__filesize_t BFile::flength() const
{
  return FLength;
}

bool BFile::chsize(__filesize_t newsize)
{
    __filesize_t length, fillsize;
    char * buf;
    unsigned  bufsize, numtowrite;
    bool ret;
    if(is_mmf) {
      if((ret=__mmfResize(b.mmb->mmf,newsize))) {
	 b.mmb->mmf_addr = __mmfAddress(b.mmb->mmf);
	 FLength  = __mmfSize(b.mmb->mmf);
      }
    } else {
      length = FLength;
      if(length >= newsize) { /* truncate size */
	__seek(newsize, SEEKF_START);
	length = newsize;
	ret = __OsTruncFile(b.vfb.handle, length) != 0;
	FLength = newsize;
	if(b.vfb.FBufStart > FLength) { b.vfb.FBufStart = FLength; b.vfb.MBufLen = 0; }
	if(b.vfb.FBufStart + b.vfb.MBufLen > FLength) b.vfb.MBufLen = (unsigned)(FLength - b.vfb.FBufStart);
	ret = ret ? false : true;
      } else {
	fillsize=newsize-length;  /* increase size */
	ret = false;
	bufsize = b.vfb.MBuffer ? b.vfb.MBufSize : 8192;
	bufsize = (unsigned) std::min(fillsize,__filesize_t(bufsize));
	if((buf = new char [bufsize]) != NULL)
	{
	  ret = true;
	  memset(buf, 0, bufsize);   /* write zeros to pad file */
	  seek(0L,SEEKF_END);
	  do
	  {
	    numtowrite = (unsigned)std::min(__filesize_t(bufsize),fillsize);
	    if(!write_buffer((any_t* )buf, numtowrite)) { ret = false; break; }
	    fillsize-=numtowrite;
	  } while(fillsize);
	  delete buf;
	}
      }
   }
   return ret;
}

unsigned BFile::set_optimization(unsigned flags)
{
  unsigned ret;
  ret = optimize;
  optimize = flags;
  return ret;
}

unsigned BFile::get_optimization() const
{
  return optimize;
}

bhandle_t BFile::handle() const
{
   return b.vfb.handle;
}

const char* BFile::filename() const
{
  return FileName;
}

any_t* BFile::buffer() const
{
  return is_mmf ? b.mmb->mmf_addr : b.vfb.MBuffer;
}

unsigned BFile::bufflen() const
{
  return is_mmf ? FLength : b.vfb.MBufLen;
}

unsigned BFile::buffpos() const
{
  return is_mmf ? 0L : b.vfb.MBufLen - (unsigned)(FilePos - b.vfb.FBufStart);
}

BFile* BFile::dup_ex(unsigned buff_size) const
{
    BFile* ret = NULL;
    unsigned len;
    ret = new BFile;
    if(ret) {
	ret->openmode = openmode;
	ret->optimize = optimize;
	ret->is_eof = is_eof;
	if(!(ret->FileName = new char [strlen(FileName)+1])) {
	    delete ret;
	    return &bNull;
	}
	strcpy(ret->FileName,FileName);

	if(is_mmf) {
	    ret->b.mmb = b.mmb;
	    ret->FilePos = FilePos;
	    ret->primary_mmf = false;
	} else {
	    ret->b.vfb.handle = __OsDupHandle(b.vfb.handle);
	    ret->b.vfb.updated  = b.vfb.updated;
	}
	ret->is_mmf = is_mmf;
	if(ret->b.vfb.handle == NULL_HANDLE) {
	    delete ret->FileName;
	    delete ret;
	    return &bNull;
	}
	ret->FLength = FLength;
	len = buff_size;
	if(len && !ret->is_mmf) {
	    if((ret->b.vfb.MBuffer = new char [len]) != NULL) {
		if(is_cache_valid()) {
		    ret->b.vfb.MBufLen = std::min(len,b.vfb.MBufLen);
		    ret->b.vfb.MBufSize = len;
		    ret->FilePos = std::min(FilePos,b.vfb.FBufStart+(len/2));
		    ret->b.vfb.FBufStart = b.vfb.FBufStart;
		    memcpy(ret->b.vfb.MBuffer,b.vfb.MBuffer,ret->b.vfb.MBufLen);
		} else {
		    ret->b.vfb.MBufLen = 0;
		    ret->b.vfb.MBufSize = 0;
		    ret->FilePos = 0L;
		    ret->b.vfb.FBufStart = 0;
		}
	    } else {
		delete ret->FileName;
		__OsClose(ret->b.vfb.handle);
		delete ret;
		return &bNull;
	    }
	} else ret->b.vfb.MBuffer = 0;
    } else ret = &bNull;
    return ret;
}

BFile* BFile::dup() const
{
    return dup_ex(b.vfb.MBufSize);
}

bool BFile::eof() const
{
    bool retval;
    if(is_cache_valid() || is_mmf) retval = is_eof;
    else                           retval = (__filesize_t)__OsTell(b.vfb.handle) >= FLength;
    return retval;
}
} // namespace beye
