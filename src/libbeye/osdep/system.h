#ifndef __SYSTEM_HPP_INCLUDED
#define __SYSTEM_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

#include <string>

namespace	usr {
    typedef void timer_callback(); /**< This is the code type used to represent user supplied function of timer callback */
    class System {
	public:
	    System();
		virtual ~System();

		   /** Realizes time slice between waiting of input events
		     * @return                none
		     * @note                  This function provides mechanism of
		     *                        system speedup, when application is
		     *                        waiting for input events. For realizing
		     *                        time slice inside of computing loops
		     *                        application must call SLEEP or it
		     *                        analogs.
		    **/
	    void		yield_timeslice() const;
		   /** Gets ctrl-break signal
		     * @return                true if occured; false - otherwise
		     * @note                  Function does not differ soft and
		     *                        hard c-break.
		     * @warning               After getting signal by program
					      semaphore is not being cleaned.
		    **/
	    bool		get_cbreak() const;

		   /** Sets control-break signal
		     * @return                none
		     * @param state           indicates new state of
					      control-break semaphore
		     * @note                  Function does not differ soft and
		     *                        hard c-break.
		    **/
	    void		set_cbreak( bool state ) const;

		   /** Builds OS specific name of home directory
		     * @return                Slash terminated path to home directory
		     * @param progname        indicates name of program, that can be used to revert to a fallback in case the path gets too long
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_home_dir
		    **/
	    std::string		get_home_dir(const std::string& progname) const;

		   /** Builds OS specific name of initializing file
		     * @return                fully qualified name of .ini file
		     * @param progname        indicates name of program, that will be used as part of .ini file name
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_rc_dir
		    **/
	    std::string		get_ini_name(const std::string& progname) const;

		   /** Builds OS specific name of program resource directory
		     * @return                Slash terminated path to program resource directory
		     * @param progname        indicates name of program.
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_ini_name
		    **/
	    std::string		get_rc_dir(const std::string& progname) const;

		   /** Sets user defined function as timer callback with given time interval
		     * @return                Real call back interval in milliseconds
		     * @param ms              indicates desired call back interval in milliseconds
		     * @param func            indicates user supplied function to be used as timer callback
		     * @see                   __OsRestoreTimer
		    **/
	    unsigned	set_timer_callback(unsigned ms,timer_callback *func) const;

		   /** Restores time callback function to original state
		     * @return                none
		     * @see                   __OsSetTimercallBack
		    **/
	    void		restore_timer() const;

/* National Language Support */

	   /** Checks whether the specified character is OEM pseudographical symbol */
	    static inline bool NLS_IS_OEMPG(unsigned char ch) __CONST_FUNC__ { return ch >= 0xB0 && ch <= 0xDF; }

		   /** Prepares tvioBuff buffer from OEM codepage to currently used by OS.
		     * @return                none
		     * @param it              buffer to be converted
		     * @param size            size of buffer elemets in bytes
		     * @see                   __nls_OemToOsdep __nls_CmdlineToOem NLS_IS_OEMPG
		    **/
	    void		nls_prepare_oem_for_vio(tvioBuff *it,unsigned size) const;

		   /** Converts buffer from OEM codepage to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_CmdlineToOem __nls_CmdlineToFs
		    **/
	    void		nls_oem2osdep(unsigned char *str,unsigned size) const;

		   /** Converts buffer from codepage of command line to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @depricated            This function only used in Win32
		     *                        where codepages of console and command
		     *                        line is differ.
		     * @see                   __nls_OemToOsdep __nls_CmdlineToFs
		    **/
	    void		nls_cmdline2oem(unsigned char *str,unsigned size) const;

		   /** Converts buffer from OEM codepage to currently used by OS's file system.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_OemToOsdep __nls_CmdlineToOem
		    **/
	    void		nls_oem2fs(unsigned char *str,unsigned size) const;
    };
} // namespace 	usr

#endif
