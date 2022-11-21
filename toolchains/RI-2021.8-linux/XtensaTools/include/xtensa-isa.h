/* Interface definition for configurable Xtensa ISA support.

   Copyright (c) 2001-2013 Tensilica Inc.

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

#ifndef XTENSA_LIBISA_H
#define XTENSA_LIBISA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version number: This is intended to help support code that works with
   versions of this library from multiple Xtensa releases.  */

#define XTENSA_ISA_VERSION 8000

#ifndef uint32
#define uint32 unsigned int
#endif

/* This file defines the interface to the Xtensa ISA library.  This
   library contains most of the ISA-specific information for a
   particular Xtensa processor.  For example, the set of valid
   instructions, their opcode encodings and operand fields are all
   included here.  To support Xtensa's configurability and user-defined
   instruction extensions (i.e., TIE), the library is initialized by
   loading one or more dynamic libraries; only a small set of interface
   code is present in the statically-linked portion of the library.

   This interface basically defines a number of abstract data types.

   . an instruction buffer - for holding the raw instruction bits
   . ISA info - information about the ISA as a whole
   . instruction formats - instruction size and slot structure
   . opcodes - information about individual instructions
   . operands - information about register and immediate instruction operands
   . stateOperands - information about processor state instruction operands
   . interfaceOperands - information about interface instruction operands
   . register files - register file information
   . processor states - internal processor state information
   . system registers - "special registers" and "user registers"
   . ctypes - user-defined datatypes associated with particular regfiles
   . protos - prototypes to show the compiler how to use ctypes
   . coprocessors - groups of register files and states
   . interfaces - TIE interfaces that are external to the processor
   . functional units - TIE shared functions

   The interface defines a set of functions to access each data type.
   With the exception of the instruction buffer, the internal
   representations of the data structures are hidden.  All accesses must
   be made through the functions defined here.  */

typedef struct xtensa_isa_opaque { int unused; } *xtensa_isa;


/* Most of the Xtensa ISA entities (e.g., opcodes, regfiles, etc.) are
   represented here using sequential integers beginning with 0.  The
   specific values are only fixed for a particular instantiation of an
   xtensa_isa structure, so these values should only be used
   internally.  */

typedef int xtensa_opcode;
typedef int xtensa_format;
typedef int xtensa_regfile;
typedef int xtensa_state;
typedef int xtensa_sysreg;
#if !STATIC_LIBISA
typedef int xtensa_ctype;
typedef int xtensa_proto;
typedef int xtensa_coproc;
typedef int xtensa_xfer_proto;
#endif /* !STATIC_LIBISA */
typedef int xtensa_interface;
typedef int xtensa_funcUnit;


/* Define a unique value for undefined items.  */

#define XTENSA_UNDEFINED -1


/* Overview of using this interface to decode/encode instructions:

   Each Xtensa instruction is associated with a particular instruction
   format, where the format defines a fixed number of slots for
   operations.  The formats for the core Xtensa ISA have only one slot,
   but FLIX instructions may have multiple slots.  Within each slot,
   there is a single opcode and some number of associated operands.

   The encoding and decoding functions operate on instruction buffers,
   not on the raw bytes of the instructions.  The same instruction
   buffer data structure is used for both entire instructions and
   individual slots in those instructions -- the contents of a slot need
   to be extracted from or inserted into the buffer for the instruction
   as a whole.

   Decoding an instruction involves first finding the format, which
   identifies the number of slots, and then decoding each slot
   separately.  A slot is decoded by finding the opcode and then using
   the opcode to determine how many operands there are.  For example:

   xtensa_insnbuf_from_chars
   xtensa_format_decode
   for each slot {
     xtensa_format_get_slot
     xtensa_opcode_decode
     for each operand {
       xtensa_operand_get_field
       xtensa_operand_decode
     }
   }

   Encoding an instruction is roughly the same procedure in reverse:

   xtensa_format_encode
   for each slot {
     xtensa_opcode_encode
     for each operand {
       xtensa_operand_encode
       xtensa_operand_set_field
     }
     xtensa_format_set_slot
   }
   xtensa_insnbuf_to_chars
*/


/* Error handling.  */

/* Error codes.  The code for the most recent error condition can be
   retrieved with the "errno" function.  For any result other than
   xtensa_isa_ok, an error message containing additional information
   about the problem can be retrieved using the "error_msg" function.
   The error messages are stored in an internal buffer, which should
   not be freed and may be overwritten by subsequent operations.  */

typedef enum xtensa_isa_status_enum
{
  xtensa_isa_ok = 0,
  xtensa_isa_bad_format,
  xtensa_isa_bad_slot,
  xtensa_isa_bad_opcode,
  xtensa_isa_bad_operand,
  xtensa_isa_bad_field,
  xtensa_isa_bad_iclass,
  xtensa_isa_bad_regfile,
  xtensa_isa_bad_sysreg,
#if !STATIC_LIBISA
  xtensa_isa_bad_sysreg_field,
#endif
  xtensa_isa_bad_state,
#if !STATIC_LIBISA
  xtensa_isa_bad_state_field,
  xtensa_isa_bad_ctype,
  xtensa_isa_bad_proto,
  xtensa_isa_bad_proto_tmp,
  xtensa_isa_bad_proto_insn,
  xtensa_isa_bad_proto_arg,
  xtensa_isa_bad_xfer_proto,
  xtensa_isa_bad_coproc,
#endif
  xtensa_isa_bad_interface,
  xtensa_isa_bad_funcUnit,
  xtensa_isa_wrong_slot,
  xtensa_isa_no_field,
#if !STATIC_LIBISA
  xtensa_isa_invalid_module,
#endif
  xtensa_isa_out_of_memory,
  xtensa_isa_buffer_overflow,
  xtensa_isa_internal_error,
  xtensa_isa_bad_value
} xtensa_isa_status;

extern xtensa_isa_status
xtensa_isa_errno (xtensa_isa isa);

extern char *
xtensa_isa_error_msg (xtensa_isa isa);



/* Instruction buffers.  */

typedef uint32 xtensa_insnbuf_word;
typedef xtensa_insnbuf_word *xtensa_insnbuf;


/* Get the size in "insnbuf_words" of the xtensa_insnbuf array.  */

extern int
xtensa_insnbuf_size (xtensa_isa isa); 


/* Allocate an xtensa_insnbuf of the right size.  */

extern xtensa_insnbuf
xtensa_insnbuf_alloc (xtensa_isa isa);


/* Release an xtensa_insnbuf.  */

extern void
xtensa_insnbuf_free (xtensa_isa isa, xtensa_insnbuf buf);


