/**
 * @namespace	usr
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
#include <limits>
#include <string>

#include "libbeye/bstream.h"
#include "beye.h"

namespace	usr {
    enum {
	BBIO_CACHE_SIZE        =0xFFFF,  /* 64k */
	BBIO_SMALL_CACHE_SIZE  =0x4000  /* 16k */
    };

    inline bool		is_BMUse64() { return beye_context().bm_file().flength() > std::numeric_limits<uint32_t>::max(); }
    inline unsigned	HA_LEN() { return is_BMUse64()?18:10; }

    inline int		BMHandle() { return beye_context().bm_file().handle(); }
    inline binary_stream&	BMbioHandle() { return beye_context().bm_file(); }
    inline std::string	BMName() { return beye_context().bm_file().filename(); }
    inline __filesize_t	BMGetCurrFilePos() { return beye_context().bm_file().tell(); }
    inline __filesize_t	BMGetFLength() { return beye_context().bm_file().flength(); }
    inline bool		BMEOF() { return beye_context().bm_file().eof(); }
    inline void		BMSeek(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); }
    inline void		BMReRead()  { beye_context().bm_file().reread(); }
    inline uint8_t	BMReadByte() { return beye_context().bm_file().read(type_byte); }
    inline uint16_t	BMReadWord() { return beye_context().bm_file().read(type_word); }
    inline uint32_t	BMReadDWord() { return beye_context().bm_file().read(type_dword); }
    inline uint64_t	BMReadQWord() { return beye_context().bm_file().read(type_qword); }
    inline bool		BMReadBuffer(any_t* buffer,unsigned len) { return beye_context().bm_file().read(buffer,len); }
    inline uint8_t	BMReadByteEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().read(type_byte); }
    inline uint16_t	BMReadWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().read(type_word); }
    inline uint32_t	BMReadDWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().read(type_dword); }
    inline uint64_t	BMReadQWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().read(type_qword); }
    inline bool		BMReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,binary_stream::e_seek origin) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().read(buffer,len); }

    inline bool		BMWriteBuff(any_t* buff,unsigned len) { bool ret; ret = beye_context().bm_file().write(buff,len);  beye_context().bm_file().flush(); return ret; }
    inline bool		BMWriteBuffEx(__fileoff_t pos,binary_stream::e_seek origin,any_t* buff,unsigned len) { beye_context().bm_file().seek(pos,origin); return beye_context().bm_file().write(buff,len); }

/** Below analogs with using small cache size */

    inline bool		bmEOF() { return beye_context().sc_bm_file().eof(); }
    inline void		bmSeek(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); }
    inline void		bmReRead() { beye_context().sc_bm_file().reread(); }
    inline uint8_t	bmReadByte() { return beye_context().sc_bm_file().read(type_byte); }
    inline uint16_t	bmReadWord() { return beye_context().sc_bm_file().read(type_word); }
    inline uint32_t	bmReadDWord() { return beye_context().sc_bm_file().read(type_dword); }
    inline uint64_t	bmReadQWord() { return beye_context().sc_bm_file().read(type_qword); }
    inline bool		bmReadBuffer(any_t* buffer,unsigned len) { return beye_context().sc_bm_file().read(buffer,len); }
    inline uint8_t	bmReadByteEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); return beye_context().sc_bm_file().read(type_byte); }
    inline uint16_t	bmReadWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); return beye_context().sc_bm_file().read(type_word); }
    inline uint32_t	bmReadDWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); return beye_context().sc_bm_file().read(type_dword); }
    inline uint64_t	bmReadQWordEx(__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); return beye_context().sc_bm_file().read(type_qword); }
    inline bool		bmReadBufferEx(any_t* buffer,unsigned len,__fileoff_t pos,binary_stream::e_seek origin) { beye_context().sc_bm_file().seek(pos,origin); return beye_context().sc_bm_file().read(buffer,len); }
    inline int		bmHandle() { return beye_context().sc_bm_file().handle(); }
    inline binary_stream&	bmbioHandle() { return beye_context().sc_bm_file(); }
    inline std::string	bmName() { return beye_context().sc_bm_file().filename(); }
    inline __filesize_t	bmGetCurrFilePos() { return beye_context().sc_bm_file().tell(); }
    inline __filesize_t	bmGetFLength() {  return beye_context().sc_bm_file().flength(); }
    inline bool		bmWriteBuff(any_t* buff,unsigned len) { bool ret; ret = beye_context().sc_bm_file().write(buff,len); beye_context().sc_bm_file().flush(); return ret; }
    inline bool		bmWriteBuffEx(__fileoff_t pos,binary_stream::e_seek origin,any_t* buff,unsigned len) { beye_context().sc_bm_file().seek(pos,origin);  return beye_context().sc_bm_file().write(buff,len); }
} // namespace	usr
#endif
