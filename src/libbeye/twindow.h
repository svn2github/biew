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

namespace	usr {
    class System;
    class TConsole;
    /** Enumerates all colors used in twin library */
    enum Color {
	Black = 0,	/**< Equivalent of RGB: 0 0 0 */
	Blue,		/**< Equivalent of RGB: 0 0 128 */
	Green,		/**< Equivalent of RGB: 0 128 0 */
	Cyan,		/**< Equivalent of RGB: 0 128 128 */
	Red,		/**< Equivalent of RGB: 128 0 0 */
	Magenta,	/**< Equivalent of RGB: 128 0 128 */
	Brown,		/**< Equivalent of RGB: 128 128 0 */
	LightGray,	/**< Equivalent of RGB: 192 192 192 */
	Gray,		/**< Equivalent of RGB: 128 128 128 */
	LightBlue,	/**< Equivalent of RGB: 0 0 255 */
	LightGreen,	/**< Equivalent of RGB: 0 255 0 */
	LightCyan,	/**< Equivalent of RGB: 0 255 255 */
	LightRed,	/**< Equivalent of RGB: 255 0 0 */
	LightMagenta,	/**< Equivalent of RGB: 255 0 255 */
	Yellow,		/**< Equivalent of RGB: 255 255 0 */
	White		/**< Equivalent of RGB: 255 255 255 */
    };

/* Pseudographics and special characters support
  Box drawing in short:
   S - single  D - double
   H - horizontal  V - vertical
   u - up  d - down  l - left  r - right
*/

    enum {
	TWC_DEF_FILLER	=0x20,  /**< default filler char */

#ifndef HAVE_TERMINAL_OUT
	TWC_UP_ARROW	='',   /**< Up arrow character */
	TWC_DN_ARROW	='',   /**< Down arrow character */
	TWC_LT_ARROW	='',   /**< Left arrow character */
	TWC_RT_ARROW	='',   /**< Right arrow character */
	TWC_THUMB	='\xFE',/**< Thumb character */
#else
	TWC_UP_ARROW	='^',   /**< Up arrow character */
	TWC_DN_ARROW	='v',   /**< Down arrow character */
	TWC_LT_ARROW	='<',   /**< Left arrow character */
	TWC_RT_ARROW	='>',   /**< Right arrow character */
	TWC_THUMB	='#',   /**< Thumb character */
#endif

	TWC_RADIO_CHAR	='*',   /**< Character for radio buttons */
	TWC_CHECK_CHAR	='x',   /**< Character for check buttons */

	TWC_LT_SHADE	=0xB0,  /**< Light shade characters */
	TWC_MED_SHADE	=0xB1,  /**< Medium shade characters */
	TWC_DK_SHADE	=0xB2,  /**< Dark shade characters */

