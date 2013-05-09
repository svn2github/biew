#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr
 * @file        colorset.c
 * @brief       This file contains color part of BEYE project.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       2000
 * @note        Development, fixes and improvements
**/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "beye.h"
#include "colorset.h"
#include "bconsole.h"
#include "libbeye/file_ini.h"
#include "libbeye/osdep/tconsole.h"

namespace	usr {
extern const namedColorDef named_color_def[16] =
{
   { "Black",        Black        },
   { "Blue",         Blue         },
   { "Green",        Green        },
   { "Cyan",         Cyan         },
   { "Red",          Red          },
   { "Magenta",      Magenta      },
   { "Brown",        Brown        },
   { "LightGray",    LightGray    },
   { "Gray",         Gray         },
   { "LightBlue",    LightBlue    },
   { "LightGreen",   LightGreen   },
   { "LightCyan",    LightCyan    },
   { "LightRed",     LightRed     },
   { "LightMagenta", LightMagenta },
   { "Yellow",       Yellow       },
   { "White",        White        }
};

static Color __FASTCALL__ getColorByName(const std::string& name,Color defval,bool *has_err)
{
  unsigned i;
  for(i = 0;i < sizeof(named_color_def)/sizeof(namedColorDef);i++)
  {
    if(name==named_color_def[i].name)
    {
      *has_err = false;
      return named_color_def[i].color;
    }
  }
  beye_context().last_skin_error=name;
  *has_err = true;
  return defval;
}

static ColorAttr  __FASTCALL__ getColorPairByName(const std::string& name, ColorAttr defval,bool* has_err)
{
  char wstr[80];
  char *p;
  bool he;
  unsigned i,j,len;
  ColorAttr retval;
  Color fore,back;
  len = name.length();
  for(j = i = 0;i < len;i++) if(!isspace((unsigned char)name[i])) wstr[j++] = name[i];
  wstr[j] = 0;
  p = strchr(wstr,':');
  if(!p)
  {
    *has_err = true;
    beye_context().last_skin_error="':' is missing";
    return defval;
  }
  *p = 0;
  p++;
  back = getColorByName(wstr,BACK_COLOR(defval),has_err);
  fore = getColorByName(p,FORE_COLOR(defval),&he);
  retval = LOGFB_TO_PHYS(fore,back);
  if(he) *has_err = he;
  return retval;
}

static bool  __FASTCALL__ readColorPair(Ini_Profile& ini,const std::string& section,
				   const std::string& subsection,
				   const std::string& item,
				   ColorAttr *value)
{
  char cval[80];
  bool has_err;
  std::string cstr;
  sprintf(cval,"%s:%s"
	  ,named_color_def[BACK_COLOR(*value) & 0x0F].name
	  ,named_color_def[FORE_COLOR(*value) & 0x0F].name);
  cstr=ini.read(section,subsection,item,cval);
  *value = getColorPairByName(cstr,*value,&has_err);
  return has_err;
}

static bool  __FASTCALL__ readButton(Ini_Profile& ini,const std::string& section,
					      const std::string& subsection,
					      ButtonCSet *to)
{
  bool has_err;
  has_err = readColorPair(ini,section,subsection,"Active",&to->active);
  has_err |= readColorPair(ini,section,subsection,"Focused",&to->focused);
  has_err |= readColorPair(ini,section,subsection,"Disabled",&to->disabled);
  return has_err;
}

bool csetReadIniFile(const std::string& ini_name)
{
  Ini_Profile& cset = *new Ini_Profile;
  char cval[80],csec[80];
  std::string cstr,stmp;
  unsigned value,i,j;
  bool has_err,cur_err;
  has_err=cset.open(ini_name);
  beye_context().last_skin_error.clear();
  if(has_err) { delete &cset; return false; } /** return no error, because ini_name was not found or unavailable */
  stmp=cset.read("Skin info","","Name","Unnamed");
  beye_context().scheme_name=stmp;
  for(i = 0;i < 16;i++)
  {
    sprintf(cval,"%i",named_color_def[i].color);
    cstr=cset.read("Color map","",named_color_def[i].name,cval);
    value = atoi(cstr.c_str());
    TWindow::remap_color(named_color_def[i].color,value);
  }
  for(i = 0;i < 8;i++)
  {
    sprintf(cval,"Trans%i",i+1);
    cstr=cset.read("Terminal","",cval,"");
    if(!cstr.empty())
    {
      Color col;
      col = getColorByName(cstr,Black,&cur_err);
      if(cur_err) has_err = cur_err;
      else beye_context().tconsole().vio_set_transparent_color(TWindow::get_mapped_color(col));
    }
  }
  has_err |= readColorPair(cset,"Browser","","Main",&browser_cset.main);
  has_err |= readColorPair(cset,"Browser","","Bound",&browser_cset.bound);
  has_err |= readColorPair(cset,"Browser","","HighLight",&browser_cset.hlight);
  has_err |= readColorPair(cset,"Browser","","HighLightLine",&browser_cset.highline);
  has_err |= readColorPair(cset,"Browser","","Title",&browser_cset.title);
  has_err |= readColorPair(cset,"Browser","","Footer",&browser_cset.footer);
  has_err |= readColorPair(cset,"Browser","","Scroller",&browser_cset.scroller);
  has_err |= readColorPair(cset,"Browser","Edit","Main",&browser_cset.edit.main);
  has_err |= readColorPair(cset,"Browser","Edit","Change",&browser_cset.edit.change);
  has_err |= readColorPair(cset,"Browser","Edit","Selected",&browser_cset.edit.selected);
  has_err |= readColorPair(cset,"Prompt","","Button",&prompt_cset.button);
  has_err |= readColorPair(cset,"Prompt","","Digit",&prompt_cset.digit);
  has_err |= readColorPair(cset,"Prompt","","Control",&prompt_cset.control);
  has_err |= readColorPair(cset,"Title","","Main",&title_cset.main);
  has_err |= readColorPair(cset,"Title","","Change",&title_cset.change);
  has_err |= readColorPair(cset,"Text","","Normal",&text_cset.normal);
  has_err |= readColorPair(cset,"Text","","Bold",&text_cset.bold);
  has_err |= readColorPair(cset,"Text","","Italic",&text_cset.italic);
  has_err |= readColorPair(cset,"Text","","Underline",&text_cset.underline);
  has_err |= readColorPair(cset,"Text","","StrikeThrough",&text_cset.strikethrough);
  has_err |= readColorPair(cset,"Disasm","","FamilyId",&disasm_cset.family_id);
  has_err |= readColorPair(cset,"Disasm","","Opcodes",&disasm_cset.opcodes);
  has_err |= readColorPair(cset,"Disasm","","Opcodes0",&disasm_cset.opcodes0);
  has_err |= readColorPair(cset,"Disasm","","Opcodes1",&disasm_cset.opcodes1);
  has_err |= readColorPair(cset,"Disasm","","Opcodes2",&disasm_cset.opcodes2);
  has_err |= readColorPair(cset,"Disasm","","Opcodes3",&disasm_cset.opcodes3);
  has_err |= readColorPair(cset,"Disasm","","Opcodes4",&disasm_cset.opcodes4);
  has_err |= readColorPair(cset,"Disasm","","Opcodes5",&disasm_cset.opcodes5);
  has_err |= readColorPair(cset,"Disasm","","Comments",&disasm_cset.comments);
  for(j = 0;j < sizeof(disasm_cset.cpu_cset)/sizeof(CPUCSet);j++)
  {
    sprintf(csec,"Processor%i",j);
    for(i = 0;i < sizeof(disasm_cset.cpu_cset[0])/sizeof(ColorAttr);i++)
    {
      sprintf(cval,"Clone%i",i+1);
      has_err |= readColorPair(cset,"Disasm",csec,cval,&disasm_cset.cpu_cset[j].clone[i]);
    }
  }
  strcpy(csec,"Engine");
  for(j = 0;j < sizeof(disasm_cset.engine)/sizeof(EngineCSet);j++)
  {
    sprintf(cval,"Engine%i",j);
    has_err |= readColorPair(cset,"Disasm",csec,cval,&disasm_cset.engine[j].engine);
    sprintf(cval,"Engine%i_unopt",j);
    has_err |= readColorPair(cset,"Disasm",csec,cval,&disasm_cset.engine[j].engine_unopt);
  }
  has_err |= readColorPair(cset,"Programming","","BadExpr",&prog_cset.bads);
  has_err |= readColorPair(cset,"Programming","Comments","base",&prog_cset.comments.base);
  has_err |= readColorPair(cset,"Programming","Comments","extended",&prog_cset.comments.extended);
  has_err |= readColorPair(cset,"Programming","Comments","reserved",&prog_cset.comments.reserved);
  has_err |= readColorPair(cset,"Programming","Comments","alt",&prog_cset.comments.alt);
  has_err |= readColorPair(cset,"Programming","Keyword","base",&prog_cset.keywords.base);
  has_err |= readColorPair(cset,"Programming","Keyword","extended",&prog_cset.keywords.extended);
  has_err |= readColorPair(cset,"Programming","Keyword","reserved",&prog_cset.keywords.reserved);
  has_err |= readColorPair(cset,"Programming","Keyword","alt",&prog_cset.keywords.alt);
  has_err |= readColorPair(cset,"Programming","Constant","base",&prog_cset.constants.base);
  has_err |= readColorPair(cset,"Programming","Constant","extended",&prog_cset.constants.extended);
  has_err |= readColorPair(cset,"Programming","Constant","reserved",&prog_cset.constants.reserved);
  has_err |= readColorPair(cset,"Programming","Constant","alt",&prog_cset.constants.alt);
  has_err |= readColorPair(cset,"Programming","Operator","base",&prog_cset.operators.base);
  has_err |= readColorPair(cset,"Programming","Operator","extended",&prog_cset.operators.extended);
  has_err |= readColorPair(cset,"Programming","Operator","reserved",&prog_cset.operators.reserved);
  has_err |= readColorPair(cset,"Programming","Operator","alt",&prog_cset.operators.alt);
  has_err |= readColorPair(cset,"Programming","Preproc","base",&prog_cset.preproc.base);
  has_err |= readColorPair(cset,"Programming","Preproc","extended",&prog_cset.preproc.extended);
  has_err |= readColorPair(cset,"Programming","Preproc","reserved",&prog_cset.preproc.reserved);
  has_err |= readColorPair(cset,"Programming","Preproc","alt",&prog_cset.preproc.alt);
  has_err |= readColorPair(cset,"Error","","Main",&error_cset.main);
  has_err |= readColorPair(cset,"Error","","Border",&error_cset.border);
  has_err |= readButton(cset,"Error","Button",&error_cset.button);
  has_err |= readColorPair(cset,"Warning","","Main",&warn_cset.main);
  has_err |= readColorPair(cset,"Warning","","Border",&warn_cset.border);
  has_err |= readButton(cset,"Warning","Button",&warn_cset.button);
  has_err |= readColorPair(cset,"Notify","","Main",&notify_cset.main);
  has_err |= readColorPair(cset,"Notify","","Border",&notify_cset.border);
  has_err |= readButton(cset,"Notify","Button",&notify_cset.button);
  has_err |= readColorPair(cset,"Dialog","","Main",&dialog_cset.main);
  has_err |= readColorPair(cset,"Dialog","","Border",&dialog_cset.border);
  has_err |= readColorPair(cset,"Dialog","","Title",&dialog_cset.title);
  has_err |= readColorPair(cset,"Dialog","","Footer",&dialog_cset.footer);
  has_err |= readColorPair(cset,"Dialog","","SelFooter",&dialog_cset.selfooter);
  has_err |= readColorPair(cset,"Dialog","","Entry",&dialog_cset.entry);
  has_err |= readColorPair(cset,"Dialog","","AltEntry",&dialog_cset.altentry);
  has_err |= readColorPair(cset,"Dialog","","AddInfo",&dialog_cset.addinfo);
  has_err |= readColorPair(cset,"Dialog","","AltInfo",&dialog_cset.altinfo);
  has_err |= readColorPair(cset,"Dialog","","ExtraInfo",&dialog_cset.extrainfo);
  has_err |= readButton(cset,"Dialog","Group",&dialog_cset.group);
  has_err |= readButton(cset,"Dialog","Button",&dialog_cset.button);
  has_err |= readButton(cset,"Dialog","Any",&dialog_cset.any);
  has_err |= readColorPair(cset,"Dialog","Editor","Active",&dialog_cset.editor.active);
  has_err |= readColorPair(cset,"Dialog","Editor","Focused",&dialog_cset.editor.focused);
  has_err |= readColorPair(cset,"Dialog","Editor","Disabled",&dialog_cset.editor.disabled);
  has_err |= readColorPair(cset,"Dialog","Editor","Select",&dialog_cset.editor.select);
  has_err |= readColorPair(cset,"Menu","","Main",&menu_cset.main);
  has_err |= readColorPair(cset,"Menu","","Border",&menu_cset.border);
  has_err |= readColorPair(cset,"Menu","","Title",&menu_cset.title);
  has_err |= readColorPair(cset,"Menu","","HighLight",&menu_cset.highlight);
  has_err |= readButton(cset,"Menu","HotKey",&menu_cset.hotkey);
  has_err |= readButton(cset,"Menu","Item",&menu_cset.item);

  has_err |= readColorPair(cset,"Help","","Main",&help_cset.main);
  has_err |= readColorPair(cset,"Help","","Border",&help_cset.border);
  has_err |= readColorPair(cset,"Help","","Title",&help_cset.title);
  has_err |= readColorPair(cset,"Help","","Bold",&help_cset.bold);
  has_err |= readColorPair(cset,"Help","","Italic",&help_cset.italic);
  has_err |= readColorPair(cset,"Help","","Reverse",&help_cset.reverse);
  has_err |= readColorPair(cset,"Help","","Underline",&help_cset.underline);
  has_err |= readColorPair(cset,"Help","","StrikeThrough",&help_cset.strikethrough);
  has_err |= readColorPair(cset,"Help","","Link",&help_cset.link);
  has_err |= readColorPair(cset,"Help","","SelLink",&help_cset.sellink);
  delete &cset;
  return has_err;
}

BrowserCSet browser_cset =
{
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(Blue, Cyan),
  LOGFB_TO_PHYS(Blue, Green),
  LOGFB_TO_PHYS(Yellow, Blue),
  { LOGFB_TO_PHYS(White, Blue),
    LOGFB_TO_PHYS(LightRed, Blue),
    LOGFB_TO_PHYS(Blue, LightGray) }
};

PromptCSet prompt_cset =
{
  LOGFB_TO_PHYS(Black, Cyan),
  LOGFB_TO_PHYS(LightCyan, Black),
  LOGFB_TO_PHYS(LightGray, Black),
};

TitleCSet title_cset =
{
  LOGFB_TO_PHYS(Black, Cyan),
  LOGFB_TO_PHYS(Red, Cyan)
};

MessageCSet error_cset =
{
  LOGFB_TO_PHYS(Yellow, Red),
  LOGFB_TO_PHYS(LightRed, Red),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

MessageCSet notify_cset =
{
  LOGFB_TO_PHYS(White, Green),
  LOGFB_TO_PHYS(Yellow, Green),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

MessageCSet warn_cset =
{
  LOGFB_TO_PHYS(White, Magenta),
  LOGFB_TO_PHYS(LightMagenta, Magenta),
  { LOGFB_TO_PHYS(Yellow, Green),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

TextCSet text_cset =
{
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(LightGreen, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(LightBlue, Blue)
};

DisasmCSet disasm_cset =
{
  LOGFB_TO_PHYS(LightBlue, Blue),
  LOGFB_TO_PHYS(LightCyan, Blue),
  LOGFB_TO_PHYS(White, Blue),
  LOGFB_TO_PHYS(LightGreen, Blue),
  LOGFB_TO_PHYS(LightRed, Blue),
  LOGFB_TO_PHYS(LightMagenta, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(LightBlue, Blue),
  LOGFB_TO_PHYS(Yellow, Blue),
  {
   {
    {
     LOGFB_TO_PHYS(LightCyan, Blue),
     LOGFB_TO_PHYS(LightGray, Blue),
     LOGFB_TO_PHYS(Yellow, Blue),
     LOGFB_TO_PHYS(LightGreen, Blue),
     LOGFB_TO_PHYS(Red, Blue),
     LOGFB_TO_PHYS(LightRed, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(Cyan, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Brown, Blue),
     LOGFB_TO_PHYS(Green, Blue),
     LOGFB_TO_PHYS(Magenta, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(White, Blue),
     LOGFB_TO_PHYS(LightMagenta, Blue),
     LOGFB_TO_PHYS(LightBlue, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Gray, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   },
   {
    {
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue),
     LOGFB_TO_PHYS(Black, Blue)
    }
   }
  },
  {
    { LOGFB_TO_PHYS(LightCyan, Blue), LOGFB_TO_PHYS(LightGray, Blue) },
    { LOGFB_TO_PHYS(Cyan, Blue), LOGFB_TO_PHYS(Gray, Blue) },
    { LOGFB_TO_PHYS(LightBlue, Blue), LOGFB_TO_PHYS(Black, Blue) },
    { LOGFB_TO_PHYS(White, Blue), LOGFB_TO_PHYS(Yellow, Blue) },
    { LOGFB_TO_PHYS(LightGreen, Blue), LOGFB_TO_PHYS(Green, Blue) },
    { LOGFB_TO_PHYS(LightMagenta, Blue), LOGFB_TO_PHYS(Magenta, Blue) },
    { LOGFB_TO_PHYS(LightRed, Blue), LOGFB_TO_PHYS(Red, Blue) }
  }
};

ProgCSet prog_cset =
{
  LOGFB_TO_PHYS(Black, Red),
  { LOGFB_TO_PHYS(LightGray, Blue),
    LOGFB_TO_PHYS(Gray, Blue),
    LOGFB_TO_PHYS(Brown, Blue),
    LOGFB_TO_PHYS(Black, Blue) },
  { LOGFB_TO_PHYS(Yellow, Blue),
    LOGFB_TO_PHYS(LightMagenta, Blue),
    LOGFB_TO_PHYS(LightRed, Blue),
    LOGFB_TO_PHYS(White, Blue) },
  { LOGFB_TO_PHYS(LightGreen, Blue),
    LOGFB_TO_PHYS(LightBlue, Blue),
    LOGFB_TO_PHYS(Green, Blue),
    LOGFB_TO_PHYS(LightMagenta, Blue) },
  { LOGFB_TO_PHYS(White, Blue),
    LOGFB_TO_PHYS(LightBlue, Blue),
    LOGFB_TO_PHYS(LightGreen, Blue),
    LOGFB_TO_PHYS(LightRed, Blue) },
  { LOGFB_TO_PHYS(LightRed, Blue),
    LOGFB_TO_PHYS(LightMagenta, Blue),
    LOGFB_TO_PHYS(Black, Blue),
    LOGFB_TO_PHYS(Brown, Blue) }
};

DialogCSet dialog_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(White, Black),
  LOGFB_TO_PHYS(White, Black),
  LOGFB_TO_PHYS(Yellow, Blue),
  LOGFB_TO_PHYS(Yellow, LightGray),
  LOGFB_TO_PHYS(LightCyan, LightGray),
  LOGFB_TO_PHYS(Red, LightGray),
  { LOGFB_TO_PHYS(Black, Cyan),
    LOGFB_TO_PHYS(Black, Cyan),
    LOGFB_TO_PHYS(Gray, Cyan) },
  { LOGFB_TO_PHYS(Yellow, Blue),
    LOGFB_TO_PHYS(White, Green),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(White, Cyan),
    LOGFB_TO_PHYS(Gray, Cyan),
    LOGFB_TO_PHYS(Cyan, LightGray) }
};

MenuCSet menu_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, Green),
  { LOGFB_TO_PHYS(Red, LightGray),
    LOGFB_TO_PHYS(Red, Black),
    LOGFB_TO_PHYS(Gray, LightGray) },
  { LOGFB_TO_PHYS(Black, LightGray),
    LOGFB_TO_PHYS(White, Black),
    LOGFB_TO_PHYS(Gray, LightGray) }
};

HelpCSet help_cset =
{
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Black, LightGray),
  LOGFB_TO_PHYS(Yellow, LightGray),
  LOGFB_TO_PHYS(Blue, LightGray),
  LOGFB_TO_PHYS(Magenta, LightGray),
  LOGFB_TO_PHYS(Red, LightGray),
  LOGFB_TO_PHYS(Gray, LightGray),
  LOGFB_TO_PHYS(Yellow, LightCyan),
  LOGFB_TO_PHYS(White, Black)
};
} // namespace	usr

