#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_tools
 * @file        tools/lzss/lzssutil.c
 * @brief       A Data Compression Functions.
 * @version     -
 * @remark      Use, distribute, and modify this program freely.
 *              Please send me your improved versions.
 * @note        Requires POSIX compatible development system
 *
 * @author      Haruhiko Okumura
 * @since       1989
 * @author      Nickols_K
 * @date        22.10.1999
 * @note        Modified for using with BEYE
**/
/**************************************************************
	LZSS.C -- A Data Compression Program
	(tab = 4 spaces)
***************************************************************
	4/6/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN		SCIENCE
		NIFTY-Serve	PAF01022
		CompuServe	74050,1022
**************************************************************/

#define N		4096	/** size of ring buffer */
#define F		18	/** upper limit for match_length */
#define THRESHOLD	2   	/** encode string into position and length
				     if match_length is greater than this */
#define NIL		N	/** index for root of binary search trees */

static unsigned long int
		textsize = 0,	/** text size counter */
		codesize = 0,	/** code size counter */
		printcount = 0;	/** counter for reporting progress every 1K bytes */
static unsigned char
		*text_buf;	/** ring buffer of size N,
				     with extra F-1 bytes to facilitate string comparison */
static int		match_position, match_length,  /** of longest match.  These are
						    set by the InsertNode() procedure. */
		*lson, *rson, *dad;  /** left & right children &
					  parents -- These constitute binary search trees. */
static binary_stream*	infile;
static std::ofstream ofs;  /** input & output files */
#ifdef INTERACTIVE
static void InitTree()  /** initialize trees */
{
	int  i;

	/** For i = 0 to N - 1, rson[i] and lson[i] will be the right and
	    left children of node i.  These nodes need not be initialized.
	    Also, dad[i] is the parent of node i.  These are initialized to
	    NIL (= N), which stands for 'not used.'
	    For i = 0 to 255, rson[N + i + 1] is the root of the tree
	    for strings that begin with character i.  These are initialized
	    to NIL.  Note there are 256 trees. */

	for (i = N + 1; i <= N + 256; i++) rson[i] = NIL;
	for (i = 0; i < N; i++) dad[i] = NIL;
}

	/** Inserts string of length F, text_buf[r..r+F-1], into one of the
	    trees (text_buf[r]'th tree) and returns the longest-match position
	    and length via the global variables match_position and match_length.
	    If match_length = F, then removes the old node in favor of the new
	    one, because the old one will be deleted sooner.
	    Note r plays double role, as tree node and position in buffer. */
static void InsertNode(int r)
{
	int  i, p, cmp;
	unsigned char  *key;

	cmp = 1;  key = &text_buf[r];  p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;  match_length = 0;
	for ( ; ; ) {
		if (cmp >= 0) {
			if (rson[p] != NIL) p = rson[p];
			else {  rson[p] = r;  dad[r] = p;  return;  }
		} else {
			if (lson[p] != NIL) p = lson[p];
			else {  lson[p] = r;  dad[r] = p;  return;  }
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)  break;
		if (i > match_length) {
			match_position = p;
			if ((match_length = i) >= F)  break;
		}
	}
	dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
	dad[lson[p]] = r;  dad[rson[p]] = r;
	if (rson[dad[p]] == p) rson[dad[p]] = r;
	else                   lson[dad[p]] = r;
	dad[p] = NIL;  /** remove p */
}

/** deletes node p from tree */
static void DeleteNode(int p)
{
	int  q;

	if (dad[p] == NIL) return;  /** not in tree */
	if (rson[p] == NIL) q = lson[p];
	else if (lson[p] == NIL) q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {  q = rson[q];  } while (rson[q] != NIL);
			rson[dad[q]] = lson[q];  dad[lson[q]] = dad[q];
			lson[q] = lson[p];  dad[lson[p]] = q;
		}
		rson[q] = rson[p];  dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p) rson[dad[p]] = q;  else lson[dad[p]] = q;
	dad[p] = NIL;
}

