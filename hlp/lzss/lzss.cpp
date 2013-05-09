#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_tools
 * @file        tools/lzss/lzss.c
 * @brief       A Data Compression Program.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#include "libbeye/bbio.h"
#include "libbeye/osdep/system.h"
#define INTERACTIVE
#include "lzssutil.cpp"

char **ArgVector;

int main(int argc, char *argv[])
{
	char  *s;
	int retcode;
	if (argc != 4)
	{
	    printf("'lzss e file1 file2' encodes file1 into file2.\n"
		   "'lzss d file2 file1' decodes file2 into file1.\n");
	    return EXIT_FAILURE;
	}
	ArgVector=argv;
	infile = new binary_stream;
	bool rc;
	if ((s = argv[1], s[1] || strpbrk(s, "DEde") == NULL)
	    || (s = argv[2], (rc = infile->open(s,O_RDONLY)) == false))
	{
		printf("??? %s\n", s);
		return EXIT_FAILURE;
	}
	System msystem();
	s = argv[3];
	if(binary_stream::exists(s)) if(binary_stream::unlink(s)) { Err: printf("Problem with %s\n",s); return EXIT_FAILURE; }
	outfile = new binary_stream;
	if(outfile->create(s) == false) goto Err;
	if (toupper(*argv[1]) == 'E') retcode = Encode();
#if 0
	else                          retcode = Decode(infile,NULL,0L,infile->flength());
#endif
	if(!retcode) fprintf(stderr,"Error allocating memory during operation\n");
	delete infile;
	delete outfile;
	return EXIT_SUCCESS;
}

