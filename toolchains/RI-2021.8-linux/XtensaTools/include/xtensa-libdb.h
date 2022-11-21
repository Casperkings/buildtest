/*  xtensa-libdb.h  -  Xtensa libdb library header file  */

/* $Id: //depot/rel/Homewood/ib.8/Xtensa/Software/libdb/xtensa-libdb.h#1 $ */

/* Copyright (c) 2003-2015 Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/*
 * The purpose of this library is to give a debugger writer the ability to
 * find out everything they need to know to debug a target.  This includes
 * the Xtensa simulator, OCD, XMON, RedBoot and other targets.  To do this,
 * the debugger writer needs to know certain information, including a list
 * of register names, and how to instruct the target to return that information
 * if the target does not already know how to return that information.
 *
 * INTERNAL NOTES:
 *
 * The Xtensa libdb library calls upon Xtensa libisa and libxtparams DLLs/libraries.
 * It also calls upon various C library functions, including malloc()/free()
 * for general allocation.  NEVER calls printf, fprintf, or exit.
 */



#ifndef __H_LIBDB
#define __H_LIBDB

#ifndef STATIC_DB
#include "xtensa-params.h"
#endif
#include "xtensa-libdb-macros.h"
#include "xtensa-isa.h"

#ifdef __cplusplus
extern "C" {
#endif


/*  Register property flags for the 'flags' member of xtensa_dbreg_info:  */
#define XTENSA_DBREGF_STATE	0x0001	/* is a "state" for listing by "info states" */
#define XTENSA_DBREGF_USER	0x0002	/* is defined by user (otherwise, by core) */
#define XTENSA_DBREGF_REAL_VIEW	0x0004	/* is view or portion(s) of other register(s)
					   (in terms of simulator, or actual registers) */
#define XTENSA_DBREGF_CODE_VIEW	0x0008	/* is view or portion(s) of other register(s)
					   (in terms of what can directly be accessed
					    by the programmer, i.e. via instructions) */
#define XTENSA_DBREGF_PARTMAP	0x0010	/* mapping array is incomplete (ie. state not
					   mapped or only partially mapped onto sysregs) */
#define XTENSA_DBREGF_PRIV	0x0020	/* register is privileged (access at CRING==0) */
#define XTENSA_DBREGF_READABLE	0x0040	/* register is readable */
#define XTENSA_DBREGF_WRITABLE	0x0080	/* register is writable */
#define XTENSA_DBREGF_RVOLATILE	0x0100	/* register is volatile on reads; ie. either:
					   - value may change from external factors
					     between reads (eg. INTERRUPT register)
					   - reads have side-effects */
#define XTENSA_DBREGF_WVOLATILE	0x0200	/* register is volatile on writes; ie. either:
					   - writes are not "normal writes" (either
					     to stateful bits or ignored)
					     (eg. INTSET and INTCLEAR are set-to-set and
					      set-to-clear, not straightforward writes,
					      so they have this bit set)
					   - writes have side-effects that can't be
					     undone by restoring the value written,
					     or have side-effects when writing the
					     value that's already there
					     (eg. CCOMPAREn have such a side-effect,
					     but not PS because its effects are a
					     function of its value rather than a
					     function of its being written to)
					 */
#define XTENSA_DBREGF_HIDDEN	0x0400	/* register is not visible to users
					   in debuggers, etc (true of certain collapsed
					   states defined for convenience of core TIE) */
#define XTENSA_DBREGF_SPLIT	0x0800	/* register is one of a pair of register entries
					   (consecutive in xtensa_dbreg indices) sharing
					   the same target_number, but with different
					   names, the first of which is read-only and
					   the second write-only */
/*#define XTENSA_DBREGF_MCONTEXT 0x1000*//* if multiple contexts are configured,
					     register is replicated for each context */
#define XTENSA_DBREGF_CALLEE_S	0x2000	/* register is callee-saved (for regfiles only) */
#define XTENSA_DBREGF_EXTERNAL	0x4000	/* external register accessible with RER/WER
					     (register address is in regfile_index) */


/*
   This interface basically defines a number of abstract data types.

   . debug info - information useful to debuggers in general
   . a debug register - info about all debugger accessible state
   . a debug register array - info about arrays of such states

   The interface defines a set of functions to access each data type.
   */

typedef struct xtensa_debug_opaque { int unused; } *xtensa_debug;                                 

