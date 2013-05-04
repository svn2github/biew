#include "config.h"
#include "libbeye/libbeye.h"
using namespace beye;
/**
 * @namespace   beye
 * @file        beyeutil.c
 * @brief       This file contains useful primitives of BEYE project.
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>

#include "bmfile.h"
#include "beyeutil.h"
#include "bconsole.h"
#include "tstrings.h"
#include "libbeye/libbeye.h"

namespace beye {
bool DumpMode = false;
bool EditMode = false;

int __FASTCALL__ Gebool(bool _bool) { return _bool ? TWC_CHECK_CHAR : TWC_DEF_FILLER; }

void FFreeArr(any_t** arr,unsigned n)
{
  unsigned i;
  for(i = 0;i < n;i++) delete arr[i];
}

unsigned __FASTCALL__ Summ(unsigned char *str,unsigned size)
{
  unsigned res,i;
  res = 0;
  for(i = 0;i < size;i++) res += str[i];
  return res;
}

char * __FASTCALL__ GetBinary(char val)
{
  static char bstr[9];
  int i;
  bstr[8] = 0;
  for(i = 0;i < 8;i++) bstr[7-i] = ((val >> i) & 1) + '0';
  return bstr;
}

#define GET2DIGIT(str,legs,val)\
{\
  char *s = (char *)str;\
  s[0] = legs[(((unsigned char)val) >> 4) & 0x0F];\
  s[1] = legs[((unsigned char)val) & 0x0F];\
}

char * __FASTCALL__ Get2Digit(uint8_t val)
{
  static char str[3] = "  ";
  const char *legs = &legalchars[2];
  GET2DIGIT(str,legs,val);
  return str;
}

char * __FASTCALL__ Get2SignDig(int8_t val)
{
  static char str[4] = "   ";
  const char *legs = &legalchars[2];
  str[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = abs(val);
  GET2DIGIT(&str[1],legs,val);
  return str;
}

char * __FASTCALL__ Get4Digit(uint16_t val)
{
  static char rstr[5] = "    ";
  const char *legs = &legalchars[2];
  unsigned char v;
  v = val>>8;
  GET2DIGIT(rstr,legs,v);
  GET2DIGIT(&rstr[2],legs,val);
  return rstr;
}

char * __FASTCALL__ Get4SignDig(int16_t val)
{
  static char rstr[6] = "     ";
  const char *legs = &legalchars[2];
  unsigned char v;
  rstr[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = abs(val);
  v = val>>8;
  GET2DIGIT(&rstr[1],legs,v);
  GET2DIGIT(&rstr[3],legs,val);
  return rstr;
}

char * __FASTCALL__ Get8Digit(uint32_t val)
{
  static char rstr[9] = "        ";
  const char *legs = &legalchars[2];
  unsigned char v;
  v = val>>24;
  GET2DIGIT(rstr,legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[2],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[4],legs,v);
  GET2DIGIT(&rstr[6],legs,val);
  return rstr;
}

char * __FASTCALL__ Get8SignDig(int32_t val)
{
  static char rstr[10] = "         ";
  const char *legs = &legalchars[2];
  unsigned char v;
  rstr[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = labs(val);
  v = val>>24;
  GET2DIGIT(&rstr[1],legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[3],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[5],legs,v);
  GET2DIGIT(&rstr[7],legs,val);
  return rstr;
}

char * __FASTCALL__ Get16Digit(uint64_t val)
{
  static char rstr[17] = "                ";
  const char *legs = &legalchars[2];
  unsigned char v;
  v = val>>56;
  GET2DIGIT(rstr,legs,v);
  v = val>>48;
  GET2DIGIT(&rstr[2],legs,v);
  v = val>>40;
  GET2DIGIT(&rstr[4],legs,v);
  v = val>>32;
  GET2DIGIT(&rstr[6],legs,v);
  v = val>>24;
  GET2DIGIT(&rstr[8],legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[10],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[12],legs,v);
  GET2DIGIT(&rstr[14],legs,val);
  return rstr;
}

char * __FASTCALL__ Get16SignDig(int64_t val)
{
  static char rstr[18] = "                 ";
  const char *legs = &legalchars[2];
  unsigned char v;
  rstr[0] = val >= 0 ? '+' : '-';
  if(val < 0) val = -val;
  v = val>>56;
  GET2DIGIT(&rstr[1],legs,v);
  v = val>>48;
  GET2DIGIT(&rstr[3],legs,v);
  v = val>>40;
  GET2DIGIT(&rstr[5],legs,v);
  v = val>>32;
  GET2DIGIT(&rstr[7],legs,v);
  v = val>>24;
  GET2DIGIT(&rstr[9],legs,v);
  v = val>>16;
  GET2DIGIT(&rstr[11],legs,v);
  v = val>>8;
  GET2DIGIT(&rstr[13],legs,v);
  GET2DIGIT(&rstr[15],legs,val);
  return rstr;
}

static char  __FASTCALL__ GetHexAnalog(char val)
{
  return val >= '0' && val <= '9' ? val-'0' : ((toupper(val)-'A'+10)) & 0x0F;
}

void __FASTCALL__ CompressHex(unsigned char * dest,const char * src,unsigned sizedest,bool usespace)
{
  unsigned i,j;
  for(i = j = 0;j < sizedest;j++)
  {
      dest[j] = (GetHexAnalog(src[i]) << 4) | GetHexAnalog(src[i + 1]);
      i += 2;	/* [dBorca] avoid ambiguous side-effects */
      if(usespace) i++;
  }
}

memArray * __FASTCALL__ ma_Build( int nitems, bool interact )
{
  memArray * ret;
  ret = new memArray;
  if(ret)
  {
    memset(ret,0,sizeof(memArray));
    if(nitems)
    {
      ret->data = (any_t**)new char*[nitems];
      if(ret->data)
      {
	ret->nSize = nitems;
      }
    }
  }
  else
  {
    if(interact) MemOutBox("List creation");
  }
  return ret;
}

void  __FASTCALL__ ma_Destroy(memArray *obj)
{
  unsigned i;
  for(i = 0;i < obj->nItems;i++)
  {
    delete obj->data[i];
  }
  delete obj->data;
  delete obj;
}

static const unsigned LST_STEP=16;

bool  __FASTCALL__ ma_AddData(memArray *obj,const any_t*udata,unsigned len,bool interact)
{
  char *new_item;
  if(obj->nSize > UINT_MAX - (LST_STEP+1)) return 0;
  if(obj->nItems + 1 > obj->nSize)
  {
    any_t*ptr;
    if(!obj->data) ptr = new char*[obj->nSize+LST_STEP];
    else           ptr = mp_realloc(obj->data,sizeof(char *)*(obj->nSize+LST_STEP));
    if(ptr)
    {
      obj->nSize = obj->nSize+LST_STEP;
      obj->data = (any_t**)ptr;
    }
    else goto err;
  }
  new_item = new char [len];
  if(new_item)
  {
    memcpy(new_item,udata,len);
    obj->data[obj->nItems++] = new_item;
    return true;
  }
  else
  {
    err:
    if(interact)
    {
      MemOutBox("Building list");
    }
  }
  return false;
}

bool __FASTCALL__ ma_AddString(memArray *obj,const std::string& udata,bool interact)
{
  return ma_AddData(obj,udata.c_str(),udata.size()+1,interact);
}

int __FASTCALL__ ma_Display(memArray *obj,const std::string& title,int flg, unsigned defsel)
{
  return CommonListBox((char**)obj->data,obj->nItems,title,flg,defsel);
}
} // namespace beye