	TWC_SV		=0xB3,  /**< SINGLE VERTICAL */
	TWC_SV_Sl	=0xB4,  /**< SINGLE VERTICAL and SINGLE left */
	TWC_Sl_SV	=TWC_SV_Sl,
	TWC_SV_Dl	=0xB5,  /**< SINGLE VERTICAL and DOUBLE left */
	TWC_Dl_SV	=TWC_SV_Dl,
	TWC_DV_Sl	=0xB6,  /**< DOUBLE VERTICAL and SINGLE left */
	TWC_Sl_DV	=TWC_DV_Sl,
	TWC_Dd_Sl	=0xB7,  /**< DOUBLE down and SINGLE left */
	TWC_Sl_Dd	=TWC_Dd_Sl,
	TWC_Sd_Dl	=0xB8,  /**< SINGLE down and DOUBLE left */
	TWC_Dl_Sd	=TWC_Sd_Dl,
	TWC_DV_Dl	=0xB9,  /**< DOUBLE VERTICAL and DOUBLE left */
	TWC_Dl_DV	=TWC_DV_Dl,
	TWC_DV		=0xBA,  /**< DOUBLE VERTICAL */
	TWC_Dd_Dl	=0xBB,  /**< DOUBLE down and DOUBLE left */
	TWC_Dl_Dd	=TWC_Dd_Dl,
	TWC_Du_Dl	=0xBC,  /**< DOUBLE up and DOUBLE left */
	TWC_Dl_Du	=TWC_Du_Dl,
	TWC_Du_Sl	=0xBD,  /**< DOUBLE up and SINGLE left */
	TWC_Sl_Du	=TWC_Du_Sl,
	TWC_Su_Dl	=0xBE,  /**< SINGLE up and DOUBLE left */
	TWC_Dl_Su	=TWC_Su_Dl,
	TWC_Sd_Sl	=0xBF,  /**< SINGLE down and SINGLE left */
	TWC_Sl_Sd	=TWC_Sd_Sl,
	TWC_Su_Sr	=0xC0,  /**< SINGLE up and SINGLE right */
	TWC_Sr_Su	=TWC_Su_Sr,
	TWC_SH_Su	=0xC1,  /**< SINGLE HORIZONTAL and SINGLE up */
	TWC_Su_SH	=TWC_SH_Su,
	TWC_SH_Sd	=0xC2,  /**< SINGLE HORIZONTAL and SINGLE down */
	TWC_Sd_SH	=TWC_SH_Sd,
	TWC_SV_Sr	=0xC3,  /**< SINGLE VERTICAL and SINGLE right */
	TWC_Sr_SV	=TWC_SV_Sr,
	TWC_SH		=0xC4,  /**< SINGLE HORIZONTAL */
	TWC_SH_SV	=0xC5,  /**< SINGLE HORIZONTAL and SINGLE VERTICAL */
	TWC_SV_SH	=TWC_SH_SV,
	TWC_SV_Dr	=0xC6,  /**< SINGLE VERTICAL and DOUBLE right */
	TWC_Dr_SV	=TWC_SV_Dr,
	TWC_DV_Sr	=0xC7,  /**< DOUBLE VERTICAL and SINGLE right */
	TWC_Sr_DV	=TWC_DV_Sr,
	TWC_Du_Dr	=0xC8,  /**< DOUBLE up and DOUBLE right */
	TWC_Dr_Du	=TWC_Du_Dr,
	TWC_Dd_Dr	=0xC9,  /**< DOUBLE down and DOUBLE right */
	TWC_Dr_Dd	=TWC_Dd_Dr,
	TWC_DH_Du	=0xCA,  /**< DOUBLE HORIZONTAL and DOUBLE up */
	TWC_Du_DH	=TWC_DH_Du,
	TWC_DH_Dd	=0xCB,  /**< DOUBLE HORIZONTAL and DOUBLE down */
	TWC_Dd_DH	=TWC_DH_Dd,
	TWC_DV_Dr	=0xCC,  /**< DOUBLE VERTICAL and DOUBLE right */
	TWC_Dr_DV	=TWC_DV_Dr,
	TWC_DH		=0xCD,  /**< DOUBLE HORIZONTAL */
	TWC_DH_DV	=0xCE,  /**< DOUBLE HORIZONTAL and DOUBLE VERTICAL */
	TWC_DV_DH	=TWC_DH_DV,
	TWC_DH_Su	=0xCF,  /**< DOUBLE HORIZONTAL and SINGLE up */
	TWC_Su_DH	=TWC_DH_Su,
	TWC_SH_Du	=0xD0,  /**< SINGLE HORIZONTAL and DOUBLE up */
	TWC_Du_SH	=TWC_SH_Du,
	TWC_DH_Sd	=0xD1,  /**< DOUBLE HORIZONTAL and SINGLE down */
	TWC_Sd_DH	=TWC_DH_Sd,
	TWC_SH_Dd	=0xD2,  /**< SINGLE HORIZONTAL and DOUBLE down */
	TWC_Dd_SH	=TWC_SH_Dd,
	TWC_Du_Sr	=0xD3,  /**< DOUBLE up and SINGLE right */
	TWC_Sr_Du	=TWC_Du_Sr,
	TWC_Su_Dr	=0xD4,  /**< SINGLE up and DOUBLE right */
	TWC_Dr_Su	=TWC_Su_Dr,
	TWC_Sd_Dr	=0xD5,  /**< SINGLE down and DOUBLE right */
	TWC_Dr_Sd	=TWC_Sd_Dr,
	TWC_Dd_Sr	=0xD6,  /**< DOUBLE down and SINGLE right */
//	TWC_Sr_Dd	=TWC_Sr_Dd,
	TWC_DV_SH	=0xD7,  /**< DOUBLE VERTICAL and SINGLE HORIZONTAL */
	TWC_SH_DV	=TWC_DV_SH,
	TWC_SV_DH	=0xD8,  /**< SINGLE VERTICAL and DOUBLE HORIZONTAL */
	TWC_DH_SV	=TWC_SV_DH,
	TWC_Su_Sl	=0xD9,  /**< SINGLE up and SINGLE left */
	TWC_Sl_Su	=TWC_Su_Sl,
	TWC_Sd_Sr	=0xDA,  /**< SINGLE down and SINGLE right */
	TWC_Sr_Sd	=TWC_Sd_Sr,

	TWC_FL_BLK	=0xDB,  /**< Full block character */
	TWC_LW_HBLK	=0xDC,  /**< Lower half block character */
	TWC_LF_HBLK	=0xDD,  /**< Left half block character */
	TWC_RT_HBLK	=0xDE,  /**< Right half block character */
	TWC_UP_HBLK	=0xDF,  /**< Up half block character */

	TWC_BLACK_SQUARE=0xFE  /**< Black square */
    };

    /** Describes window-related coordinate type */
    typedef unsigned tRelCoord;

    /** Defines color pair that contains original user color and converted system color */
    struct DefColor {
	ColorAttr user;        /**< Original user color value */
	ColorAttr system;      /**< Converted system color value */
    };

		      /** Converts logical foreground and background into physical color attributes */
    inline ColorAttr LOGFB_TO_PHYS(Color fore,Color back) { return ((back << 4) & 0xF0) | (fore & 0x0F); }
		      /** Gets background color from physical attributes */
    inline Color BACK_COLOR(ColorAttr attr) { return Color((attr >> 4) & 0x0F); }
		      /** Gets foreground color from physical attributes */
    inline Color FORE_COLOR(ColorAttr attr) { return Color(attr & 0x0F); }
		      /** Converts physical color attributes into logical foreground and background */
    inline void PHYS_TO_LOGFB(ColorAttr attr,Color& fore,Color& back) { fore = FORE_COLOR(attr); back = BACK_COLOR(attr); }