/* Debug registers are represented here using sequential integers
   beginning with 0.  The specific values are only fixed for a particular
   instantiation of an Xtensa ISA, so these values should only be used
   internally.  An invalid xtensa_dbreg value is represented by the macro
   XTENSA_UNDEFINED, defined in xtensa-isa.h to be -1.  */

typedef int xtensa_dbreg;	/* debugger-visible register or state */
typedef int xtensa_dbarray;	/* array of dbregs */




/*----------------------------------------------------------------
 * This structure defines an element of an array that describes
 * a sequence of bits (eg. a TIE state), or "bit run",
 * that is logically seen as a single independent entity but is
 * really mapped onto one or more other registers.
 * Entries in the array describe successive portions
 * of this sequence of bits, starting from the least significant
 * bit of the sequence (logically numbered zero) up to
 * the most significant bit.  Thus, the total length of the
 * sequence of bits described by such an array is the sum of
 * the numBits field of all entries in the array.
 *
 * This feature is used to describe such things as PS.INTLEVEL or b0..b15,
 * and TIE states (which map to any portions of user registers,
 * in any order, as defined by the TIE description).
 */

typedef struct xtensa_dbreg_mapping_struct
{
  int	num_bits;	/* number of bits in mapped bitfield */
  int	this_low_bit;	/* lsb of bitfield in mapped register (0=lsbit)
			   (increases monotonically in array of mappings,
			    without any overlap in mapped register;
			    there may be gaps, assumed zero, but only if
			    the XTENSA_DBREGF_PARTMAP flag bit is set) */
  int	target_low_bit;	/* lsb of bitfield in register 'target_number' (0=lsbit) */
  int	target_number;	/* target_number of register from which a bitfield
			   is used to construct the mapped register,
			   or XTENSA_UNDEFINED when referring to the
			   sign-extension from the previous instance
			   of this structure in the array */
} xtensa_dbreg_mapping;





/*----------------------------------------------------------------
 *  xtensa_dbarray_info
 * 
 *  This structure contains the definition of an array of registers.
 */

typedef struct xtensa_dbarray_info_struct
{
  int			struct_size;	/* size of this structure in bytes (unused?) */
  xtensa_dbarray	dbarray;	/* index of register array in list of reg arrays */
  xtensa_dbarray	dbarray_parent;	/* dbarray containing this one, or XTENSA_UNDEFINED */
  const char *		name;		/* name of register array (long name of regfile,
					   or "_sreg" or "_ureg" ...) */
  int			num_entries;	/* number of entries in this register array */

  xtensa_dbreg		dbreg_base;	/* first register of array in libdb's list of registers,
					   if all array members are consecutive and contiguous
					   in that list; XTENSA_UNDEFINED otherwise,
					   eg. for sparse arrays (such as sregs, uregs);
					   xtensa_dbreg_lookup_dbarray() always works, even for
					   sparse arrays */

  xtensa_regfile	regfile;	/* libisa ID of register file, or XTENSA_UNDEFINED */
} xtensa_dbarray_info;




/*----------------------------------------------------------------
 *  xtensa_dbreg_info
 * 
 *  This structure contains the definition of a register.
 *
 *  Notes on specific fields:
 *
 *	name		Includes decimal register index suffix, if part of a register file.
 *			Currently, all names are forced to all lowercase (!!).
 *			Guaranteed unique among all registers reported by libdb.
 *
 *	mem_size
 *	mem_align	For register file entries, memory size and alignment are based
 *			on the register file's assigned ctype; otherwise alignment is
 *			four and size is a multiple of four bytes (future releases *may*
 *			optimize to smaller memory sizes where possible, eg. special
 *			registers that take 16 bits or less, etc).
 *
 *	target_number	Register number that a debug agent on a remote target can use.
 *			This is used in the GDB protocol, so all of GDB's debug agents
 *			see this number when references are made to specific registers.
 *			Guaranteed unique among all registers reported by libdb.
 *			This is also used as the register number for ELF (stabs/dwarf/etc)
 *			debug info sections (dwarf not using this yet as of this writing).
 *			See XTENSA_DBREGN_xxx macros in xtensa-libdb-macros.h for details
 *			on the encoding of this number.
 *
 *			HISTORICAL NOTE:  Previous releases (T1050 and earlier) used
 *			separate numbers for target agents and for debug info sections;
 *			target numbers were formatted in 3 specific fields (type in
 *			bits 31..24, TIE regfile ID in bits 23..16, and index in 15..0);
 *			debug numbers were same as today's target_number for a0..a15
 *			and b0..b15, but were 32..35 for m0..m3 and 256-and-up for
 *			TIE regfiles (in the order read in from the libcc DLL).
 *
 *	num_mappings
 *	mapping		These can be used to construct the value of the register
 *			in terms of other registers (e.g. if the REAL_VIEW or CODE_VIEW
 *			flag bit is set)
 */

