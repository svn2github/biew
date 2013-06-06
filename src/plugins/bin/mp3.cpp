#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_plugins_auto
 * @file        plugins/bin/mp3.c
 * @brief       This file contains implementation of decoder for MP3
 *              file format.
 * @version     -
 * @remark      this source file is part of mp3ary vIEW project (BEYE).
 *              The mp3ary vIEW (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1995
 * @note        Development, fixes and improvements
**/
#include <algorithm>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "libbeye/bswap.h"
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
    class MP3_Parser : public Binary_Parser {
	public:
	    MP3_Parser(BeyeContext& b,binary_stream&,CodeGuider&,udn&);
	    virtual ~MP3_Parser();

	    virtual const char*		prompt(unsigned idx) const;

	    virtual __filesize_t	show_header() const;
	    virtual int			query_platform() const;

	private:
	    int			mp_decode_mp3_header(unsigned char *hbuf,int *fmt,int *brate,int *samplerate,int *channels) const;
	    int			mp_mp3_get_lsf(unsigned char *hbuf) const __PURE_FUNC__;
	    int			Xing_test(char *hdr,int *scale,int *lsf,int *srate,long *nframes,long *nbytes) const;
	    void		find_next_mp3_hdr(unsigned char *hdr) const;
	    int			read_id3v2_tags() const;
	    int			read_id3v24_tags(unsigned flags,unsigned hsize) const;
	    int			read_id3v23_tags(unsigned flags,unsigned hsize) const;
	    int			read_id3v22_tags(unsigned flags,unsigned hsize) const;

	    BeyeContext&	bctx;
	    binary_stream&	main_handle;
	    udn&		_udn;
    };
static const char* txt[]={ "", "", "", "", "", "", "", "", "", "" };
const char* MP3_Parser::prompt(unsigned idx) const { return txt[idx]; }
//----------------------- mp3 audio frame header parser -----------------------
static const int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};
static const long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000 };

int MP3_Parser::mp_mp3_get_lsf(unsigned char* hbuf) const {
    unsigned long newhead =
      hbuf[0] << 24 |
      hbuf[1] << 16 |
      hbuf[2] <<  8 |
      hbuf[3];
    if( newhead & ((long)1<<20) ) {
      return (newhead & ((long)1<<19)) ? 0x0 : 0x1;
    }
    return 1;
}

/*
 * return frame size or -1 (bad frame)
 */
static const unsigned MAXFRAMESIZE=1280;
int MP3_Parser::mp_decode_mp3_header(unsigned char* hbuf,int *fmt,int *brate,int *samplerate,int *channels) const {
    int nch,ssize,crc,lsf,mpeg25,framesize,padding,bitrate_index,sampling_frequency,mp3_fmt;
    unsigned long newhead =
      hbuf[0] << 24 |
      hbuf[1] << 16 |
      hbuf[2] <<  8 |
      hbuf[3];

    if( (newhead & 0xffe00000) != 0xffe00000 ||
	(newhead & 0x0000fc00) == 0x0000fc00){
	return -1;
    }

    if(((newhead>>17)&3)==0) return -1;

    mp3_fmt = 4-((newhead>>17)&3);
    if(fmt) *fmt = mp3_fmt;

    if( newhead & ((long)1<<20) ) {
      lsf = (newhead & ((long)1<<19)) ? 0x0 : 0x1;
      mpeg25 = 0;
    } else {
      lsf = 1;
      mpeg25 = 1;
    }

    if(mpeg25)
      sampling_frequency = 6 + ((newhead>>10)&0x3);
    else
      sampling_frequency = ((newhead>>10)&0x3) + (lsf*3);

    if(sampling_frequency>8){
	return -1;  // valid: 0..8
    }

    crc = ((newhead>>16)&0x1)^0x1;
    bitrate_index = ((newhead>>12)&0xf);
    padding   = ((newhead>>9)&0x1);

    nch = ( (((newhead>>6)&0x3)) == 3) ? 1 : 2;

    if(channels) *channels=nch;

    if(!bitrate_index){
      return -1;
    }

    if(lsf)
      ssize = (nch == 1) ? 9 : 17;
    else
      ssize = (nch == 1) ? 17 : 32;
    if(crc) ssize += 2;

    switch(mp3_fmt)
    {
	case 0:		return -1;
	case 1:		framesize = (long) tabsel_123[lsf][0][bitrate_index]*12000;
			framesize /= freqs[sampling_frequency];
			framesize = (framesize + padding)<<2;
			break;
	default:
			framesize  = (long) tabsel_123[lsf][mp3_fmt-1][bitrate_index] * 144000;
			if(mp3_fmt == 3)framesize /= freqs[sampling_frequency]<<lsf;
			else		framesize /= freqs[sampling_frequency];
			framesize += padding;
			break;
    }

    if(framesize<=0 || framesize>MAXFRAMESIZE) return -1;
    if(brate) *brate=(tabsel_123[lsf][mp3_fmt-1][bitrate_index]*1000)/8;
    if(samplerate) *samplerate=freqs[sampling_frequency];
    return framesize;
}