/* Conversion between raw memory (char arrays) and our internal
   instruction representation.  This is complicated by the Xtensa ISA's
   variable instruction lengths.  When converting to chars, the buffer
   must contain a valid instruction so we know how many bytes to copy;
   thus, the "to_chars" function returns the number of bytes copied or
   XTENSA_UNDEFINED on error.  The "from_chars" function first reads the
   minimal number of bytes required to decode the instruction length and
   then proceeds to copy the entire instruction into the buffer; if the
   memory does not contain a valid instruction, it copies the maximum
   number of bytes required for the longest Xtensa instruction.  The
   "num_chars" argument may be used to limit the number of bytes that
   can be read or written.  Otherwise, if "num_chars" is zero, the
   functions may read or write past the end of the code.  */

extern int
xtensa_insnbuf_to_chars (xtensa_isa isa, const xtensa_insnbuf insn,
			 unsigned char *cp, int num_chars);

extern void
xtensa_insnbuf_from_chars (xtensa_isa isa, xtensa_insnbuf insn,
			   const unsigned char *cp, int num_chars);



/* ISA information.  */

#if STATIC_LIBISA

/* Initialize the ISA information.  */

extern xtensa_isa
xtensa_isa_init (xtensa_isa_status *errno_p, char **error_msg_p);

#else /* !STATIC_LIBISA */

/* Load the ISA information from set of shared libraries.  The "dlls"
   parameter must be a null-terminated array of ISA DLL paths.  If
   successful, this returns a value which identifies the ISA for use in
   subsequent calls to the ISA library.  Otherwise, on error the return
   value is null, and if the "errno_p" and/or "error_msg_p" pointers are
   non-null, an error code and message will be stored through them.
   Multiple ISAs can be loaded to support heterogeneous multiprocessor
   systems.  */

extern xtensa_isa
xtensa_isa_init (char **dlls, xtensa_isa_status *errno_p, char **error_msg_p);

#endif /* !STATIC_LIBISA */


/* Deallocate an xtensa_isa structure.  */

extern void
xtensa_isa_free (xtensa_isa isa);


/* Get the maximum instruction size in bytes.  */

extern int
xtensa_isa_maxlength (xtensa_isa isa); 


/* Decode the length in bytes of an instruction in raw memory (not an
   insnbuf).  This function reads only the minimal number of bytes
   required to decode the instruction length.  Returns
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_isa_length_from_chars (xtensa_isa isa, const unsigned char *cp);


/* Get the number of stages in the processor's pipeline.  The pipeline
   stage values returned by other functions in this library will range
   from 0 to N-1, where N is the value returned by this function.
   Note that the stage numbers used here may not correspond to the
   actual processor hardware, e.g., the hardware may have additional
   stages before stage 0.  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_isa_num_pipe_stages (xtensa_isa isa); 


#if !STATIC_LIBISA

/* Get the 2-word ConfigKey value.  Returns non-zero on error.  */

extern int
xtensa_isa_configkey (xtensa_isa isa, uint32 *pkey0, uint32 *pkey1);

#endif /* !STATIC_LIBISA */


/* Get the number of various entities that are defined for this processor.  */

extern int
xtensa_isa_num_formats (xtensa_isa isa);

extern int
xtensa_isa_num_opcodes (xtensa_isa isa);

extern int
xtensa_isa_num_regfiles (xtensa_isa isa);

extern int
xtensa_isa_num_states (xtensa_isa isa);

extern int
xtensa_isa_num_sysregs (xtensa_isa isa);

#if !STATIC_LIBISA

extern int
xtensa_isa_num_ctypes (xtensa_isa isa);

extern int
xtensa_isa_num_protos (xtensa_isa isa);

extern int
xtensa_isa_num_coprocs (xtensa_isa isa);

#endif /* !STATIC_LIBISA */

extern int
xtensa_isa_num_interfaces (xtensa_isa isa);

extern int
xtensa_isa_num_funcUnits (xtensa_isa isa);



/* Instruction formats.  */

/* Get the name of a format.  Returns null on error.  */

extern const char *
xtensa_format_name (xtensa_isa isa, xtensa_format fmt);


/* Given a format name, return the format number.  Returns
   XTENSA_UNDEFINED if the name is not a valid format.  */

extern xtensa_format
xtensa_format_lookup (xtensa_isa isa, const char *fmtname);


/* Decode the instruction format from a binary instruction buffer.
   Returns XTENSA_UNDEFINED if the format is not recognized.  */

extern xtensa_format
xtensa_format_decode (xtensa_isa isa, const xtensa_insnbuf insn);


/* Set the instruction format field(s) in a binary instruction buffer.
   All the other fields are set to zero.  Returns non-zero on error.  */

extern int
xtensa_format_encode (xtensa_isa isa, xtensa_format fmt, xtensa_insnbuf insn);


/* Find the length (in bytes) of an instruction.  Returns
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_format_length (xtensa_isa isa, xtensa_format fmt);


/* Get the number of slots in an instruction.  Returns XTENSA_UNDEFINED
   on error.  */

extern int
xtensa_format_num_slots (xtensa_isa isa, xtensa_format fmt);


#if !STATIC_LIBISA

/* Get the name and position of a slot.  These two values can be
   combined to uniquely identify the slot, which is needed in the Xtensa
   ISS but probably not anywhere else.  On error, the name is returned
   as null and the position as XTENSA_UNDEFINED.  */

extern const char *
xtensa_format_slot_name (xtensa_isa isa, xtensa_format fmt, int slot);

extern int
xtensa_format_slot_position (xtensa_isa isa, xtensa_format fmt, int slot);

#endif /* !STATIC_LIBISA */


/* Get the opcode for a no-op in a particular slot.
   Returns XTENSA_UNDEFINED on error.  */

extern xtensa_opcode
xtensa_format_slot_nop_opcode (xtensa_isa isa, xtensa_format fmt, int slot);


/* Get the bits for a specified slot out of an insnbuf for the
   instruction as a whole and put them into an insnbuf for that one
   slot, and do the opposite to set a slot.  Return non-zero on error.  */

extern int
xtensa_format_get_slot (xtensa_isa isa, xtensa_format fmt, int slot,
			const xtensa_insnbuf insn, xtensa_insnbuf slotbuf);

extern int
xtensa_format_set_slot (xtensa_isa isa, xtensa_format fmt, int slot,
			xtensa_insnbuf insn, const xtensa_insnbuf slotbuf);



/* Opcode information.  */

/* Translate a mnemonic name to an opcode.  Returns XTENSA_UNDEFINED if
   the name is not a valid opcode mnemonic.  */

extern xtensa_opcode
xtensa_opcode_lookup (xtensa_isa isa, const char *opname);


