#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/avi.c
 * @brief       This file contains implementation of decoder for AVI multimedia
 *              streams.
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
#include <stddef.h>
#include <string.h>

#include "bconsole.h"
#include "beyehelp.h"
#include "colorset.h"
#include "libbeye/kbd_code.h"
#include "plugins/disasm.h"
#include "plugins/bin/mmio.h"
#include "libbeye/bstream.h"
#include "plugins/binary_parser.h"
#include "beye.h"

namespace	usr {
struct MainAVIHeader {
    DWORD		dwMicroSecPerFrame;	// frame display rate (or 0L)
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwPaddingGranularity;	// pad to multiples of this
						// size; normally 2K.
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwTotalFrames;		// # frames in file
    DWORD		dwInitialFrames;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;

    DWORD		dwWidth;
    DWORD		dwHeight;

    DWORD		dwReserved[4];
};

/*
** Stream header
*/

struct AVIStreamHeader {
    FOURCC		fccType;
    FOURCC		fccHandler;
    DWORD		dwFlags;	/* Contains AVITF_* flags */
    WORD		wPriority;
    WORD		wLanguage;
    DWORD		dwInitialFrames;
    DWORD		dwScale;
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwStart;
    DWORD		dwLength; /* In units above... */
    DWORD		dwSuggestedBufferSize;
    DWORD		dwQuality;
    DWORD		dwSampleSize;
    RECT		rcFrame;
};

    class AVI_Parser : public Binary_Parser {
	public:
	    AVI_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~AVI_Parser();

	    virtual const char*		prompt(unsigned idx) const;
	    virtual __filesize_t	action_F2();
	    virtual __filesize_t	action_F3();

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;
	private:
	    __filesize_t		avi_find_chunk(__filesize_t off,unsigned long id) const;

	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;

	    static const uint32_t formtypeAVI             =mmioFOURCC('A', 'V', 'I', ' ');
	    static const uint32_t listtypeAVIHEADER       =mmioFOURCC('h', 'd', 'r', 'l');
	    static const uint32_t ckidAVIMAINHDR          =mmioFOURCC('a', 'v', 'i', 'h');
	    static const uint32_t listtypeSTREAMHEADER    =mmioFOURCC('s', 't', 'r', 'l');
	    static const uint32_t ckidSTREAMHEADER        =mmioFOURCC('s', 't', 'r', 'h');
	    static const uint32_t ckidSTREAMFORMAT        =mmioFOURCC('s', 't', 'r', 'f');
	    static const uint32_t ckidSTREAMHANDLERDATA   =mmioFOURCC('s', 't', 'r', 'd');
	    static const uint32_t ckidSTREAMNAME	      =mmioFOURCC('s', 't', 'r', 'n');

	    static const uint32_t listtypeAVIMOVIE        =mmioFOURCC('m', 'o', 'v', 'i');
	    static const uint32_t listtypeAVIRECORD       =mmioFOURCC('r', 'e', 'c', ' ');

	    static const uint32_t ckidAVINEWINDEX         =mmioFOURCC('i', 'd', 'x', '1');

/*
** Stream types for the <fccType> field of the stream header.
*/
	    static const uint32_t streamtypeVIDEO         =mmioFOURCC('v', 'i', 'd', 's');
	    static const uint32_t streamtypeAUDIO         =mmioFOURCC('a', 'u', 'd', 's');
	    static const uint32_t streamtypeMIDI	  =mmioFOURCC('m', 'i', 'd', 's');
	    static const uint32_t streamtypeTEXT          =mmioFOURCC('t', 'x', 't', 's');

/* Chunk id to use for extra chunks for padding. */
	    static const uint32_t ckidAVIPADDING          =mmioFOURCC('J', 'U', 'N', 'K');
    };

static const char* txt[]={ "", "Audio", "Video", "", "", "", "", "", "", "" };
const char* AVI_Parser::prompt(unsigned idx) const { return txt[idx]; }

AVI_Parser::AVI_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    unsigned long id;
    main_handle.seek(8,binary_stream::Seek_Set);
    id = main_handle.read(type_dword);
    main_handle.seek(0,binary_stream::Seek_Set);
    if(!(main_handle.read(type_dword)==mmioFOURCC('R','I','F','F') &&
	(id==mmioFOURCC('A','V','I',' ') || id==mmioFOURCC('O','N','2',' ')))) throw bad_format_exception();
}
AVI_Parser::~AVI_Parser() {}
int AVI_Parser::query_platform() const { return DISASM_DEFAULT; }