typedef struct xtensa_dbreg_info_struct
{
  int			struct_size;	/* size of this structure in bytes (unused?) */

  xtensa_dbreg		dbreg;		/* index of register in libdb's register array */
  xtensa_dbarray	dbarray;	/* register array this is part of, or XTENSA_UNDEFINED
					   (if contained in multiple arrays, this indicates
					    the smallest containing array, ie. parent array) */

  const char *		name;		/* name of register */
  int			target_number;	/* target debug-agent recognizable register number;
					   see XTENSA_DBREGN_xxx macros in xtensa-libdb-macros.h */
  unsigned int		flags;		/* register property flags (XTENSA_DBREGF_xxx) */
  int			width;		/* size (width) in bits of this register */
  unsigned int		mem_size;	/* size of register in memory, in bytes */
  unsigned int		mem_align;	/* minimum required alignment of register in memory,
					   in bytes (always power of 2, always at least 1) */
  int			coprocessor;	/* 0..7, or XTENSA_UNDEFINED if not assigned to a cp */

  const char *		package;	/* name of package containing this register, NULL if none */

  /*  Descriptions of virtual registers and mappings between registers:  */
  int			num_mappings;	/* number of mapping entries (available via xtensa_dbreg_get_mapping()) */
  xtensa_dbreg		windex_dbreg;	/* if this register is a view (has one of the flags
					   XTENSA_DBREGF_{REAL,CODE}_VIEW set), and it is a
					   windowing type of view, this is the register that
					   indexes into the xtensa_dbarray_get_info(pxdo,dbarray)->dbarray_parent array;
					   XTENSA_UNDEFINED otherwise */
  int			windex_factor;	/* amount by which to multiply the windex_dbreg register's
					   value before adding as index into parent array
					   (the regfile_index field is also added to the index);
					   total index is modulo parent array's size */

  /*  Correspondence to libisa:  */
  xtensa_state		state;		/* libisa ID of state, or XTENSA_UNDEFINED */
  xtensa_regfile	regfile;	/* libisa ID of register file, or XTENSA_UNDEFINED */
  int			regfile_index;	/* index into the register file, if regfile != XTENSA_UNDEFINED;
					   else if XTENSA_DBREGF_EXTERNAL flag is set, is ERI address;
					   else is undefined */
  xtensa_ctype		ctype;		/* libisa ID of register's ctype, or XTENSA_UNDEFINED */

  /*  Other fields to consider adding in the future:  */
  /*...		spill_code,restore_code...;*/	/* precomputed xtensa_dbreg_get_code(store==1,0) */
  /*...		...;*/			/* spill/restore code and data size requirements, etc */
  /*...		dependency...;*/	/* dependency info (eg. for spilling/restoring)... */

  /*TODO: hide this field.*/
  /*  This field must NEVER be accessed directly.  Use xtensa_dbreg_get_mapping() instead.  */
  const xtensa_dbreg_mapping* const *
			_libdb_internal_mapping;	/* ptr to array of pointers to map structs;
					   NULL if none available */
} xtensa_dbreg_info;
#define _libdb_internal_mapping	hidden_mapping


#if 0

   user code:

   while (dbreg->windex_dbreg != XTENSA_UNDEFINED)
     dbreg = index_dbreg(pxdo, dbreg, ...value of windex_dbreg...)

   index_dbreg(pxdo, dbreg, *array, *index, windex_value)
   	index = (dbreg->regfile_index + windex_value * dbreg->windex_factor)
		% dbreg->array->parent->num_entries;
   	return xtensa_dbreg_lookup_dbarray(pxdo, dbreg->array->parent->dbarray, index);

   OR:

   dbreg = ...
   index = dbreg->regfile_index;
   array = xtensa_dbarray_get_info(pxdo, dbreg->dbarray);
   while (array->windex_dbreg != XTENSA_UNDEFINED)
     dbreg = index_dbreg(pxdo, dbreg, &array, &index, ...value of windex_dbreg...)

   index_dbreg(pxdo, dbreg, *array, *index, windex_value)
   	*index += windex_value * array->windex_factor;
	*array = array->parent;
	*index %= array->num_entries;
   	return array->dbreg_{base,sparse}[*index];

