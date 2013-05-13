#ifndef BINARY_PARSER_HPP_INCLUDED
#define BINARY_PARSER_HPP_INCLUDED 1

namespace	usr {
    class binary_stream;
    class CodeGuider;
    class Binary_Parser : public Opaque {
	public:
	    Binary_Parser(binary_stream&,CodeGuider&) {}
	    virtual ~Binary_Parser() {}

	    virtual const char*		prompt(unsigned idx) const = 0;   /**< on ALT-Fx selection */

	    virtual __filesize_t	action_F1() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F2() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F3() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F4() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F5() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F6() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F7() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F8() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F9() { return Plugin::Bad_Address; }
	    virtual __filesize_t	action_F10(){ return Plugin::Bad_Address; }

	    virtual __filesize_t	show_header() { return Plugin::Bad_Address; }  /**< if not an MZ style format */
			/**
			  Binss disassembler reference to string.
			   * @param str          string buffer for append to
			   * @param shift        physical address of field, that required of binding
			   * @param flags        see above
			   * @param codelen      length of field, that required binding
			   * @param r_shift      used only if APPREF_TRY_LABEL mode is set, contains real value of field, that required binding
			   * @return             true if reference was appended
			*/
	    virtual bool		bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift) { UNUSED(_parent); UNUSED(str); UNUSED(shift); UNUSED(flg); UNUSED(codelen); UNUSED(r_shift); return false; }

			 /** Returns CPU platform, that required by format.
			   * @note           Full list of platform please see in
			   *                 plugins/disasm.h file. If this
			   *                 function return -1 then platform is
			   *                 undefined.
			  **/
	    virtual int			query_platform() const = 0;

			 /** Returns DAB_XXX. Quick version for disassembler */
	    virtual int			query_bitness(__filesize_t) const { return DAB_USE16; }

			 /** Returns DAE_XXX. */
	    virtual int			query_endian(__filesize_t) const { return DAE_LITTLE; }

			 /** For displaying offset within struct in left address column.
			   * @return         false if string is not modified.
			  **/
	    virtual bool		address_resolving(char *,__filesize_t) { return false; }

			 /** Converts virtual address to physical (means file offset).
			   * @param va       indicates virtual address to be converted
			   * @return         0 if operation meaningless
			  **/
	    virtual __filesize_t	va2pa(__filesize_t) { return Plugin::Bad_Address; }

			 /** Converts physical address to virtual.
			   * @param pa       indicates physical address to be converted
			   * @note           seg pointer can be NULL
			  **/
	    virtual __filesize_t	pa2va(__filesize_t) { return Plugin::Bad_Address; }

			 /** Fills the string with public symbol
			   * @param str       pointer to the string to be filled
			   * @param cb_str    indicates maximal length of string
			   * @param _class    pointer to the memory where can be stored class of symbol (See SC_* conatnts)
			   * @param pa        indicates physical offset within file
			   * @param as_prev   indicates direction of symbol searching from given physical offset
			   * @return          0 - if no symbol name available
			   *                  in given direction (as_prev)
			   *                  physical address of public symbol
			   *                  which is found in given direction
			  **/
	    virtual __filesize_t	get_public_symbol(char* str,unsigned cb_str,unsigned *_class,
							    __filesize_t pa,bool as_prev) { UNUSED(str); UNUSED(cb_str); UNUSED(_class); UNUSED(pa); UNUSED(as_prev); return Plugin::Bad_Address; }

			 /** Determines attributes of object at given physical file address.
			   * @param pa        indicates physical file offset of object
			   * @param name      pointer to the string which is to be filled with object name
			   * @param cb_name   indicates maximal length of string
			   * @param start     pointer to the memory where must be stored start of given object, as file offset.
			   * @param end       pointer to the memory where must be stored end of given object, as file offset.
			   * @param _class    pointer to the memory where must be stored _class of object (See OC_* constants).
			   * @param bitness   pointer to the memory where must be stored bitness of object (See DAB_* constants).
			   * @return          logical number of object or 0 if at given offset is no object.
			   * @note            all arguments exclude name of object
			   *                  must be filled.
			   * @remark          For example: if exe-format - new
			   *                  exe i.e. contains MZ and NEW
			   *                  header and given file offset
			   *                  points to old exe stub then start
			   *                  = 0, end = begin of first data or
			   *                  code object).
			  **/
	    virtual unsigned		get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,
							__filesize_t *start,__filesize_t *end,int *_class,int *bitness) { UNUSED(pa); UNUSED(name); UNUSED(cb_name); UNUSED(start); UNUSED(end); UNUSED(_class); UNUSED(bitness); return 0; }
    };

    struct Binary_Parser_Info {
	const char*	name;
	bool		(*probe)(binary_stream& handle); /**< Checks format */
	Binary_Parser* (*query_interface)(binary_stream&,CodeGuider&);
    };
} // namespace	usr
#endif