    /* Below located list of window messages. Prefix WM_ was imported from
	MSWindows SDK, but I hope it so understandable. */
    enum {
	WM_NULL		=0x0000, /**< Never to send */
	WM_CREATE	=0x0001, /**< It sent when window is being created, has no parameters */
	WM_DESTROY	=0x0002, /**< It sent when window is being destroyed, has no parameters*/
	WM_SHOW		=0x0003, /**< It sent when window is being displayed, has no parameters*/
	WM_TOPSHOW	=0x0004, /**< It sent when window is being displayed on top of all windows, has no parameters*/
	WM_SHOWBENEATH	=0x0005, /**< It sent when window is being displayed beneath of other window, has handle of top window as event_data*/
	WM_HIDE		=0x0006 /**< It sent when window is being hidded, has no parameters */
    };

    enum {
	TWIF_FORCEMONO   =0x00000001L, /**< forces monochrome mode of video output @see twInit */
    };
		   /** Initialization of twin library and video subsystem
		     * @param user_cp     indicates character's codepage or IBM866 if NULL
		     * @param vio_flags   flags for _init_vio
		     * @param twin_flgs   flags of twin library (see above)
		     * @return            none
		     * @note              Call this function before any other
		     * @see               twDestroy
		    **/
    TConsole&	__FASTCALL__ twInit(System& system,const std::string& user_cp, unsigned long vio_flags, unsigned long twin_flgs );

		   /** Terminates twin library and video subsystem
		     * @return            none
		     * @note              Call this function after all other
		     * @see               twInit
		    **/
    void	__FASTCALL__ twDestroy();

    /** Internal structure of text window */
    class TWindow : public Opaque {
	public:
	    enum twc_flag {
		Flag_None	=0x0000, /**< Indicates no flags */
		Flag_Has_Frame	=0x0001, /**< Indicates that window has frame border */
		Flag_Has_Cursor	=0x0002, /**< Indicates that window has text cursor */
		Flag_NLS	=0x0100  /**< Indicates that window works in OEM mode */
	    };
	    /** align modes for title and footer */
	    enum title_mode {
		TMode_Left = 0,	/**< left alignment */
		TMode_Center,	/**< center alignment */
		TMode_Right	/**< right alignment */
	    };

		   /** Creates window
		     * @param x1_,y1_      indicate upper-left cornen of window
		     * @param width,height indicate width and height of window
		     * @param flags        indicates TWS_* flags
		     * @return             handle of window
		    **/
	    TWindow(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, twc_flag flags=Flag_None);

		   /** Creates extended window
		     * @param x1_,y1_      indicate upper-left cornen of window
		     * @param width,height indicate width and height of window
		     * @param flags        indicates TWS_* flags
		     * @param classname    indicates name of class
		     * @return             handle of window
		     * @note               name of class must be registered before
					   calling of this function
		    **/
	    TWindow(tAbsCoord x1_, tAbsCoord y1_,
			tAbsCoord width, tAbsCoord height,
			twc_flag flags, const std::string& classname);

		   /** Destroys given window
		     * @param              handle of window
		     * @return             none
		    **/
	    virtual ~TWindow();


		   /** Calls window function with given arguments
		     * @return                function answer
		     * @param event           one of WM_* commands
		     * @param event_param     command related parameter
		     * @param event_data      command related data
		    **/
	    virtual long		send_message(unsigned event,unsigned long event_param,const any_t*event_data);

		   /** Draws frame of given type in active window
		     * @return                none
		     * @param x1_,y1_,x2_,y2_ indicate coordinates inside window
		     * @param frame           indicates frame type
		     * @param fore,back       indicate logical foreground and background attributes
		    **/
	    virtual void		draw_frame(tRelCoord x1_, tRelCoord y1_, tRelCoord x2_, tRelCoord y2_,const unsigned char *frame,Color fore, Color back);

		   /** Draws frame of given type in active window
		     * @return                none
		     * @param x1_,y1_,x2_,y2_ indicate coordinates inside window
		     * @param frame           indicates frame type
		     * @param attr            indicates physical color attributes
		    **/
	    virtual void		draw_frame(tRelCoord x1_, tRelCoord y1_, tRelCoord x2_, tRelCoord y2_,const unsigned char *frame,ColorAttr attr);

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

		   /** Updates the window buffer from the screen.
		     * @param win          handle of window
		     * @return             none
		    **/
	    virtual void		snapshot();

		   /** Set cursor into the given window.
		     * @param win          handle of window to be used
		     * @return             focus of previous window or NULL if none
		     * @note               \e win need not be the top window; not
		     *                     does \e win even have to be visible on
		     *                     the screen.
		    **/
	    virtual TWindow*		set_focus();

		   /** Alters position of given window.
		     * @param win          handle of window to be moved
		     * @param dx,dy        specify relative change of x and y coordinates.
		     * @return             none
		     * @note               The contents of the window will be moved with it.
		    **/
	    virtual void		move(tAbsCoord dx, tAbsCoord dy);

		   /** Alters size of given window.
		     * @param win          handle of window to be resized
		     * @param width,height specify new width and height of the window.
		     * @return             none
		     * @note               If the window is expanded, blanks are filled in;
					   If the window is contracted, text is clipped
		    **/
	    virtual void		resize(tAbsCoord width, tAbsCoord height);

