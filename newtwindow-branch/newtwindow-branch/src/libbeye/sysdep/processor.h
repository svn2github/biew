#ifndef __PROCESSOR_HPP_INCLUDED
#define __PROCESSOR_HPP_INCLUDED 1
#include "libbeye/libbeye.h"
namespace	usr {
    class Processor {
	public:
	    Processor();
	    virtual ~Processor();

		   /** Fills buffer with information about CPU in free form.
		     * @return                none
		     * @param buff            buffer to be filled
		     * @param cbBuff          size of buffer
		     * @param percents_callback pointer to the function that will be used to indicate execution progress
		    **/
	    void		cpu_info(char *buff,unsigned cbBuff,void (*percents_callback)(int)) const;
    };
} // namespace	usr
#endif
