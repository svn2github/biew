#ifndef __TVIDEO_BUFFER_HPP_INCLUDED
#define __TVIDEO_BUFFER_HPP_INCLUDED 1
#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;

namespace	usr {
    class tvideo_symbol {
	public:
	    tvideo_symbol();
	    tvideo_symbol(t_vchar,t_vchar,ColorAttr);
	    virtual ~tvideo_symbol();

	    virtual t_vchar	symbol() const;
	    virtual t_vchar	oempg() const;
	    virtual ColorAttr	attr() const;
	private:
	    t_vchar	_symbol;
	    t_vchar	_oempg;
	    ColorAttr	_attr;
    };

    class tvideo_buffer : public Opaque {
	public:
	    tvideo_buffer(size_t n);
	    tvideo_buffer(const std::vector<tvideo_symbol>& v);
	    tvideo_buffer(const tvideo_symbol& s,size_t n);
	    tvideo_buffer(const tvideo_buffer& it);
	    virtual ~tvideo_buffer();

	    virtual void		resize(size_t newlen);
	    tvideo_buffer&		operator=(const tvideo_buffer&);
	    const tvideo_symbol&	operator[](size_t idx) const;
	    tvideo_symbol&		operator[](size_t idx);

	    virtual tvideo_buffer	sub_buffer(size_t idx) const;
	    virtual tvideo_buffer	sub_buffer(size_t idx,size_t len) const;

	    virtual void	fill(const tvideo_symbol& s);
	    virtual void	fill_at(size_t idx,const tvideo_symbol& s,size_t sz);
	    virtual void	assign(const std::vector<tvideo_symbol>& v);
	    virtual void	assign(const tvideo_buffer& from,size_t len);
	    virtual void	assign_at(size_t idx,const tvideo_buffer&);
	    virtual void	assign_at(size_t idx,const tvideo_buffer& from,size_t rlen);
	    virtual void	assign_at(size_t idx,const std::vector<tvideo_symbol>& v);
	    virtual void	assign_at(size_t idx,tvideo_symbol s);

	    size_t		size() const { return data.size(); }
	private:
	    std::vector<tvideo_symbol> data;
    };
} // namespace	usr
#endif