		   /** Scrolls part of given window up.
		     * @param win          handle of window to be scrolled
		     * @param ypos         specifies vertical coordinate of line. All lines above it and line itself will be scrolled up
		     * @param npos         specifies number of lines to be inserted.
		     * @return             none
		     * @note               If 0 is specified as npos, no lines are scrolled.
		    **/
	    virtual void		scroll_up(tRelCoord ypos, unsigned npos);

		   /** Scrolls part of given window down.
		     * @param win          handle of window to be scrolled
		     * @param ypos         specifies vertical coordinate of line. All lines below it and line itself will be scrolled down
		     * @param npos         specifies number of lines to be inserted.
		     * @return             none
		     * @note               If 0 is specified as npos, no lines are scrolled.
		    **/
	    virtual void		scroll_down(tRelCoord ypos, unsigned npos);

		   /** Scrolls part of given window left.
		     * @param win          handle of window to be scrolled
		     * @param xpos         specifies horizontal coordinate of column. All columns most left it and column itself will be scrolled left
		     * @param npos         specifies number of columns to be inserted.
		     * @return             none
		     * @note               If 0 is specified as npos, no columns are scrolled.
		    **/
	    virtual void		scroll_left(tRelCoord xpos, unsigned npos);

		   /** Scrolls part of given window right.
		     * @param win          handle of window to be scrolled
		     * @param xpos         specifies horizontal coordinate of column. All columns most right it and column itself will be scrolled right
		     * @param npos         specifies number of columns to be inserted.
		     * @return             none
		     * @note               If 0 is specified as npos, no columns are scrolled.
		    **/
	    virtual void		scroll_right(tRelCoord xpos, unsigned npos);

		   /** Does centring of given window relatively other window or screen.
		     * @param it           handle of window to be centred
		     * @param parent       handle of parent window.
		     * @return             none
		     * @note               If \e parent is NULL, the window will
		     *                     be centred for screen
		    **/
	    virtual void		into_center(const TWindow& parent);

	    virtual void		into_center();

		   /** Freezes redrawing of window.
		     * @param parent       handle of centring window.
		     * @return             none
		     * @note               Freezed only redrawing of the window.
		     *                     All output will be produced, but
		     *                     screen will not be updated.
		    **/
	    virtual void		freeze();

		   /** Updates the line of screen by line of buffer of given window.
		     * @param win          handle of refreshing window.
		     * @param y            specify relative y coordinate of line to be refreshed.
		     * @return             none
		     * @note               If window is not freezed all output
		     *                     is automatically displayed on
		     *                     the screen.
		    **/
	    virtual void		refresh(tRelCoord y);

		   /** Updates the piece of screen line by piece of buffer of given window.
		     * @param win          handle of refreshing window.
		     * @param stx,endx     specify range of x coordinate of line to be refreshed.
		     * @param y            specify relative y coordinate of line to be refreshed.
		     * @return             none
		     * @note               If window is not freezed all output
		     *                     is automatically displayed on
		     *                     the screen.
		    **/
	    virtual void		refresh_piece(tRelCoord stx, tRelCoord endx,tRelCoord y);

		   /** Updates the screen buffer by client area of given window.
		     * @param win          handle of refreshing window.
		     * @return             none
		     * @note               If window is not freezed all output
		     *                     is automatically displayed on
		     *                     the screen.
		    **/
	    virtual void		refresh();

		   /** Updates the screen buffer by buffer of given window entire.
		     * @param win          handle of refreshing window.
		     * @return             none
		     * @note               If window is not freezed all output
		     *                     is automatically displayed on
		     *                     the screen.
		    **/
	    virtual void		refresh_full();

		   /** Returns pointer to the user data that stored in window.
		     * @param win          handle of window.
		     * @return             pointer to user data
		     * @note               If no user data previously stored
		     *                     in window then NULL is returned.
		    **/
	    virtual any_t*		get_user_data() const;

		   /** Saves pointer to the user data in window.
		     * @param win          handle of window.
		     * @param data         pointer to the user data to be stored.
		     * @return             pointer to user data that previously stored in window
		     * @note               Pointer to previously stored user
		     *                     data is overwrited.
		    **/
	    virtual any_t*		set_user_data(any_t*data);

		   /** Returns screen position of window.
		     * @param win          handle of window.
		     * @param x1_,y1_,x2_,y2_ pointers to coordinates where will be saved window position.
		     * @return             none
		    **/
	    virtual void		get_pos(tAbsCoord& x1_,tAbsCoord& y1_,tAbsCoord& x2_,tAbsCoord& y2_);

		   /** Returns width of window.
		     * @param win          handle of window.
		     * @return             width of window
		    **/
	    virtual unsigned		width() const;

		   /** Returns width of height.
		     * @param win          handle of window.
		     * @return             height of window
		    **/
	    virtual unsigned		height() const;

		   /** Returns width of window which is available for output.
		     * @param win          handle of window.
		     * @return             width of window which is available for output
		    **/
	    virtual unsigned		client_width() const;

		   /** Returns height of window which is available for output.
		     * @param win          handle of window.
		     * @return             height of window which is available for output
		    **/
	    virtual unsigned		client_height() const;

