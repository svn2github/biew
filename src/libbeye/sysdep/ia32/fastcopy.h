#ifndef __BEYE_FASTMEMCPY
#define __BEYE_FASTMEMCPY 1

#ifndef	__DISABLE_ASM
#include <stddef.h>
#include <string.h>

extern any_t* (*fast_memcpy_ptr)(any_t* to, const any_t* from, size_t len);
#ifdef memcpy
#undef memcpy
#endif
#define memcpy(a,b,c) (*fast_memcpy_ptr)(a,b,c)
extern any_t* (*fast_memset_ptr)(any_t* to, int filler, size_t len);
#ifdef memset
#undef memset
#endif
#define memset(a,b,c) (*fast_memset_ptr)(a,b,c)

#endif	/* __DISABLE_ASM */

#endif	/* __BEYE_FASTMEMCPY */
