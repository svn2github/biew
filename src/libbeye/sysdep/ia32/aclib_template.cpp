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

static void __FASTCALL__ RENAME(CharsToShorts)(uint32_t limit,
					     any_t*destbuffer,
					     const any_t*evenbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *evenptr;
  register uint32_t freq;
  destbuffptr = (char *)destbuffer;
  evenptr = (const char *)evenbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      register uint32_t delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffer on boundary of REGMM_SIZE */
      delta = ((uint32_t)evenptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *evenptr++;
	*destbuffptr++ = 0;
	delta--;
      }
      /* Perform MMX optimized loop */
      __asm __volatile("pxor	%%mm7, %%mm7":::"memory");
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movq	(%0),%%mm0\n\t"
	       "movq	8(%0),%%mm2"
	       ::"r"(evenptr):"memory");
	 evenptr+=step;
	 __asm __volatile("movq	%%mm0, %%mm1\n\t"
	       "movq	%%mm2, %%mm3\n\t"
	       "punpckhbw %%mm7, %%mm0\n\t"
	       "punpckhbw %%mm7, %%mm2"
	      :::"memory");
	 nlimit--;
	 __asm __volatile("punpcklbw %%mm7, %%mm1\n\t"
	       "punpcklbw %%mm7, %%mm3"
	       :::"memory");
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
    *destbuffptr++ = 0;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq+freq] = ((char *)evenbuffer)[freq];
    ((char *)destbuffer)[freq+freq+1] = 0;
  }
}
#endif
}

static void __FASTCALL__ RENAME(ShortsToChars)(uint32_t limit,
				     any_t* destbuffer, const any_t* srcbuffer)
{
#ifdef HAVE_MMX
  register char *destbuffptr;
  register const char *srcptr;
  register uint32_t freq;
  destbuffptr = (char *)destbuffer;
  srcptr = (const char *)srcbuffer;
  freq = 0;
  if(limit>REGMM_SIZE*4-1)
  {
      uint32_t delta, nlimit, step;
      step = REGMM_SIZE*2;
      /* Try to align buffers on boundary of REGMM_SIZE */
      delta=((uint32_t)destbuffptr)&(REGMM_SIZE-1);
      if(delta) delta=REGMM_SIZE-delta;
      nlimit=(limit-delta)/step;
      freq=delta+(nlimit*step);
      while(delta)
      {
	*destbuffptr++ = *srcptr;
	srcptr+=2;
	delta--;
      }
      /* Perform MMX optimized loop */
      while(nlimit)
      {
	 /* Interleave mmx and cpu instructions */
	 __asm __volatile("movq	(%0), %%mm0\n\t"
	       "movq	8(%0), %%mm1\n\t"
	       ::"r"(srcptr):"memory");
	 nlimit--;
	 __asm __volatile("packuswb (%0), %%mm0\n\t"
	       "packuswb 8(%0), %%mm1"
	       ::"g"(&srcptr[REGMM_SIZE]):"memory");
	 srcptr+=step*2;
	 __asm __volatile("movq	%%mm0, (%0)\n\t"
	       "movq	%%mm1, 8(%0)\n\t"
	       ::"r"(destbuffptr):"memory");
	 destbuffptr+=step;
      }
      __asm __volatile("emms":::"memory");
  }
  /* If tail exists then finish it */
  while(freq<limit)
  {
    *destbuffptr++ = *srcptr;
    srcptr+=2;
    freq++;
  }
#else
{
  register size_t freq;
  for(freq=0;freq<(size_t)limit;freq++)
  {
    ((char *)destbuffer)[freq] = ((char *)srcbuffer)[freq+freq];
  }
}
#endif
}
