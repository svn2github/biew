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

/*
   Using own code for qsort and bsearch functions is guarantee of stable work */

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
/* Modified for use with 16-bits huge arrays by Nickols_K */
/*-
 * Copyright (c) 1980, 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * qsort.c:
 * Our own version of the system qsort routine which is faster by an average
 * of 25%, with lows and highs of 10% and 50%.
 * The THRESHold below is the insertion sort threshold, and has been adjusted
 * for records of size 48 bytes.
 * The MTHREShold is where we stop finding a better median.
 */

#define		THRESH		4		/**< threshold for insertion */
#define		MTHRESH		6		/**< threshold for median */

static  func_compare qcmp;                      /**< the comparison routine */
static  int		qsz;			/**< size of each record */
static  long		thresh;			/**< THRESHold in chars */
static  long		mthresh;		/**< MTHRESHold in chars */

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

/**
 * qst:
 * Do a quicksort
 * First, find the median element, and put that one in the first place as the
 * discriminator.  (This "median" is just the median of the first, last and
 * middle elements).  (Using this median instead of the first element is a big
 * win).  Then, the usual partitioning/swapping, followed by moving the
 * discriminator into the right place.  Then, figure out the sizes of the two
 * partions, do the smaller one recursively and the larger one via a repeat of
 * this code.  Stopping when there are less than THRESH elements in a partition
 * and cleaning up with an insertion sort (in our caller) is a huge win.
 * All data swaps are done in-line, which is space-losing but time-saving.
 * (And there are only three places where this is done).
 */
static void  qst(char  *base, char  *max)
{
  long ii,lo,hi;
  char  *i,  *j, *jj;
  char  *mid,  *tmp;

  /*
   * At the top here, lo is the number of characters of elements in the
   * current partition.  (Which should be max - base).
   * Find the median of the first, last, and middle element and make
   * that the middle element.  Set j to largest of first and middle.
   * If max is larger than that guy, then it's that guy, else compare
   * max with loser of first and take larger.  Things are set up to
   * prefer the middle, then the first in case of ties.
   */
  lo = max - base;		/* number of elements as chars */
  do	{
    mid = i = base + qsz * ((lo / qsz) >> 1);
    if (lo >= mthresh)
    {
      j = (qcmp((jj = base), i) > 0 ? jj : i);
      if (qcmp(j, (tmp = max - qsz)) > 0)
      {
	/* switch to first loser */
	j = (j == jj ? i : jj);
	if (qcmp(j, tmp) < 0)
	  j = tmp;
      }
      if (j != i)
      {
	ii = qsz;
	do{
	  __XchgB__((uint8_t*)i,(uint8_t*)j);
	  i++; j++;
	} while (--ii);
      }
    }
    /*
     * Semi-standard quicksort partitioning/swapping
     */
    for (i = base, j = max - qsz; ; )
    {
      while (i < mid && qcmp(i, mid) <= 0)
	i += qsz;
      while (j > mid)
      {
	if (qcmp(mid, j) <= 0)
	{
	  j -= qsz;
	  continue;
	}
	tmp = i + qsz;		/* value of i after swap */
	if (i == mid)
	{
	  /* j <-> mid, new mid is j */
	  mid = jj = j;
	}
	else
	{
	  /* i <-> j */
	  jj = j;
	  j -= qsz;
	}
	goto swap;
      }
      if (i == mid)
      {
	break;
      }
      else
      {
	/* i <-> mid, new mid is i */
	jj = mid;
	tmp = mid = i;		/* value of i after swap */
	j -= qsz;
      }
    swap:
      ii = qsz;
      do{
	__XchgB__((uint8_t*)i,(uint8_t*)jj);
	i++; jj++;
      } while (--ii);
      i = tmp;
    }
    /*
     * Look at sizes of the two partitions, do the smaller
     * one first by recursion, then do the larger one by
     * making sure lo is its size, base and max are update
     * correctly, and branching back.  But only repeat
     * (recursively or by branching) if the partition is
     * of at least size THRESH.
     */
    i = (j = mid) + qsz;
    if ((lo = j - base) <= (hi = max - i))
    {
      if (lo >= thresh)
	qst(base, j);
      base = i;
      lo = hi;
    }
    else
    {
      if (hi >= thresh)
	qst(i, max);
      max = j;
    }
  } while (lo >= thresh);
}

/*
 * qsort:
 * First, set up some global parameters for qst to share.  Then, quicksort
 * with qst(), and then a cleanup insertion sort ourselves.  Sound simple?
 * It's not...
 */
void __FASTCALL__ HQSort(any_t*base0,unsigned long num, unsigned width,
			 func_compare compare)
{
  char  *base = (char  *)base0;
  char  *i,  *j,  *lo,  *hi;
  char  *min,  *max;
  register char c;

  if (num <= 1)
    return;
  qsz = width;
  qcmp = compare;
  thresh = qsz * THRESH;
  mthresh = qsz * MTHRESH;
  max = base + num * qsz;
  if (num >= THRESH)
  {
    qst(base, max);
    hi = base + thresh;
  }
  else
  {
    hi = max;
  }
  /*
   * First put smallest element, which must be in the first THRESH, in
   * the first position as a sentinel.  This is done just by searching
   * the first THRESH elements (or the first n if n < THRESH), finding
   * the min, and swapping it into the first position.
   */
  for (j = lo = base; (lo += qsz) < hi; )
    if (qcmp(j, lo) > 0)
      j = lo;
  if (j != base)
  {
    /* swap j into place */
    for (i = base, hi = base + qsz; i < hi; )
    {
      __XchgB__((uint8_t*)i,(uint8_t*)j);
      i++; j++;
    }
  }
  /*
   * With our sentinel in place, we now run the following hyper-fast
   * insertion sort.  For each remaining element, min, from [1] to [n-1],
   * set hi to the index of the element AFTER which this one goes.
   * Then, do the standard insertion sort shift on a character at a time
   * basis for each element in the frob.
   */
  for (min = base; (hi = min += qsz) < max; )
  {
    while (qcmp(hi -= qsz, min) > 0)
      /* any_t*/;
    if ((hi += qsz) != min) {
      for (lo = min + qsz; --lo >= min; )
      {
	c = *lo;
	for (i = j = lo; (j -= qsz) >= hi; i = j)
	  *i = *j;
	*i = c;
      }
    }
  }
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
} // namespace	usr
