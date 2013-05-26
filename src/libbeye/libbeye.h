/**
 * @namespace   libbeye
 * @file        libbeye/libbeye.h
 * @brief       This file contains extensions of standard C library, that needed
 *              for BEYE project.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#ifndef __BEYELIB_H
#define __BEYELIB_H 1
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#include "libbeye/sysdep/_sys_dep.h"

#include "libbeye/mp_malloc.h"

		   /** Converts all alphabetic characters in buffer to upper case.
		     * @return                none
		     * @param buff            buffer to be converted
		     * @param cbBuff          size of buffer
		     * @see                   memlwr
		    **/
extern void __FASTCALL__ memupr(any_t*buffer,unsigned cb_buffer);

		   /** Converts all alphabetic characters in buffer to lower case.
		     * @return                none
		     * @param buff            buffer to be converted
		     * @param cbBuff          size of buffer
		     * @see                   memlwr
		    **/
extern void __FASTCALL__ memlwr(any_t*buffer,unsigned cb_buffer);

#if defined(__GLIBC__) || defined (__UNIX__)
inline void strupr(char* s) { memupr(s,::strlen(s)); } /**< C library of *nix systems lacks strupr function */
inline void strlwr(char* s) { memlwr(s,::strlen(s)); } /**< C library of *nix systems lacks strlwr function */
inline int stricmp(const char* s1,const char* s2) { return strcasecmp(s1,s2); } /**< Alias of stricmp for *nix systems */
#endif
#ifndef HAVE_LTOA
extern char *        ltoa(long _value, char *_s, int _radix);
#endif
#ifndef HAVE_ATOLL
extern long long int atoll(const char *s);
#endif
#ifndef HAVE_STRTOLL
extern long long strtoll(const char *nptr, char **endptr, int base);
#endif
#ifndef HAVE_STRTOULL
unsigned long long strtoull(const char *nptr, char **endptr, int base);
#endif
#ifndef HAVE_ULTOA
extern char *        ultoa(unsigned long _value, char *_s, int _radix);
#endif
#ifndef HAVE_LLTOA
extern char *        lltoa(long long int _value, char *_s, int _radix);
#endif
#ifndef HAVE_ULLTOA
extern char *        ulltoa(unsigned long long int _value, char *_s, int _radix);
#endif
namespace	usr {
    typedef int64_t __fileoff_t;
    typedef uint64_t __filesize_t;
#define FILEOFF_MAX std::numeric_limits<int64_t>::max()
#define FILESIZE_MAX std::numeric_limits<uint64_t>::max()

    enum data_type_qualifier__byte_t{ type_byte=0 };
    enum data_type_qualifier__word_t{ type_word=0 };
    enum data_type_qualifier_dword_t{ type_dword=0 };
    enum data_type_qualifier_qword_t{ type_qword=0 };

    /* KEYBOARD handling */
    enum {
	KBD_NONSTOP_ON_MOUSE_PRESS=0x00000001L /**< Defines that \e kbdGetKey must receive mouse events as frequently as it possible. Otherwise each press on mouse button will send only one event. */
    };
    /* MOUSE handling */
    enum {
	MS_LEFTPRESS    =1,  /**< Defines that left button of mouse have been pressed */
	MS_RIGHTPRESS   =2,  /**< Defines that middle button of mouse have been pressed */
	MS_MIDDLEPRESS  =4   /**< Defines that right button of mouse have been pressed */
    };
    enum {
	__TVIO_MAXSCREENWIDTH		=255, /**< Defines maximal width of screen */
	__TVIO_FLG_DIRECT_CONSOLE_ACCESS=0x00000001L, /**< Defines that video subsystem must access to console directly, if it possible */
	__TVIO_FLG_USE_7BIT		=0x00000002L  /**< Defines that video subsystem must strip high bit of video characters */
    };
    typedef uint8_t ColorAttr; /**< This is the data type used to represent attributes of color */
    typedef uint8_t t_vchar;   /**< This is the data type used to represent video character */
    typedef unsigned tAbsCoord; /**< This is the data type used to represent screen-related coordinates */

/** Internal structure of video buffer */
    struct tvioBuff {
	t_vchar*   chars;       /**< Pointer to video character array */
	t_vchar*   oem_pg;      /**< Pointer to OEM pseudographics. It needed for *nix terminals */
	ColorAttr* attrs;       /**< Pointer to color attributes array */
    };

    class Opaque {
	public:
	    Opaque();
	    virtual ~Opaque();
	