/* id3v2 */
#define FOURCC_TAG mmioFOURCC
#define ID3V22_TAG FOURCC_TAG('I', 'D', '3', 2)  /* id3 v2.2 tags */
#define ID3V23_TAG FOURCC_TAG('I', 'D', '3', 3)  /* id3 v2.3 tags */
#define ID3V24_TAG FOURCC_TAG('I', 'D', '3', 4)  /* id3 v2.4 tags */

/*
 *  ID3 v2.2
 */
/* tag header */
#define ID3V22_UNSYNCH_FLAG               0x80
#define ID3V22_COMPRESS_FLAG              0x40
#define ID3V22_ZERO_FLAG                  0x3F

/* frame header */
#define ID3V22_FRAME_HEADER_SIZE             6
int MP3_Parser::read_id3v22_tags(unsigned flags,unsigned hsize) const
{
    __filesize_t pos,epos;
    if(	flags==ID3V22_ZERO_FLAG ||
	flags==ID3V22_UNSYNCH_FLAG ||
	flags==ID3V22_COMPRESS_FLAG) return 0;
    pos=main_handle.tell();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V22_FRAME_HEADER_SIZE],data[4096];
	main_handle.read(buf,ID3V22_FRAME_HEADER_SIZE);
	id=(buf[2] << 16) + (buf[1] << 8) + buf[0];
	len=(buf[3] << 14) + (buf[4] << 7) + buf[5];
	main_handle.read(data,std::min(len,unsigned(4096)));
	data[len]=0;
	pos=main_handle.tell();
    }
    return 1;
}

/*
 *  ID3 v2.3
 */
/* tag header */
#define ID3V23_UNSYNCH_FLAG               0x80
#define ID3V23_EXT_HEADER_FLAG            0x40
#define ID3V23_EXPERIMENTAL_FLAG          0x20
#define ID3V23_ZERO_FLAG                  0x1F

/* frame header */
#define ID3V23_FRAME_HEADER_SIZE            10
#define ID3V23_FRAME_TAG_PRESERV_FLAG   0x8000
#define ID3V23_FRAME_FILE_PRESERV_FLAG  0x4000
#define ID3V23_FRAME_READ_ONLY_FLAG     0x2000
#define ID3V23_FRAME_COMPRESS_FLAG      0x0080
#define ID3V23_FRAME_ENCRYPT_FLAG       0x0040
#define ID3V23_FRAME_GROUP_ID_FLAG      0x0020
#define ID3V23_FRAME_ZERO_FLAG          0x1F1F

int MP3_Parser::read_id3v23_tags(unsigned flags,unsigned hsize) const
{
    __filesize_t pos,epos;
    if(	flags==ID3V23_ZERO_FLAG ||
	flags==ID3V23_UNSYNCH_FLAG) return 0;
    if( flags==ID3V23_EXT_HEADER_FLAG )
    {
	char buf[4];
	unsigned ehsize;
	main_handle.read(buf,4);
	ehsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
	main_handle.seek(ehsize,binary_stream::Seek_Cur);
    }
    pos=main_handle.tell();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V23_FRAME_HEADER_SIZE],data[4096];
	main_handle.read(buf,ID3V23_FRAME_HEADER_SIZE);
	id=*((unsigned long *)buf);
	len=(buf[4] << 21) + (buf[5] << 14) + (buf[6] << 7) + buf[7];
	if((int)len <= 0) return 0;
	main_handle.read(data,std::min(len,unsigned(4096)));
	data[len]=0;
	pos=main_handle.tell();
    }
    return 1;
}

/*
 *  ID3 v2.4
 */
/* tag header */
#define ID3V24_UNSYNCH_FLAG               0x80
#define ID3V24_EXT_HEADER_FLAG            0x40
#define ID3V24_EXPERIMENTAL_FLAG          0x20
#define ID3V24_FOOTER_FLAG                0x10
#define ID3V24_ZERO_FLAG                  0x0F