/* Decode the opcode for one instruction slot from a binary instruction
   buffer.  Returns the opcode or XTENSA_UNDEFINED if the opcode is
   illegal.  */

extern xtensa_opcode
xtensa_opcode_decode (xtensa_isa isa, xtensa_format fmt, int slot,
		      const xtensa_insnbuf slotbuf);


/* Set the opcode field(s) for an instruction slot.  All other fields
   in the slot are set to zero.  Returns non-zero if the opcode cannot
   be encoded.  */

extern int
xtensa_opcode_encode (xtensa_isa isa, xtensa_format fmt, int slot,
		      xtensa_insnbuf slotbuf, xtensa_opcode opc);


/* Get the mnemonic name for an opcode.  Returns null on error.  */

extern const char *
xtensa_opcode_name (xtensa_isa isa, xtensa_opcode opc);


#if !STATIC_LIBISA

/* Get the name of the package that defines an opcode.
   Returns null on error.  */

extern const char *
xtensa_opcode_package (xtensa_isa isa, xtensa_opcode opc);

#endif /* !STATIC_LIBISA */


/* Check various properties of opcodes.  These functions return 0 if
   the condition is false, 1 if the condition is true, and
   XTENSA_UNDEFINED on error.  The instructions are classified as
   follows:

   branch: conditional branch; may fall through to next instruction (B*)
   jump: unconditional branch (J, JX, RET*, RF*)
   loop: zero-overhead loop (LOOP*)
   call: unconditional call; control returns to next instruction (CALL*)
   load: loads a value from memory
   store: stores a value to memory
   base_update: load or store that updates the base address register
   byte_disable: load or store with a byte-disable mask
   possible_side_effect: might have a side effect other than: modifying
     the operands and states listed in the iclass, changing the PC, or
     writing to memory; the current implementation returns many false
     positives so this may not be directly useful -- it is currently
     intended only for internal use in the XCC compiler
   user: user-defined, i.e., not provided by Tensilica
   stream_op: stream get or put operation (not a standard Xtensa feature)

   For the opcodes that affect control flow in some way, the branch
   target may be specified by an immediate operand or it may be an
   address stored in a register.  You can distinguish these by
   checking if the instruction has a PC-relative immediate
   operand.  */

