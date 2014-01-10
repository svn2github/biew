#ifndef __TOBJECT_EVENT_HPP_INCLUDED
#define __TOBJECT_EVENT_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

namespace	usr {
    class to_event {
	public:
	    /* Below located list of TObject events. */
	    enum event_type {
		Null		=0x0000, /**< Never must be send */
		Create		=0x0001, /**< It sent when TObject has been created, has no parameters */
		Destroy		=0x0002, /**< It sent when TObject is being destroyed, has no parameters*/
		Show		=0x0003, /**< It sent when TObject has been displayed, has no parameters*/
		Top_Show	=0x0004, /**< It sent when TObject has been displayed on top of all windows, has no parameters*/
		Show_Beneath	=0x0005, /**< It sent when TObject has been displayed beneath of other window, has handle of top window as event_data*/
		Hide		=0x0006, /**< It sent when TObject has been hidded, has no parameters */

		Max		=0x0007 /**< Never must be send */
	    };
	    to_event(to_event::event_type);
	    virtual ~to_event();

	    virtual to_event::event_type		get_event() const;
	private:
	    to_event::event_type			event;
    };
} // namespace	usr
#endif