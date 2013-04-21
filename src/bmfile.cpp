/**
 * @namespace   beye
 * @file        bmfile.c
 * @brief       This file has developed as Buffered stream Manager and presents
 *              front end interface to bbio library.
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bmfile.h"
#include "bconsole.h"
#include "tstrings.h"
#include "libbeye/bbio.h"

unsigned BMFileFlags=0;
extern bool fioUseMMF;
BFile* bm_file_handle = NULL,*sc_bm_file_handle = NULL;

BFile* __FASTCALL__ beyeOpenRO(const char *fname,unsigned cache_size)
{
  BFile* fret;
  fret=new BFile;
  bool rc;
  rc = fret->open(fname,FO_READONLY | SO_DENYNONE,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(rc == false)
    rc = fret->open(fname,FO_READONLY | SO_COMPAT,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(rc==false) { delete fret; fret=&bNull; }
  return fret;
}

BFile* __FASTCALL__ beyeOpenRW(const char *fname,unsigned cache_size)
{
  BFile* fret;
  fret=new BFile;
  bool rc;
  rc = fret->open(fname,FO_READWRITE | SO_DENYNONE,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(rc == false)
    rc = fret->open(fname,FO_READWRITE | SO_COMPAT,cache_size,fioUseMMF ? BIO_OPT_USEMMF : BIO_OPT_DB);
  if(rc==false) { delete fret; fret=&bNull; }
  return fret;
}

int __FASTCALL__ BMOpen(const char * fname)
{
  bm_file_handle = beyeOpenRO(fname,BBIO_CACHE_SIZE);
  if(bm_file_handle == &bNull)
  {
    errnoMessageBox(OPEN_FAIL,NULL,errno);
    return -1;
  }
  sc_bm_file_handle = bm_file_handle->dup_ex(BBIO_SMALL_CACHE_SIZE);
  if(sc_bm_file_handle == &bNull)
  {
    errnoMessageBox(DUP_FAIL,NULL,errno);
    return -1;
  }
  bm_file_handle->set_optimization(BIO_OPT_RANDOM);
  sc_bm_file_handle->set_optimization(BIO_OPT_RANDOM);
  if(BMGetFLength() > ULONG_MAX) BMFileFlags |= BMFF_USE64;
  return 0;
}

void __FASTCALL__ BMClose( void )
{
  if(bm_file_handle != &bNull) delete bm_file_handle;
  bm_file_handle = &bNull;
  if(sc_bm_file_handle != &bNull) delete sc_bm_file_handle;
  sc_bm_file_handle = &bNull;
}

uint8_t  __FASTCALL__ BMReadByteEx(__fileoff_t pos,int RELATION)
{
 bm_file_handle->seek(pos,RELATION);
 return bm_file_handle->read_byte();
}

uint16_t __FASTCALL__ BMReadWordEx(__fileoff_t pos,int RELATION)
{
 bm_file_handle->seek(pos,RELATION);
 return bm_file_handle->read_word();
}

uint32_t  __FASTCALL__ BMReadDWordEx(__fileoff_t pos,int RELATION)
{
 bm_file_handle->seek(pos,RELATION);
 return bm_file_handle->read_dword();
}

uint64_t  __FASTCALL__ BMReadQWordEx(__fileoff_t pos,int RELATION)
{
 bm_file_handle->seek(pos,RELATION);
 return bm_file_handle->read_qword();
}

bool __FASTCALL__ BMReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,int RELATION)
{
 bm_file_handle->seek(pos,RELATION);
 return bm_file_handle->read_buffer(buffer,len);
}

bool __FASTCALL__ BMWriteBuff(any_t* buff,unsigned len)
{
  bool ret;
  ret = bm_file_handle->write_buffer(buff,len);
  bm_file_handle->flush();
  return ret;
}

bool __FASTCALL__ BMWriteByteEx(__fileoff_t pos,int RELATION,uint8_t byte)
{
  bm_file_handle->seek(pos,RELATION);
  return bm_file_handle->write_byte(byte);
}

bool __FASTCALL__ BMWriteWordEx(__fileoff_t pos,int RELATION,uint16_t word)
{
  bm_file_handle->seek(pos,RELATION);
  return bm_file_handle->write_word(word);
}

bool __FASTCALL__ BMWriteDWordEx(__fileoff_t pos,int RELATION,uint32_t dword)
{
  bm_file_handle->seek(pos,RELATION);
  return bm_file_handle->write_dword(dword);
}

bool __FASTCALL__ BMWriteQWordEx(__fileoff_t pos,int RELATION,uint64_t dword)
{
  bm_file_handle->seek(pos,RELATION);
  return bm_file_handle->write_qword(dword);
}

bool  __FASTCALL__ BMWriteBuffEx(__fileoff_t pos,int RELATION,any_t* buff,unsigned len)
{
  bm_file_handle->seek(pos,RELATION);
  return bm_file_handle->write_buffer(buff,len);
}

uint8_t __FASTCALL__ bmReadByteEx(__fileoff_t pos,int RELATION)
{
 sc_bm_file_handle->seek(pos,RELATION);
 return sc_bm_file_handle->read_byte();
}

uint16_t __FASTCALL__ bmReadWordEx(__fileoff_t pos,int RELATION)
{
 sc_bm_file_handle->seek(pos,RELATION);
 return sc_bm_file_handle->read_word();
}

uint32_t __FASTCALL__ bmReadDWordEx(__fileoff_t pos,int RELATION)
{
 sc_bm_file_handle->seek(pos,RELATION);
 return sc_bm_file_handle->read_dword();
}

uint64_t __FASTCALL__ bmReadQWordEx(__fileoff_t pos,int RELATION)
{
 sc_bm_file_handle->seek(pos,RELATION);
 return sc_bm_file_handle->read_qword();
}

bool __FASTCALL__ bmReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,int RELATION)
{
 sc_bm_file_handle->seek(pos,RELATION);
 return sc_bm_file_handle->read_buffer(buffer,len);
}

bool __FASTCALL__ bmWriteBuff(any_t* buff,unsigned len)
{
  bool ret;
  ret = sc_bm_file_handle->write_buffer(buff,len);
  sc_bm_file_handle->flush();
  return ret;
}

bool __FASTCALL__ bmWriteByteEx(__fileoff_t pos,int RELATION,uint8_t byte)
{
  sc_bm_file_handle->seek(pos,RELATION);
  return sc_bm_file_handle->write_byte(byte);
}

bool __FASTCALL__ bmWriteWordEx(__fileoff_t pos,int RELATION,uint16_t word)
{
  sc_bm_file_handle->seek(pos,RELATION);
  return sc_bm_file_handle->write_word(word);
}

bool __FASTCALL__ bmWriteDWordEx(__fileoff_t pos,int RELATION,uint32_t dword)
{
  sc_bm_file_handle->seek(pos,RELATION);
  return sc_bm_file_handle->write_dword(dword);
}

bool __FASTCALL__ bmWriteQWordEx(__fileoff_t pos,int RELATION,uint64_t dword)
{
  sc_bm_file_handle->seek(pos,RELATION);
  return sc_bm_file_handle->write_qword(dword);
}

bool  __FASTCALL__ bmWriteBuffEx(__fileoff_t pos,int RELATION,any_t* buff,unsigned len)
{
  sc_bm_file_handle->seek(pos,RELATION);
  return sc_bm_file_handle->write_buffer(buff,len);
}
