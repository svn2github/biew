#ifndef SYSTEM_INTERFACE_HPP_INCLUDED
#define SYSTEM_INTERFACE_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

namespace	usr {
    typedef void timer_callback(); /**< This is the code type used to represent user supplied function of timer callback */
    class system_interface : public Opaque {
	public:
	    system_interface() {}
	    virtual ~system_interface() {}

		   /** Realizes time slice between waiting of input events
		     * @return                none
		     * @note                  This function provides mechanism of
		     *                        system speedup, when application is
		     *                        waiting for input events. For realizing
		     *                        time slice inside of computing loops
		     *                        application must call SLEEP or it
		     *                        analogs.
		    **/
	    virtual void		yield_timeslice() const = 0;

		   /** Builds OS specific name of home directory
		     * @return                Slash terminated path to home directory
		     * @param progname        indicates name of program, that can be used to revert to a fallback in case the path gets too long
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_home_dir
		    **/
	    virtual std::string		get_home_dir(const std::string& progname) = 0;

		   /** Builds OS specific name of initializing file
		     * @return                fully qualified name of .ini file
		     * @param progname        indicates name of program, that will be used as part of .ini file name
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_rc_dir
		    **/
	    virtual std::string		get_ini_name(const std::string& progname) = 0;

		   /** Builds OS specific name of program resource directory
		     * @return                Slash terminated path to program resource directory
		     * @param progname        indicates name of program.
		     * @note                  Best way it always pass to function
		     *                        argv[0] program argument.
		     * @see                   __get_ini_name
		    **/
	    virtual std::string		get_rc_dir(const std::string& progname) = 0;

		   /** Sets user defined function as timer callback with given time interval
		     * @return                Real call back interval in milliseconds
		     * @param ms              indicates desired call back interval in milliseconds
		     * @param func            indicates user supplied function to be used as timer callback
		     * @see                   __OsRestoreTimer
		    **/
	    virtual unsigned		set_timer_callback(unsigned ms,timer_callback *func) = 0;

		   /** Restores time callback function to original state
		     * @return                none
		     * @see                   __OsSetTimercallBack
		    **/
	    virtual void		restore_timer() const = 0;

/* National Language Support */

		   /** Converts buffer from OEM codepage to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_CmdlineToOem __nls_CmdlineToFs
		    **/
	    virtual void		nls_oem2osdep(unsigned char *str,unsigned size) const = 0;

		   /** Converts buffer from codepage of command line to currently used by OS.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @depricated            This function only used in Win32
		     *                        where codepages of console and command
		     *                        line is differ.
		     * @see                   __nls_OemToOsdep __nls_CmdlineToFs
		    **/
	    virtual void		nls_cmdline2oem(unsigned char *str,unsigned size) const = 0;

		   /** Converts buffer from OEM codepage to currently used by OS's file system.
		     * @return                none
		     * @param str             buffer to be converted
		     * @param size            size of buffer in bytes
		     * @see                   __nls_OemToOsdep __nls_CmdlineToOem
		    **/
	    virtual void		nls_oem2fs(unsigned char *str,unsigned size) const = 0;
    };

    struct system_interface_info {
	const char*		name;
	system_interface*	(*query_interface)();
    };
} // namespace	usr
#endif
