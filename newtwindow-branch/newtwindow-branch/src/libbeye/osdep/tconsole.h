#ifndef __TCONSOLE_HPP_INCLUDED
#define __TCONSOLE_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

namespace	usr {
    class TConsole {
	public:
	    TConsole(const char *user_cp, unsigned long vio_flags);
	    virtual ~TConsole();

		   /** Receives next keyboard event or note about mouse event.
		     * @return                event. For detail see KE_* flag definitions in keycode.h file.
		     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
		     * @note                  This function only sends note about
		     *                        occured mouse event. For detailizing
		     *                        event you must call one of__Ms*
		     *                        function family.
		     * @see                   __kbdTestKey __kbdGetShiftKey
		    **/
	    int			kbd_get_key( unsigned long flg ) const;

		   /** Checks the keyboard and mouse to determine whether there is available event.
		     * @return                event if available, 0 - otherwise. For detail see KE_* flag definitions in keycode.h file.
		     * @param flg             Indicates how to react on mouse events. See KBD_* flag definitions.
		     * @note                  This function only sends note about
		     *                        occured mouse event. For detailizing
		     *                        event you must call one of__Ms*
		     *                        function family.
		     * @see                   __kbdGetKey __kbdGetShiftKey
		    **/
	    int 		kbd_test_key( unsigned long flg ) const;

		   /** Determines the current SHIFT key status.
		     * @return                Current SHIFT key state. For detail see KS_* flag definitions in keycode.h file.
		     * @see                   __kbdTestKey __kbdGetKey
		    **/
	    int			kbd_get_shifts() const;

		   /** Returns mouse cursor visibility.
		     * @return                true if mouse cursor is in visible state
		     * @see                   __MsGetPos __MsGetBtns __MsSetState
		    **/
	    bool		mouse_get_state() const;

		   /** Sets mouse cursor visibility.
		     * @return                none
		     * @param is_visible      Indicates visibility of mouse cursor
		     * @see                   __MsGetState
		    **/
	    void		mouse_set_state( bool is_visible ) const;

		   /** Returns mouse position in screen coordinates.
		     * @return                none
		     * @param x,y             Pointers to memory where will be stored current mouse coordinates.
		     * @note                  Coordinates is measured in text cells of screen
		     * @see                   __MsGetBtns __MsGetState
		    **/
	    void		mouse_get_pos( tAbsCoord& x, tAbsCoord& y ) const;

		   /** Returns mouse buttons state.
		     * @return                State of mouse button. For detail see MS_ flag definitions.
		     * @see                   __MsGetState __MsGetPos
		    **/
	    int			mouse_get_buttons() const;


		   /** Returns type of cursor.
		     * @return                cursor type. For detail see __TVIO_CUR* flag definitions.
		     * @see                   __vioSetCursorType
		    **/
	    int			vio_get_cursor_type() const;

		   /** Sets type of cursor.
		     * @return                none
		     * @param c_type          Type of cursor. For detail see __TVIO_CUR* flag definitions
		     * @see                   __vioGetCursorType
		    **/
	    void		vio_set_cursor_type( int c_type ) const;

		   /** Returns cursor position.
		     * @return                none
		     * @param x,y             pointers to memory where will be stored current cursor coordinates.
		     * @note                  coordinates of cursor is measured in text cells
		     * @see                   __vioSetCursorPos
		    **/
	    void		vio_get_cursor_pos(tAbsCoord& x,tAbsCoord& y) const;

		   /** Sets cursor position.
		     * @return                none
		     * @param x,y             indicate cursor coordinates.
		     * @note                  coordinates of cursor is measured in text cells
		     * @see                   __vioGetCursorPos
		    **/
	    void		vio_set_cursor_pos(tAbsCoord x,tAbsCoord y) const;

		   /** Reads buffer from console (or terminal) video memory at given offset.
		     * @return                none
		     * @param x,y             indicate x and y coordinates at which buffer must be readed
		     * @param buff            pointer to logical video buffer where will be stored readed information
		     * @param len             indicates length of buffer
		     * @note                  When program is run in terminal
		     *                        mode then information will readed from
		     *                        console emulator but not from terminal
		     *                        directly.
		     * @see                   __tvioWriteBuff
		    **/
	    tvideo_buffer		vio_read_buff(tAbsCoord x,tAbsCoord y,size_t len) const;

		   /** Writes buffer to console (or terminal) video memory at given offset.
		     * @return                none
		     * @param x,y             indicate x and y coordinates at which buffer must be readed
		     * @param buff            pointer to logical video buffer to be written
		     * @param len             indicates length of buffer
		     * @see                   __tvioReadBuff
		    **/
	    void		vio_write_buff(tAbsCoord x,tAbsCoord y,const tvideo_buffer& buff) const;

	    tAbsCoord		vio_width() const __PURE_FUNC__;     /**< Contains actual width of console (or terminal) */
	    tAbsCoord		vio_height() const __PURE_FUNC__;    /**< Contains actual height of console (or terminal) */
	    unsigned		vio_num_colors() const __PURE_FUNC__;/**< Contains number of actual colors that used by console (or terminal) */

		   /** Sets color to be transparented
		     * @return                none
		     * @param value           indicates logical number of physical color to be transparented
		     * @note                  Function works only if current
		     *                        terminal has support of transparent
		     *                        colors. Returns immediately otherwise.
		    **/
	    void		vio_set_transparent_color(unsigned char value) const;

		   /** Describes info about input events.
		     * @param head            Pointer to header to be filled
		     * @param text            Pointer to formatted text to be filled
		     * @return                0 if ESCAPE's functionality key pressed;
					      1 if regulare event occured
					      -1 if functionality is not implemented(supported).
		    **/
	    int       __FASTCALL__ input_raw_info(char *head, char *text) const;

    };
} // namespace	usr
#endif