		   /** Checks visibility of window piece.
		     * @param x,y          specify coordinates of the location.
		     * @return             true if specified window is visible
		     *                     in specified location (Is not obscured).
		    **/
	    virtual bool		is_piece_visible(tRelCoord x, tRelCoord y) const;

		   /** Converts window-relative coordinates to screen coordinates
		     * @param win          handle of window
		     * @param x,y          specify the relative coordinates.
		     * @param xs,ys        specify the screen coordinates.
		     * @return             none
		    **/
	    virtual void		cvt_win_coords(tRelCoord x, tRelCoord y,tAbsCoord& xs,tAbsCoord& ys) const;

		   /** Converts screen-relative coordinates to relative window coordinates
		     * @param win          handle of window
		     * @param x,y          specify screen coordinates.
		     * @param xr,yr        specify pointers to the relative coordinates.
		     * @return             true if successful, false otherwise.
		    **/
	    virtual bool		cvt_screen_coords(tAbsCoord x, tAbsCoord y,tRelCoord& xr,tRelCoord& yr) const;

		   /** Clears the current window window with given filler.
		     * @param filler       character for filling the window
		     * @return             none
		    **/
	    virtual void		clear(unsigned char filler=TWC_DEF_FILLER);

		   /** Clears the current window from current position to the end of line with given filler.
		     * @param filler       character for filling the window
		     * @return             none
		    **/
	    virtual void		clreol(unsigned char filler=TWC_DEF_FILLER);

		   /** Sets logical foreground and background colors of the text.
		     * @param fore,back    specify colors to be set.
		     * @return             Previously set color attributes
		     * @note               The function affects only those
		     *                     characters output after the call to
		     *                     foreground and background; the function
		     *                     does not affect characters that are
		     *                     already displayed.
		    **/
	    virtual ColorAttr		set_color(Color fore,Color back);

		   /** Sets physical color attributes of the text.
		     * @param attr         specifies color attributes.
		     * @return             Previously set color attributes
		     * @note               The function affects only those
		     *                     characters output after the call to
		     *                     foreground and background; the function
		     *                     does not affect characters that are
		     *                     already displayed.
		    **/
	    virtual ColorAttr		set_color(ColorAttr attr);

		   /** Returns logical foreground and background colors of the text.
		     * @param fore,back    specify pointers to memory where will be saved logical background and foreground of the text.
		     * @return             none
		    **/
	    virtual void		get_color(Color& fore, Color& back) const;

		   /** Returns physical color attributes of the text.
		     * @return             physical attributes of the text
		    **/
	    virtual ColorAttr		get_color() const;

		   /** Sets logical foreground color of the text.
		     * @param col          specifies color to be set.
		     * @return             Previously set foreground color
		     * @note               The function affects only those
		     *                     characters output after the call to
		     *                     foreground; the function does not
		     *                     affect characters that are already
		     *                     displayed.
		    **/
	    virtual Color		text_color(Color col);

		   /** Sets logical background color of the text.
		     * @param col          specifies color to be set.
		     * @return             Previously set background color
		     * @note               The function affects only those
		     *                     characters output after the call to
		     *                     background; the function does not
		     *                     affect characters that are already
		     *                     displayed.
		    **/
	    virtual Color		text_bkgnd(Color col );

		   /** Returns frame and frame attributes of given window.
		     * @param win          handle of window.
		     * @param frame        pointer to memory area (8-byte length) where will be stored frame.
		     * @param fore,back    pointers to memory area where will be stored logical colors of frame.
		     * @return             none
		    **/
	    virtual void		get_frame(unsigned char *frame,Color& fore,Color& back) const;

		   /** Returns frame and frame attributes of given window.
		     * @param win          handle of window.
		     * @param frame        pointer to memory area (8-byte length) where will be stored frame.
		     * @param attr         pointer to memory area where will be stored physical color attributes of frame.
		     * @return             none
		    **/
	    virtual void		get_frame(unsigned char *frame,ColorAttr& attr) const;

		   /** Returns title and title attributes of given window.
		     * @param win          handle of window.
		     * @param title        pointer to memory area where will be stored title.
		     * @param cb_title     size of memory area where will be stored title.
		     * @param fore,back    pointers to memory area where will be stored logical colors of title.
		     * @return             alignment mode of title
		    **/
	    virtual title_mode		get_title(char* title,unsigned cb_title,Color& fore,Color& back) const;

		   /** Returns title and title attributes of given window.
		     * @param win          handle of window.
		     * @param title        pointer to memory area where will be stored title.
		     * @param cb_title     size of memory area where will be stored title.
		     * @param attr         pointers to memory area where will be stored physical color attributes of title.
		     * @return             alignment mode of title
		    **/
	    virtual title_mode		get_title(char* title,unsigned cb_title,ColorAttr& attr) const;

		   /** Returns footer and footer attributes of given window.
		     * @param win          handle of window.
		     * @param footer       pointer to memory area where will be stored footer.
		     * @param cb_footer    size of memory area where will be stored footer.
		     * @param fore,back    pointers to memory area where will be stored logical colors of footer.
		     * @return             alignment mode of footer
		    **/
	    virtual title_mode		get_footer(char* footer,unsigned cb_footer,Color& fore,Color& back) const;

