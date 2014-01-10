#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
#include <sstream>
#include <iomanip>

#include "tvideo_buffer.h"

namespace	usr {
tvideo_symbol::tvideo_symbol():_symbol(0),_oempg(0),_attr(0) {}
tvideo_symbol::tvideo_symbol(t_vchar s,t_vchar o,ColorAttr a):_symbol(s),_oempg(o),_attr(a) {}
tvideo_symbol::~tvideo_symbol() {}

t_vchar tvideo_symbol::symbol() const { return _symbol; }
t_vchar tvideo_symbol::oempg() const { return _oempg; }
ColorAttr tvideo_symbol::attr() const { return _attr; }

tvideo_buffer::tvideo_buffer(size_t n)
	    :data(n)
{
}

tvideo_buffer::tvideo_buffer(const tvideo_symbol& s,size_t n)
	    :data(n,s)
{
}

tvideo_buffer::tvideo_buffer(const std::vector<tvideo_symbol>& v)
	    :data(v)
{
}

tvideo_buffer::tvideo_buffer(const tvideo_buffer& it)
	    :data(it.data)
{
}

tvideo_buffer::~tvideo_buffer() {}

tvideo_buffer& tvideo_buffer::operator=(const tvideo_buffer& it) {
    data = it.data;
    return *this;
}

tvideo_buffer tvideo_buffer::sub_buffer(size_t idx) const {
    size_t len=data.size()-idx;
    tvideo_buffer rc(len);
    for(size_t i=0;i<len;i++) rc.data[i]=data[idx+i];
    return rc;
}

tvideo_buffer tvideo_buffer::sub_buffer(size_t idx,size_t len) const {
    tvideo_buffer rc(len);
    for(size_t i=0;i<len;i++) rc.data[i]=data[idx+i];
    return rc;
}

const tvideo_symbol&	tvideo_buffer::operator[](size_t idx) const {
    if(idx>=data.size()) {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::operator["<<idx<<"] const";
	throw std::out_of_range(os.str());
    }
    return data[idx];
}
tvideo_symbol&		tvideo_buffer::operator[](size_t idx) {
    if(idx>=data.size()) {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::operator["<<idx<<"]";
	throw std::out_of_range(os.str());
    }
    return data[idx];
}

void tvideo_buffer::resize(size_t newlen) {
    if(data.size()!=newlen) {
	if(newlen) data.resize(newlen,tvideo_symbol());
	else data.clear();
    }
}

void tvideo_buffer::fill(const tvideo_symbol& s) {
    data.assign(data.size(),s);
}

void tvideo_buffer::fill_at(size_t idx,const tvideo_symbol& s,size_t sz) {
    if(idx+sz < data.size()) {
	for(size_t i=0;i<sz;i++) data[idx+i] = s;
    } else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::fill_at("<<idx<<","<<sz<<")";
	throw std::out_of_range(os.str());
    }
}

void tvideo_buffer::assign(const std::vector<tvideo_symbol>& v) {
    size_t rlen = std::min(v.size(),data.size());
    for(size_t i=0;i<rlen;i++) data[i] = v[i];
}

void tvideo_buffer::assign(const tvideo_buffer& from,size_t rlen) {
    rlen = std::min(rlen,data.size());
    for(size_t i=0;i<rlen;i++) data[i] = from[i];
}

void tvideo_buffer::assign_at(size_t idx,const tvideo_buffer& from) {
    size_t sz=from.size();
    if(sz+idx<data.size()) {
	for(size_t i=0;i<sz;i++) data[idx+i] = from[i];
    } else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::assign_at("<<idx<<")";
	throw std::out_of_range(os.str());
    }
}

void tvideo_buffer::assign_at(size_t idx,const tvideo_buffer& from,size_t rlen) {
    size_t sz=rlen;
    if(sz+idx<data.size()) {
	for(size_t i=0;i<sz;i++) data[idx+i] = from[i];
    } else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::assign_at("<<idx<<","<<rlen<<")";
	throw std::out_of_range(os.str());
    }
}

void tvideo_buffer::assign_at(size_t idx,const std::vector<tvideo_symbol>& v) {
    size_t newlen=v.size();
    if(idx+newlen < data.size()) {
	for(size_t i=0;i<newlen;i++) data[i+idx]=v[i];
    } else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::assign_at("<<idx<<")";
	throw std::out_of_range(os.str());
    }
}

void tvideo_buffer::assign_at(size_t idx,tvideo_symbol s) {
    if(idx<data.size()) data[idx] = s;
    else {
	std::ostringstream os;
	os<<"."<<get_caller_address()<<" => tvideo_buffer::assign_at("<<idx<<")";
	throw std::out_of_range(os.str());
    }
}
} // namespace	usr
