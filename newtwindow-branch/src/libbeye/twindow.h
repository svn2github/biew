/**
 * @namespace   libbeye
 * @file        libbeye/twindow.h
 * @brief       This file contains prototypes of Text Window manager functions.
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
 * @warning     Program is destroyed, from printf misapplication
 * @bug         Limitation of printf using
**/
#ifndef __TWINDOW_HPP_INCLUDED
#define __TWINDOW_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

#include "libbeye/twidget.h"

namespace	usr {
    class System;
    class TConsole;

    /** Internal structure of text window */
    class TWindow : public TWidget {
	public:
		   /** Creates window
		     * @param x1_,y1_      indicate upper-left cornen of window
		     * @param width,height indicate width and height of window
		     * @param flags        indicates TWS_* flags
		     * @return             handle of window
		    **/
	    TWindow(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, twc_flag flags=Flag_None);

		   /** Destroys given window
		     * @param              handle of window
		     * @return             none
		    **/
	    virtual ~TWindow();

		   /** Hides given window
		     * @param win          handle of window
		     * @return             none
		     * @note               All operations are available for
		     *                     hidden window. Does not change
		     *                     Z-order of window.
		    **/
	    virtual void		hide();

		   /** Shows given window.
		     * @param win          handle of window
		     * @return             none
		     * @note               Does not change Z-order of window.
		    **/
	    virtual void		show();

		   /** Shows given window beneath other window.
		     * @param win          handle of window
		     * @return             none
		     * @note               Does change Z-order of window
		    **/
	    virtual void		show_beneath(TWindow& prev);

		   /** Shows given window on the top of the window stack.
		     * @param win          handle of window
		     * @return             none
		     * @note               Does change Z-order of window
		    **/
	    virtual void		show_on_top();

		   /** Alters size of given window.
		     * @param win          handle of window to be resized
		     * @param width,height specify new width and height of the window.
		     * @return             none
		     * @note               If the window is expanded, blanks are filled in;
					   If the window is contracted, text is clipped
		    **/
	    virtual void		resize(tAbsCoord width, tAbsCoord height);

	protected:
	    virtual void		updatescreencharfrombuff(tRelCoord x,tRelCoord y,const tvideo_buffer& buff,tvideo_buffer *accel) const;
	private:
	    void		savedwin2screen();

	    void		paint_internal();
	    void		__draw_frame(tRelCoord xs, tRelCoord ys, tRelCoord xe, tRelCoord ye,const any_t*_frame, DefColor color);
	    void		updatewinmemcharfromscreen(tRelCoord x,tRelCoord y,const tvideo_buffer& accel);
	    void		screen2win();
	    void		updatewinmem();

	    void		restorescreenchar(tRelCoord x,tRelCoord y,tvideo_buffer* accel) const { updatescreencharfrombuff(x-1,y-1,saved,accel); }

	    tvideo_buffer&	saved;       /**< Buffer containing saved image under window */
    };
} // namespace	usr

#endif