__filesize_t AVI_Parser::avi_find_chunk(__filesize_t off,unsigned long id) const
{
    unsigned long ids,size,type;
    main_handle.seek(off,binary_stream::Seek_Set);
    while(!main_handle.eof())
    {
/*	fpos=main_handle.tell();*/
	ids=main_handle.read(type_dword);
	if(ids==id) return main_handle.tell();
	size=main_handle.read(type_dword);
	size=(size+1)&(~1);
/*fprintf(stderr,"%08X:%4s %08X\n",fpos,(char *)&ids,size);*/
	if(ids==mmioFOURCC('L','I','S','T'))
	{
	    type=main_handle.read(type_dword);
	    if(type==id) return main_handle.tell();
	    continue;
	}
	main_handle.seek(size,binary_stream::Seek_Cur);
    }
    return -1;
}

__filesize_t AVI_Parser::show_header() const
{
    unsigned keycode;
    TWindow * hwnd;
    MainAVIHeader avih;
    __filesize_t fpos,fpos2;
    fpos = bctx.tell();
    fpos2 = avi_find_chunk(12,mmioFOURCC('a','v','i','h'));
    if((__fileoff_t)fpos2==-1) { bctx.ErrMessageBox("Main AVI Header not found",""); return fpos; }
    main_handle.seek(fpos2,binary_stream::Seek_Set);
    main_handle.read(type_dword); /* skip section size */
    binary_packet bp=main_handle.read(sizeof(MainAVIHeader)); memcpy(&avih,bp.data(),bp.size());
    fpos2 = avi_find_chunk(12,mmioFOURCC('m','o','v','i'));
    if((__fileoff_t)fpos2!=-1) fpos2-=4;
    hwnd = CrtDlgWndnls(" AVI File Header ",43,9);
    hwnd->goto_xy(1,1);
    hwnd->printf(
	  "MicroSecond per Frame= %lu\n"
	  "Max bytes per second = %lu\n"
	  "Padding granularity  = %lu\n"
	  "Flags                = %lu\n"
	  "Total frames         = %lu\n"
	  "Initial frames       = %lu\n"
	  "Number of streams    = %lu\n"
	  "Suggested buffer size= %lu\n"
	  "Width x Height       = %lux%lu\n"
	  ,avih.dwMicroSecPerFrame
	  ,avih.dwMaxBytesPerSec
	  ,avih.dwPaddingGranularity
	  ,avih.dwFlags
	  ,avih.dwTotalFrames
	  ,avih.dwInitialFrames
	  ,avih.dwStreams
	  ,avih.dwSuggestedBufferSize
	  ,avih.dwWidth
	  ,avih.dwHeight);
    while(1) {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
	else if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    delete hwnd;
    return fpos;
}

__filesize_t AVI_Parser::action_F2()
{
    unsigned keycode;
    TWindow * hwnd;
    AVIStreamHeader strh;
    WAVEFORMATEX wavf;
    __filesize_t newcpos,fpos,fpos2;
    fpos = bctx.tell();
    memset(&wavf,0,sizeof(wavf));
    fpos2=12;
    do {
	fpos2 = avi_find_chunk(fpos2,mmioFOURCC('s','t','r','h'));
	if((__fileoff_t)fpos2==-1) { bctx.ErrMessageBox("Audio Stream Header not found",""); return fpos; }
	main_handle.seek(fpos2,binary_stream::Seek_Set);
	newcpos=main_handle.read(type_dword);
	binary_packet bp=main_handle.read(sizeof(AVIStreamHeader)); memcpy(&strh,bp.data(),bp.size());
	fpos2+=newcpos+4;
    }while(strh.fccType!=streamtypeAUDIO);
    if(main_handle.read(type_dword)==mmioFOURCC('s','t','r','f')) {
	main_handle.read(type_dword); /* skip header size */
	binary_packet bp=main_handle.read(sizeof(WAVEFORMATEX)); memcpy(&wavf,bp.data(),bp.size());
    }
    hwnd = CrtDlgWndnls(" Stream File Header ",43,18);
    hwnd->goto_xy(1,1);
    hwnd->printf(
	  "Stream type          = %c%c%c%c\n"
	  "FOURCC handler       = %c%c%c%c(%08Xh)\n"
	  "Flags                = %lu\n"
	  "Priority             = %u\n"
	  "Language             = %u\n"
	  "Initial frames       = %lu\n"
	  "Rate/Scale           = %lu/%lu=%5.3f\n"
	  "Start                = %lu\n"
	  "Length               = %lu\n"
	  "Suggested buffer size= %lu\n"
	  "Quality              = %lu\n"
	  "SampleSize           = %lu\n"
	  "====== WAVE HEADER ====================\n"
	  "FormatTag            = 0x%04X (%s)\n"
	  "ChannelsxSampleSecs  = %ux%u\n"
	  "AvgBytesSecs         = %lu\n"
	  "BlockAlign           = %u\n"
	  "BitsPerSample        = %u\n"
	  INT_2_CHAR_ARG(strh.fccType)
	  INT_2_CHAR_ARG(strh.fccHandler),strh.fccHandler
	 ,strh.dwFlags
	 ,strh.wPriority
	 ,strh.wLanguage
	 ,strh.dwInitialFrames
	 ,strh.dwRate
	 ,strh.dwScale
	 ,(float)strh.dwRate/(float)strh.dwScale
	 ,strh.dwStart
	 ,strh.dwLength
	 ,strh.dwSuggestedBufferSize
	 ,strh.dwQuality
	 ,strh.dwSampleSize
	 ,wavf.wFormatTag,Wave_Parser::wtag_find_name(wavf.wFormatTag)
	 ,wavf.nChannels,wavf.nSamplesPerSec
	 ,wavf.nAvgBytesPerSec
	 ,wavf.nBlockAlign
	 ,wavf.wBitsPerSample);
    while(1) {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    delete hwnd;
    return fpos;
}

__filesize_t AVI_Parser::action_F3()
{
    unsigned keycode;
    TWindow * hwnd;
    AVIStreamHeader strh;
    BITMAPINFOHEADER bmph;
    __filesize_t newcpos,fpos,fpos2;
    fpos = bctx.tell();
    memset(&bmph,0,sizeof(BITMAPINFOHEADER));
    fpos2=12;
    do {
	fpos2 = avi_find_chunk(fpos2,mmioFOURCC('s','t','r','h'));
	if((__fileoff_t)fpos2==-1) { bctx.ErrMessageBox("Video Stream Header not found",""); return fpos; }
	main_handle.seek(fpos2,binary_stream::Seek_Set);
	newcpos=main_handle.read(type_dword); /* skip section size */
	binary_packet bp=main_handle.read(sizeof(AVIStreamHeader)); memcpy(&strh,bp.data(),bp.size());
	fpos2+=newcpos+4;
    }while(strh.fccType!=streamtypeVIDEO);
    if(main_handle.read(type_dword)==mmioFOURCC('s','t','r','f')) {
	main_handle.read(type_dword); /* skip header size */
	binary_packet bp=main_handle.read(sizeof(BITMAPINFOHEADER)); memcpy(&bmph,bp.data(),bp.size());
    }
    hwnd = CrtDlgWndnls(" Stream File Header ",43,20);
    hwnd->goto_xy(1,1);
    hwnd->printf(
	  "Stream type          = %c%c%c%c\n"
	  "FOURCC handler       = %c%c%c%c(%08Xh)\n"
	  "Flags                = %lu\n"
	  "Priority             = %u\n"
	  "Language             = %u\n"
	  "Initial frames       = %lu\n"
	  "Rate/Scale           = %lu/%lu=%5.3f\n"
	  "Start                = %lu\n"
	  "Length               = %lu\n"
	  "Suggested buffer size= %lu\n"
	  "Quality              = %lu\n"
	  "SampleSize           = %lu\n"
	  "FrameRect            = %u.%ux%u.%u\n"
	  "===== BITMAP INFO HEADER ===============\n"
	  "WxH                  = %lux%lu\n"
	  "PlanesxBitCount      = %ux%u\n"
	  "Compression          = %c%c%c%c\n"
	  "ImageSize            = %lu\n"
	  "XxYPelsPerMeter      = %lux%lu\n"
	  "ColorUsedxImportant  = %lux%lu\n"
	  INT_2_CHAR_ARG(strh.fccType)
	  INT_2_CHAR_ARG(strh.fccHandler),strh.fccHandler
	 ,strh.dwFlags
	 ,strh.wPriority
	 ,strh.wLanguage
	 ,strh.dwInitialFrames
	 ,strh.dwRate
	 ,strh.dwScale
	 ,(float)strh.dwRate/(float)strh.dwScale
	 ,strh.dwStart
	 ,strh.dwLength
	 ,strh.dwSuggestedBufferSize
	 ,strh.dwQuality
	 ,strh.dwSampleSize
	 ,strh.rcFrame.left,strh.rcFrame.top,strh.rcFrame.right,strh.rcFrame.bottom
	 ,bmph.biWidth,bmph.biHeight
	 ,bmph.biPlanes,bmph.biBitCount
	 INT_2_CHAR_ARG(bmph.biCompression)
	 ,bmph.biSizeImage
	 ,bmph.biXPelsPerMeter,bmph.biYPelsPerMeter
	 ,bmph.biClrUsed,bmph.biClrImportant);
    while(1) {
	keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
	if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
    }
    delete hwnd;
    return fpos;
}

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) AVI_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info avi_info = {
    "Audio Video Interleaved format",	/**< plugin name */
    query_interface
};
} // namespace	usr