	any_t*		false_pointers[RND_CHAR0];
	any_t*		unusable;
    };

    template <typename T> class LocalPtr : public Opaque {
	public:
	    LocalPtr(T* value):ptr(value) {}
	    virtual ~LocalPtr() { delete ptr; }

	    T& operator*() const { return *ptr; }
	    T* operator->() const { return ptr; }
	private:
	    LocalPtr<T>& operator=(LocalPtr<T> a) { return this; }
	    LocalPtr<T>& operator=(LocalPtr<T>& a) { return this; }
	    LocalPtr<T>& operator=(LocalPtr<T>* a) { return this; }

	    Opaque	opaque1;
	    T*		ptr;
	    Opaque	opaque2;
    };

    class tvideo_buffer : public Opaque {
	public:
	    tvideo_buffer(size_t n);
	    tvideo_buffer(const t_vchar* chars,const t_vchar* oempg,const ColorAttr* attrs,size_t n);
	    tvideo_buffer(t_vchar chars,t_vchar oempg,ColorAttr attrs,size_t n);
	    virtual ~tvideo_buffer();

	    virtual void	resize(size_t newlen);
	    tvideo_buffer&	operator=(const tvideo_buffer& it);
	    tvideo_buffer	operator[](size_t idx) const;

	    virtual void	fill(t_vchar chars,t_vchar oempg,ColorAttr attrs);
	    virtual void	fill_at(size_t idx,t_vchar chars,t_vchar oempg,ColorAttr attrs,size_t sz);
	    virtual void	assign(const t_vchar* chars,const t_vchar* oempg,const ColorAttr* attrs,size_t len);
	    virtual void	assign(const tvideo_buffer& from,size_t len);
	    virtual void	assign_at(size_t idx,const tvideo_buffer&);
	    virtual void	assign_at(size_t idx,const tvideo_buffer&,size_t rlen);
	    virtual void	assign_at(size_t idx,const t_vchar* chars,const t_vchar* oempg,const ColorAttr* attrs,size_t len);
	    virtual void	assign_at(size_t idx,t_vchar chars,t_vchar oempg,ColorAttr attrs);

	    size_t		length() const { return len; }
	    const t_vchar*	get_chars() const { return chars; }
	    const t_vchar*	get_oempg() const { return oempg; }
	    const ColorAttr*	get_attrs() const { return attrs; }
	private:
	    void		_construct();

	    size_t	len;
	    t_vchar*	chars; /**< Pointer to video character array */
	    t_vchar*	oempg; /**< Pointer to OEM pseudographics. It needed for *nix terminals */
	    ColorAttr*	attrs; /**< Pointer to color attributes array */
    };

#define TESTFLAG(x,y) (((x) & (y)) == (y)) /**< Test y bits in x */

		   /** Tests wether character is a separator
		     * @return                true if given character is separator
		     * @param ch              character to be tested
		     * @note                  returns true if character is space
		     *                        or punctuator
		    **/
    bool  __FASTCALL__ isseparate(int ch);

/** ASCIIZ string extended support */

		   /** Removes all trailing spaces from string
		     * @return                number of removed spaces
		     * @param str             pointer to string to be trimmed
		     * @see                   szTrimLeadingSpace szKillSpaceAround
		    **/
    int   __FASTCALL__ szTrimTrailingSpace(char *str);

		   /** Removes all leading spaces from string
		     * @return                number of removed spaces
		     * @param str             pointer to string to be trimmed
		     * @see                   szTrimTrailingSpace szKillSpaceAround
		    **/
    int   __FASTCALL__ szTrimLeadingSpace(char *str);

		   /** Converts space into tabulation characters
		     * @return                none
		     * @param dest            pointer to string where will be placed result
		     * @param src             pointer to source string
		     * @see                   szTab2Space
		    **/
    void  __FASTCALL__ szSpace2Tab(char *dest,const char *src);

		   /** Expands all tabulation characters with spaces
		     * @return                length of new string
		     * @param dest            pointer to string where will be placed result
		     * @param src             pointer to source string
		     * @see                   szSpace2Tab
		    **/
    int   __FASTCALL__ szTab2Space(char *dest,const char *src);

		   /** Removes all spaces around given position
		     * @return                pointer onto next non space character
		     * @param str             pointer to string to be converted
		     * @param point_to        specifies position to be unspaced
		     * @see                   szTrimLeadingSpace szTrimTrailingSpace
		    **/
    char *__FASTCALL__ szKillSpaceAround(char *str,char *point_to);
} // namespace	usr
#endif