#endif /*0*/



/*----------------------------------------------------------------
 * Function:	xtensa_debug_init
 *
 * Description:	Initialize the libdb API.
 *		Returns a value ("handle") that must be passed to
 *		all other libdb API functions.
 *
 *		INTERNALLY:  enumerates Xtensa processor registers/state, etc.
 *		The handle is opaque to the caller.  Internally it is usually
 *		a pointer to a block of memory used internally by the implementation
 *		of this API, however the structure of this block is highly subject
 *		to change and must never be accessed directly.
 *
 * Parameters:	isa    -- handle to Xtensa ISA as returned by xtensa_isa_init()
 *			  (see xtensa-isa.h); if NULL is passed, calls xtensa_isa_init()
 *			  using information taken from 'params' (the parameter file)
 * 		params -- handle to core parameters as returned by xtensa_getopt()
 *			  or xtensa_params_init() (see xtensa-params.h);
 *			  in the static version of libdb, Xtensa parameters are fixed
 *			  so this parameter is not needed
 *		errno_p -- ignored if NULL; otherwise, *errno_p is set to 0 on
 *			  success, and to an error code on failure (XTENSA_DEBUG_Exxx);
 *			  see also xtensa_debug_errno()
 *		error_msg_p -- ignored if NULL; otherwise, *error_msg_p is set
 *			  to an empty string on success, and to an error message
 *			  on failure; see also xtensa_debug_error_msg()
 *
 * Returns:	Handle to pass to other libdb API functions,
 *		or NULL on failure.
 */

xtensa_debug xtensa_debug_init(xtensa_isa isa, xtensa_params params,
				int *errno_p, const char **error_msg_p);


/*----------------------------------------------------------------
 * Function:	xtensa_debug_free
 *
 * Description:	Releases memory associated with the specified debug handle
 *		created using xtensa_debug_init().
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *
 * Returns:	Nothing
 */

void xtensa_debug_free(xtensa_debug debug);


/*----------------------------------------------------------------
 * Function:	xtensa_debug_errno
 *		xtensa_debug_error_msg
 *
 * Description:	xtensa_debug_errno() returns a code for the most
 *		recent error condition (XTENSA_DEBUG_xxx constants).
 *		For any result other than XTENSA_DEBUG_OK, an error
 *		message containing human-readable information about the
 *		problem can be retrieved using xtensa_debug_error_msg().
 *
 *		Error messages are stored in an internal buffer,
 *		which should not should be freed and may be overwritten
 *		by subsequent libdb operations.
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init()
 *			(or NULL when inquiring about errors returned
 *			from xtensa_debug_init() and xtensa_debug_free();
 *			NOTE: passing a NULL handle is not portable
 *			to future releases)
 *
 * Returns:	xtensa_debug_errno():
 *			0 or error code
 *
 *		xtensa_debug_error_msg():
 *			error message string; possibly empty string,
 *			but never NULL; not newline-terminated
 */

#define XTENSA_DEBUG_OK		0	/* no error */
#define XTENSA_DEBUG_ERROR	1	/* generic error */
#define XTENSA_DEBUG_ENOMEM	2	/* out of memory */
#define XTENSA_DEBUG_EINTERNAL	3	/* internal error */
#define XTENSA_DEBUG_EPARAMS	4	/* error in params file or info */
#define XTENSA_DEBUG_EISA	5	/* error loading ISA using xtensa_isa_init() */
#define XTENSA_DEBUG_EINVAL	6	/* invalid parameters */

int         xtensa_debug_errno(xtensa_debug debug);
const char* xtensa_debug_error_msg(xtensa_debug debug);

/*  TENSILICA INTERNAL USE ONLY: setup a function to report warnings;
 *  this function may be called before xtensa_debug_init():  */
typedef void xtensa_debug_warn_func(xtensa_debug debug, char *msg);
void	xtensa_debug_set_warning_callback(xtensa_debug_warn_func *warn_fn);


/*----------------------------------------------------------------
 * Function:	xtensa_debug_num_dbregs
 *
 * Description:	Returns the total number of registers.
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *
 * Returns:	Number of registers,
 *		or 0 on failure (eg. if passed a NULL handle).
 */

int xtensa_debug_num_dbregs( xtensa_debug debug );


