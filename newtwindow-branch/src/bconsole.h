/**
 * @namespace	usr
 * @file        bconsole.h
 * @brief       This file included BEYE console functions description.
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
 * @author      Mauro Giachero
 * @date        02.11.2007
 * @note        Added "ungotstring" function to enable inline assemblers
**/
#ifndef __BCONSOLE__H
#define __BCONSOLE__H
#include <limits>
#include <vector>

#include "search.h"
#include "libbeye/kbd_code.h"
#include "libbeye/twindow.h"

namespace	usr {
    bool        __FASTCALL__ IsKbdTerminate();
    void         __FASTCALL__ CleanKbdTermSig();

    TWindow *    __FASTCALL__ CrtDlgWndnls(const std::string&,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CrtHlpWndnls(const std::string&,tAbsCoord,tAbsCoord);
    TWindow *    __FASTCALL__ CreateEditor(tAbsCoord X1,tAbsCoord Y1,tAbsCoord X2,tAbsCoord Y2,TWindow::twc_flag flags);

/** Edit string styles */
enum {
    __ESS_ENABLEINSERT =0x0001U, /**< enable insert mode */
    __ESS_HARDEDIT     =0x0002U, /**< inform, that editing within hard multiline
					editor without insert mode */
    __ESS_WANTRETURN   =0x0004U, /**< return from routine after each pressed key
					need for contest depended painting. */
    __ESS_ASHEX        =0x0008U, /**< worked, as hexadecimal editor, i.e. insert
					space on each third position */
    __ESS_NOTUPDATELEN =0x0010U, /**< if attr & __ESS_ASHEX procedure not will
					update field *maxlength on each return */
    __ESS_FILLER_7BIT  =0x0020U, /**< Editor for Assemebler mode */
    __ESS_NON_C_STR    =0x0040U, /**< Notify editor about non-C string */
    __ESS_NOREDRAW     =0x8000U /**< Force no redraw string */
};
/** Arguments for (x,e)editstring:
   s         - pointer to a buffer with editing strings
   legal     - pointer to a legal character set (all if NULL)
   maxlength - pointer to maximal possible length of string
	       contains real length on return from routine
      y      - if __ESS_HARDEDIT y position within using window
	       else if __ESS_NON_C - real length of
	       string, i.e. enabled for input CHAR 0x00
      stx    - pointer to a x position within using window (0 - base)
	       contains last x position on return
	       if NULL position = 0
      attr   - attributes of the editor (See 'Edit string styles')
     undo    - if not NULL twUsedWin as UnDo buffer, i.e.
	       restore contest on CtrlBkSpace
     func    - if not NULL then called to display prompt string
*/
    int         __FASTCALL__ eeditstring(TWindow* w,char *s,const char *legal,
					unsigned *maxlength, unsigned y,
					unsigned *stx,unsigned attr,char *undo,
					void (*func)());
    int          __FASTCALL__ xeditstring(TWindow* w,char *s,const char *legal,
					unsigned maxlength, void(*func)());
    TWindow *    __FASTCALL__ PleaseWaitWnd();

    bool        __FASTCALL__ Get2DigitDlg(const std::string& title,const std::string& text,unsigned char *xx);
    bool        __FASTCALL__ Get16DigitDlg(const std::string& title,const std::string& text,char attr,
						unsigned long long int *xx);
    bool        __FASTCALL__ Get8DigitDlg(const std::string& title,const std::string& text,char attr,unsigned long *xx);

    bool        __FASTCALL__ GetStringDlg(char * buff,const std::string& title,const std::string& subtitle,
				     const std::string& prompt);
enum {
    GJDLG_FILE_TOP  =0x00000000UL,
    GJDLG_RELATIVE  =0x00000001UL,
    GJDLG_REL_EOF   =0x00000002UL,
    GJDLG_VIRTUAL   =0x00000003UL,
    GJDLG_PERCENTS  =0x00000004UL
};
    bool        __FASTCALL__ GetJumpDlg( __filesize_t * addr,unsigned long *flags);

enum {
    FSDLG_BINMODE   =0x00000000UL,
    FSDLG_ASMMODE   =0x00000001UL,
    FSDLG_NOCOMMENT =0x00000000UL,
    FSDLG_COMMENT   =0x00000002UL,
    FSDLG_STRUCTS   =0x00000004UL,
    FSDLG_NOMODES   =0x00000000UL,
    FSDLG_USEMODES  =0x80000000UL,

    FSDLG_BTNSMASK  =0x00000003UL, /**< 0=8bit 1=16bit 2=32bit 3=64bit */
    FSDLG_USEBITNS  =0x40000000UL
};
    bool        __FASTCALL__ GetFStoreDlg(const std::string& title,char* fname,
				     unsigned long *flags,
				     __filesize_t *start,
				     __filesize_t *end,
				     const std::string& prompt);
    bool        __FASTCALL__ GetInsDelBlkDlg(const std::string& title,__filesize_t *start,
					__fileoff_t *size);
    class PercentWindow : public TWindow {
	public:
	    PercentWindow();
	    virtual ~PercentWindow();

	    virtual bool	show_percents(unsigned percents);
	private:
	    time_t		_time;
	    time_t		prev_time;
	    unsigned	_percents;
	    bool		is_first;
    };
    PercentWindow*   __FASTCALL__ PercentWnd(const std::string& text,const std::string& title);

    int          __FASTCALL__ GetEvent(void (*)(),int (*)(),TWindow *);
    void         __FASTCALL__ PostEvent(int kbdcode);

    void __FASTCALL__ __drawSinglePrompt(const char *prmt[]);

    bool __FASTCALL__ ungotstring(char *string);

    void   drawEmptyPrompt();
    template<class S,class T>
    int PageBox(unsigned width,unsigned height,const T& obj,const S& s,void (S::*f)(TWindow&,const T&,unsigned) const)
    {
	TWindow* wlist;
	int start,ostart,ret;
	size_t nobj = obj.size();

	wlist = CrtDlgWndnls("",width-1,height);
	ostart = start = 0;
	(s.*f)(*wlist,obj,(unsigned)start);
	for(;;) {
	unsigned ch;
	ch = GetEvent(drawEmptyPrompt,NULL,wlist);
	if(ch == KE_ESCAPE || ch == KE_F(10)) { ret = -1; break; }
	if(ch == KE_ENTER)                    { ret = start; break; }
	switch(ch) {
	    case KE_PGDN : start ++; break;
	    case KE_PGUP   : start --; break;
	    case KE_CTL_PGDN : start = nobj - 1; break;
	    case KE_CTL_PGUP : start = 0; break;
	    default : break;
	};
	if(start < 0) start = 0;
	if((unsigned)start > nobj - 1) start = nobj - 1;
	if(start != ostart) {
	    ostart = start;
	    wlist->goto_xy(1,1);
	    (s.*f)(*wlist,obj,(unsigned)start);
	}
    }
    delete wlist;
    return ret;
    }

} // namespace	usr
#endif
