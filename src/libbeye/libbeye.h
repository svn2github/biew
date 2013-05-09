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

#ifndef __NORECURSIVE
#include "libbeye/sysdep/__config.h"
#include "libbeye/sysdep/_sys_dep.h"
#include "libbeye/osdep/__os_dep.h"
#include "libbeye/sysdep/_hrd_inf.h"
#endif
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

#define TESTFLAG(x,y) (((x) & (y)) == (y)) /**< Test y bits in x */

    typedef int tCompare; /**< This is the data type used to represent comparition results */

		   /** Pointer to a user supplied function that compares array elements.
		     * @return                tCompare value indicated relationship between elements of array:
					      if e1 < e2, return < 0
					      if e1 > e2, return > 0
					      if e1 == e2, return = 0
		     * @param e1,e2           pointers to array elements
		    **/
    typedef tCompare (__FASTCALL__ *func_compare)(const any_t*e1,const any_t*e2);

		   /** Implements quick sort algorithm.
		     * @return                none
		     * @param base            specifies array being sorted
		     * @param num             specifies number of elements in array
		     * @param width           specifies with (in bytes) of one element of array
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               After function call the original array
		     *                        is overwritten with sorted array in
		     *                        ascending order.
		     * @note                  Using own code for qsort and bsearch
		     *                        functions is guarantee of stable work
		     * @see                   HLFind HLFindNearest
		    **/
    void  __FASTCALL__ HQSort(any_t*base, unsigned long num, unsigned width,
				 func_compare fcompare);

		   /** Performs a quick search on a sorted array.
		     * @return                pointer to the first matching element if found, otherwise NULL is returned
		     * @param key             pointer to the key
		     * @param base            specifies array being sorted
		     * @param nelem           specifies number of elements in array
		     * @param width           specifies with (in bytes) of one element of array
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               Function can to cause infinity loop
		     *                        if array is unsorted
		     * @note                  Using own code for qsort and bsearch
		     *                        functions is guarantee of stable work
		     * @see                   HQSort HLFindNearest
		    **/
    any_t* __FASTCALL__ HLFind(const any_t*key,
				     any_t*base,
				     unsigned long nelem,unsigned width,
				     func_compare fcompare);

		   /** Performs a quick search on a sorted array of nearest element.
		     * @return                index of nearest element of array toward zero.
		     * @param key             pointer to the key
		     * @param base            specifies array being sorted
		     * @param nelem           specifies number of elements in array
		     * @param width           specifies with (in bytes) of one element of array
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               Function can to cause infinity loop
		     *                        if array is unsorted
		     * @note                  Using own code for qsort and bsearch
		     *                        functions is guarantee of stable work
		     * @see                   HQSort HLFind
		    **/
    unsigned long __FASTCALL__ HLFindNearest(const any_t*key,
				     any_t*base,
				     unsigned long nelem,unsigned width,
				     func_compare fcompare);

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

    /** Internal structure of Linear memory container */
    typedef struct tag_linearArray {
	unsigned long   nItems;    /**< Number of stored items */
	any_t* data;      /**< Pointer into linear array */
	unsigned long   nSize;     /**< Size of linear array (May differ from nItems) */
	unsigned        itemSize;  /**< Size of one item in linear array */
    }linearArray;

		   /** Builds linear arrays
		     * @return                pointer to builded array or NULL if error
		     * @param maxitems        specifies maximal number of item that can be stored in array. 0 - indicates dynamic change of size
		     * @param size_of_item    specifies size of each array element.
		     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
		     * @note                  Linear array consist from elements
		     *                        same width.
		     * @see                   la_Destroy la_IterDestroy
		    **/
    linearArray *__FASTCALL__ la_Build( unsigned long maxitems,unsigned size_of_item,
					   void (__FASTCALL__ *mem_out)(const std::string&));

		   /** Adds new element to linear array
		     * @return                location of new element or NULL if no memory
		     * @param obj             specifies linear array where new element will be stored
		     * @param data            specifies new element
		     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
		     * @see                   la_Build la_Find
		    **/
    any_t* __FASTCALL__ la_AddData(linearArray *obj,const any_t*data,void (__FASTCALL__ *mem_out)(const std::string&));

		   /** Removes given element from linear array
		     * @param obj             specifies linear array where element will be removed
		     * @param idx             specifies index of element to be removed
		     * @param mem_out         specifies user-defined function to be called when low-memory. May be NULL.
		     * @see                   la_Build la_Find
		    **/
    void __FASTCALL__          la_DeleteData(linearArray *obj,unsigned long idx);

		   /** Destroys of linear array
		     * @return                none
		     * @param obj             specifies linear array to be destroyed
		     * @warning               if elements contain pointers to
		     *                        dynamically allocated memory, then
		     *                        it will be lost
		     * @see                   la_Build la_IterDestroy
		    **/
    void         __FASTCALL__ la_Destroy(linearArray *obj);

		   /** Destroys of linear array and calls "destructor" for each element of array.
		     * @return                none
		     * @param obj             specifies linear array to be destroyed
		     * @param del_func        specifies user-defined function, that will be used as destructor
		     * @note                  Before freeing memory of linear array
		     *                        will be called user-defined function
		     *                        with pointer onto each elemet as
		     *                        arguments
		     * @see                   la_Build la_Destroy
		    **/
    void         __FASTCALL__ la_IterDestroy(linearArray *obj,void (__FASTCALL__ *del_func)(any_t*));

		   /** Calls the given iterator function on each array element
		     * @return                none
		     * @param obj             specifies linear array to be destroyed
		     * @param iter_func       specifies iterator function which is to be called for each array element
		     * @see                   la_IterDestroy
		    **/
    void         __FASTCALL__ la_ForEach(linearArray *obj,void (__FASTCALL__ *iter_func)(any_t*));

		   /** Implements quick sort algorithm for linear array
		     * @return                none
		     * @param obj             specifies linear array to be destroyed
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               After function call the original array
		     *                        is overwritten with sorted array in
		     *                        ascending order.
		     * @note                  Based on HQSort function
		     * @see                   la_Find la_FindNearest
		    **/
    void         __FASTCALL__ la_Sort(linearArray *obj,func_compare fcompare);

		   /** Performs a quick search on a sorted linear array.
		     * @return                pointer to the first matching element if found, otherwise NULL is returned
		     * @param key             pointer to the key
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               Function can to cause infinity loop
		     *                        if array is unsorted
		     * @note                  Based on HLFind function
		     *                        functions is guarantee of stable work
		     * @see                   la_Sort la_FindNearest
		    **/
    any_t*__FASTCALL__ la_Find(linearArray *obj,const any_t*key,
					   func_compare fcompare);

		   /** Performs a quick search on a sorted linear array of nearest element.
		     * @return                index of nearest element of array toward zero.
		     * @param key             pointer to the key
		     * @param fcompare        specifies pointer to user defined function
		     * @warning               Function can to cause infinity loop
		     *                        if array is unsorted
		     * @note                  Based on HLFindNearest function
		     * @see                   HQSort HLFind
		    **/
    unsigned long __FASTCALL__ la_FindNearest(linearArray *obj, const any_t*key,
					   func_compare fcompare);

/** Frees pointer and nullifies it */
#define PFREE(ptr)      { mp_free(ptr); ptr = 0; }
} // namespace	usr
#endif
