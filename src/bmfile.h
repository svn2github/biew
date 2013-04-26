/**
 * @namespace   beye
 * @file        bmfile.h
 * @brief       This file contains prototypes of Buffering streams Manager.
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
#ifndef __BMFILE_INC
#define __BMFILE_INC

#include <string>
#ifndef __BBIO_H
#include "libbeye/bbio.h"
#endif

namespace beye {

    enum {
	BMFF_NONE=0x00000000,
	BMFF_USE64=0x00000001
    };
    extern unsigned BMFileFlags;
    inline unsigned HA_LEN() { return (BMFileFlags&BMFF_USE64)?18:10; }

    enum {
	BBIO_CACHE_SIZE        =0xFFFF,  /* 64k */
	BBIO_SMALL_CACHE_SIZE  =0x4000  /* 16k */
    };

    BFile*        __FASTCALL__ beyeOpenRO(const std::string& fname,unsigned cache_size);
    BFile*        __FASTCALL__ beyeOpenRW(const std::string& fname,unsigned cache_size);

    enum {
	BM_SEEK_SET=BFile::Seek_Set,
	BM_SEEK_CUR=BFile::Seek_Cur,
	BM_SEEK_END=BFile::Seek_End
    };
    extern BFile& bm_file_handle,&sc_bm_file_handle;

    int            __FASTCALL__ BMOpen(const std::string& fname);
    void           __FASTCALL__ BMClose( void );
    inline bhandle_t		__FASTCALL__ BMHandle( void ) { return bm_file_handle.handle(); }
    inline BFile&		__FASTCALL__ BMbioHandle( void ) { return bm_file_handle; }
    inline const char *		__FASTCALL__ BMName( void ) { return bm_file_handle.filename(); }
    inline __filesize_t	__FASTCALL__ BMGetCurrFilePos( void ) { return bm_file_handle.tell(); }
    inline __filesize_t	__FASTCALL__ BMGetFLength( void ) { return bm_file_handle.flength(); }
    inline bool	__FASTCALL__ BMEOF( void ) { return bm_file_handle.eof(); }
    inline void	__FASTCALL__ BMSeek(__fileoff_t pos,int RELATION) { bm_file_handle.seek(pos,RELATION); }
    inline void	__FASTCALL__ BMReRead( void )  { bm_file_handle.reread(); }
    inline uint8_t	__FASTCALL__ BMReadByte( void ) { return bm_file_handle.read_byte(); }
    inline uint16_t	__FASTCALL__ BMReadWord( void ) { return bm_file_handle.read_word(); }
    inline uint32_t	__FASTCALL__ BMReadDWord( void ) { return bm_file_handle.read_dword(); }
    inline uint64_t	__FASTCALL__ BMReadQWord( void ) { return bm_file_handle.read_qword(); }
    inline bool	__FASTCALL__ BMReadBuffer(any_t* buffer,unsigned len) { return bm_file_handle.read_buffer(buffer,len); }
    uint8_t        __FASTCALL__ BMReadByteEx(__fileoff_t pos,int RELATION);
    uint16_t       __FASTCALL__ BMReadWordEx(__fileoff_t pos,int RELATION);
    uint32_t       __FASTCALL__ BMReadDWordEx(__fileoff_t pos,int RELATION);
    uint64_t       __FASTCALL__ BMReadQWordEx(__fileoff_t pos,int RELATION);
    bool         __FASTCALL__ BMReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,int RELATION);
    inline bool	__FASTCALL__ BMWriteByte(uint8_t byte) { return bm_file_handle.write_byte(byte); }
    inline bool	__FASTCALL__ BMWriteWord(uint16_t word) { return bm_file_handle.write_word(word); }
    inline bool	__FASTCALL__ BMWriteDWord(uint32_t dword) { return bm_file_handle.write_dword(dword); }
    inline bool	__FASTCALL__ BMWriteQWord(uint64_t qword) { return bm_file_handle.write_qword(qword); }
    bool		__FASTCALL__ BMWriteBuff(any_t* buff,unsigned len);
    bool		__FASTCALL__ BMWriteByteEx(__fileoff_t pos,int RELATION,uint8_t byte);
    bool		__FASTCALL__ BMWriteWordEx(__fileoff_t pos,int RELATION,uint16_t word);
    bool		__FASTCALL__ BMWriteDWordEx(__fileoff_t pos,int RELATION,uint32_t dword);
    bool		__FASTCALL__ BMWriteBuffEx(__fileoff_t pos,int RELATION,any_t* buff,unsigned len);

