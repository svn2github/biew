#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include <sstream>
#include <iomanip>

#include "tobject_event.h"

namespace	usr {
to_event::to_event(to_event::event_type e)
	:event(e)
{
    if(!(e>to_event::Null && e<to_event::Max)) {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => to_event::to_event("<<e<<")";
	throw std::out_of_range(os.str());
    }
}
to_event::~to_event() {}

to_event::event_type to_event::get_event() const { return event; }
} // namespace	usr
