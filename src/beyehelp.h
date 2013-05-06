/**
 * @namespace   beye
 * @file        beyehelp.h
 * @brief       This file contains prototypes of BEYE help system.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#ifndef __BEYEHELP__H
#define __BEYEHELP__H
#include <fstream>

#include "libbeye/twin.h"

namespace beye {
#define BEYE_HELP_VER "BEYE_HLP v6.1.0"

    enum {
	HLP_SLONG_LEN=9,
	HLP_VER_LEN=16
    };
/** Maximal size of help topic - is 0xFFFF bytes */
    struct beye_help_item {
	char	item_id[HLP_SLONG_LEN]; /**< character representation of short type. Null-terminated */
	char	item_off[HLP_SLONG_LEN];
	char	item_length[HLP_SLONG_LEN];
	char	item_decomp_size[HLP_SLONG_LEN];
    };

    struct beye_help {
	char		help_version[HLP_VER_LEN]; /**< identifiaction signature */
	char		item_count[HLP_SLONG_LEN]; /**< total count of items */
	beye_help_item	items[1];         /**< Array of items */
/**< Binary data of help */
    };

/** Color definition */
    enum {
	HLPC_BOLD_ON		=0x01,
	HLPC_ITALIC_ON		=0x02,
	HLPC_UNDERLINE_ON	=0x03,
	HLPC_STRIKETHROUGH_ON	=0x04,
	HLPC_REVERSE_ON		=0x05,
	HLPC_LINK_ON		=0x06,
	HLPC_BOLD_OFF		=0x11,
	HLPC_ITALIC_OFF		=0x12,
	HLPC_UNDERLINE_OFF	=0x13,
	HLPC_STRIKETHROUGH_OFF	=0x14,
	HLPC_REVERSE_OFF	=0x15,
	HLPC_LINK_OFF		=0x16
    };

    class Beye_Help : public Opaque {
	public:
	    Beye_Help();
	    virtual ~Beye_Help();

	    virtual bool	open(bool interactive);
	    virtual void	close();
		       /** Return uncompressed size of help item
			  0 - if error occured */
	    virtual unsigned long	get_item_size(unsigned long item_id);
	    virtual bool		load_item(unsigned long item_id, any_t* buffer);

		       /** Returns array of char pointers.
			  Title always is data[0] */
	    virtual char **		point_strings(char* data,unsigned long data_size,
							unsigned long *nstr);
		       /** Filles buffer as video memory from string */
	    virtual unsigned		fill_buffer(tvioBuff * dest,unsigned int cw_dest,
						    const std::string& str,unsigned int cb_str,
						    unsigned int shift,unsigned *n_tabs,
						    bool is_hl);
		       /** Paints line of help */
	    virtual void		paint_line(TWindow *win,unsigned y,const std::string& str, bool is_hl);
	    virtual int			__ListBox(const char** names,unsigned nlist,const std::string& title);
	private:
	    void			paint(TWindow *win,const char * * names,unsigned nlist,unsigned start,unsigned height,unsigned width);
	    bool			find_item(unsigned long item_id);

	    std::fstream		fs;
	    beye_help_item		bhi;
    };
		       /** Fully-functionallity utility for displaying help */
    void            __FASTCALL__ hlpDisplay(unsigned long id);
} // namespace beye

#endif
