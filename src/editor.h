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
    struct editor_mem {
	unsigned char *buff;
	unsigned char *save;
	unsigned char *alen;
	unsigned       size;
	unsigned       width;
    };

    class Editor : public Opaque {
	public:
	    Editor(TWindow& enwd,unsigned width);
	    Editor(TWindow& enwd,unsigned width,const unsigned char *buff,unsigned size);
	    virtual ~Editor();

	    virtual int		run(TWindow* hexwnd=NULL);
	    virtual bool	default_navigation(int _lastbyte);
	    virtual bool	default_action(int _lastbyte);
	    virtual void	goto_xy(unsigned x,unsigned y);
	    virtual unsigned	where_x() const;
	    virtual unsigned	where_y() const;
	    virtual const editor_mem&	get_mem() const;
	    virtual editor_mem&	get_mem();
	    virtual uint8_t	get_template() const;
	    virtual void	CheckBounds();
	    virtual void	CheckYBounds();
	    virtual void	CheckXYBounds();
	    virtual void	save_context();
	    virtual void	paint_title(int shift,bool use_shift) const;
	    virtual void	show_help() const;
	private:
	    void		init(unsigned width,const unsigned char *buff,unsigned size);
	
	    editor_mem		EditorMem;
	    int			edit_x,edit_y;
	    __fileoff_t		edit_cp;
	    uint8_t		edit_XX;
	    TWindow&		ewnd;
    };

} // namespace	usr
#endif
