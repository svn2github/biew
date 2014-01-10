#ifndef BINARY_PACKET_HPP_INCLUDED
#define BINARY_PACKET_HPP_INCLUDED 1
#include "libbeye/libbeye.h"
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace	usr {
    class binary_packet : public Opaque {
	public:
	    binary_packet(size_t size);
	    binary_packet(const any_t* src,size_t size);
	    binary_packet(const binary_packet& from);
	    virtual ~binary_packet();

	    binary_packet&	operator=(const binary_packet& from);
	    binary_packet&	operator+=(const binary_packet& from);
	    binary_packet	operator+(const binary_packet& rhs) const;
	    uint8_t&		operator[](size_t idx);
	    const uint8_t&	operator[](size_t idx) const;

	    bool		operator==(const binary_packet& from) const;
	    bool		operator!=(const binary_packet& from) const;
	    bool		operator<(const binary_packet& from) const;
	    bool		operator<=(const binary_packet& from) const;
	    bool		operator>(const binary_packet& from) const;
	    bool		operator>=(const binary_packet& from) const;

	    virtual binary_packet&	assign(const any_t* data,size_t len);
	    virtual binary_packet&	append(const any_t* data,size_t len);
	    virtual binary_packet&	append(const binary_packet& it);
	    virtual binary_packet&	insert(size_t pos,const any_t* data,size_t len);
	    virtual binary_packet&	insert(size_t pos,const binary_packet& it);
	    virtual binary_packet&	replace(size_t pos,const any_t* data,size_t len);
	    virtual binary_packet&	replace(size_t pos,const binary_packet& it);
	    virtual binary_packet&	remove(size_t pos,size_t n);
	    virtual uint8_t&		at(size_t idx);
	    virtual const uint8_t&	at(size_t idx) const;
	    virtual uint8_t&		front();
	    virtual const uint8_t&	front() const;
	    virtual uint8_t&		back();
	    virtual const uint8_t&	back() const;

	    virtual binary_packet	subpacket(size_t start,size_t length) const;

	    virtual bool		empty() const { return len==0; }
	    virtual binary_packet&	clear();
	    virtual binary_packet&	resize(size_t newsz);
	    virtual size_t		size() const { return len; }
	    virtual any_t*		data() { return buffer; }
	    virtual const any_t*	data() const { return buffer; }
	    virtual uint8_t*		cdata() { return (uint8_t*)buffer; }
	    virtual const uint8_t*	cdata() const { return (uint8_t*)buffer; }
	private:
	    any_t*	buffer;
	    size_t	len;
    };

    std::ostream&		operator<<(std::ostream& os,const binary_packet&);
    std::istream&		operator>>(std::istream& is,binary_packet&); // use std::setw() or istream.width() before using this operator

    template<class T>
    class objects_container : public binary_packet {
	public:
	    objects_container(size_t sz):binary_packet(sz) {}
	    objects_container(const any_t* src,size_t sz):binary_packet(src,sz) {}
	    objects_container(const objects_container& from):binary_packet(from) {}
	    virtual ~objects_container() {}

	    T& operator[](size_t idx) {
		if((idx+1)*sizeof(T)<=size()) return ((T*)data())[idx];
		std::ostringstream os;
		os<<"."<<get_caller_address()<<" => objects_container<"<<typeid(T).name()<<">["<<idx<<"]";
		throw std::out_of_range(os.str());
	    }

	    const T& operator[](size_t idx) const {
		if((idx+1)*sizeof(T)<=size()) return ((const T*)data())[idx];
		std::ostringstream os;
		os<<"."<<get_caller_address()<<" => objects_container"<<typeid(T).name()<<">["<<idx<<"] const";
		throw std::out_of_range(os.str());
	    }

	    virtual T*		tdata() { return (T*)data(); }
	    virtual const T*	tdata() const { return (const T*)data(); }
    };
} // namespace	usr
#endif
