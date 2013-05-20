#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_II
 * @file        plugins/disasm/ix86/ix86_fpu.c
 * @brief       This file contains implementation of Intel x86 disassembler for
 *              FPU instructions set.
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
#include <stdlib.h>
#include <string.h>

#include "beyeutil.h"
#include "reg_form.h"
#include "plugins/disasm/ix86/ix86.h"

namespace	usr {
extern char *ix86_appstr;

char*  ix86_Disassembler::SetNameTab(char * str,const char * name) const
{
 strcpy(str,name);
 TabSpace(str,TAB_POS);
 return str;
}

char*  ix86_Disassembler::SC(const char * name1,const char * name2) const
{
 SetNameTab(ix86_appstr,name1);
 strcat(ix86_appstr,name2);
 return ix86_appstr;
}

char*  ix86_Disassembler::SetNameTabD(char * str,const char * name,unsigned char size,ix86Param& DisP) const
{
 strcpy(str,name);
 strcat(str," ");
 if(size < 7 && ((DisP.RealCmd[1] >> 6) & 0x03) != 3) strcat(str,ix86_sizes[size]);
 TabSpace(str,TAB_POS);
 return str;
}

static char stx[] = "st(x)";
#define MakeST(str,num) { stx[3] = (num)+'0'; strcat(str,stx); }

char*  ix86_Disassembler::__UniFPUfunc(char * str,const char * cmd,char opsize,char direct,ix86Param& DisP) const
{
 char mod = ( DisP.RealCmd[1] & 0xC0 ) >> 6;
 char reg = DisP.RealCmd[1] & 0x07;
 char *modrm;
 modrm = ix86_getModRM(true,mod,reg,DisP);
 SetNameTabD(str,cmd,opsize,DisP);
 if(!direct)  MakeST(str,0)
 else         strcat(str,modrm);
 strcat(str,",");
 if(direct)  MakeST(str,0)
 else        strcat(str,modrm);
 return str;
}

char*  ix86_Disassembler::__MemFPUfunc(char * str,const char * cmd,char opsize,ix86Param& DisP) const
{
 char mod = ( DisP.RealCmd[1] & 0xC0 ) >> 6;
 char reg = DisP.RealCmd[1] & 0x07;
 char *modrm;
 modrm = ix86_getModRM(true,mod,reg,DisP);
 SetNameTabD(str,cmd,opsize,DisP);
 strcat(str,modrm);
 return str;
}

char*  ix86_Disassembler::FPUmem(char * str,const char * cmd,ix86Param& DisP) const
{
  return __MemFPUfunc(str,cmd,DUMMY_PTR,DisP);
}

char*  ix86_Disassembler::FPUmem64mem32(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DisP.RealCmd[0] & 0x04 ? QWORD_PTR : DWORD_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUmem64mem32st(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DisP.RealCmd[0] & 0x04 ? QWORD_PTR : DWORD_PTR,1,DisP);
}

char*  ix86_Disassembler::FPUint16int32(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DisP.RealCmd[0] & 0x04 ? WORD_PTR : DWORD_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUint16int32st(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DisP.RealCmd[0] & 0x04 ? WORD_PTR : DWORD_PTR,1,DisP);
}

char*  ix86_Disassembler::FPUint64(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,QWORD_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUint64st(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,QWORD_PTR,1,DisP);
}

char*  ix86_Disassembler::FPUstint32(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DWORD_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUld(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,DUMMY_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUstisti(char * str,const char * cmd,char code1,char code2) const
{
 SetNameTab(str,cmd);
 MakeST(str,code1 & 0x07);
 strcat(str,",");
 MakeST(str,code2 & 0x07);
 return str;
}

char*  ix86_Disassembler::FPUst0sti(char * str,const char * cmd,char code1) const
{
 return FPUstisti(str,cmd,0,code1);
}

char*  ix86_Disassembler::FPUstist0(char * str,const char * cmd,char code1) const
{
 return FPUstisti(str,cmd,code1,0);
}

char*  ix86_Disassembler::FPUldtword(char * str,const char * cmd,ix86Param& DisP) const
{
 return __UniFPUfunc(str,cmd,TWORD_PTR,0,DisP);
}

char*  ix86_Disassembler::FPUsttword(char * str,const char * cmd,ix86Param& DisP) const
{
  return __UniFPUfunc(str,cmd,TWORD_PTR,1,DisP);
}

char*  ix86_Disassembler::FPUcmdsti(char * str,const char * name,char code) const
{
  SetNameTab(str,name);
  MakeST(str,code & 0x07);
  return str;
}

char*  ix86_Disassembler::FPUcmdst0(char * str,const char * name) const
{
  return FPUcmdsti(str,name,0);
}

char*  ix86_Disassembler::FPUcmdsti_2(char * str,const char * name1,const char * name2,char code) const
{
  return FPUcmdsti(str,code & 0x08 ? name2 : name1,code);
}

char*  ix86_Disassembler::FPUst0sti_2(char * str,const char * name1,const char * name2,char code) const
{
 return FPUst0sti(str,code & 0x08 ? name2 : name1,code);
}

char*  ix86_Disassembler::FPUstist0_2(char * str,const char * name1,const char * name2,char code) const
{
 return FPUstist0(str,code & 0x08 ? name2 : name1,code);
}

const char* ix86_Disassembler::mem64mem32[] =
{
  "fadd", "fmul", "fcom", "fcomp", "fsub", "fsubr", "fdiv", "fdivr"
};

const char* ix86_Disassembler::int16int32[] =
{
 "fiadd", "fimul", "ficom", "ficomp", "fisub", "fisubr", "fidiv", "fidivr"
};

const char* ix86_Disassembler::DBEx[] = { "feni", "fdisi", "fclex", "finit", "fsetpm" };
const char* ix86_Disassembler::D9Ex[] = { "fchs", "fabs", "f???", "f???", "ftst", "fxam", "f???", "f???",
			"fld1", "fldl2t", "fldl2e", "fldpi", "fldlg2", "fldln2", "fldz", "f???" };
const char* ix86_Disassembler::D9Fx[] = { "f2xm1", "fyl2x", "fptan", "fpatan", "fxtract", "fprem1", "fdecstp", "fincstp",
			"fprem", "fyl2xp1", "fsqrt", "fsincos", "frndint", "fscale", "fsin", "fcos" };

const FPUcall ix86_Disassembler::D9rm[8] =
{
  { &ix86_Disassembler::FPUstint32,      "fld" },
  { &ix86_Disassembler::FPUld,           "f???" },
  { &ix86_Disassembler::FPUmem64mem32st, "fst" },
  { &ix86_Disassembler::FPUmem64mem32st, "fstp" },
  { &ix86_Disassembler::FPUmem,          "fldenv" },
  { &ix86_Disassembler::FPUmem,          "fldcw" },
  { &ix86_Disassembler::FPUmem,          "fstenv" },
  { &ix86_Disassembler::FPUmem,          "fstcw" }
};

const FPUcall ix86_Disassembler::DBrm[8] =
{
  { &ix86_Disassembler::FPUint16int32,   "fild" },
  { &ix86_Disassembler::FPUld,           "fistpp" },
  { &ix86_Disassembler::FPUint16int32st, "fist" },
  { &ix86_Disassembler::FPUsttword,      "fistp" },
  { &ix86_Disassembler::FPUld,           "f???" },
  { &ix86_Disassembler::FPUldtword,      "fld" },
  { &ix86_Disassembler::FPUld,           "f???" },
  { &ix86_Disassembler::FPUint64st,      "fstp" }
};

const FPUcall ix86_Disassembler::DDrm[8] =
{
  { &ix86_Disassembler::FPUint64,        "fld" },
  { &ix86_Disassembler::FPUld,           "fistpp" },
  { &ix86_Disassembler::FPUmem64mem32st, "fst" },
  { &ix86_Disassembler::FPUmem64mem32st, "fstp" },
  { &ix86_Disassembler::FPUld,           "frstor" },
  { &ix86_Disassembler::FPUld,           "f???" },
  { &ix86_Disassembler::FPUld,           "fsave" },
  { &ix86_Disassembler::FPUmem,          "fstsw" }
};

const FPUcall ix86_Disassembler::DFrm[8] =
{
  { &ix86_Disassembler::FPUint16int32,   "fild" },
  { &ix86_Disassembler::FPUld,           "fistpp" },
  { &ix86_Disassembler::FPUint16int32st, "fist" },
  { &ix86_Disassembler::FPUint16int32st, "fistp" },
  { &ix86_Disassembler::FPUldtword,      "fbld" },
  { &ix86_Disassembler::FPUint64,        "fild" },
  { &ix86_Disassembler::FPUsttword,      "fbstp" },
  { &ix86_Disassembler::FPUint64st,      "fistp" }
};

const DualStr ix86_Disassembler::D8str[4] =
{
  { "fadd" , "fmul" },
  { "fcom" , "fcomp" },
  { "fsub" , "fsubr" },
  { "fdiv" , "fdivr" }
};

const DualStr ix86_Disassembler::DEstr[4] =
{
  { "faddp" , "fmulp" },
  { "fcomp" , "fcompp" },
  { "fsubrp", "fsubp" },
  { "fdivrp", "fdivp" }
};

const char* ix86_Disassembler::FCMOVc[] = { "fcmovl", "fcmove", "fcmovle", "fcmovu", "fcmov?", "fcmov?", "fcmov?", "fcmov?" };
const char* ix86_Disassembler::FCMOVnc[] = { "fcmovge", "fcmovne", "fcmovg", "fcmovnu", "fcmov?", "fucomi", "fcomi", "f?comi" };
const char* ix86_Disassembler::FxCOMIP[] = { "f???", "f???", "f???", "f???", "f???", "fucomip", "fcomip", "f???" };

void ix86_Disassembler::ix86_FPUCmd(char * str,ix86Param& DisP) const
{
 unsigned char code = DisP.RealCmd[0],code1 = DisP.RealCmd[1];
 unsigned char rm = ( code1 & 0x38 ) >> 3;
 FPUroutine mtd;
 DisP.codelen = 2;
 DisP.pro_clone |= INSN_FPU;
 SetNameTab(str,"f???");
 switch(code)
 {
   case 0xD8 :
	    if((code1 & 0xF0) >= 0xC0)
	    {
	      unsigned char _index = (code1 & 0x30) >> 4;
	      FPUst0sti_2(str,D8str[_index].c1,D8str[_index].c2,code1);
	    }
	    else     FPUmem64mem32(str,mem64mem32[rm],DisP);
	    break;
   case 0xD9 :
	    if((code1 & 0xF0) == 0xE0) FPUcmdst0(str,D9Ex[code1 & 0x0F]);
	    else
	      if((code1 & 0xF0) == 0xF0)
	      {
		if(code1 == 0xF6 || code1 == 0xF7) strcpy(str,D9Fx[code1 & 0x0F]);
		else    FPUcmdst0(str,D9Fx[code1 & 0x0F]);
		if(code1 == 0xF5 || code1 == 0xFB || code1 == 0xFE || code1 == 0xFF)
		if(x86_Bitness != DAB_USE64)
			DisP.pro_clone &= ~IX86_CPUMASK;
			DisP.pro_clone |= IX86_CPU386|INSN_FPU;
	      }
	      else
		if(code1 == 0xD0) strcpy(str,"fnop");
		else
		  if((code1 & 0xF0) == 0xC0) FPUcmdsti_2(str,"fld","fxch",code1);
		  else {
		    mtd=D9rm[rm].f;
		    (this->*mtd)(str,D9rm[rm].c,DisP);
		  }
	    break;
   case 0xDA :
	    if(code1 == 0xE9)
	    {
		if(x86_Bitness != DAB_USE64) DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
	      DisP.pro_clone |= IX86_CPU386|INSN_FPU;
	      strcpy(str,SC("fucompp","st(1)"));
	    }
	    else
	      if((code1 & 0xC0) == 0xC0)
	      {
		if(x86_Bitness != DAB_USE64) DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		DisP.pro_clone |= IX86_CPU686|INSN_FPU;
		FPUst0sti(str,FCMOVc[(code1 >> 3) & 0x07],code1);
	      }
	      else
		FPUint16int32(str,int16int32[rm],DisP);
	    break;
   case 0xDB :
	   switch(code1)
	   {
	     case 0xFC:
		if(x86_Bitness != DAB_USE64)
		{
			strcpy(str,SC("frint2","st(0)"));
			DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP|IX86_CLONEMASK);
			DisP.pro_clone |= IX86_CPU486|INSN_FPU|IX86_CYRIX;
		}
		else strcpy(str,"f???");
		break;
	     default:
	     if((code1 & 0xF0) == 0xE0)
	     {
	       if((code1 & 0x0F) <= 0x04)
	       {
		  unsigned char _index = code1 & 0x07;
		  strcpy(str,DBEx[_index]);
		  if(x86_Bitness != DAB_USE64)
		  if(_index == 4) {
		    DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		    DisP.pro_clone |= IX86_CPU286|INSN_FPU;
		  }
	       }
	       else
		 if((code1 & 0x0F) >= 0x08) goto XC0;
		 else strcpy(str,"f???");
	     }
	     else
	       if((code1 & 0xC0) == 0xC0)
	       {
		  XC0:
		  if(x86_Bitness != DAB_USE64)
		  DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		  DisP.pro_clone |= IX86_CPU386|INSN_FPU;
		  FPUst0sti(str,FCMOVnc[(code1 >> 3) & 0x07],code1);
	       }
	       else
	       {
		 mtd=DBrm[rm].f;
		 (this->*mtd)(str,DBrm[rm].c,DisP);
		 if(rm==1
		  && x86_Bitness != DAB_USE64
		 ) {
		    DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		    DisP.pro_clone |= IX86_P5|INSN_FPU;
		 }
	       }
	    }
	    break;
   case 0xDC:
	    if((code1 & 0xF0) >= 0xC0)
	    {
	      unsigned char _index = (code1 & 0x30) >> 4;
	      FPUstist0_2(str,_index > 1 ? D8str[_index].c2 : D8str[_index].c1,_index > 1 ? D8str[_index].c1 : D8str[_index].c2,code1);
	    }
	    else          FPUmem64mem32(str,mem64mem32[rm],DisP);
	    break;
   case 0xDD:
	    switch(code1)
	    {
	      case 0xFC:
		if(x86_Bitness != DAB_USE64)
		{
			strcpy(str,SC("frichop","st(0)"));
			DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP|IX86_CLONEMASK);
			DisP.pro_clone |= IX86_CPU486|INSN_FPU|IX86_CYRIX;
		}
		else strcpy(str, "f???");
		break;
	      default:
	      if((code1 & 0xF0) == 0xC0) FPUcmdsti_2(str,"ffree","f???",code1);
	      else
		if((code1 & 0xF0) == 0xD0) FPUstist0_2(str,"fst","fstp",code1);
		else
		  if((code1 & 0xF0) == 0xE0)
		  {
		    if(x86_Bitness != DAB_USE64)
		    DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		    DisP.pro_clone |= IX86_CPU386|INSN_FPU;
		    FPUcmdsti_2(str,"fucom","fucomp",code1);
		  }
		  else
		  {
		    mtd=DDrm[rm].f;
		    (this->*mtd)(str,DDrm[rm].c,DisP);
		    if(rm==1
		  && x86_Bitness != DAB_USE64
		    ) {
			DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
			DisP.pro_clone |= IX86_P5|INSN_FPU;
		    }
		  }
	      break;
	    }
	    break;
   case 0xDE:
	    if((code1 & 0xF0) >= 0xC0)
	    {
	      unsigned char _index = (code1 & 0x30) >> 4;
	      FPUstist0_2(str,DEstr[_index].c1,DEstr[_index].c2,code1);
	    }
	    else    FPUint16int32(str,int16int32[rm],DisP);
	    break;
   default:
   case 0xDF:
	   switch(code1)
	   {
	     case 0xE0:  strcpy(str,SC("fstsw","ax"));
			 break;
	     case 0xE1:  strcpy(str,SC("fnstdw","ax"));
			 if(x86_Bitness != DAB_USE64)
			 DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
			 DisP.pro_clone |= IX86_CPU386|INSN_FPU;
			 break;
	     case 0xE2:  strcpy(str,SC("fnstsg","ax"));
			 if(x86_Bitness != DAB_USE64)
			 DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
			 DisP.pro_clone |= IX86_CPU386|INSN_FPU;
			 break;
	     case 0xFC:
			 if(x86_Bitness != DAB_USE64)
			 {
			 strcpy(str,SC("frinear","st(0)"));
			 DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP|IX86_CLONEMASK);
			 DisP.pro_clone |= IX86_CPU486|INSN_FPU|IX86_CYRIX;
			 }
			 else strcpy(str,"f???");
			 break;
	     default:
	      {
		if((code1 & 0xE0) == 0xE0)
		{
		  if(x86_Bitness != DAB_USE64)
		  DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		  DisP.pro_clone |= IX86_CPU686|INSN_FPU;
		  FPUst0sti(str,FxCOMIP[(code1 >> 3) & 0x07],code1);
		}
		else
		if((code1 & 0xC0) == 0xC0)
		{
		 if(x86_Bitness != DAB_USE64)
		  DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		  DisP.pro_clone |= IX86_CPU386|INSN_FPU;
		  FPUcmdsti_2(str,"ffreep","f???",code1);
		}
		else
		{
		   mtd=DFrm[rm].f;
		   (this->*mtd)(str,DFrm[rm].c,DisP);
		   if(rm==1
		  && x86_Bitness != DAB_USE64
		   ) {
		    DisP.pro_clone &= ~(IX86_CPUMASK|INSN_REGGROUP);
		    DisP.pro_clone |= IX86_P5|INSN_FPU;
		   }
		}
	      }
	      break;
	   }
	   break;
 }
}
} // namespace	usr
