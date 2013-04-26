/*
  aclib - advanced C library ;)
  This file contains functions which improve and expand standard C-library
*/

#ifdef REGMM_SIZE
#undef REGMM_SIZE
#endif
#define REGMM_SIZE 16
static void __FASTCALL__ RENAME(InterleaveBuffers)(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer)
{
  register char *destbuffptr;
  register const char *oddptr, *evenptr;
  register uint32_t freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  oddptr = (const char *)oddbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register uint64_t delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta = ((uint64_t)evenptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *evenptr++;
	*destbuffptr++ = *oddptr++;
	delta--;
      }
      /* Perform MMX optimized interleaving */
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movdqa	(%0), %%xmm0\n\t"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movdqa	%%xmm0, %%xmm1\n\t"
	       "punpckhbw (%0), %%xmm0\n\t"
	      ::"r"(oddptr):"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw (%0), %%xmm1\n\t"
	       ::"r"(oddptr):"memory");
	 oddptr+=step;
	 __asm __volatile("movdqu	%%xmm0, (%0)\n\t"
	       "movdqu	%%xmm2, 16(%0)\n\t"
	      ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = *oddptr++;
    freq++;
  }
}