static int Encode()
{
	int  i, c, len, r, s, last_match_length, code_buf_ptr,reach_eof;
#ifdef INTERACTIVE
	int percent,ppercent;
	time_t sttime,endtime;
	long tval,tval2;
#endif
	__filesize_t flen;
	unsigned char  code_buf[17], mask;

	text_buf = new unsigned char [N+F-1];
	lson = new int [N+1];
	rson = new int [N+257];
	dad  = new int [N+1];
	if(!text_buf || !lson || !rson || !dad)
	{
	  err:
	  if(text_buf) delete text_buf;
	  if(lson) delete lson;
	  if(rson) delete rson;
	  if(dad)  delete dad;
	  return 0;
	}
#ifdef INTERACTIVE
	time(&sttime);
	ppercent = -1;
#endif
	infile->seek(0L,binary_stream::Seek_End);
	flen = infile->tell();
	infile->seek(0L,binary_stream::Seek_Set);
	InitTree();  /** initialize trees */
	code_buf[0] = 0;  /** code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;  r = N - F;
	for (i = s; i < r; i++) text_buf[i] = ' ';  /** Clear the buffer with
		any character that will appear often. */
	for (len = 0; len < F; len++)
	{
	  reach_eof = infile->eof() || infile->tell() > flen;
	  if(reach_eof) break;
	  c = infile->read(type_byte);
	  text_buf[r + len] = c;  /** Read F bytes into the last F bytes of
			the buffer */
	}
	if ((textsize = len) == 0) goto err;  /** text of size zero */
	for (i = 1; i <= F; i++) InsertNode(r - i);  /** Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /** Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */
	do {
		if (match_length > len) match_length = len;  /** match_length
			may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD) {
			match_length = 1;  /** Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /** 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /** Send uncoded. */
		} else {
			code_buf[code_buf_ptr++] = (unsigned char) match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
			  | (match_length - (THRESHOLD + 1)));  /** Send position and
					length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0) {  /** Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /** Send at most 8 units of */
				ofs.put((uint8_t)code_buf[i]);     /** code together */
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length; i++)
		{
			reach_eof = infile->eof() || infile->tell() > flen;
			if(reach_eof) break;
			c = infile->read(type_byte);
			DeleteNode(s);		/** Delete old strings and */
			text_buf[s] = c;	/** read new bytes */
			if (s < F - 1) text_buf[s + N] = c;  /** If the position is
				near the end of buffer, extend the buffer to make
				string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
				/** Since this is a ring buffer, increment the position
				   modulo N. */
			InsertNode(r);	/** Register the string in text_buf[r..r+F-1] */
		}
		textsize += i;
#ifdef INTERACTIVE
		percent = (unsigned)(textsize*100/flen);
		if (percent != ppercent)
		{
		    printf("%u%%\r", percent);
		    ppercent = percent;
		}
#endif
		while (i++ < last_match_length) {	/** After the end of text, */
			DeleteNode(s);			/** no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len) InsertNode(r);	/** buffer may not be empty. */
		}
	} while (len > 0);	/** until length of string to be processed is zero */
	if (code_buf_ptr > 1) {	/** Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++) ofs.put((uint8_t)code_buf[i]);
		codesize += code_buf_ptr;
	}
	delete text_buf;
	delete lson;
	delete rson;
	delete dad;
#ifdef INTERACTIVE
	time(&endtime);
	endtime -= sttime;
	printf("Original       : %lu bytes\n", textsize);
	printf("Packed         : %lu bytes\n", codesize);
	tval = codesize * 1000 / textsize;
	tval2 = (tval / 10)*10;
	percent = (unsigned)(tval - tval2);
	tval /= 10;
	printf("Compress ratio : %ld.%u%%\n",tval,percent);
	printf("Elapsed  time  : %ld msec\n",endtime*55);
#endif
	return 1;
}
#endif

/** Just the reverse of Encode(). */
static bool Decode(any_t* buff,const uint8_t* instream,unsigned long length)
{
    unsigned long in_idx=0;
	int  i, j, k, r, c,reach_eof;
#ifdef INTERACTIVE
	int percent,ppercent;
	time_t sttime,endtime;
	long cfpos;
#endif
	unsigned int  flags;
	unsigned long buff_ptr = 0;

	text_buf = new unsigned char[N+F-1];
	lson = new int[N+1];
	rson = new int [N+257];
	dad  = new int [N+1];
#ifdef INTERACTIVE
	{
	  ppercent = -1;
	  time(&sttime);
	}
#endif
	for (i = 0; i < N - F; i++) text_buf[i] = ' ';
	r = N - F;  flags = 0;
	for ( ; ; )
	{
		if (((flags >>= 1) & 256) == 0)
		{
		   reach_eof = in_idx>=length;
		   if(reach_eof) break;
		   c = instream[in_idx++];
		   flags = c | 0xff00;		/** uses higher byte cleverly */
		}				/** to count eight */
		if (flags & 1)
		{
		   reach_eof = in_idx>=length;
		   if(reach_eof) break;
		   c = instream[in_idx++];
		   if(buff) ((char  *)buff)[buff_ptr++] = c;
		   else ofs.put((uint8_t)c);
		   text_buf[r++] = c;
		   r &= (N - 1);
		}
		else
		{
		     reach_eof = in_idx>=length;
		     if(reach_eof) break;
		     i = instream[in_idx++];
		     reach_eof = in_idx>=length;
		     if(reach_eof) break;
		     j = instream[in_idx++];

		     i |= ((j & 0xf0) << 4);  j = (j & 0x0f) + THRESHOLD;
		     for (k = 0; k <= j; k++)
		     {
				c = text_buf[(i + k) & (N - 1)];
				if(buff) ((char  *)buff)[buff_ptr++] = c;
				else ofs.put((uint8_t)c);
				text_buf[r++] = c;
				r &= (N - 1);
		     }
		}
#ifdef INTERACTIVE
		cfpos = in_idx;
		{
		  percent = (unsigned)(cfpos*100/length);
		  if (percent != ppercent)
		  {
		    printf("%u%%\r",percent);
		    ppercent=percent;
		  }
		}
#endif
	}
#ifdef INTERACTIVE
	  time(&endtime);
	  printf("Elapsed  time  : %ld msec\n",(endtime-sttime)*55);
#endif
	delete text_buf;
	delete lson;
	delete rson;
	delete dad;
	return true;
}