/*----------------------------------------------------------------
 * Function:	xtensa_debug_num_dbarrays
 *
 * Description:	Returns the total number of register arrays.
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *
 * Returns:	Number of register arrays,
 *		or 0 on failure (eg. if passed a NULL handle).
 */

int xtensa_debug_num_dbarrays( xtensa_debug debug );



/*----------------------------------------------------------------
 * Function:	xtensa_dbreg_get_info
 *		xtensa_dbreg_lookup
 *		xtensa_dbreg_lookup_target_number
 *		xtensa_dbreg_lookup_dbarray
 *
 * Description:	Returns a pointer to a structure containing register information
 *		for the specified Xtensa processor register.
 *		(NOTE: the term "register" is used loosely and may refer to any
 *		programmer-visible or TIE-defined state.)
 *		The register can be specified as follows:
 *			by index from 0 to xtensa_num_dbregs()-1 (inclusive)
 *			by name in a case-insensitive manner
 *			by "target number" (see xtensa-libdb-macros.h)
 *			by index into an array (subset) of registers
 *				(see xtensa_dbarray_get_info())
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *		dbreg -- value between 0 and xtensa_num_dbregs() - 1 (inclusive)
 *		name -- name of the register to find
 *		target_number -- target number of register to find;
 *			if a single target number is assigned two names (register entries),
 *			one name for reading and another for writing, OR'ing the
 *			XTENSA_DBREGN_WRITE_SIDE flag to the target number specified
 *			selects the write side instead of the read side;
 *			for all other (non-split) target numbers (almost all of them),
 *			OR'ing this value has no effect;
 *			the only split register at this time is the INTERRUPT register
 *			(special register 226) which is "interrupt" for reads and
 *			"intset" for writes
 *		dbarray -- value between 0 and xtensa_num_dbarrays() - 1 (inclusive)
 *		index -- value between 0 and dbarray's num_entries - 1 (inclusive);
 *			OR'ing the XTENSA_DBARRAY_INDEX_WRITE_SIDE flag has the same effect
 *			on this parameter as the XTENSA_DBREGN_WRITE_SIDE flag has
 *			on the target_number parameter
 *			(note: num_entries can never be greater than 0x10000)
 *
 * Returns:	Pointer to a const xtensa_dbreg_info structure,
 *		or NULL if register not found (eg. index out of range, name not found, etc)
 */

#define XTENSA_DBARRAY_INDEX_WRITE_SIDE		XTENSA_DBREGN_WRITE_SIDE

const xtensa_dbreg_info* xtensa_dbreg_get_info( xtensa_debug debug, xtensa_dbreg dbreg);
const xtensa_dbreg_info* xtensa_dbreg_lookup( xtensa_debug debug, const char *name);
const xtensa_dbreg_info* xtensa_dbreg_lookup_target_number( xtensa_debug debug, int target_number);
const xtensa_dbreg_info* xtensa_dbreg_lookup_dbarray( xtensa_debug debug, xtensa_dbarray dbarray, int index);


/*----------------------------------------------------------------
 * Function:	xtensa_dbreg_get_mapping
 *
 * Description:	Returns a pointer to a structure containing register mapping
 *		information for the specified Xtensa processor register.
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *		dbreg -- specifies register; this is a value between 0 and
 *			xtensa_num_dbregs() - 1 (inclusive)
 *		index -- value between 0 and register info's num_mappings - 1 (inclusive)
 *
 * Returns:	Pointer to a const xtensa_dbreg_mapping structure,
 *		or NULL if not found (eg. dbreg or index out of range)
 */

const xtensa_dbreg_mapping* xtensa_dbreg_get_mapping( xtensa_debug debug, xtensa_dbreg dbreg, int index);


/*----------------------------------------------------------------
 * Function:	xtensa_dbarray_get_info
 *		xtensa_dbarray_lookup
 *
 * Description:	Returns a pointer to a structure containing register array
 *		information for the specified Xtensa processor register array.
 *		(NOTE: the term "register" is used loosely and may refer to any
 *		programmer-visible or TIE-defined state.  Arrays are any
 *		groupings of such registers -- not all possible/useful groupings
 *		are necessarily reported.)
 *		The register can be specified as follows:
 *			by index from 0 to xtensa_num_dbarrays()-1 (inclusive)
 *			by name in a case-insensitive manner
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *		dbarray -- value between 0 and xtensa_num_dbarrays() - 1 (inclusive)
 *		name -- name of the register array to find.
 *
 * Returns:	Pointer to a const xtensa_dbarray_info structure,
 *		or NULL if array not found (eg. index out of range, or name not found).
 */