extern int
xtensa_opcode_is_branch (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_jump (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_loop (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_call (xtensa_isa isa, xtensa_opcode opc);

#if !STATIC_LIBISA

extern int
xtensa_opcode_is_load (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_gathera (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_gatherd (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_scatter (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_scatterw (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_double_load (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_store (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_vector (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_base_update (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_base_post_update (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_has_byte_disable (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_has_possible_side_effect (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_user (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_is_stream_op (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_may_rmw (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_must_rmw (xtensa_isa isa, xtensa_opcode opc);

/* For a load or store instruction, get the number of bytes that are
   loaded or stored (not taking into account any byte-disable masks).
   Returns XTENSA_UNDEFINED on error or if the opcode does not perform
   exactly one load or store.  */

extern int
xtensa_opcode_ldst_bytes (xtensa_isa isa, xtensa_opcode opc);


/* For a load or store instruction, get the operand number corresponding
   to the value being loaded/stored, the base register, and the offset,
   respectively.  Returns XTENSA_UNDEFINED on error, or if the opcode
   does not perform exactly one load or store, or if the TIE compiler
   was unable to determine that the instruction has such an operand.  If
   the instruction semantics in TIE are not written in a straightforward
   manner, the TIE compiler will not be able to identify these
   operands.  */

extern int
xtensa_opcode_ldst_value_operand (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_ldst_base_operand (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_ldst_offset_operand (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_ldst_postincr_operand (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_aligning_ldst_base_operand (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_aligning_ldst_offset_operand (xtensa_isa isa, xtensa_opcode opc);

/* Some special Xtensa instructions can issue only once every N cycles.
   The value of N is treated here as the "issue alignment".  The
   alignment is 1 for standard Xtensa instructions.  This is intended
   for use by the compiler in instruction scheduling.  The return value
   is the issue alignment or XTENSA_UNDEFINED on error.  */

extern int
xtensa_opcode_issue_alignment (xtensa_isa isa, xtensa_opcode opc);

#endif /* !STATIC_LIBISA */

/* Find the number of ordinary operands, state operands, and interface
   operands for an instruction.  These return XTENSA_UNDEFINED on
   error.  */

extern int
xtensa_opcode_num_operands (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_num_stateOperands (xtensa_isa isa, xtensa_opcode opc);

extern int
xtensa_opcode_num_interfaceOperands (xtensa_isa isa, xtensa_opcode opc);


/* Get functional unit usage requirements for an opcode.  Each "use"
   is identified by a <functional unit, pipeline stage> pair.  The
   "num_funcUnit_uses" function returns the number of these "uses" or
   XTENSA_UNDEFINED on error.  The "funcUnit_use" function returns
   a pointer to a "use" pair or null on error.  */

typedef struct xtensa_funcUnit_use_struct
{
  xtensa_funcUnit unit;
  int stage;
} xtensa_funcUnit_use;

extern int
xtensa_opcode_num_funcUnit_uses (xtensa_isa isa, xtensa_opcode opc);

extern xtensa_funcUnit_use *
xtensa_opcode_funcUnit_use (xtensa_isa isa, xtensa_opcode opc, int u);



/* Operand information.  */

/* Get the name of an operand.  Returns null on error.  */

extern const char *
xtensa_operand_name (xtensa_isa isa, xtensa_opcode opc, int opnd);


/* Some operands are "invisible", i.e., not explicitly specified in
   assembly language.  When assembling an instruction, you need not set
   the values of invisible operands, since they are either hardwired or
   derived from other field values.  The values of invisible operands
   can be examined in the same way as other operands, but remember that
   an invisible operand may get its value from another visible one, so
   the entire instruction must be available before examining the
   invisible operand values.  This function returns 1 if an operand is
   visible, 0 if it is invisible, or XTENSA_UNDEFINED on error.  Note
   that whether an operand is visible is orthogonal to whether it is
   "implicit", i.e., whether it is encoded in a field in the
   instruction.  */

extern int
xtensa_operand_is_visible (xtensa_isa isa, xtensa_opcode opc, int opnd);


/* Check if an operand is an input ('i'), output ('o'), or inout ('m')
   operand.  Note: The output operand of a conditional assignment
   (e.g., movnez) appears here as an inout ('m') even if it is declared
   in the TIE code as an output ('o'); this allows the compiler to
   properly handle register allocation for conditional assignments.
   Returns 0 on error.  */

extern char
xtensa_operand_inout (xtensa_isa isa, xtensa_opcode opc, int opnd);


/* Get and set the raw (encoded) value of the field for the specified
   operand.  The "set" function does not check if the value fits in the
   field; that is done by the "encode" function below.  Both of these
   functions return non-zero on error, e.g., if the field is not defined
   for the specified slot.  */

extern int
xtensa_operand_get_field (xtensa_isa isa, xtensa_opcode opc, int opnd,
			  xtensa_format fmt, int slot,
			  const xtensa_insnbuf slotbuf, uint32 *valp);

extern int 
xtensa_operand_set_field (xtensa_isa isa, xtensa_opcode opc, int opnd,
			  xtensa_format fmt, int slot,
			  xtensa_insnbuf slotbuf, uint32 val);


/* Encode and decode operands.  The raw bits in the operand field may
   be encoded in a variety of different ways.  These functions hide
   the details of that encoding.  The result values are returned through
   the argument pointer.  The return value is non-zero on error.  */

extern int
xtensa_operand_encode (xtensa_isa isa, xtensa_opcode opc, int opnd,
		       uint32 *valp);

extern int
xtensa_operand_decode (xtensa_isa isa, xtensa_opcode opc, int opnd,
		       uint32 *valp);


/* An operand may be either a register operand or an immediate of some
   sort (e.g., PC-relative or not).  The "is_register" function returns
   0 if the operand is an immediate, 1 if it is a register, and
   XTENSA_UNDEFINED on error.  The "regfile" function returns the
   regfile for a register operand, or XTENSA_UNDEFINED on error.  */

extern int
xtensa_operand_is_register (xtensa_isa isa, xtensa_opcode opc, int opnd);

extern xtensa_regfile
xtensa_operand_regfile (xtensa_isa isa, xtensa_opcode opc, int opnd);


/* Register operands may span multiple consecutive registers, e.g., a
   64-bit data type may occupy two 32-bit registers.  Only the first
   register is encoded in the operand field.  This function specifies
   the number of consecutive registers occupied by this operand.  For
   non-register operands, the return value is undefined.  Returns
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_operand_num_regs (xtensa_isa isa, xtensa_opcode opc, int opnd);
				 

/* Some register operands do not completely identify the register being
   accessed.  For example, the operand value may be added to an internal
   state value.  By definition, this implies that the corresponding
   regfile is not allocatable.  Unknown registers should generally be
   treated with worst-case assumptions.  The function returns 0 if the
   register value is unknown, 1 if known, and XTENSA_UNDEFINED on
   error.  */

extern int
xtensa_operand_is_known_reg (xtensa_isa isa, xtensa_opcode opc, int opnd);


#if !STATIC_LIBISA

/* Register operands may be read and written in various pipeline stages.
   If a register operand is read by an instruction, the "use_stage"
   function identifies the first pipeline stage where it is read.
   Likewise, if a register operand is written, the "def_stage" function
   returns the stage of the write.  This information can be used for
   instruction scheduling.  If the operand is not read/written or if an
   error occurs, the return value is XTENSA_UNDEFINED.  */

extern int
xtensa_operand_use_stage (xtensa_isa isa, xtensa_opcode opc, int opnd);

extern int
xtensa_operand_def_stage (xtensa_isa isa, xtensa_opcode opc, int opnd);

#endif /* !STATIC_LIBISA */


/* Check if an immediate operand is PC-relative.  Returns 0 for register
   operands and non-PC-relative immediates, 1 for PC-relative
   immediates, and XTENSA_UNDEFINED on error.  */
 
extern int
xtensa_operand_is_PCrelative (xtensa_isa isa, xtensa_opcode opc, int opnd);


/* For PC-relative offset operands, the interpretation of the offset may
   vary between opcodes, e.g., is it relative to the current PC or that
   of the next instruction?  The following functions are defined to
   perform PC-relative relocations and to undo them (as in the
   disassembler).  The "do_reloc" function takes the desired address
   value and the PC of the current instruction and sets the value to the
   corresponding PC-relative offset (which can then be encoded and
   stored into the operand field).  The "undo_reloc" function takes the
   unencoded offset value and the current PC and sets the value to the
   appropriate address.  The return values are non-zero on error.  Note
   that these functions do not replace the encode/decode functions; the
   operands must be encoded/decoded separately and the encode functions
   are responsible for detecting invalid operand values.  */

extern int
xtensa_operand_do_reloc (xtensa_isa isa, xtensa_opcode opc, int opnd,
			 uint32 *valp, uint32 pc);

extern int
xtensa_operand_undo_reloc (xtensa_isa isa, xtensa_opcode opc, int opnd,
			   uint32 *valp, uint32 pc);



/* State Operands.  */

/* Get the state accessed by a state operand.  Returns XTENSA_UNDEFINED
   on error.  */

extern xtensa_state
xtensa_stateOperand_state (xtensa_isa isa, xtensa_opcode opc, int stOp);


/* Check if a state operand is an input ('i'), output ('o'), or inout
   ('m') operand.  Returns 0 on error.  */

extern char
xtensa_stateOperand_inout (xtensa_isa isa, xtensa_opcode opc, int stOp);


#if !STATIC_LIBISA

/* State operands may be read and written in various pipeline stages.
   If a state operand is read by an instruction, the "use_stage"
   function identifies the first pipeline stage where it is read.
   Likewise, if a state operand is written, the "def_stage" function
   returns the stage of the write.  This information can be used for
   instruction scheduling.  If the operand is not read/written or if an
   error occurs, the return value is XTENSA_UNDEFINED.  */

extern int
xtensa_stateOperand_use_stage (xtensa_isa isa, xtensa_opcode opc, int stOp);

extern int
xtensa_stateOperand_def_stage (xtensa_isa isa, xtensa_opcode opc, int stOp);


/* Some references to a state operand, such as for a "shared_or" state,
   may be reordered with respective to other references to the same
   state without changing the behavior of the instruction.  This
   function checks if reordering is allowed for a particular state
   operand and returns 1 if so or 0 if not.  The return value is
   XTENSA_UNDEFINED on error.  Note that two state operand references
   can only be reordered when they refer to the same state and both
   state operands allow reordering.  */

extern int
xtensa_stateOperand_allow_reorder (xtensa_isa isa, xtensa_opcode opc,
				   int stOp);

#endif /* !STATIC_LIBISA */



/* Interface Operands.  */

/* Get the external interface accessed by an interface operand.
   Returns XTENSA_UNDEFINED on error.  */

extern xtensa_interface
xtensa_interfaceOperand_interface (xtensa_isa isa, xtensa_opcode opc,
				   int ifOp);



/* Register Files.  */

/* Regfiles include both "real" regfiles and "views", where a view
   allows a group of adjacent registers in a real "parent" regfile to be
   viewed as a single register.  A regfile view has all the same
   properties as its parent except for its (long) name, bit width, number
   of entries, and default ctype.  You can use the parent function to
   distinguish these two classes.  */

/* Look up a regfile by either its name or its abbreviated "short name".
   Returns XTENSA_UNDEFINED on error.  The "lookup_shortname" function
   ignores "view" regfiles since they always have the same shortname as
   their parents.  */

extern xtensa_regfile
xtensa_regfile_lookup (xtensa_isa isa, const char *name);

extern xtensa_regfile
xtensa_regfile_lookup_shortname (xtensa_isa isa, const char *shortname);


/* Get the name or abbreviated "short name" of a regfile.
   Returns null on error.  */

extern const char *
xtensa_regfile_name (xtensa_isa isa, xtensa_regfile rf);

extern const char *
xtensa_regfile_shortname (xtensa_isa isa, xtensa_regfile rf);


/* Get the parent regfile of a "view" regfile.  If the regfile is not a
   view, the result is the same as the input parameter.  Returns
   XTENSA_UNDEFINED on error.  */

extern xtensa_regfile
xtensa_regfile_view_parent (xtensa_isa isa, xtensa_regfile rf);


#if !STATIC_LIBISA

/* Get the name of the package that defines a regfile.
   Returns null on error.  */

extern const char *
xtensa_regfile_package (xtensa_isa isa, xtensa_regfile rf);

#endif /* !STATIC_LIBISA */


/* Get the bit width of a regfile or regfile view.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_regfile_num_bits (xtensa_isa isa, xtensa_regfile rf);


/* Get the number of regfile entries.  Returns XTENSA_UNDEFINED on
   error.  */

extern int
xtensa_regfile_num_entries (xtensa_isa isa, xtensa_regfile rf);

/* Get latency reduction setting for a given register file <rf>

   Returns '0' if the regfile has full late bypass and the latency computed
   based on the def_stage and use_stage is accurate. No more latency
   reduction is needed for operands in this register file.

   Returns '1' if there is no full late bypass and some selective late
   bypassing may exist and can be looked up with
   xtensa_regfile_latency_reduction() below to adjust the latency.
*/
extern int
xtensa_regfile_has_latency_reduction(xtensa_isa, xtensa_regfile rf);
extern int
xtensa_operand_def_bypass_class(xtensa_isa isa, xtensa_opcode def_opc, int def_opnd);
extern int
xtensa_operand_use_bypass_class(xtensa_isa isa, xtensa_opcode use_opc, int use_opnd);
extern int
xtensa_operand_bypass(xtensa_isa isa, int src_class, int dst_class);

/* Returns the latency cycle reduction as a result of explicit late
   bypassing, given a source opcode, operand, and the destination
   opcode, operand. 

   The returned reduction value is either 0 or 1.

   If 1 is returned, the latency cycle calculated using use_stage and
   def_stage should be reduced by one indicating a shorter latency from
   the late bypassing. 
*/
extern int
xtensa_regfile_latency_reduction(xtensa_isa isa, xtensa_opcode src_opc,
				  int src_opnd, xtensa_opcode dst_opc, int dst_opnd);

/* Check if a register file is external or not  */
extern int
xtensa_regfile_is_external(xtensa_isa, xtensa_regfile rf);

#if !STATIC_LIBISA

/* Get the number of entries in a register file that are designated as
   callee-saved.  The callee-saved registers, if any, are the last entries.
   The first register will always be caller-saved; i.e.,
   num_callee_saved < num_entries.  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_regfile_num_callee_saved (xtensa_isa isa, xtensa_regfile rf);


/* Check if a register file is suitable for automatic register
   allocation by the compiler.  (Some register files have special
   constraints that the compiler cannot handle, e.g., the MAC16 option's
   MR regfile.)  Returns 0 if the condition is false, 1 if the condition
   is true, and XTENSA_UNDEFINED on error.  */

extern int
xtensa_regfile_is_allocatable (xtensa_isa isa, xtensa_regfile rf);


/* Check if a register file may be written by a "split" pipeline,
   where long latency operations that write values after the "W" stage
   are handled in a separate pipeline.  Returns 0 if the condition is
   false, 1 if the condition is true, and XTENSA_UNDEFINED on error.  */

extern int
xtensa_regfile_has_split_pipe (xtensa_isa isa, xtensa_regfile rf);


/* Get the default ctype associated with a regfile or regfile view.
   Returns XTENSA_UNDEFINED on error.  If the regfile is not
   allocatable, there is no default ctype and this is treated as an
   error.  */

extern xtensa_ctype
xtensa_regfile_default_ctype (xtensa_isa isa, xtensa_regfile rf);


/* Get the coprocessor to which a regfile belongs.
   Returns XTENSA_UNDEFINED on error or if the regfile does not belong
   to any coprocessor.  */

extern xtensa_coproc
xtensa_regfile_coproc (xtensa_isa isa, xtensa_regfile rf);

#endif /* !STATIC_LIBISA */



/* Processor States.  */

/* Look up a state by name.  Returns XTENSA_UNDEFINED on error.  */

extern xtensa_state
xtensa_state_lookup (xtensa_isa isa, const char *name);


/* Get the name for a processor state.  Returns null on error.  */

extern const char *
xtensa_state_name (xtensa_isa isa, xtensa_state st);


#if !STATIC_LIBISA

/* Get the name of the package that defines a state.
   Returns null on error.  */

extern const char *
xtensa_state_package (xtensa_isa isa, xtensa_state st);

#endif /* !STATIC_LIBISA */


/* Get the bit width for a processor state.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_state_num_bits (xtensa_isa isa, xtensa_state st);


/* Check if a state is exported from the processor core.  Returns 0 if
   the condition is false, 1 if the condition is true, and
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_state_is_exported (xtensa_isa isa, xtensa_state st);


/* Check for a "shared_or" state.  Returns 0 if the condition is false,
   1 if the condition is true, and XTENSA_UNDEFINED on error.  */

extern int
xtensa_state_is_shared_or (xtensa_isa isa, xtensa_state st);


#if !STATIC_LIBISA

/* Get the coprocessor to which a state belongs.
   Returns XTENSA_UNDEFINED on error or if the state does not belong
   to any coprocessor.  */

extern xtensa_coproc
xtensa_state_coproc (xtensa_isa isa, xtensa_state st);


/* Processor states can be mapped into sysregs.  The entire state may
   not be available from a single sysreg (for example, when it is
   wider than 32-bits), so the state is split into fields that are
   mapped separately.  The following structure describes the mapping
   for one field of a state.  The bit numbering used here is
   consistently little-endian (0 = least significant).  No bit of a
   state may be mapped to more than one place.  */

typedef struct xtensa_state_mapping_struct
{
  xtensa_sysreg sysreg;		/* the sysreg holding this field */
  int num_bits;			/* width of this field */
  int sysreg_low_bit;		/* lsb of the sysreg field */
  int state_low_bit;		/* lsb of the state field */
} xtensa_state_mapping;


/* Get the number of fields in a processor state that are mapped to
   sysregs.  Returns XTENSA_UNDEFINED on error.  The result may also be
   zero, which means that the state is not mapped to sysregs.  */

extern int
xtensa_state_num_mappings (xtensa_isa isa, xtensa_state st);


/* Get the mapping from field N of a processor state to a sysreg.
   Returns null on error.  */

extern const xtensa_state_mapping *
xtensa_state_get_mapping (xtensa_isa isa, xtensa_state st, int n);

#endif /* !STATIC_LIBISA */



/* Sysregs ("special registers" and "user registers").  */

/* Look up a register by its number and whether it is a "user register"
   or a "special register".  Returns XTENSA_UNDEFINED if the sysreg does
   not exist.  */

extern xtensa_sysreg
xtensa_sysreg_lookup (xtensa_isa isa, int num, int is_user);


/* Check if there exists a sysreg with a given name.
   If not, this function returns XTENSA_UNDEFINED.  */

extern xtensa_sysreg
xtensa_sysreg_lookup_name (xtensa_isa isa, const char *name);


/* Get the name of a sysreg.  Returns null on error.  */

extern const char *
xtensa_sysreg_name (xtensa_isa isa, xtensa_sysreg sysreg);


/* Get the register number.  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_sysreg_number (xtensa_isa isa, xtensa_sysreg sysreg);


/* Check if a sysreg is a "special register" or a "user register".
   Returns 0 for special registers, 1 for user registers and
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_sysreg_is_user (xtensa_isa isa, xtensa_sysreg sysreg);


#if !STATIC_LIBISA

/* A sysreg may contain a combination of three kinds of values:
   processor state, register file entries, and constants.  Only selected
   portions of these items may be mapped to a particular sysreg, and
   some portions of a sysreg may also be left undefined.  No bit in a
   sysreg may be mapped to more than one value.  The following structure
   describes the contents of one field in a sysreg.  The bit numbering
   used here is consistently little-endian (0 = least significant).  */

typedef enum xtensa_sysreg_contents_kind_enum
{
  xtensa_sysreg_contents_state,
  xtensa_sysreg_contents_regfile,
  xtensa_sysreg_contents_const,
  xtensa_sysreg_contents_signextension
} xtensa_sysreg_contents_kind;

typedef struct xtensa_sysreg_contents_struct
{
  int num_bits;			/* width of this field in the sysreg */
  int sysreg_low_bit;		/* lsb of the field */

  xtensa_sysreg_contents_kind kind;
  union
  {
    /* xtensa_sysreg_contents_state */
    struct
    {
      xtensa_state state;	/* the state */
      int state_low_bit;	/* lsb of the corresponding state field */
    } st;

    /* xtensa_sysreg_contents_regfile */
    struct
    {
      xtensa_regfile regfile;	/* the regfile */
      int reg_low_bit;		/* lsb of the corresponding register field */
      int regnum;		/* register file index */
    } rf;

    /* xtensa_sysreg_contents_const */
    uint32 immed;		/* the constant value */

    /* xtensa_sysreg_contents_signextension */
    int content_index;		/* index of the field that is extended */

  } u;
} xtensa_sysreg_contents;


/* Get the number of fields in a sysreg.  Returns XTENSA_UNDEFINED on
   error.  The result may also be zero, which means that the contents of
   the register are undefined.  */

extern int
xtensa_sysreg_num_contents (xtensa_isa isa, xtensa_sysreg sysreg);


/* Get a description of the contents of field n in a sysreg.  Returns
   null on error.  */

extern const xtensa_sysreg_contents *
xtensa_sysreg_get_contents (xtensa_isa isa, xtensa_sysreg sysreg, int n);

#endif /* !STATIC_LIBISA */



#if !STATIC_LIBISA
/* Ctypes.  */

/* Find a ctype by its name.  Returns XTENSA_UNDEFINED if the name is
   not a valid ctype.  */

extern xtensa_ctype
xtensa_ctype_lookup (xtensa_isa isa, const char *name);


/* Get the name of a ctype.  Returns null on error.  */

extern const char *
xtensa_ctype_name (xtensa_isa isa, xtensa_ctype ct);


/* Get the name of the package that defines a ctype.
   Returns null on error.  */

extern const char *
xtensa_ctype_package (xtensa_isa isa, xtensa_ctype ct);


/* Get the bit width of a ctype.  Note that this is the number of
   useful bits in the ctype, NOT the size of the ctype in memory.
   It counts all register file entries used in the ctype; use the
   "num_regs" function to see how many register file entries are
   occupied by a ctype value.  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_ctype_num_bits (xtensa_isa isa, xtensa_ctype ct);


/* Get the alignment requirement in bits for a ctype (always 8
   times a power of 2).  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_ctype_alignment (xtensa_isa isa, xtensa_ctype ct);


/* Values for a ctype can be stored in memory or in the regfile
   associated with the ctype.  Get the associated regfile.  Returns
   XTENSA_UNDEFINED on error.  */

extern xtensa_regfile
xtensa_ctype_regfile (xtensa_isa isa, xtensa_ctype ct);

/* Values for a ctype can be stored in memory or in the regfile
   associated with the ctype.  Get the associated regfile view.
   If the ctype is not associated with a regfile view, it returns
   the regfile itself.  Returns XTENSA_UNDEFINED on error.  */

extern xtensa_regfile
xtensa_ctype_regfile_view (xtensa_isa isa, xtensa_ctype ct);

/* Get the number of regfile entries required to hold a ctype value.  If
   the ctype occupies more than one register, then each register must
   correspond to a field of the ctype, so this function is also
   available as "xtensa_ctype_num_fields".  Returns XTENSA_UNDEFINED on
   error.  */

extern int
xtensa_ctype_num_regs (xtensa_isa isa, xtensa_ctype ct);

#define xtensa_ctype_num_fields xtensa_ctype_num_regs


/* For a ctype with more than one field, get the name of a field.
   Returns null on error.  */

extern const char *
xtensa_ctype_field_name (xtensa_isa isa, xtensa_ctype ct, int fld);


/* For a ctype with more than one field, get the type of a field.
   Returns XTENSA_UNDEFINED on error.  */

extern xtensa_ctype
xtensa_ctype_field_type (xtensa_isa isa, xtensa_ctype ct, int fld);


/* The TIE language predefines some built-in types, e.g., uint16, etc.
   These are not "user-defined" types but must be included here so they
   can be referenced by protos and other constructs.  This function can
   be used to identify the built-in types.  Returns 0 if the condition
   is false, 1 if the condition is true, and XTENSA_UNDEFINED on error.  */

extern int
xtensa_ctype_is_builtin (xtensa_isa isa, xtensa_ctype ct);


/* Retrieve various protos associated with a ctype.  These protos show
   how to load, store, move, and convert values of the ctype.  Many of
   these protos may not be defined, in which case the result is
   XTENSA_UNDEFINED.  */

extern xtensa_proto
xtensa_ctype_loadi_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storei_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_loadx_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storex_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_loadiu_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_loadip_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storeiu_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storeip_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_loadxu_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_loadxp_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storexu_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_storexp_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_move_proto (xtensa_isa isa, xtensa_ctype ct);

extern xtensa_proto
xtensa_ctype_rtor_proto (xtensa_isa isa, xtensa_ctype ct,
			 xtensa_ctype dest_type);
extern xtensa_proto
xtensa_ctype_rtom_proto (xtensa_isa isa, xtensa_ctype ct,
			 xtensa_ctype dest_type);
extern xtensa_proto
xtensa_ctype_mtor_proto (xtensa_isa isa, xtensa_ctype ct,
			 xtensa_ctype dest_type);



/* Protos.  */

/* Find a prototype by its name.  Returns XTENSA_UNDEFINED if the name is
   not a valid proto.  */

extern xtensa_proto
xtensa_proto_lookup (xtensa_isa isa, const char *name);


/* Get the name of a prototype.  Returns null on error.  */

extern const char *
xtensa_proto_name (xtensa_isa isa, xtensa_proto p);


/* Get the name of the package that defines a prototype.
   Returns null on error.  */

extern const char *
xtensa_proto_package (xtensa_isa isa, xtensa_proto p);


/* If a prototype implements an overloaded C/C++ operator, return the
   operator name.  Returns null on error or if the prototype does not
   implement an operator.  */

extern const char *
xtensa_proto_operator (xtensa_isa isa, xtensa_proto p);


/* Get the number of operands for a prototype.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_proto_num_operands (xtensa_isa isa, xtensa_proto p);


/* Get the name of a prototype operand.  This isn't really needed for
   anything except that it's nice to be able to display protos (in error
   messages, etc.) using the same names as specified in the TIE.  Returns
   null on error.  */

extern const char *
xtensa_proto_operand_name (xtensa_isa isa, xtensa_proto p, int opnd);


/* A prototype operand may be either a ctype value in a register,
   an immediate, or a label.  These functions return 1 if the condition
   is true, 0 if it is false, and XTENSA_UNDEFINED on error.  */

extern int
xtensa_proto_operand_is_register (xtensa_isa isa, xtensa_proto p, int opnd);

extern int
xtensa_proto_operand_is_immed (xtensa_isa isa, xtensa_proto p, int opnd);

extern int
xtensa_proto_operand_is_label (xtensa_isa isa, xtensa_proto p, int opnd);


/* If a prototype operand is a register value, this function returns the
   ctype for the operand.  The operand may be either a value of this
   ctype or a pointer to this ctype, as indicated by "*is_pointer_p".
   Furthermore, a pointer type may be declared "const" if the prototype
   does not write to memory through the pointer, and this is indicated
   by "*is_const_p" if is_const_p is non-null.  Returns XTENSA_UNDEFINED
   on error or if the operand is an immediate or label.  */

extern xtensa_ctype
xtensa_proto_operand_type (xtensa_isa isa, xtensa_proto p, int opnd,
			   int *is_pointer_p, int *is_const_p);


/* Check if a prototype operand is an input ('i'), output ('o'), or
   inout ('m') operand.  Only register operands may be "output" and
   "inout".  Returns 0 on error.  */

extern char
xtensa_proto_operand_inout (xtensa_isa isa, xtensa_proto p, int opnd);


/* Get the number of temporary variables used within a prototype.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_proto_num_tmps (xtensa_isa isa, xtensa_proto p);


/* Get the name of a prototype temporary.  This isn't really needed for
   anything except that it's nice to be able to display protos (in error
   messages, etc.) using the same names as specified in the TIE.  Returns
   null on error.  */

extern const char *
xtensa_proto_tmp_name (xtensa_isa isa, xtensa_proto p, int tmp);


/* Get the ctype of a prototype temporary.  The temporary may be
   either a value of this ctype or a pointer to the ctype, as
   indicated by "*is_pointer_p".  Returns XTENSA_UNDEFINED on
   error.  */

extern xtensa_ctype
xtensa_proto_tmp_type (xtensa_isa isa, xtensa_proto p, int tmp,
		       int *is_pointer_p);


/* Get the number of instructions into which a prototype expands.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_proto_num_insns (xtensa_isa isa, xtensa_proto p);


/* Get the opcode of a prototype instruction.
   Returns XTENSA_UNDEFINED on error.  */

extern xtensa_opcode
xtensa_proto_insn_opcode (xtensa_isa isa, xtensa_proto p, int insn);


/* Get the number of arguments for an instruction in a prototype or
   XTENSA_UNDEFINED on error.  The number of arguments for an
   instruction in a prototype definition is usually the same as the
   number of operands for the corresponding opcode, but there are some
   exceptions: operands that are not visible are not included when the
   instruction is used in a prototype; a single operand that occupies
   more than one register may serve as a reference to multiple ctype
   values.  */

extern int
xtensa_proto_insn_num_args (xtensa_isa isa, xtensa_proto p, int insn);


/* This function returns the operand index corresponding to an argument
   in a prototype, or XTENSA_UNDEFINED on error.  If multiple argument
   values map to one operand, the values are numbered consecutively
   beginning with 0 and the number for the specified argument is
   returned through the "offsetp" pointer.  */

extern int
xtensa_proto_insn_arg_to_opnd (xtensa_isa isa, xtensa_proto p, int insn,
			       int arg, int *offsetp);


/* Arguments in prototypes may be either immediates or variables in
   registers.  This function returns 1 if the argument is an immediate,
   0 if the argument is a variable, or XTENSA_UNDEFINED on error.  For
   the most part variable arguments correspond to register operands of
   instructions, and immediate arguments correspond to non-register
   operands.  An exception is register operands for non-allocatable
   regfiles, where the proto must specify the register numbers as
   immediates.  Label operands, i.e., PC-relative immediates, are always
   specified with immediate arguments.  */

extern int
xtensa_proto_insn_arg_is_immed (xtensa_isa isa, xtensa_proto p, int insn,
				int arg);


/* Get the value of an immediate argument.  An immediate argument may be
   an immediate or label proto operand, a constant integer, or a sum of
   both.  The index of a proto operand, or XTENSA_UNDEFINED if the
   argument is just a constant integer, is returned through the "opndp"
   pointer.  The constant integer value is returned through the "immedp"
   pointer.  The return value is non-zero on error.  */

extern int
xtensa_proto_insn_arg_immed (xtensa_isa isa, xtensa_proto p, int insn,
			     int arg, int *opndp, int *immedp);


/* A variable argument may be either a proto operand or a temporary
   variable.  This function gets the index of the proto operand or
   temporary and returns it through the "variablep" pointer.  A non-zero
   value returned through the "tmpp" pointer indicates that the index
   refers to a temporary variable instead of a proto operand.  The
   return value is non-zero on error.  */

extern int
xtensa_proto_insn_arg_variable (xtensa_isa isa, xtensa_proto p, int insn,
				int arg, int *variablep, int *tmpp);


/* When a variable argument has a ctype with multiple fields, the
   argument can include a field index.  This functions returns the field
   index through the "fldp" pointer.  The return value is non-zero on
   error.  */

extern int
xtensa_proto_insn_arg_field (xtensa_isa isa, xtensa_proto p, int insn,
			     int arg, int *fldp);

/* Xfer_protos */
// IMPORTANT: the following numbers should be in sync with the ones in 
// tie2isa/tie2isa_proto.c
typedef enum xtensa_xfer_proto_kind_enum
{
    XFER_PROTO_KIND_UNDEFINED               = -1,
    XFER_PROTO_KIND_VOID                    = 0,
    XFER_PROTO_KIND_MOVE                    = 1,
    XFER_PROTO_KIND_IMM                     = 2,
    XFER_PROTO_KIND_INDEXED                 = 3,
    XFER_PROTO_KIND_PRE_UPDATE              = 4,
    XFER_PROTO_KIND_POST_UPDATE             = 5,
    XFER_PROTO_ENDPOINT_KIND_UREG           = 10,
    XFER_PROTO_ENDPOINT_KIND_REG            = 11,
    XFER_PROTO_ENDPOINT_KIND_SYSMEM         = 12,
    XFER_PROTO_ENDPOINT_KIND_SPECIAL_MEM    = 13
} xtensa_xfer_proto_kind;

xtensa_xfer_proto
xtensa_xfer_proto_lookup(xtensa_isa isa, xtensa_xfer_proto_kind kind,
        xtensa_xfer_proto_kind ep_kind1, const char *ep_name1,
        xtensa_xfer_proto_kind ep_kind2, const char *ep_name2);

xtensa_proto
xtensa_xfer_proto_get_proto(xtensa_isa isa, xtensa_xfer_proto xp, int first);

xtensa_xfer_proto_kind
xtensa_xfer_proto_get_kind(xtensa_isa isa, xtensa_xfer_proto xp);



/* Coprocessors.  */

/* Find a coprocessor by number or name.  The return value is
   XTENSA_UNDEFINED if the specified coprocessor is not found.  */

extern xtensa_coproc
xtensa_coproc_lookup (xtensa_isa isa, int num);

extern xtensa_coproc
xtensa_coproc_lookup_name (xtensa_isa isa, const char *name);


/* Get the name of a coprocessor.  Returns null on error.  */

extern const char *
xtensa_coproc_name (xtensa_isa isa, xtensa_coproc c);


/* Get the numeric value for a coprocessor.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_coproc_number (xtensa_isa isa, xtensa_coproc c);



#endif /* !STATIC_LIBISA */
/* Interfaces.  */

/* Find an interface by name.  The return value is XTENSA_UNDEFINED if
   the specified interface is not found.  */

extern xtensa_interface
xtensa_interface_lookup (xtensa_isa isa, const char *ifname);


/* Get the name of an interface.  Returns null on error.  */

extern const char *
xtensa_interface_name (xtensa_isa isa, xtensa_interface intf);


/* Get the bit width for an interface.
   Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_interface_num_bits (xtensa_isa isa, xtensa_interface intf);


/* Check if an interface is an input ('i') or output ('o') with respect
   to the Xtensa processor core.  Returns 0 on error.  */

extern char
xtensa_interface_inout (xtensa_isa isa, xtensa_interface intf);


#if !STATIC_LIBISA

/* Each interface is accessed in a particular stage of the processor
   pipeline.  This function returns the stage for an interface or
   XTENSA_UNDEFINED on error.  */

extern int
xtensa_interface_stage (xtensa_isa isa, xtensa_interface intf);

#endif /* !STATIC_LIBISA */


/* Check if accessing an interface has potential side effects.
   Currently "data" interfaces have side effects and "control"
   interfaces do not.  Returns 1 if there are side effects, 0 if not,
   and XTENSA_UNDEFINED on error.  */

extern int
xtensa_interface_has_side_effect (xtensa_isa isa, xtensa_interface intf);


/* Some interfaces may be related such that accessing one interface
   has side effects on a set of related interfaces.  The interfaces
   are partitioned into equivalence classes of related interfaces, and
   each class is assigned a unique identifier number.  This function
   returns the class identifier for an interface, or XTENSA_UNDEFINED
   on error.  These identifiers can be compared to determine if two
   interfaces are related; the specific values of the identifiers have
   no particular meaning otherwise.  */

extern int
xtensa_interface_class_id (xtensa_isa isa, xtensa_interface intf);



/* Functional Units.  */

/* Find a functional unit by name.  The return value is XTENSA_UNDEFINED if
   the specified unit is not found.  */

extern xtensa_funcUnit
xtensa_funcUnit_lookup (xtensa_isa isa, const char *fname);


/* Get the name of a functional unit.  Returns null on error.  */

extern const char *
xtensa_funcUnit_name (xtensa_isa isa, xtensa_funcUnit fun);


/* Functional units may be replicated.  See how many instances of a
   particular function unit exist.  Returns XTENSA_UNDEFINED on error.  */

extern int
xtensa_funcUnit_num_copies (xtensa_isa isa, xtensa_funcUnit fun);


// propert ignore_state_output
extern int
xtensa_has_ignore_state_output(xtensa_isa isa, xtensa_state state, xtensa_opcode opcode);


#ifdef __cplusplus
}
#endif
#endif /* XTENSA_LIBISA_H */