/* frame header */
#define ID3V24_FRAME_HEADER_SIZE            10
#define ID3V24_FRAME_TAG_PRESERV_FLAG   0x4000
#define ID3V24_FRAME_FILE_PRESERV_FLAG  0x2000
#define ID3V24_FRAME_READ_ONLY_FLAG     0x1000
#define ID3V24_FRAME_GROUP_ID_FLAG      0x0040
#define ID3V24_FRAME_COMPRESS_FLAG      0x0008
#define ID3V24_FRAME_ENCRYPT_FLAG       0x0004
#define ID3V24_FRAME_UNSYNCH_FLAG       0x0002
#define ID3V24_FRAME_DATA_LEN_FLAG      0x0001
#define ID3V24_FRAME_ZERO_FLAG          0x8FB0

int MP3_Parser::read_id3v24_tags(unsigned flags,unsigned hsize) const
{
    __filesize_t pos,epos;
    if(	flags==ID3V24_ZERO_FLAG ||
	flags==ID3V24_UNSYNCH_FLAG) return 0;
    if( flags==ID3V24_EXT_HEADER_FLAG )
    {
	char buf[4];
	unsigned ehsize;
	main_handle.read(buf,4);
	ehsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
	main_handle.seek(ehsize,binary_stream::Seek_Cur);
    }
    pos=main_handle.tell();
    epos=pos+hsize;
    while(pos<epos)
    {
	unsigned long id;
	unsigned len;
	unsigned char buf[ID3V23_FRAME_HEADER_SIZE],data[4096];
	main_handle.read(buf,ID3V23_FRAME_HEADER_SIZE);
	id=*((unsigned long *)buf);
	len=(buf[4] << 21) + (buf[5] << 14) + (buf[6] << 7) + buf[7];
	main_handle.read(data,std::min(len,unsigned(4096)));
	data[len]=0;
	pos=main_handle.tell();
    }
    return 1;
}

int MP3_Parser::read_id3v2_tags() const
{
    char buf[4];
    unsigned vers,rev,flags,hsize;
    main_handle.seek(3,binary_stream::Seek_Set); /* skip 'ID3' */
    vers=main_handle.read(type_byte);
    rev=main_handle.read(type_byte);
    flags=main_handle.read(type_byte);
    main_handle.read(buf,4);
    hsize=(buf[0] << 21) + (buf[1] << 14) + (buf[2] << 7) + buf[3];
    if(vers==2) return read_id3v22_tags(flags,hsize);
    else
    if(vers==3) return read_id3v23_tags(flags,hsize);
    else
    if(vers==4) return read_id3v24_tags(flags,hsize);
    else
    return 1;
}

void MP3_Parser::find_next_mp3_hdr(unsigned char *hdr) const {
  int len;
  __filesize_t spos;
  while(!main_handle.eof()) {
    spos=main_handle.tell();
    main_handle.read(hdr,4);
    if(main_handle.eof()) break;
    len = mp_decode_mp3_header(hdr,NULL,NULL,NULL,NULL);
    if(len < 0) {
      main_handle.seek(-3,binary_stream::Seek_Cur);
      continue;
    }
    main_handle.seek(spos,binary_stream::Seek_Set);
    break;
  }
}

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008
#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)
#define MPG_MD_MONO     3

int MP3_Parser::Xing_test(char *hdr,int *scale,int *lsf,int *srate,long *nframes,long *nbytes) const
{
    __filesize_t fpos;
    unsigned mpeg1, mode, sr_index;
    unsigned off,head_flags;
    char buf[4];
    const int sr_table[4] = { 44100, 48000, 32000, 99999 };
    int is_xing=0;
    *scale=-1;
    mpeg1    = (hdr[1]>>3)&1;
    sr_index = (hdr[2]>>2)&3;
    mode     = (hdr[3]>>6)&3;
    if(mpeg1)	off=mode!=MPG_MD_MONO?32:17;
    else	off=mode!=MPG_MD_MONO?17:9;/* mpeg2 */
    fpos = main_handle.tell();
    main_handle.seek(off,binary_stream::Seek_Cur);
    main_handle.read(buf,4);
    if(memcmp(buf,"Xing",4) == 0 || memcmp(buf,"Info",4) == 0)
    {
	is_xing=1;
	*nbytes=*nframes=0;
	*lsf=mpeg1?0:1;
	*srate=sr_table[sr_index&0x3];
	head_flags = be2me_32(main_handle.read(type_dword));
	if(head_flags & FRAMES_FLAG)	*nframes=be2me_32(main_handle.read(type_dword));
	if(head_flags & BYTES_FLAG)	*nbytes=be2me_32(main_handle.read(type_dword));
	if(head_flags & TOC_FLAG)	main_handle.seek(100,binary_stream::Seek_Cur); /* skip indexes */
	if(head_flags & VBR_SCALE_FLAG)	*scale = be2me_32(main_handle.read(type_dword));
    }
    main_handle.seek(fpos,binary_stream::Seek_Set);
    return is_xing;
}

