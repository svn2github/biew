#ifndef __TOBJECT_HPP_INCLUDED
#define __TOBJECT_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

namespace	usr {
    class System;
    class TConsole;

    /** Describes window-related coordinate type */
    typedef unsigned tRelCoord;

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
    TConsole*	__FASTCALL__ twInit(System& system,const std::string& user_cp, unsigned long vio_flags, unsigned long twin_flgs );

		   /** Terminates twin library and video subsystem
		     * @return            none
		     * @note              Call this function after all other
		     * @see               twInit
		    **/
    void	__FASTCALL__ twDestroy();

    /** Internal structure of text window */
    class TObject : public Opaque {
	public:
	    enum twc_flag {
		Flag_None	=0x0000, /**< Indicates no flags */
		Flag_Has_Frame	=0x0001, /**< Indicates that window has frame border */
		Flag_Has_Cursor	=0x0002, /**< Indicates that window has text cursor */
		Flag_NLS	=0x0100  /**< Indicates that window works in OEM mode */
	    };

	    enum twi_flag {
		Visible		=0x00000001UL,
		Enabled		=0x00000002UL,
		CursorBeenOff	=0x80000000UL
	    };

		   /** Creates window
		     * @param x1_,y1_      indicate upper-left cornen of window
		     * @param width,height indicate width and height of window
		     * @param flags        indicates TWS_* flags
		     * @return             handle of window
		    **/
	    TObject(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, twc_flag flags=Flag_None);

		   /** Creates extended window
		     * @param x1_,y1_      indicate upper-left cornen of window
		     * @param width,height indicate width and height of window
		     * @param flags        indicates TWS_* flags
		     * @param classname    indicates name of class
		     * @return             handle of window
		     * @note               name of class must be registered before
					   calling of this function
		    **/
	    TObject(tAbsCoord x1_, tAbsCoord y1_,
			tAbsCoord width, tAbsCoord height,
			twc_flag flags, const std::string& classname);

		   /** Destroys given window
		     * @param              handle of window
		     * @return             none
		    **/
	    virtual ~TObject();


		   /** Calls window function with given arguments
		     * @return                function answer
		     * @param event           one of WM_* commands
		     * @param event_param     command related parameter
		     * @param event_data      command related data
		    **/
	    virtual long		send_message(unsigned event,unsigned long event_param,const any_t*event_data);

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
	    virtual void		show_beneath(TObject& prev);

		   /** Shows given window on the top of the window stack.
		     * @param win          handle of window
		     * @return             none
		     * @note               Does change Z-order of window
		    **/
	    virtual void		show_on_top();

		   /** Set cursor into the given window.
		     * @param win          handle of window to be used
		     * @return             focus of previous window or NULL if none
		     * @note               \e win need not be the top window; not
		     *                     does \e win even have to be visible on
		     *                     the screen.
		    **/
	    virtual TObject*		set_focus();

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

		   /** Does centring of given window relatively other window or screen.
		     * @param it           handle of window to be centred
		     * @param parent       handle of parent window.
		     * @return             none
		     * @note               If \e parent is NULL, the window will
		     *                     be centred for screen
		    **/
	    virtual void		into_center(const TObject& parent);

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
	    virtual any_t*		get_user_data() const __PURE_FUNC__;

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
	    virtual unsigned		width() const __PURE_FUNC__;

		   /** Returns width of height.
		     * @param win          handle of window.
		     * @return             height of window
		    **/
	    virtual unsigned		height() const __PURE_FUNC__;

		   /** Returns width of window which is available for output.
		     * @param win          handle of window.
		     * @return             width of window which is available for output
		    **/
	    virtual unsigned		client_width() const __PURE_FUNC__;

		   /** Returns height of window which is available for output.
		     * @param win          handle of window.
		     * @return             height of window which is available for output
		    **/
	    virtual unsigned		client_height() const __PURE_FUNC__;

		   /** Converts window-relative coordinates to screen coordinates
		     * @param win          handle of window
		     * @param x,y          specify the relative coordinates.
		     * @param xs,ys        specify the screen coordinates.
		     * @return             none
		    **/
	    virtual void		cvt_win_coords(tRelCoord x, tRelCoord y,tAbsCoord& xs,tAbsCoord& ys) const __PURE_FUNC__;

		   /** Converts screen-relative coordinates to relative window coordinates
		     * @param win          handle of window
		     * @param x,y          specify screen coordinates.
		     * @param xr,yr        specify pointers to the relative coordinates.
		     * @return             true if successful, false otherwise.
		    **/
	    virtual bool		cvt_screen_coords(tAbsCoord x, tAbsCoord y,tRelCoord& xr,tRelCoord& yr) const __PURE_FUNC__;
		   /** Checks visibility of window piece.
		     * @param x,y          specify coordinates of the location.
		     * @return             true if specified window is visible
		     *                     in specified location (Is not obscured).
		    **/
	    virtual bool		is_piece_visible(tRelCoord x, tRelCoord y) const __PURE_FUNC__;

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
	    virtual tRelCoord		where_x() const __PURE_FUNC__;