const xtensa_dbarray_info* xtensa_dbarray_get_info(  xtensa_debug debug, 
                                                xtensa_dbarray  dbarray);
const xtensa_dbarray_info* xtensa_dbarray_lookup( xtensa_debug debug, const char *name);




/*----------------------------------------------------------------
 * Function:	xtensa_dbreg_get_code
 *
 * Description:	This function produces an instruction sequence that
 *		loads (restores) from memory or stores (writes/saves) to memory
 *		a specified register.
 *
 *		NOTE:  This facility is generally only available for
 *		user-defined (or TIE-defined) register files for which proper
 *		load and store prototypes were provided.  Other mechanisms
 *		exist for other accessible registers (e.g. direct access possibly
 *		in combination with ROTW for address registers, RSR/WSR for
 *		special registers, and RUR/WUR for user registers and state).
 *
 *		The list of Xtensa instructions produced saves and restores
 *		any temporary registers needed (including temporaries needed
 *		to load/store other temporaries, etc; the TIE compiler ensures
 *		there are no cycles in temporary register references among
 *		register file prototypes).
 *
 *		The instruction sequence produced uses a specified address register
 *		(address_register) to locate where in memory to load or store
 *		the specified register (dbreg) as well as any required temporary registers.
 *		The sequence does not modify address register address_register.
 *		Before executing the sequence, address register address_register
 *		must be loaded with the required address, which must normally be
 *		16-byte aligned (or at least be aligned sufficiently for dbreg
 *		and all direct or indirect temporary registers if any).
 *		Contents of register dbreg are loaded from or stored to this
 *		address, and contents of any temporary registers are saved and
 *		restored following dbreg at this address.  The total amount
 *		of read/writable memory that must be available at this address
 *		is returned in *required_temp_space by this function.
 *
 * Parameters:	debug -- handle returned by xtensa_debug_init().
 *
 *		dbreg -- index of register to spill-to or restore-from memory.
 *
 *		address_register -- number of the address register (a0..a15) that contains
 *			the memory address that dbreg will be stored to / loaded from
 *			(this is an integer in the range 0 thru 15, not a register index).
 *			Any temporary registers are saved/restored past dbreg at
 *			this same address.
 *			NOTE:  In practice, this is assumed to be a4 (or lower).
 *
 *		buf_length -- Number of bytes in the opcodes array
 *			(ignored if parameter opcodes == NULL).
 *
 *		opcodes -- Pointer to an array of bytes in which the Xtensa
 *			instructions are returned, or NULL if calling this
 *			function simply to determine how large this array needs to be
 *			(this function returns the required size in bytes).
 *			If opcodes is not NULL, the array it points to is filled
 *			with a sequence of instructions, each of which consists of
 *			a length byte followed by an instruction of that many bytes.
 *			The instruction itself is stored as a sequence of *bytes* as
 *			they appear in the targeted Xtensa processor's memory;
 *			no endianness-dependent swapping required.
 *			For example, for 3 Xtensa instructions of length 3, 2, and 3,
 *			the contents of the array are the following 11 bytes:
 *				3, op1a, op1b, op1c,
 *				2, op2a, op2b,
 *				3, op3a, op3b, op3c
 *			where eg. op1[a..c] are the three bytes for the first
 *			Xtensa instruction.
 *			Separating the instructions this way makes it easier
 *			to execute them in certain contexts, eg. when feeding
 *			the instructions to a target processor over OCD (JTAG).
 *			To execute these instructions directly from memory,
 *			the length bytes must be stripped and the whole sequence
 *			surrounded by appropriate setup and return code.
 *
 *		required_temp_space -- number of bytes that are required to load or store
 *			this register.  This includes space for the register itself,
 *			as well as space for any temporary registers saved if any.
 *			[These bytes must be available at the address pointed to by
 *			the address_register.]
 *
 * Returns:	Number of bytes written to the opcodes array
 *		(or that would be written to the opcodes array, if opcodes == NULL),
 *		or zero (0) on error.
 */

int xtensa_dbreg_get_code( xtensa_debug  debug,
			   xtensa_dbreg  dbreg,
			   int		 store,
			   int           address_register,
			   int           buf_length, 
			   unsigned char *opcodes, 
			   int           *required_temp_space);




#ifdef __cplusplus
}
#endif

#endif /* __H_LIBDB */