		   /** Returns footer and footer attributes of given window.
		     * @param win          handle of window.
		     * @param footer       pointer to memory area where will be stored footer.
		     * @param cb_footer    size of memory area where will be stored footer.
		     * @param fore,back    pointers to memory area where will be stored physical color attributes of footer.
		     * @return             alignment mode of footer
		    **/
	    virtual title_mode		get_footer(char* footer,unsigned cb_footer,ColorAttr& attr) const;

		   /** Changes frame around given window with specified logical color attributes.
		     * @param win          handle of window
		     * @param frame        specifies new frame (8-byte length)
		     * @param fore,back    specify new background and foreground colors
		     * @return             none
		    **/
	    virtual void		set_frame(const unsigned char *frame,Color fore,Color back);

		   /** Changes frame around given window with specified physical color attributes.
		     * @param win          handle of window
		     * @param frame        specifies new frame (8-byte length)
		     * @param attr         specifies new color attributes
		     * @return             none
		    **/
	    virtual void		set_frame(const unsigned char *frame,ColorAttr attr);

		   /** Updates window title with specified logical color attributes.
		     * @param win          handle of window
		     * @param title        specifies new title
		     * @param titlemode    specifies new title mode
		     * @param fore,back    specify new background and foreground colors
		     * @return             none
		    **/
	    virtual void		set_title(const std::string& title,title_mode titlemode,Color fore,Color back);

		   /** Updates window title with specified physical color attributes.
		     * @param win          handle of window
		     * @param title        specifies new title
		     * @param titlemode    specifies new title mode
		     * @param attr         specifies new color attributes
		     * @return             none
		    **/
	    virtual void		set_title(const std::string& title,title_mode titlemode,ColorAttr attr);

		   /** Updates window footer with specified logical color attributes.
		     * @param win          handle of window
		     * @param footer       specifies new footer
		     * @param footermode   specifies new footer mode
		     * @param fore,back    specify new background and foreground colors
		     * @return             none
		    **/
	    virtual void		set_footer(const std::string& footer,title_mode footermode,Color fore,Color back);


		   /** Updates window footer with specified physical color attributes.
		     * @param win          handle of window
		     * @param footer       specifies new footer
		     * @param footermode   specifies new footer mode
		     * @param attr         specifies new logical color attributes
		     * @return             none
		    **/
	    virtual void		set_footer(const std::string& footer,title_mode footermode,ColorAttr attr);

		   /** Sets the cursor coordiantes relative to the used window
		     * @param x,y         specify horizontal and vertical coordinates for the location
		     * @return            none
		     * @note              If X or Y are outside the window frame,
		     *                    they will be clipped.
		    **/
	    virtual void		goto_xy(tRelCoord x,tRelCoord y);

		   /** Returns the x coordinate of the current cursor position, within currently used window.
		     * @return            none
		     * @note              If X or Y are outside the window frame,
		     *                    they will be clipped.
		    **/
	    virtual tRelCoord		where_x() const;

		   /** Returns the y coordinate of the current cursor position, within currently used window.
		     * @return            none
		     * @note              If X or Y are outside the window frame,
		     *                    they will be clipped.
		    **/
	    virtual tRelCoord		where_y() const;

		   /** Outputs the character to the active window at current cursor position.
		     * @param ch          character to be written
		     * @return            none
		    **/
	    virtual void		putch(char ch);

		   /** Reads the character from the active window at current cursor position.
		     * @return            character read
		     * @note              there is no error return value.
		    **/
	    virtual char		getch() const;

		   /** Writes the null-terminated string to the active window at current cursor position.
		     * @param str         null-terminated string to be written
		     * @return            number of really written characters
		     * @note              Function has special reaction on carriage
		     *                    return and linefeed characters. If such
		     *                    characters occured, then cursor position
		     *                    will be changed automatically.
		     *                    Character '\n' changes both x and y
		     *                    cursor coordinates (for unix compatibility)
		     *                    as x=1 and y++
		     *                    Character '\r' changes only x coordinate
		     *                    as x=1.
		     *                    If written text is large than window, then
		     *                    it will be clipped.
		    **/
	    virtual int			puts(const std::string& str);

		   /** Provides formatted output directly to the active window at current cursor position.
		     * @param fmt         specifies formatted string
		     * @return            number of really written characters
		     * @note              Function has same input format as
		     *                    standard *printf function family.
		     *                    Functiom has same reaction on carriage
		     *                    return and linefeed as \e twPutS.
		     *                    If written text is large than window, then
		     *                    it will be clipped.
		     * @bug               Program is destroyed, when final output
		     *                    is greated than size of console or terminal
		     *                    screen. I.e. greated than tvioWidth*tvioHeight
		    **/
	    virtual int			printf(const std::string& fmt,...);

		   /** Writes buffer directly to the active window at specified location.
		     * @param x,y         specify location of output
		     * @param buff        specifies buffer to be written
		     * @param len         specifies length of buffer
		     * @return            number of really written characters
		     * @note              Function writes all characters as is,
		     *                    without checking for carriage return and
		     *                    linefeed.
		     *                    If written text is large than window, then
		     *                    it will be clipped.
		    **/
	    virtual int			write(tRelCoord x,tRelCoord y,const uint8_t* buff,unsigned len);
	    virtual int			write(tRelCoord x,tRelCoord y,const uint8_t* chars,const ColorAttr* attrs,unsigned len);
/* Static members */
		   /** Returns the window currently being under focus.
		     * @return             handle of window currently being used
		     * @note               If no window has focus then return NULL
		    **/
	    static TWindow*		get_focus();
		   /** Returns handle of the window currently displayed at the screen position x and y.
		     * @param x,y          specify coordinates of the location.
		     * @return             Handle for the window displayed at the
		     *                     specified position if a window is
		     *                     displayed. If no window is displayed
		     *                     at the this position, NULL is returned.
		    **/
	    static TWindow*		at_pos(tAbsCoord x, tAbsCoord y);