		   /** Returns the y coordinate of the current cursor position, within currently used window.
		     * @return            none
		     * @note              If X or Y are outside the window frame,
		     *                    they will be clipped.
		    **/
	    virtual tRelCoord		where_y() const __PURE_FUNC__;

/* Static members */
		   /** Returns the window currently being under focus.
		     * @return             handle of window currently being used
		     * @note               If no window has focus then return NULL
		    **/
	    static TObject*		get_focus() __PURE_FUNC__;
		   /** Returns handle of the window currently displayed at the screen position x and y.
		     * @param x,y          specify coordinates of the location.
		     * @return             Handle for the window displayed at the
		     *                     specified position if a window is
		     *                     displayed. If no window is displayed
		     *                     at the this position, NULL is returned.
		    **/
	    static TObject*		at_pos(tAbsCoord x, tAbsCoord y) __PURE_FUNC__;

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

	    static TConsole*	tconsole;
	    static System*	msystem;

	    static unsigned long twin_flags;
	protected:
	    void		create(tAbsCoord x1_, tAbsCoord y1_, tAbsCoord width, tAbsCoord height, twc_flag flags);

	    TObject*		__prevwin();
	    TObject*		__find_over(tAbsCoord x,tAbsCoord y) const __PURE_FUNC__;
	    static TObject*	__findcursorablewin() __PURE_FUNC__;
	    static TObject*	__at_point(TObject* iter,tAbsCoord x,tAbsCoord y) __PURE_FUNC__;
	    inline bool		__topmost() const __PURE_FUNC__ { return !is_overlapped(); }

	    inline bool		is_valid_xy(tAbsCoord x,tAbsCoord y) const __CONST_FUNC__ {
					return ((flags & Flag_Has_Frame) == Flag_Has_Frame ?
						x && x < wwidth-1 && y && y < wheight-1 :
						x < wwidth && y < wheight); }
	    inline bool		is_valid_x(tAbsCoord x) const { return ((flags & Flag_Has_Frame) == Flag_Has_Frame ? x && x < wwidth-1: x < wwidth); }
	    inline bool		is_valid_y(tAbsCoord y) const { return ((flags & Flag_Has_Frame) == Flag_Has_Frame ? y && y < wheight-1:y < wheight); }

	    void		set_xy(tRelCoord x,tRelCoord y);
	    void		igoto_xy(tRelCoord x,tRelCoord y);

	    twc_flag		flags;       /**< Window flags */
	    twi_flag		iflags;      /**< contains internal flags of window state */
	    unsigned		wwidth;      /**< width of window */
	    unsigned		wheight;     /**< height of window */
	    tAbsCoord		X1,Y1,X2,Y2; /**< coordinates of window on the screen */
	    tRelCoord		cur_x,cur_y; /**< coordinates of cursor position inside the window */
	private:
	    void		__unlistwin();
	    bool		is_overlapped() const __PURE_FUNC__;
	    void		paint_cursor() const;
	    void		__athead();
	    void		into_center(tAbsCoord w,tAbsCoord h);

	    inline void		__atwin(TObject* prev) { if(!prev) __athead(); else { next = prev->next; prev->next = this; }}

	    TObject*		next;        /**< pointer to next window in list */
	    any_t*		usrData;     /**< user data pointer */
	    any_t*		method;      /**< Class callback */
	    unsigned		class_flags; /**< Class flags */

	    static TObject*	head;
	    static TObject*	cursorwin;
	    static e_cursor	c_type;
    };
    inline TObject::twc_flag operator~(TObject::twc_flag a) { return static_cast<TObject::twc_flag>(~static_cast<unsigned>(a)); }
    inline TObject::twc_flag operator|(TObject::twc_flag a, TObject::twc_flag b) { return static_cast<TObject::twc_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline TObject::twc_flag operator&(TObject::twc_flag a, TObject::twc_flag b) { return static_cast<TObject::twc_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline TObject::twc_flag operator^(TObject::twc_flag a, TObject::twc_flag b) { return static_cast<TObject::twc_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline TObject::twc_flag operator|=(TObject::twc_flag& a, TObject::twc_flag b) { return (a=static_cast<TObject::twc_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline TObject::twc_flag operator&=(TObject::twc_flag& a, TObject::twc_flag b) { return (a=static_cast<TObject::twc_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline TObject::twc_flag operator^=(TObject::twc_flag& a, TObject::twc_flag b) { return (a=static_cast<TObject::twc_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }

    inline TObject::twi_flag operator~(TObject::twi_flag a) { return static_cast<TObject::twi_flag>(~static_cast<unsigned>(a)); }
    inline TObject::twi_flag operator|(TObject::twi_flag a, TObject::twi_flag b) { return static_cast<TObject::twi_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }
    inline TObject::twi_flag operator&(TObject::twi_flag a, TObject::twi_flag b) { return static_cast<TObject::twi_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b)); }
    inline TObject::twi_flag operator^(TObject::twi_flag a, TObject::twi_flag b) { return static_cast<TObject::twi_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b)); }
    inline TObject::twi_flag operator|=(TObject::twi_flag& a, TObject::twi_flag b) { return (a=static_cast<TObject::twi_flag>(static_cast<unsigned>(a)|static_cast<unsigned>(b))); }
    inline TObject::twi_flag operator&=(TObject::twi_flag& a, TObject::twi_flag b) { return (a=static_cast<TObject::twi_flag>(static_cast<unsigned>(a)&static_cast<unsigned>(b))); }
    inline TObject::twi_flag operator^=(TObject::twi_flag& a, TObject::twi_flag b) { return (a=static_cast<TObject::twi_flag>(static_cast<unsigned>(a)^static_cast<unsigned>(b))); }
} // namespace	usr
#include "libbeye/tw_class.h"

#endif






