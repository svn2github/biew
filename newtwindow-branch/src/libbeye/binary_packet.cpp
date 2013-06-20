#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

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
	buffer=new char[from.len];
	::memcpy(buffer,from.buffer,len);
    }
}

binary_packet::binary_packet(const any_t* src,size_t sz)
	    :buffer(NULL)
	    ,len(sz)
{
    if(sz) {
	buffer=new char[sz];
	::memcpy(buffer,src,sz);
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

binary_packet&	binary_packet::insert(size_t pos,const any_t* sdata,size_t slen) {
    resize(len+slen);
    memmove(&((uint8_t*)buffer)[pos+slen],&((uint8_t*)buffer)[pos],slen);
    memcpy(&((uint8_t*)buffer)[pos],sdata,slen);
    return *this;
}

binary_packet&	binary_packet::insert(size_t pos,const binary_packet& it) { return insert(pos,it.buffer,it.len); }

binary_packet&	binary_packet::replace(size_t pos,const any_t* sdata,size_t slen) {
    if(pos+slen>len) {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => binary_packet::replace("<<pos<<","<<slen<<")";
	throw std::out_of_range(os.str());
    }
    memcpy(&((uint8_t*)buffer)[pos],sdata,slen);
    return *this;
}

binary_packet&	binary_packet::replace(size_t pos,const binary_packet& it) { return replace(pos,it.buffer,it.len); }

binary_packet&	binary_packet::remove(size_t pos,size_t n) {
    if(pos+n==len) resize(len-n);
    else if(pos+n<len){
	memmove(&((uint8_t*)buffer)[pos],&((uint8_t*)buffer)[pos+n],n);
	resize(len-n);
    } else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => binary_packet::remove("<<pos<<","<<n<<")";
	throw std::out_of_range(os.str());
    }
    return *this;
}

binary_packet	binary_packet::operator+(const binary_packet& rhs) const {
    binary_packet rc(len+rhs.len);
    ::memcpy(rc.buffer,buffer,len);
    ::memcpy(&((char*)rc.buffer)[len],rhs.buffer,rhs.len);
    return rc;
}

binary_packet& binary_packet::clear() {
    return resize(0);
}
binary_packet& binary_packet::resize(size_t newsz) {
    len=newsz;
    if(newsz) buffer=mp_realloc(buffer,newsz);
    else { delete (char*)buffer; buffer=NULL; }
    return *this;
}

uint8_t& binary_packet::operator[](size_t idx) { return at(idx); }
const uint8_t& binary_packet::operator[](size_t idx) const { return at(idx); }

bool binary_packet::operator==(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)==0;
    return false;
}
bool binary_packet::operator!=(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)!=0;
    return true;
}
bool binary_packet::operator<(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)<0;
    return len<from.len;
}
bool binary_packet::operator<=(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)<=0;
    return len<from.len;
}
bool binary_packet::operator>(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)>0;
    return len>from.len;
}
bool binary_packet::operator>=(const binary_packet& from) const {
    if(len==from.len) return ::memcmp(buffer,from.buffer,len)>=0;
    return len>from.len;
}

uint8_t&	binary_packet::front() { return ((uint8_t*)buffer)[0]; }
const uint8_t&	binary_packet::front() const { return ((const uint8_t*)buffer)[0]; }
uint8_t&	binary_packet::back() { return ((uint8_t*)buffer)[len-1]; }
const uint8_t&	binary_packet::back() const { return ((const uint8_t*)buffer)[len-1]; }

uint8_t& binary_packet::at(size_t idx) {
    if(idx<len) return ((uint8_t*)buffer)[idx];
    std::ostringstream os;
    os<<"."<<get_caller_address()<<" => binary_packet::at("<<idx<<")";
    throw std::out_of_range(os.str());
}

const uint8_t& binary_packet::at(size_t idx) const {
    if(idx<len) return ((const uint8_t*)buffer)[idx];
    std::ostringstream os;
    os<<"."<<get_caller_address()<<" => binary_packet::at("<<idx<<")";
    throw std::out_of_range(os.str());
}

binary_packet binary_packet::subpacket(size_t start,size_t length) const {
    if(start+length>len) {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => binary_packet::subpacket("<<start<<","<<length<<")";
	throw std::out_of_range(os.str());
    }
    binary_packet rc(length);
    ::memcpy(rc.buffer,&((char*)buffer)[start],length);
    return rc;
}

std::ostream& operator<<(std::ostream& os,const binary_packet& bp) {
    os.write((const char*)bp.data(),bp.size());
    return os;
}

std::istream& operator>>(std::istream& is,binary_packet& bp) {
    bp.resize(is.width());
    is.read((char*)bp.data(),bp.size());
    return is;
}

binary_packet::~binary_packet() { if(buffer) delete (char*)buffer; }
} // namespace	usr
