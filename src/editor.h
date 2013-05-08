/**
 * @namespace	usr
 * @file        editor.h
 * @brief       This file contains editing function prototypes.
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
#ifndef __EDITOR__H
#define __EDITOR__H

#include "libbeye/libbeye.h"

namespace	usr {
    class TWindow;
    struct tag_emem {
	unsigned char *buff;
	unsigned char *save;
	unsigned char *alen;
	unsigned       size;
	unsigned       width;
    };

    extern struct tag_emem EditorMem;

    extern int edit_x,edit_y;
    extern unsigned char edit_XX;
    extern __fileoff_t edit_cp;

    void   __FASTCALL__ PaintETitle( int shift,bool use_shift );
    void   __FASTCALL__ CheckBounds();
    void   __FASTCALL__ CheckYBounds();
    void   __FASTCALL__ CheckXYBounds();
    bool  __FASTCALL__ edit_defaction(int _lastbyte);
    void   __FASTCALL__ editSaveContest();
    bool  __FASTCALL__ editDefAction(int _lastbyte);
    int    __FASTCALL__ FullEdit(TWindow * ewnd,TWindow* hexwnd,Opaque& _this,void (*save)(Opaque& _this,unsigned char *,unsigned));
    bool  __FASTCALL__ editInitBuffs(unsigned width,unsigned char *buff,unsigned size);
    void   __FASTCALL__ editDestroyBuffs();
} // namespace	usr
#endif
