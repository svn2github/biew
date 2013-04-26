/*
  aclib - advanced C library ;)
  This file contains functions which improve and expand standard C-library
*/

#ifdef REGMM_SIZE
#undef REGMM_SIZE
#endif
#define REGMM_SIZE 8 /* In the future it can be safety replaced with 16 for SSE2 */
static void __FASTCALL__ RENAME(InterleaveBuffers)(uint32_t limit,
				    any_t*destbuffer,
				    const any_t*evenbuffer,
				    const any_t*oddbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *oddptr, *evenptr;
  register uint32_t freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  oddptr = (const char *)oddbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register uint32_t delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta = ((uint32_t)evenptr)&(REGMM_SIZE-1);
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
	 __asm __volatile("movq	(%0), %%mm0\n\t"
	       "movq	8(%0), %%mm2\n\t"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movq	%%mm0, %%mm1\n\t"
	       "movq	%%mm2, %%mm3\n\t"
	       "punpckhbw (%0), %%mm0\n\t"
	       "punpckhbw 8(%0), %%mm2\n\t"
	      ::"r"(oddptr):"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw (%0), %%mm1\n\t"
	       "punpcklbw 8(%0), %%mm3\n\t"
	       ::"r"(oddptr):"memory");
	 oddptr+=step;
	 __asm __volatile("movq	%%mm0, 8(%0)\n\t"
	       "movq	%%mm1, (%0)\n\t"
	       "movq	%%mm2, 24(%0)\n\t"
	       "movq	%%mm3, 16(%0)"
	      ::"r"(destbuffptr):"memory");
	 destbuffptr+=step*2;
      }
      __asm __volatile("emms":::"memory");
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *evenptr++;
    *destbuffptr++ = *oddptr++;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];
    ((char *)destbuffer)[freq+freq+1] = ((char *)oddbuffer)[freq];
  }
}
#endif
}
