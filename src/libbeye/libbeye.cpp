#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace   libbeye
 * @file        libbeye/libbeye.c
 * @brief       This file contains implementation of extension of C library.
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
 * @todo        Increase number of functions
**/
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

void __FASTCALL__ memupr(any_t*ptr,unsigned n)
{
   unsigned i;
   for(i = 0;i < n;i++)
   ((char *)ptr)[i] = toupper(((char *)ptr)[i]);
}

void __FASTCALL__ memlwr(any_t*ptr,unsigned n)
{
   unsigned i;
   for(i = 0;i < n;i++)
   ((char *)ptr)[i] = tolower(((char *)ptr)[i]);
}

#ifndef HAVE_ATOLL
long long int atoll(const char* s) { return strtoll(s, NULL, 10); } /* temporal workaround */
#endif
#ifndef HAVE_LTOA
/* (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */
char *ltoa (long value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    unsigned long x;
    int i, n;
    char digits[32];
    if (radix == 10 && value < 0)
    {
      *dst++ = '-';
      x = -value;
    }
    else x = value;
    i = 0;
    do
    {
      n = x % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      x /= radix;
    } while (x != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}
#endif
#ifndef HAVE_ULTOA
char *ultoa (unsigned long value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    int i;
    unsigned n;
    char digits[32];
    i = 0;
    do
    {
      n = value % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      value /= radix;
    } while (value != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}
#endif
#ifndef HAVE_LLTOA
char *lltoa (long long int value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    unsigned long long int x;
    int i, n;
    char digits[64];
    if (radix == 10 && value < 0)
    {
      *dst++ = '-';
      x = -value;
    }
    else x = value;
    i = 0;
    do
    {
      n = x % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      x /= radix;
    } while (x != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}
#endif
#ifndef HAVE_ULLTOA
char *ulltoa (unsigned long long int value, char *string, int radix)
{
  char *dst;

  dst = string;
  if (radix < 2 || radix > 36) *dst = 0;
  else
  {
    int i;
    unsigned n;
    char digits[64];
    i = 0;
    do
    {
      n = value % radix;
      digits[i++] = n+(n < 10 ? '0' : 'A'-10);
      value /= radix;
    } while (value != 0);
    while (i > 0) *dst++ = digits[--i];
    *dst = 0;
  }
  return string;
}
#endif
namespace	usr {
bool __FASTCALL__ isseparate(int ch) { return (isspace(ch) || ispunct(ch)); }

int __FASTCALL__ szTrimTrailingSpace(char *str)
{
  unsigned len;
  int ret;
  len = strlen(str);
  ret = 0;
  while(len)
  {
      unsigned char ch;
      ch = str[len-1];
      if(isspace(ch) && ch < 0x80) { str[--len] = '\0'; ret++; }
      else break;
  }
  return ret;
}

int __FASTCALL__ szTrimLeadingSpace(char *str)
{
  unsigned i,freq,len;
  len = strlen(str);
  for(i = freq = 0;i < len;i++)
  {
    unsigned char ch;
    ch = str[i];
    if(isspace(ch) && ch < 0x80) freq++;
    else                         break;
  }
  if(freq)
  {
    len -= freq;
    memmove(str,&str[freq],len+1);
  }
  return freq;
}

static const unsigned TEXT_TAB=8;

void __FASTCALL__ szSpace2Tab(char *dest,const char * src)
{
  unsigned int i,len,limit,dest_idx;
  int j;
  unsigned char buff[8],nspc;
  len = strlen(src);
  i = 0;
  dest_idx = 0;
  while(1)
  {
     if(i + TEXT_TAB < len)
     {
	memcpy(buff,&src[i],8);
	i+=8;
	/* scan */
	nspc = 0;
	for(j = TEXT_TAB-1;j >= 0;j--)
	{
	  if(buff[j] != ' ') break;
	  else nspc++;
	}
	limit = TEXT_TAB - nspc;
	memcpy(&dest[dest_idx],buff,limit);
	dest_idx += limit;
	if(nspc) dest[dest_idx++] = '\t';
     }
     else
     {
       limit = len - i;
       memcpy(&dest[dest_idx],&src[i],limit);
       dest_idx += limit;
       i += limit;
       break;
     }
  }
  dest[dest_idx] = '\0';
}

int __FASTCALL__ szTab2Space(char * dest,const char * src)
{
  int i,k,len;
  size_t size;
  unsigned int freq;
  unsigned char ch;
  len = strlen(src);
  for(freq = 0,i = k = 0;i < len;i++,freq++)
  {
    ch = src[i];
    if(ch == '\t')
    {
       size = TEXT_TAB - (freq%TEXT_TAB);
       memset(&dest[k],' ',size);
       k += size;
       freq += size-1;
    }
    else
    {
      dest[k] = ch;
      k++;
    }
  }
  return k;
}

char * __FASTCALL__ szKillSpaceAround(char *str,char *place)
{
  char *sptr;
  unsigned nmoves,len,idx,freq;
  unsigned char prev;
  unsigned char ch;
  prev = *place;
  len = strlen(str);
  *place = 0;
  idx = place - str;
  nmoves = szTrimTrailingSpace(str);
  sptr = place;
  freq = 0;
  sptr++;
  while((ch = *sptr) != 0)
  {
    if(isspace(ch)) freq++;
    else            break;
    sptr++;
  }
  memmove(&str[idx-nmoves],&str[idx+freq],len-idx+1-freq);
  str[idx-nmoves] = prev;
  return &str[idx-nmoves];
}

		/** Exchanges two bytes in memory.
		  * @return         none
		  * @param _val1    specified pointer to the first byte to be exchanged
		  * @param _val2    specified pointer to the second byte to be exchanged
		  * @note           Main difference from ByteSwap function family -
				    it is work with different number, rather than
				    changing byte order within given number.
		 **/
inline void __XchgB__(uint8_t* _val1,uint8_t* _val2) {
#if defined(__i386__) || defined(__x86_64__)
    register char _tmp;
    __asm("xchgb	%b1,(%2)":
	"=q"(_tmp):
	"0"(*_val2),
	"r"(_val1));
    *_val2 = _tmp;
#else
    register uint8_t _charv;
    _charv = *((uint8_t *)_val2);
    *((uint8_t *)_val2) = *((uint8_t *)_val1);
    *((uint8_t *)_val1) = _charv;
#endif
}

any_t* rnd_fill(any_t* buffer,size_t size)
{
    unsigned i;
    char ch;
    for(i=0;i<size;i++) {
	ch=::rand()%255;
	((char *)buffer)[i]=ch;
    }
    return buffer;
}

any_t* make_false_pointer(any_t* tmplt) {
    long lo_mask=(sizeof(any_t*)*8/2)-1;
    long hi_mask=~lo_mask;
    long false_pointer;
    false_pointer=::rand()&lo_mask;
    false_pointer|=(reinterpret_cast<long>(tmplt)&hi_mask);
    return reinterpret_cast<any_t*>(false_pointer);
}

any_t*	__FASTCALL__ make_false_pointer_to(any_t* tmplt,unsigned size) {
    long false_pointer=reinterpret_cast<long>(tmplt);
    false_pointer+=::rand()%size;
    return reinterpret_cast<any_t*>(false_pointer);
}

any_t* fill_false_pointers(any_t* buffer,size_t size)
{
    unsigned i,psize=(size/sizeof(any_t*))*sizeof(any_t*);
    any_t* filler;
    for(i=0;i<psize/sizeof(long);i++) {
	filler=make_false_pointer(buffer);
	((long *)buffer)[i]=::rand()%2?reinterpret_cast<long>(filler):0;
    }
    ::memset(&((char *)buffer)[psize],0,size-psize);
    return buffer;
}

any_t* get_caller_address(unsigned num_caller) {
    any_t*	stack[3+num_caller];
    ::backtrace(stack,3+num_caller);
    return stack[2+num_caller];
}

Opaque::Opaque() {
    fill_false_pointers(&false_pointers,reinterpret_cast<long>(&unusable)-reinterpret_cast<long>(&false_pointers));
    fill_false_pointers(&unusable,sizeof(any_t*));
}

Opaque::~Opaque() {}

void tvideo_buffer::_construct() {
    chars = new t_vchar[len];
    oempg = new t_vchar[len];
    attrs = new ColorAttr[len];
}

tvideo_buffer::tvideo_buffer(size_t n)
	    :len(n)
{
    _construct();
}

tvideo_buffer::tvideo_buffer(const t_vchar* _chars,const t_vchar* _oempg,const ColorAttr* _attrs,size_t n)
	    :len(n) {
    _construct();
    ::memcpy(chars,_chars,len);
    ::memcpy(oempg,_oempg,len);
    ::memcpy(attrs,_attrs,len);
}

tvideo_buffer::tvideo_buffer(t_vchar c,t_vchar o,ColorAttr a,size_t n)
	    :len(n) {
    _construct();
    fill(c,o,a);
}

tvideo_buffer::~tvideo_buffer() {
    if(chars) delete chars;
    if(oempg) delete oempg;
    if(attrs) delete attrs;
}

tvideo_buffer& tvideo_buffer::operator=(const tvideo_buffer& it) {
    if(chars) delete chars;
    if(oempg) delete oempg;
    if(attrs) delete attrs;
    len = it.len;
    chars = new t_vchar[len];
    oempg = new t_vchar[len];
    attrs = new ColorAttr[len];
    ::memcpy(chars,it.chars,len);
    ::memcpy(oempg,it.oempg,len);
    ::memcpy(attrs,it.attrs,len);
    return *this;
}

tvideo_buffer tvideo_buffer::operator[](size_t idx) const {
    size_t sz=len-idx;
    tvideo_buffer rc(sz);
    rc.assign(&chars[idx],&oempg[idx],&attrs[idx],sz);
    return rc;
}

void tvideo_buffer::resize(size_t newlen) {
    if(len!=newlen) {
	if(newlen) {
	    t_vchar*  c = new t_vchar[newlen];
	    t_vchar*  o = new t_vchar[newlen];
	    ColorAttr*a = new ColorAttr[newlen];
	    ::memcpy(c,chars,newlen);
	    ::memcpy(o,oempg,newlen);
	    ::memcpy(a,attrs,newlen);
	    delete chars;
	    delete oempg;
	    delete attrs;
	    chars = c;
	    oempg = o;
	    attrs = a;
	}
	else {
	    if(chars) { delete chars; chars=NULL; }
	    if(oempg) { delete oempg; oempg=NULL; }
	    if(attrs) { delete attrs; attrs=NULL; }
	}
	len=newlen;
    }
}


void tvideo_buffer::fill(t_vchar c,t_vchar o,ColorAttr a) {
    ::memset(chars,c,len);
    ::memset(oempg,o,len);
    ::memset(attrs,a,len);
}

void tvideo_buffer::fill_at(size_t idx,t_vchar c,t_vchar o,ColorAttr a,size_t sz) {
    if(idx+sz < len) {
	::memset(&chars[idx],c,sz);
	::memset(&oempg[idx],o,sz);
	::memset(&attrs[idx],a,sz);
    }
}

void tvideo_buffer::assign(const t_vchar* _chars,const t_vchar* _oempg,const ColorAttr* _attrs,size_t newlen) {
    size_t rlen = std::min(newlen,len);
    ::memcpy(chars,_chars,rlen);
    ::memcpy(oempg,_oempg,rlen);
    ::memcpy(attrs,_attrs,rlen);
}

void tvideo_buffer::assign(const tvideo_buffer& from,size_t rlen) {
    rlen = std::min(rlen,len);
    ::memcpy(chars,from.chars,rlen);
    ::memcpy(oempg,from.oempg,rlen);
    ::memcpy(attrs,from.attrs,rlen);
}

void tvideo_buffer::assign_at(size_t idx,const tvideo_buffer& from) {
    size_t sz=from.length();
    if(sz+idx<len) {
	::memcpy(&chars[idx],from.chars,sz);
	::memcpy(&oempg[idx],from.oempg,sz);
	::memcpy(&attrs[idx],from.attrs,sz);
    }
}

void tvideo_buffer::assign_at(size_t idx,const tvideo_buffer& from,size_t rlen) {
    size_t sz=rlen;
    if(sz+idx<len) {
	::memcpy(&chars[idx],from.chars,sz);
	::memcpy(&oempg[idx],from.oempg,sz);
	::memcpy(&attrs[idx],from.attrs,sz);
    }
}

void tvideo_buffer::assign_at(size_t idx,const t_vchar* _chars,const t_vchar* _oempg,const ColorAttr* _attrs,size_t newlen) {
    if(idx+newlen < len) {
	::memcpy(&chars[idx],_chars,newlen);
	::memcpy(&oempg[idx],_oempg,newlen);
	::memcpy(&attrs[idx],_attrs,newlen);
    }
}

void tvideo_buffer::assign_at(size_t idx,t_vchar c,t_vchar o,ColorAttr a) {
    if(idx<len) {
	chars[idx]=c;
	oempg[idx]=o;
	attrs[idx]=a;
    }
}

} // namespace	usr