		enum e_cursor {
		    Cursor_Unknown	=-1, /**< Defines that cursor in invisible state */
		    Cursor_Off		=0, /**< Defines that cursor in invisible state */
		    Cursor_Normal	=1, /**< Defines that cursor in normal state (filles 20% of the character cell) */
		    Cursor_Solid	=2 /**< Defines that cursor in solid state (filles 100% of the character cell) */
		};
		   /** Sets the size and visibility of the cursor
		     * @param  type       indicates type of cursor
		     * @return            none
		     * @note              The cursor in particular window is
		     *                    visible only when the type of cursor
		     *                    is normal or solid and window is
		     *                    visible and has focus. Main difference
		     *                    between twSetCursorType and __vioSetCursorType
		     *                    is rememberring of cursor type and caching
		     *                    every call to the OS.
		    **/
	    static void			set_cursor_type(e_cursor type);

		   /** Retrieves information about the size and visibility of the cursor
		     * @param             none
		     * @return            Current type of the cursor
		     * @note              Main difference between twGetCursorType
		     *                    and __vioGetCursorType is rememberring
		     *                    of cursor type and caching every call
		     *                    to the OS.
		    **/
	    static e_cursor		get_cursor_type();

		   /** Remaps logical color with new physical value.
		     * @param color       lopical color to be remapped
		     * @param value       specifies new physical value of logical color
		     * @return            none
		    **/
	    static void			remap_color(Color color,unsigned char value);

		   /** Returns physical value of logical color.
		     * @param color       lopical color value
		     * @param value       specifies new physical value of logical color
		     * @return            physical value of logical color
		    **/
	    static unsigned char	get_mapped_color(Color color);

	    static const unsigned char SINGLE_FRAME[];    /**< Flat single-line frame */
	    static const unsigned char DOUBLE_FRAME[];    /**< Flat double-line frame */
	    static const unsigned char MEDIUM_FRAME[];    /**< Flat frame of medium width (filles 50% of the character cells) */
	    static const unsigned char THICK_FRAME[];     /**< Flat frame of full width (filles 100% of the character cells) */
	    static const unsigned char UP3D_FRAME[];      /**< Emulates 3D-frame that similar unpressed button */
	    static const unsigned char DN3D_FRAME[];      /**< Emulates 3D-frame that similar pressed button */
	    static TConsole*	tconsole;
	    static System*	msystem;
	protected:
		   /** Writes buffer directly to the active window at specified location.
		     * @param x,y         specify location of output
		     * @param buff        specifies buffer to be written
		     * @param len         specifies length of buffer
		     * @return            number of really written characters
		     * @note              Function writes all characters as is,
		     *                    without checking for carriage return and
		     *                    linefeed.
		     *                    If written text is large than window, then
		     *                    it will be clipped.
		    **/
	    virtual int			direct_write(tRelCoord x,tRelCoord y,const any_t* buff,unsigned len);
	    virtual int			direct_write(tRelCoord x,tRelCoord y,const any_t* chars,const ColorAttr* attrs,unsigned len);
	private:
		   /** Accesses to the active window directly, reading a single line.
		     * @param win         handle of window
		     * @param x,y         specify location of input
		     * @param buff        pointer to location at which readed material will be stored.
		     * @param len         specifies length of location
		     * @return            none
		     * @note              Function reads specified number of
		     *                    characters and attributes from window,
		     *                    beginning at specified location.
		    **/
	    virtual tvideo_buffer	read(tRelCoord x,tRelCoord y,size_t len) const;
		   /** Accesses to the active window directly, writing a single line.
		     * @param win         handle of window
		     * @param x,y         specify location of output
		     * @param buff        pointer to virtual video buffer to be written
		     * @param len         specifies length of virtual video buffer
		     * @return            none
		     * @note              Function writes specified number of
		     *                    characters and attributes to window,
		     *                    beginning at specified location.
		     *                    If written output is large than window,
		     *                    then it will be clipped.
		    **/
	    virtual void		write(tRelCoord x,tRelCoord y,const tvideo_buffer& buff);

	    void		create(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, unsigned flags);
	    void		makewin(tAbsCoord x1, tAbsCoord y1, tAbsCoord width, tAbsCoord height);
	    void		__unlistwin();
	    TWindow*		__prevwin();
	    bool		test_win() const;
	    void		check_win() const;
	    void		savedwin2screen();
	    void		updatescreen(bool full_area);
	    TWindow*		__find_over(tAbsCoord x,tAbsCoord y) const;
	    bool		is_overlapped() const;
	    void		paint_cursor() const;
	    void		set_xy(tRelCoord x,tRelCoord y);
	    void		igoto_xy(tRelCoord x,tRelCoord y);
	    void		wputc_oem(char ch,char oempg,char color,bool update);
	    void		paint_internal();
	    void		make_frame();
	    void		__draw_frame(tRelCoord xs, tRelCoord ys, tRelCoord xe, tRelCoord ye,const any_t*_frame, DefColor color);
	    void		updatescreencharfrombuff(tRelCoord x,tRelCoord y,const tvioBuff& buff,tvioBuff *accel) const;
	    void		updatewinmemcharfromscreen(tRelCoord x,tRelCoord y,const tvioBuff& accel);
	    void		screen2win();
	    void		updatescreenpiece(tRelCoord stx,tRelCoord endx,tRelCoord y);
	    void		updatewinmem();
	    void		__athead();
	    void		into_center(tAbsCoord w,tAbsCoord h);

