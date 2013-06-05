#ifndef BINARY_PACKET_HPP_INCLUDED
#define BINARY_PACKET_HPP_INCLUDED 1
#include "libbeye/libbeye.h"

namespace	usr {
    class binary_packet : public Opaque {
	public:
	    binary_packet(size_t size);
	    binary_packet(const binary_packet& from);
	    virtual ~binary_packet();

	    binary_packet&	operator=(const binary_packet& from);
	    binary_packet&	operator+=(const binary_packet& from);
	    char&		operator[](size_t idx);
	    const char&		operator[](size_t idx) const;

	    virtual binary_packet&	assign(const any_t* data,size_t len);
	    virtual binary_packet&	append(const any_t* data,size_t len);
	    virtual binary_packet&	append(const binary_packet& it);
	    char&			at(size_t idx);
	    const char&			at(size_t idx) const;
	    char&			front();
	    const char&			front() const;
	    char&			back();
	    const char&			back() const;

	    virtual bool		empty() const { return len==0; }
	    virtual void		clear();
	    virtual void		resize(size_t newsz);
	    virtual size_t		length() const { return len; }
	    virtual any_t*		data() { return buffer; }
	    virtual const any_t*	data() const { return buffer; }
	    virtual char*		cdata() { return (char*)buffer; }
	    virtual const char*		cdata() const { return (char*)buffer; }
	private:
	    any_t*	buffer;
	    size_t	len;
    };
} // namespace	usr
#endif
