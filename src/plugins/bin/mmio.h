#ifndef __MMIO
#define __MMIO 1

#include "config.h"
#include "plugin.h"
#include "plugins/binary_parser.h"

namespace	usr {
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#define FOURCC DWORD

typedef struct
{
    WORD  left;
    WORD  top;
    WORD  right;
    WORD  bottom;
} RECT, LPRECT;

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct __attribute__((__packed__)) _WAVEFORMATEX {
  WORD   wFormatTag;
  WORD   nChannels;
  DWORD  nSamplesPerSec;
  DWORD  nAvgBytesPerSec;
  WORD   nBlockAlign;
  WORD   wBitsPerSample;
  WORD   cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef _BITMAPINFOHEADER_
#define _BITMAPINFOHEADER_
typedef struct __attribute__((__packed__))
{
    int 	biSize;
    int  	biWidth;
    int  	biHeight;
    short 	biPlanes;
    short 	biBitCount;
    int 	biCompression;
    int 	biSizeImage;
    int  	biXPelsPerMeter;
    int  	biYPelsPerMeter;
    int 	biClrUsed;
    int 	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;
#endif

/* this macro helps print FOURCC */
#define INT_2_CHAR_ARG(i)\
,((char *)&i)[0]?((char *)&i)[0]:' '\
,((char *)&i)[1]?((char *)&i)[1]:' '\
,((char *)&i)[2]?((char *)&i)[2]:' '\
,((char *)&i)[3]?((char *)&i)[3]:' '

    struct wTagNames {
	unsigned short wTag;
	const char *name;
    };

    class Wave_Parser : public Binary_Parser {
	public:
	    Wave_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~Wave_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;

	    static const char*		wtag_find_name(unsigned short wtag) __CONST_FUNC__;
	private:
	    __filesize_t		wav_find_chunk(__filesize_t off,unsigned long id) const;

	    BeyeContext&		bctx;
	    binary_stream&		main_handle;
	    udn&			_udn;
    };
} // namespace	usr
#endif