	    static inline bool NLS_IS_OEMPG(unsigned char ch) { return ch >= 0xB0 && ch <= 0xDF; }
	    inline char		do_oem_pg(char ch) const { return ((flags & Flag_NLS) == Flag_NLS ? NLS_IS_OEMPG(ch) ? ch : 0 : 0); }
	    inline void		wputc(char ch,char color,bool update) { wputc_oem(ch,0,color,update); }
	    inline void		__atwin(TWindow* prev) { if(!prev) __athead(); else { next = prev->next; prev->next = this; }}
	    inline bool		__topmost() const { return !is_overlapped(); }
	    inline bool		is_valid_xy(tAbsCoord x,tAbsCoord y) const {
					return ((flags & Flag_Has_Frame) == Flag_Has_Frame ?
						x && x < wwidth-1 && y && y < wheight-1 :
						x < wwidth && y < wheight); }
	    inline bool		is_valid_x(tAbsCoord x) const { return ((flags & Flag_Has_Frame) == Flag_Has_Frame ? x && x < wwidth-1: x < wwidth); }
	    inline bool		is_valid_y(tAbsCoord y) const { return ((flags & Flag_Has_Frame) == Flag_Has_Frame ? y && y < wheight-1:y < wheight); }
	    inline void		updatescreenchar(tRelCoord x,tRelCoord y,tvioBuff* accel) const { updatescreencharfrombuff(x-1,y-1,body,accel); }
	    inline void		restorescreenchar(tRelCoord x,tRelCoord y,tvioBuff* accel) const { updatescreencharfrombuff(x-1,y-1,saved,accel); }

	    static TWindow*	__findcursorablewin();
	    static TWindow*	__at_point(TWindow* iter,tAbsCoord x,tAbsCoord y);
	    static tRelCoord	calc_title_off(title_mode mode,unsigned w,unsigned slen);
	    static void		adjustColor(Color& fore,Color& back);
	    static DefColor	__set_color(Color fore,Color back);

	    char		Frame[8];   /**< Buffer, contaning frame component */
					    /* Frame encoding:   1---2---3 */
					    /*                   |       | */
					    /*                   4       5 */
					    /*                   |       | */
					    /*                   6---7---8 */
	    unsigned long	iflags;      /**< contains internal flags of window state */
	    TWindow*		next;        /**< pointer to next window in list */
	    any_t*		usrData;     /**< user data pointer */
	    tvioBuff		body;        /**< Buffer containing image of window frame */
	    tvioBuff		saved;       /**< Buffer containing saved image under window */
	    char*		Title;       /**< Caption of window */
	    char*		Footer;      /**< Footer of window */
	    any_t*		method;      /**< Class callback */
	    unsigned		class_flags; /**< Class flags */
	    unsigned		wsize;       /**< Size of buffers in bytes */
	    unsigned		wwidth;      /**< width of window */
	    unsigned		wheight;     /**< height of window */
	    unsigned		flags;       /**< Window flags */
	    tAbsCoord		X1,Y1,X2,Y2; /**< coordinates of window on the screen */
	    tRelCoord		cur_x,cur_y; /**< coordinates of cursor position inside the window */
	    title_mode		TitleMode;   /**< alignment mode of title */
	    title_mode		FooterMode;  /**< alignment mode of footer */
	    DefColor		text;        /**< default color of text */
	    DefColor		frame;       /**< default color of frame */
	    DefColor		title;       /**< default color of title */
	    DefColor		footer;      /**< default color of footer text */

	    static TWindow*	head;
	    static TWindow*	cursorwin;
	    static const unsigned char brightness[16];
	    static uint8_t	color_map[16];
	    static e_cursor	c_type;
    };
    inline TWindow::twc_flag operator~(TWindow::twc_flag a) { return static_cast<TWindow::twc_flag>(~static_cast<unsigned>(a)); }
    inline TWindow::twc_flag operator|(TWindow::twc_flag a, TWindow::twc_flag b) { return static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline TWindow::twc_flag operator&(TWindow::twc_flag a, TWindow::twc_flag b) { return static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline TWindow::twc_flag operator^(TWindow::twc_flag a, TWindow::twc_flag b) { return static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline TWindow::twc_flag operator|=(TWindow::twc_flag& a, TWindow::twc_flag b) { return (a=static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline TWindow::twc_flag operator&=(TWindow::twc_flag& a, TWindow::twc_flag b) { return (a=static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline TWindow::twc_flag operator^=(TWindow::twc_flag& a, TWindow::twc_flag b) { return (a=static_cast<TWindow::twc_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace	usr
#include "libbeye/tw_class.h"

#endif