__filesize_t MP3_Parser::show_header() const
{
 unsigned keycode;
 TWindow * hwnd;
 __filesize_t fpos,fpos2;
 int fmt = 0,mp3_brate,mp3_samplerate,mp3_channels;
 unsigned char hdr[4];
 int len;
 int scale,lsf,srate;
 long nframes,nbytes,ave_brate;
 int is_xing=0;
 fpos2 = fpos = bctx.tell();
 main_handle.seek(0,binary_stream::Seek_Set);
 main_handle.read(hdr,4);
 if( hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && (hdr[3] >= 2))
 {
	main_handle.seek(2,binary_stream::Seek_Cur);
	main_handle.read(hdr,4);
	len = (hdr[0]<<21) | (hdr[1]<<14) | (hdr[2]<<7) | hdr[3];
	read_id3v2_tags();
	main_handle.seek(len+10,binary_stream::Seek_Set);
	find_next_mp3_hdr(hdr);
	fpos2=main_handle.tell();
 }
 is_xing=Xing_test((char*)hdr,&scale,&lsf,&srate,&nframes,&nbytes);
 mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels);
 ave_brate=mp3_brate*8;
 if(is_xing)
 {
    ave_brate=(((float)(nbytes)/nframes)*(float)(srate<<lsf))/144.;
    find_next_mp3_hdr(hdr);
    fpos2=main_handle.tell();
 }
 hwnd = CrtDlgWndnls(" MP3 File Header ",43,4);
 hwnd->goto_xy(1,1);
 hwnd->printf(
	  "Type                 = MP3v%u:%s\n"
	  "Bitrate              = %u\n"
	  "SampleRate           = %u\n"
	  "nChannels            = %u\n"
	  ,fmt
	  ,is_xing?"VBR":"CBR"
	  ,ave_brate
	  ,is_xing?srate:mp3_samplerate
	  ,mp3_channels
	  );
 while(1)
 {
   keycode = GetEvent(drawEmptyPrompt,NULL,hwnd);
   if(keycode == KE_F(5) || keycode == KE_ENTER) { fpos = fpos2; break; }
   else
     if(keycode == KE_ESCAPE || keycode == KE_F(10)) break;
 }
 delete hwnd;
 return fpos;
}

MP3_Parser::MP3_Parser(BeyeContext& b,binary_stream& h,CodeGuider& code_guider,udn& u)
	    :Binary_Parser(b,h,code_guider,u)
	    ,bctx(b)
	    ,main_handle(h)
	    ,_udn(u)
{
    unsigned i;
    unsigned long off;
    int fmt = 0,mp3_brate,mp3_samplerate,mp3_channels;
    unsigned char hdr[4];
    main_handle.seek(0,binary_stream::Seek_Set);
    main_handle.read(hdr,4);
    if( hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && (hdr[3] >= 2)) {
	int len;
	main_handle.seek(2,binary_stream::Seek_Cur);
	main_handle.read(hdr,4);
	len = (hdr[0]<<21) | (hdr[1]<<14) | (hdr[2]<<7) | hdr[3];
	read_id3v2_tags();
	main_handle.seek(len+10,binary_stream::Seek_Set);
	find_next_mp3_hdr(hdr);
//	Xing_test(hdr,&scale,&lsf,&srate,&nframes,&nbytes);
	if(mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels) <= 0) throw bad_format_exception();
    } else {
	if(mp_decode_mp3_header(hdr,NULL,NULL,NULL,NULL) <= 0) throw bad_format_exception();
	find_next_mp3_hdr(hdr);
	if(main_handle.eof()) throw bad_format_exception();
	for(i=0;i<5;i++) {
	    if((long)(off=mp_decode_mp3_header(hdr,&fmt,&mp3_brate,&mp3_samplerate,&mp3_channels)) <= 0) throw bad_format_exception();
	    main_handle.seek(off,binary_stream::Seek_Cur);
	    if(main_handle.eof()) throw bad_format_exception();
	}
    }
}
MP3_Parser::~MP3_Parser() {}
int MP3_Parser::query_platform() const { return DISASM_DEFAULT; }

static Binary_Parser* query_interface(BeyeContext& b,binary_stream& h,CodeGuider& _parent,udn& u) { return new(zeromem) MP3_Parser(b,h,_parent,u); }
extern const Binary_Parser_Info mp3_info = {
    "MP3 file format",	/**< plugin name */
    query_interface
};
} // namespace	usr