/** Below analogs with using small cache size */

    inline bool	__FASTCALL__ bmEOF( void ) { return sc_bm_file_handle.eof(); }
    inline void	__FASTCALL__ bmSeek(__fileoff_t pos,int RELATION) { sc_bm_file_handle.seek(pos,RELATION); }
    inline void	__FASTCALL__ bmReRead( void ) { sc_bm_file_handle.reread(); }
    inline uint8_t	__FASTCALL__ bmReadByte( void ) { return sc_bm_file_handle.read_byte(); }
    inline uint16_t	__FASTCALL__ bmReadWord( void ) { return sc_bm_file_handle.read_word(); }
    inline uint32_t	__FASTCALL__ bmReadDWord( void ) { return sc_bm_file_handle.read_dword(); }
    inline uint64_t	__FASTCALL__ bmReadQWord( void ) { return sc_bm_file_handle.read_qword(); }
    inline bool	__FASTCALL__ bmReadBuffer(any_t* buffer,unsigned len) { return sc_bm_file_handle.read_buffer(buffer,len); }
    uint8_t        __FASTCALL__ bmReadByteEx(__fileoff_t pos,int RELATION);
    uint16_t       __FASTCALL__ bmReadWordEx(__fileoff_t pos,int RELATION);
    uint32_t       __FASTCALL__ bmReadDWordEx(__fileoff_t pos,int RELATION);
    uint64_t       __FASTCALL__ bmReadQWordEx(__fileoff_t pos,int RELATION);
    bool         __FASTCALL__ bmReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,int RELATION);
    inline bhandle_t	__FASTCALL__ bmHandle( void ) { return sc_bm_file_handle.handle(); }
    inline BFile&	__FASTCALL__ bmbioHandle( void ) { return sc_bm_file_handle; }
    inline const char*	__FASTCALL__ bmName( void ) { return sc_bm_file_handle.filename(); }

    inline __filesize_t	__FASTCALL__ bmGetCurrFilePos( void ) { return sc_bm_file_handle.tell(); }
    inline __filesize_t	__FASTCALL__ bmGetFLength( void ) {  return sc_bm_file_handle.flength(); }

    inline bool	__FASTCALL__ bmWriteByte(uint8_t byte) { return sc_bm_file_handle.write_byte(byte); }
    inline bool	__FASTCALL__ bmWriteWord(uint16_t word) { return sc_bm_file_handle.write_word(word); }
    inline bool	__FASTCALL__ bmWriteDWord(uint32_t dword) { return sc_bm_file_handle.write_dword(dword); }
    inline bool	__FASTCALL__ bmWriteQWord(uint64_t qword) { return sc_bm_file_handle.write_qword(qword); }
    bool          __FASTCALL__ bmWriteBuff(any_t* buff,unsigned len);
    bool          __FASTCALL__ bmWriteByteEx(__fileoff_t pos,int RELATION,uint8_t byte);
    bool          __FASTCALL__ bmWriteWordEx(__fileoff_t pos,int RELATION,uint16_t word);
    bool          __FASTCALL__ bmWriteDWordEx(__fileoff_t pos,int RELATION,uint32_t dword);
    bool          __FASTCALL__ bmWriteQWordEx(__fileoff_t pos,int RELATION,uint64_t dword);
    bool          __FASTCALL__ bmWriteBuffEx(__fileoff_t pos,int RELATION,any_t* buff,unsigned len);
} // namespace beye
#endif
