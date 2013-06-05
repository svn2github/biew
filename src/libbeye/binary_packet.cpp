#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include <stdexcept>

#include "binary_packet.h"

namespace	usr {
binary_packet::binary_packet(size_t sz)
	    :buffer(NULL)
	    ,len(sz)
{
    if(sz) buffer = new char[sz];
}

binary_packet::binary_packet(const binary_packet& from)
	    :buffer(NULL)
	    ,len(from.len)
{
    if(from.len) {
	buffer=new char[from.length()];
	::memcpy(buffer,from.data(),len);
    }
}

binary_packet&	binary_packet::operator=(const binary_packet& from) { return assign(from.buffer,from.len); }
binary_packet&	binary_packet::operator+=(const binary_packet& from) { return append(from); }

binary_packet&	binary_packet::assign(const any_t* _data,size_t sz) {
    if(buffer) { delete (char*)buffer; buffer=NULL; }
    len=sz;
    if(sz) {
	buffer = new char[sz];
	::memcpy(buffer,_data,sz);
    }
    return *this;
}

binary_packet&	binary_packet::append(const any_t* _data,size_t sz) {
    if(sz) {
	buffer = mp_realloc(buffer,len+sz);
	::memcpy(&((char*)buffer)[len],_data,sz);
    }
    len+=sz;
    return *this;
}

binary_packet&	binary_packet::append(const binary_packet& it) { return append(it.buffer,it.len); }

void binary_packet::clear() {
    resize(0);
}
void binary_packet::resize(size_t newsz) {
    len=newsz;
    if(newsz) buffer=mp_realloc(buffer,newsz);
    else { delete (char*)buffer; buffer=NULL; }
}

char& binary_packet::operator[](size_t idx) { return at(idx); }
const char& binary_packet::operator[](size_t idx) const { return at(idx); }

char&		binary_packet::front() { return ((char*)buffer)[0]; }
const char&	binary_packet::front() const { return ((const char*)buffer)[0]; }
char&		binary_packet::back() { return ((char*)buffer)[len-1]; }
const char&	binary_packet::back() const { return ((const char*)buffer)[len-1]; }

char& binary_packet::at(size_t idx) {
    if(idx<len) return ((char*)buffer)[idx];
    throw std::out_of_range("binary_packet");
}

const char& binary_packet::at(size_t idx) const {
    if(idx<len) return ((const char*)buffer)[idx];
    throw std::out_of_range("binary_packet");
}

binary_packet::~binary_packet() { if(buffer) delete (char*)buffer; }
} // namespace	usr
