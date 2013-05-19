#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        beyehelp.c
 * @brief       This file contains LZSS-based help system functions.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "colorset.h"
#include "bconsole.h"
#include "beyehelp.h"
#include "search.h"
#include "setup.h"
#include "tstrings.h"
#include "libbeye/osdep/tconsole.h"
#include "libbeye/osdep/system.h"
#include "libbeye/kbd_code.h"
#include "beye.h"
#include "libbeye/bstream.h"

#include "lzss/lzssutil.cpp"

namespace	usr {
static const unsigned TEXT_TAB=8;

extern void drawHelpListPrompt();

enum {
    HPROP_BOLD                  =0x01,
    HPROP_ITALIC                =0x02,
    HPROP_UNDERLINE             =0x04,
    HPROP_STRIKETHROUGH         =0x08,
    HPROP_REVERSE               =0x10,
    HPROP_LINK                  =0x20
};
unsigned Beye_Help::fill_buffer(tvioBuff * dest,unsigned int alen,const std::string& str,unsigned int len,unsigned int shift,unsigned *n_tabs,bool is_hl) const
{
  t_vchar ch;
  ColorAttr defcol;
  unsigned char text_prop;
  int size,j;
  size_t i,k,freq;
  if(n_tabs) *n_tabs = 0;
  text_prop = 0;
  defcol = is_hl ? menu_cset.highlight : help_cset.main;
  for(i = 0,freq = 0,k = 0;i < len;i++,freq++)
  {
    ch = str[i];
    if(iscntrl(ch))
    {
      switch(ch)
      {
	 case HLPC_BOLD_ON: text_prop |= HPROP_BOLD; break;
	 case HLPC_ITALIC_ON: text_prop |= HPROP_ITALIC; break;
	 case HLPC_UNDERLINE_ON: text_prop |= HPROP_UNDERLINE; break;
	 case HLPC_STRIKETHROUGH_ON: text_prop |= HPROP_STRIKETHROUGH; break;
	 case HLPC_REVERSE_ON: text_prop |= HPROP_REVERSE; break;
	 case HLPC_LINK_ON: text_prop |= HPROP_LINK; break;
	 case HLPC_BOLD_OFF: text_prop &= ~HPROP_BOLD; break;
	 case HLPC_ITALIC_OFF: text_prop &= ~HPROP_ITALIC; break;
	 case HLPC_UNDERLINE_OFF: text_prop &= ~HPROP_UNDERLINE; break;
	 case HLPC_STRIKETHROUGH_OFF: text_prop &= ~HPROP_STRIKETHROUGH; break;
	 case HLPC_REVERSE_OFF: text_prop &= ~HPROP_REVERSE; break;
	 case HLPC_LINK_OFF: text_prop &= ~HPROP_LINK; break;
	 case '\t': /** HT   horizontal tab*/
	       {
		  size = TEXT_TAB - (freq%TEXT_TAB);
		  for(j = 0;j < size;j++,freq++)
		  {
		    if(k < alen && freq >= shift)
		    {
		      if(dest)
		      {
			dest->chars[k] = TWC_DEF_FILLER;
			dest->attrs[k] = defcol;
		      }
		      k++;
		    }
		  }
		  if(n_tabs) (*n_tabs)++;
	       }
	       freq--;
	       break;
	 case '\n':
	 case '\r':
		  goto End;
      }
      if(text_prop & HPROP_BOLD)  defcol = help_cset.bold;
      else
      if(text_prop & HPROP_ITALIC)  defcol = help_cset.italic;
      else
      if(text_prop & HPROP_UNDERLINE)  defcol = help_cset.underline;
      else
      if(text_prop & HPROP_STRIKETHROUGH)  defcol = help_cset.strikethrough;
      else
      if(text_prop & HPROP_REVERSE)  defcol = help_cset.reverse;
      else
      if(text_prop & HPROP_LINK)  defcol = help_cset.link;
      else                        defcol = help_cset.main;
    }
    else
    {
      if(k < alen && freq >= shift)
      {
	if(dest)
	{
	  dest->chars[k] = ch;
	  dest->attrs[k] = defcol;
	}
	k++;
      }
    }
  }
  End:
  if(dest) beye_context().system().nls_prepare_oem_for_vio(dest,k);
  return k;
}

void Beye_Help::paint_line(TWindow *win,unsigned i,const std::string& name,bool is_hl) const
{
  tvioBuff it;
  unsigned rlen;
  t_vchar chars[__TVIO_MAXSCREENWIDTH];
  t_vchar oem_pg[__TVIO_MAXSCREENWIDTH];
  ColorAttr attrs[__TVIO_MAXSCREENWIDTH];
  it.chars = chars;
  it.oem_pg = oem_pg;
  it.attrs = attrs;
  rlen = name.length();
  rlen = fill_buffer(&it,__TVIO_MAXSCREENWIDTH,name,rlen,0,NULL,is_hl);
  win->write(4,i+2,&it,rlen);
  win->goto_xy(3+rlen,i+1);
  win->clreol();
}

void Beye_Help::paint(TWindow *win,const char * * names,unsigned nlist,unsigned start,unsigned height,unsigned width) const
{
 unsigned i, pos = 0;
 win->freeze();
 width -= 3;
 if (height>2 && height<nlist)
     pos = 1 + start*(height-2)/nlist;
 win->set_color(help_cset.main);
 for(i = 0;i < height;i++)
 {
   win->goto_xy(1,i + 1);
   if (i == 0)
       win->putch(start ? TWC_UP_ARROW : TWC_DEF_FILLER);
   else if(i == height-1)
       win->putch(start + height < nlist ? TWC_DN_ARROW : TWC_DEF_FILLER);
   else if (i == pos)
       win->putch(TWC_THUMB);
   else win->putch(TWC_DEF_FILLER);
  win->goto_xy(2,i + 1);
   win->putch(TWC_SV);
   paint_line(win,i,names[i + start],0);
 }
 win->refresh();
}

typedef char *lpstr;

int Beye_Help::ListBox(const char** names,unsigned nlist,const std::string& title) const
{
 TWindow * wlist;
 unsigned i,j,height,mwidth = title.length();
 int ret,start,ostart,cursor,ocursor,scursor;
 scursor = -1;
 for(i = 0;i < nlist;i++)
 {
    j = fill_buffer(NULL,UINT_MAX,names[i],strlen(names[i]),0,NULL,0);
    if(j > mwidth) mwidth = j;
 }
 mwidth += 4;
 if(mwidth > (unsigned)(beye_context().tconsole().vio_width()-2)) mwidth = beye_context().tconsole().vio_width()-2;
 height = nlist < (unsigned)(beye_context().tconsole().vio_height() - 4) ? nlist : beye_context().tconsole().vio_height() - 4;
 wlist = CrtHlpWndnls(title,mwidth,height+1);
 ostart = start = cursor = ocursor = 0;
 paint(wlist,names,nlist,start,height,mwidth);
 for(;;)
 {
   unsigned ch;
   ch = GetEvent(drawHelpListPrompt,NULL,wlist);
   if(ch == KE_ESCAPE || ch == KE_F(10)) { ret = -1; break; }
   if(ch == KE_ENTER)                    { ret = start + cursor; break; }
   if(ch!=KE_F(7) && ch!=KE_SHIFT_F(7))  scursor = -1;
   switch(ch)
   {
     case KE_F(7): /** perform binary search in help */
     case KE_SHIFT_F(7):
	     {
	       static char searchtxt[21] = "";
	       static unsigned char searchlen = 0;
	       static unsigned sflg = SF_NONE;

	       if (!(ch==KE_SHIFT_F(7) && searchlen) &&
		   !SearchDialog(SD_SIMPLE,searchtxt,&searchlen,&sflg))
		   break;

	       {
		  int direct,ii;
		  bool found;
		  int endsearch,startsearch,cache[UCHAR_MAX];
		  searchtxt[searchlen] = 0;
		  endsearch = sflg & SF_REVERSE ? -1 : (int)nlist;
		  direct = sflg & SF_REVERSE ? -1 : 1;
		  startsearch =    scursor != -1 ?
				   scursor :
				   start;
		  if(startsearch > (int)(nlist-1)) startsearch = nlist-1;
		  if(startsearch < 0) startsearch = 0;
		  if(scursor != -1)
		  {
		    sflg & SF_REVERSE ? startsearch-- : startsearch++;
		  }
		  found = false;
		  fillBoyerMooreCache(cache,searchtxt,searchlen, sflg & SF_CASESENS);
		  for(ii = startsearch;ii != endsearch;ii+=direct)
		  {
		     if(_lb_searchtext(names[ii],searchtxt,searchlen,cache,sflg))
		     {
			start = scursor = ii;
			if((unsigned)start > nlist - height) start = nlist - height;
			ostart = start - 1;
			found = true;
			break;
		     }
		  }
		  if(!found) scursor = -1;
		  if(scursor == -1) beye_context().ErrMessageBox(STR_NOT_FOUND,SEARCH_MSG);
	       }
	     }
	     break;
     case KE_DOWNARROW : start ++; break;
     case KE_UPARROW   : start--; break;
     case KE_PGDN   : start += height; break;
     case KE_PGUP   : start -= height; break;
     case KE_CTL_PGDN  : start = nlist - height; cursor = height; break;
     case KE_CTL_PGUP  : start = cursor = 0; break;
     default : break;
   }
   if(start < 0) start = 0;
   if((unsigned)start > nlist - height) start = nlist - height;
   if(start != ostart)
   {
     ostart = start;
     paint(wlist,names,nlist,start,height,mwidth);
   }
   if(scursor >= 0)
   {
     wlist->set_color(menu_cset.highlight);
     if(scursor >= start && (unsigned)scursor < start + height) paint_line(wlist,scursor - start,names[scursor],true);
   }
 }
 delete wlist;
 return ret;
}

bool Beye_Help::open( bool interact )
{
  char hlp_id[sizeof(BEYE_HELP_VER)];
  if(fs.is_open()) return false; /*means: help file is already opened */
  std::string help_name = beyeGetHelpName();
  fs.open(help_name.c_str(),std::ios_base::binary|std::ios_base::in);
  if(!fs.is_open()) {
    if(interact) beye_context().errnoMessageBox("Can't open help file","",errno);
    return false;
  }
  fs.seekg(0L,std::ios_base::beg);
  fs.read(hlp_id,sizeof(hlp_id));
  if(memcmp(hlp_id,BEYE_HELP_VER,strlen(BEYE_HELP_VER)) != 0) {
    if(interact) {
      beye_context().ErrMessageBox("Incorrect help version","");
    }
    fs.close();
    return false;
  }
  return true;
}

void Beye_Help::close()
{
    fs.close();
}

bool Beye_Help::find_item(unsigned long item_id)
{
  unsigned long i,nsize,lval;
  char sout[HLP_SLONG_LEN];
  fs.seekg(HLP_VER_LEN,std::ios_base::beg);
  fs.read(sout,sizeof(sout));
  nsize = strtoul(sout,NULL,16);
  for(i = 0;i < nsize;i++)
  {
    fs.read((char*)&bhi,sizeof(beye_help_item));
    lval = strtoul(bhi.item_id,NULL,16);
    if(lval == item_id) return true;
  }
  return false;
}

unsigned long Beye_Help::get_item_size(unsigned long item_id)
{
  unsigned long ret = 0;
  if(find_item(item_id)) ret = strtoul(bhi.item_decomp_size,NULL,16);
  else                   beye_context().ErrMessageBox("Find: Item not found","");
  return ret;
}

bool Beye_Help::load_item(unsigned long item_id, any_t* buffer)
{
    unsigned long hlp_off,hlp_size;
    bool ret = false;
    if(fs.is_open()) {
	if(find_item(item_id)) {
	    hlp_off = strtoul(bhi.item_off,NULL,16);
	    hlp_size = strtoul(bhi.item_length,NULL,16);
	    uint8_t* inbuff = new uint8_t[hlp_size];
	    fs.seekg(hlp_off,std::ios_base::beg);
	    fs.read((char*)inbuff,hlp_size);
	    if(!Decode(buffer,inbuff,hlp_size)) MemOutBox("Help uncompression");
	    else ret = true;
	    delete inbuff;
	}
	else beye_context().ErrMessageBox("Load: Item not found","");
    }
    return ret;
}

char** Beye_Help::point_strings(char* data,unsigned long data_size,unsigned long *nstr) const
{
  char **str_ptr,**new_ptr;
  unsigned long i;
  char ch,ch1;
  *nstr = 0;
  if((str_ptr = new char*) != NULL)
  {
     for(i = 0;i < data_size;i++)
     {
       ch = data[i];
       if(ch == '\n' || ch == '\r')
       {
	 data[i] = 0;
	 if(!(new_ptr = (char**)mp_realloc(str_ptr,((unsigned)(*nstr)+1)*sizeof(char *)))) goto mem_off;
	 str_ptr = new_ptr;
	 ch1 = data[i+1];
	 if((ch1 == '\n' || ch1 == '\r') && ch != ch1) ++i;
	 str_ptr[(*nstr)++] = &data[i+1];
       }
       if(*nstr > UINT_MAX-2) { mem_off: delete str_ptr; return NULL; }
     }
  }
  return str_ptr;
}

Beye_Help::Beye_Help() {}
Beye_Help::~Beye_Help() {}

void __FASTCALL__ hlpDisplay( unsigned long item_id )
{
  const char **str_ptr = 0;
  char  *data = 0;
  char *title;
  unsigned long data_size,nstr;
  Beye_Help bhelp;
  if(!bhelp.open(true)) return;
  data_size = bhelp.get_item_size(item_id);
  if(!data_size) goto hlp_bye;
  data = new char [data_size+1];
  if(!data)
  {
    mem_off:
    MemOutBox("Loading help");
    if(data) delete data;
    goto hlp_bye;
  }
  data[data_size] = 0;
  if(bhelp.load_item(item_id,data))
  {
    if(!(str_ptr = (const char**)bhelp.point_strings(data,data_size,&nstr))) goto mem_off;
    title = data;
    bhelp.ListBox(str_ptr,(unsigned)nstr,title);
    delete str_ptr;
  }
  delete data;
  hlp_bye:
  bhelp.close();
}
} // namespace	usr

