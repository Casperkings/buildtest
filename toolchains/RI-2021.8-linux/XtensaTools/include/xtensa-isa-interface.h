/* The Xtensa ISA DLL interface used by TC.
   This is _NOT_ the interface for users -- see xtensa-isa.h.  */

/* Copyright (c) 2004-2008 Tensilica Inc.

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

#ifndef XTENSA_ISA_INTERFACE_H
#define XTENSA_ISA_INTERFACE_H

/* General note: An ISA DLL can reference items defined by other DLLs.
   Each DLL should define only the items that are defined by the
   corresponding TIE code.  E.G., if user TIE references a field from
   the core ISA, the user TIE DLL should refer to that field without
   providing a definition. */

#ifndef int8
#define int8 signed char
#endif

#ifndef int16
#define int16 signed short
#endif

#ifndef uint32
#define uint32 unsigned int
#endif

typedef uint32 xtensa_insnbuf_word;
typedef xtensa_insnbuf_word *xtensa_insnbuf;


/* Interface version: Used to provide meaningful error messages when
   the DLL interface changes; could potentially be used to implement
   backward compatibility at some point.  If you change this, be sure
   to change the matching number in the internal header file. */

#define ISA_INTERFACE_VERSION 118
extern int interface_version (void);


/* Config table: A set of name/value pairs to record various
   configuration values, e.g., "IsaMemoryOrder", "PIFReadDataBits",
   "PIFWriteDataBits", etc. */

struct config_struct {
    char *param_name;
    char *param_value;
};
extern struct config_struct *get_config_table (void);


/* Instruction formats: Functions to provide the number of formats and
   the name, length and encoding for each format.  The length is the
   instruction length in bytes.  The encoding function zeros out the
   instruction buffer (or at least the part of the buffer used by
   the specified format) except for the bits that specify the format. */

typedef void (*xtensa_format_encode_fn) (xtensa_insnbuf insn);

extern int num_formats (void);
extern const char *format_name (int fmt);
extern int format_length (int fmt);
extern xtensa_format_encode_fn format_encode_fn (int fmt);


/* Instruction slots: Functions to provide the number of slots and the
   name, format, position, get function and set function, and no-op
   opcode for each slot.  The name is not necessarily unique.  The
   position is an arbitrary integer used to sort the slots within a
   format, with lower numbers appearing first. */

typedef void (*xtensa_get_slot_fn) (const xtensa_insnbuf insn,
				    xtensa_insnbuf slotbuf);
typedef void (*xtensa_set_slot_fn) (xtensa_insnbuf insn,
				    const xtensa_insnbuf slotbuf);

extern int num_slots (void);
extern const char *slot_name (int slot);
extern const char *slot_format (int slot);
extern int slot_position (int slot);
extern xtensa_get_slot_fn slot_get_fn (int slot);
extern xtensa_set_slot_fn slot_set_fn (int slot);
extern const char *slot_nop_name (int slot);


/* Fields: Functions to return the number of (field x slot) combinations
   defined by the DLL, along with the field and slot names and get/set
   functions for each combination.  If the slot is null, this indicates
   that the field is for an implicit operand and should be treated as if
   it exists in all slots.  */

typedef uint32 (*xtensa_get_field_fn) (const xtensa_insnbuf slotbuf);
typedef void (*xtensa_set_field_fn) (xtensa_insnbuf slotbuf, uint32 val);

extern int num_fields (void);
extern const char *field_name (int fld);
extern const char *field_slot (int fld);
extern xtensa_get_field_fn field_get_fn (int fld);
extern xtensa_set_field_fn field_set_fn (int fld);


/* Operands: Functions to return lots of stuff about operands.  The
   behavior of the various functions below should be fairly obvious.... */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_OPERAND_IS_REGISTER	0x00000001
#define XTENSA_OPERAND_IS_PCRELATIVE	0x00000002
#define XTENSA_OPERAND_IS_INVISIBLE	0x00000004
#define XTENSA_OPERAND_IS_UNKNOWN	0x00000008

typedef int (*xtensa_immed_decode_fn) (uint32 *valp);
typedef int (*xtensa_immed_encode_fn) (uint32 *valp);

typedef int (*xtensa_do_reloc_fn) (uint32 *valp, uint32 pc);
typedef int (*xtensa_undo_reloc_fn) (uint32 *valp, uint32 pc);

extern int num_operands (void);
extern const char *operand_name (int opnd);
extern const char *operand_field (int opnd);
extern const char *operand_regfile (int opnd);
extern int operand_num_regs (int opnd);
extern uint32 operand_flags (int opnd);
extern xtensa_immed_encode_fn operand_encode_fn (int opnd);
extern xtensa_immed_decode_fn operand_decode_fn (int opnd);
extern xtensa_do_reloc_fn operand_do_reloc_fn (int opnd);
extern xtensa_undo_reloc_fn operand_undo_reloc_fn (int opnd);


/* Iclasses: Functions to provide iclass information.  The array of operands
   for each iclass are referenced by name. */

extern int num_iclasses (void);
extern const char *iclass_name (int iclass);
extern int iclass_num_operands (int iclass);
extern const char *iclass_operand_name (int iclass, int opnd);
extern char iclass_operand_inout (int iclass, int opnd);
extern int iclass_num_stateOperands (int iclass);
extern const char *iclass_stateOperand_state (int iclass, int st_opnd);
extern char iclass_stateOperand_inout (int iclass, int st_opnd);
extern int iclass_num_interfaceOperands (int iclass);
extern const char *iclass_interfaceOperand_interface (int iclass, int if_opnd);


/* Opcodes: Functions to provide opcode information. */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_OPCODE_IS_BRANCH		0x00000001
#define XTENSA_OPCODE_IS_JUMP		0x00000002
#define XTENSA_OPCODE_IS_LOOP		0x00000004
#define XTENSA_OPCODE_IS_CALL		0x00000008
#define XTENSA_OPCODE_IS_LOAD		0x00000010
#define XTENSA_OPCODE_IS_STORE		0x00000020
#define XTENSA_OPCODE_IS_BASE_UPDATE	0x00000040
#define XTENSA_OPCODE_HAS_BYTE_DISABLE	0x00000080
#define XTENSA_OPCODE_HAS_SIDE_EFFECT	0x00000100
#define XTENSA_OPCODE_IS_USER		0x00000200
#define XTENSA_OPCODE_IS_STREAM_OP	0x00000400
#define XTENSA_OPCODE_IS_BASE_POST_UPDATE \
					0x00000800
#define XTENSA_OPCODE_IS_DOUBLE_LOAD	0x00001000
#define XTENSA_OPCODE_MUST_RMW       0x00002000
#define XTENSA_OPCODE_MAY_RMW        0x00004000
#define XTENSA_OPCODE_IS_GATHERA     0x00008000
#define XTENSA_OPCODE_IS_GATHERD     0x00010000
#define XTENSA_OPCODE_IS_SCATTER     0x00020000
#define XTENSA_OPCODE_IS_SCATTERW    0x00040000
#define XTENSA_OPCODE_IS_VECTOR      0x00080000


extern int num_opcodes (void);
extern const char *opcode_name (int opc);
extern const char *opcode_package (int opc);
extern const char *opcode_iclass (int opc);
extern uint32 opcode_flags (int opc);
extern int8 opcode_operand_use_stage (int opc, int opnd);
extern int8 opcode_operand_def_stage (int opc, int opnd);
extern int8 opcode_stateOperand_use_stage (int opc, int st_opnd);
extern int8 opcode_stateOperand_def_stage (int opc, int st_opnd);
extern int8 opcode_stateOperand_allow_reorder (int opc, int st_opnd);
extern int opcode_num_funcUnit_uses (int opc);
extern const char *opcode_funcUnit_use_unit (int opc, int u);
extern int opcode_funcUnit_use_stage (int opc, int u);
extern int16 opcode_ldst_bytes (int opc);
extern int8 opcode_ldst_value_opnd (int opc);
extern int8 opcode_ldst_base_opnd (int opc);
extern int8 opcode_ldst_offset_opnd (int opc);
extern int8 opcode_ldst_postincr_opnd (int opc);
extern int16 opcode_bypass_use_group_idx(int opc, int opnd);
extern int16 opcode_bypass_def_group_idx(int opc, int opnd);

/* Bypass functions. They may not be present. */
extern int num_bypass_groups(void);
extern int num_bypass_group_chunks(void);
extern uint32 *bypass_entry(int i);


/* Opcode encode functions: There may be a different encode function
   for each (opcode x slot) combination.  The encode function is
   expected to set the opcode fields and zero out the rest of the slot
   buffer.  */

typedef void (*xtensa_opcode_encode_fn) (xtensa_insnbuf slotbuf);

extern int num_encode_fns (void);
extern const char *encode_fn_opcode (int fn);
extern const char *encode_fn_slot (int fn);
extern xtensa_opcode_encode_fn encode_fn (int fn);


/* Opcode decode functions: Functions to provide (partial) decode
   functions for each slot.  The decode function is not necessarily
   complete: it should recognize all the opcodes defined for the slot
   by the input TIE code, but other DLLs may provide additional decode
   functions for the same slot.  If one decode function fails to
   recognize the opcode, the next decode function is tried.  */

typedef const char *(*xtensa_opcode_decode_fn) (const xtensa_insnbuf slotbuf);

extern int num_decode_fns (void);
extern const char *decode_fn_slot (int fn);
extern xtensa_opcode_decode_fn decode_fn (int fn);


/* Format decode function: Each DLL must provide a function to decode
   the instruction formats defined in that DLL. The result is an index
   into the local table of formats or -1 if the format does not match
   any of the formats from the DLL. */

typedef int (*xtensa_format_decode_fn) (const xtensa_insnbuf insn);
extern xtensa_format_decode_fn decode_format_fn (void);

typedef int (*xtensa_length_decode_fn) (const unsigned char *);
extern xtensa_length_decode_fn decode_length_fn (void);

typedef int (*xtensa_ignore_state_output_fn)(const char *state, const char *instr);
extern xtensa_ignore_state_output_fn ignore_state_output_fn(void);


/* Register files: Functions to get regfile info.  */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_REGFILE_IS_ALLOCATABLE  0x00000001
#define XTENSA_REGFILE_HAS_SPLIT_PIPE  0x00000002
#define XTENSA_REGFILE_HAS_LATE_BYPASS 0x00000004
#define XTENSA_REGFILE_IS_EXTERNAL     0x00000008

extern int num_regfiles (void);
extern const char *regfile_name (int rf);
extern const char *regfile_short_name (int rf);
extern const char *regfile_package (int rf);
extern int regfile_num_bits (int rf);
extern int regfile_num_entries (int rf);
extern int regfile_num_callee_saved (int rf);
extern uint32 regfile_flags (int rf);
extern const char *regfile_ctype (int rf);
extern const char *regfile_coproc (int rf);

extern int num_regfile_views (void);
extern const char *regfile_view_name (int rf);
extern const char *regfile_view_parent (int rf);
extern int regfile_view_num_bits (int rf);
extern const char *regfile_view_ctype (int rf);


/* States: Info about processor states.  The mapping to sysregs is
   computed from the sysreg content information; it is not specified
   in this interface.  */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_STATE_IS_EXPORTED	0x00000001
#define XTENSA_STATE_IS_SHARED_OR	0x00000002

extern int num_states (void);
extern const char *state_name (int st);
extern const char *state_package (int st);
extern int state_num_bits (int st);
extern uint32 state_flags (int st);
extern const char *state_coproc (int st);


/* Sysregs: Functions to get sysreg information.  Rather than passing
   xtensa_sysreg_contents structures for sysreg_contents(), the contents
   are encoded in a character string.  The encoding format is one of the
   following:

   <num_bits>:<sysreg_low_bit>:s:<state>:<state_low_bit>
   <num_bits>:<sysreg_low_bit>:r:<regfile>:<reg_low_bit>:<regnum>
   <num_bits>:<sysreg_low_bit>:c:<immed>
   <num_bits>:<sysreg_low_bit>:x:<content_index>

   The <state> and <regfile> values are strings; the rest are integers.  */

extern int num_sysregs (void);
extern const char *sysreg_name (int sr);
extern int sysreg_number (int sr);
extern int sysreg_is_user (int sr);
extern int sysreg_num_contents (int sr);
extern const char *sysreg_contents (int sr, int n);


/* Ctypes: Functions to get ctype info.  */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_CTYPE_IS_BUILTIN		0x00000001

extern int num_ctypes (void);
extern const char *ctype_name (int ct);
extern const char *ctype_package (int ct);
extern int ctype_num_bits (int ct);
extern int ctype_alignment (int ct);
extern const char *ctype_regfile (int ct);
extern int ctype_num_regs (int ct);
extern uint32 ctype_flags (int ct);
extern const char *ctype_field_name (int ct, int fld);
extern const char *ctype_field_type (int ct, int fld);

/* A DLL may define protos for a previously defined ctype, so the ctype
   protos are retrieved apart from the ctypes themselves.  */

extern int num_ctype_protos (void);
extern const char *ctype_proto_name (int cp);
extern const char *ctype_proto_ctype (int cp);
extern const char *ctype_proto_kind (int cp);
extern const char *ctype_proto_other_type (int cp);


/* Protos: Functions to get prototype info.  If a proto operand is an
   immediate, the proto_operand_type function returns null.  If the
   result of the proto_insn_name function is a real opcode, then the
   insn is treated as a real opcode; otherwise, the name must be a
   valid prototype.  */

extern int num_protos (void);
extern const char *proto_name (int p);
extern const char *proto_package (int p);
extern const char *proto_operator (int p);
extern int proto_num_operands (int p);
extern const char *proto_operand_name (int p, int n);
extern uint32 proto_operand_flags (int p, int n);
extern const char *proto_operand_type (int p, int n, int *is_ptr_p,
				       int *is_cnst_p);
extern char proto_operand_inout (int p, int n);
extern int proto_num_tmps (int p);
extern const char *proto_tmp_name (int p, int n);
extern const char *proto_tmp_type (int p, int n, int *is_ptr_p);
extern int proto_num_insns (int p);
extern const char *proto_insn_name (int p, int n);
extern int proto_insn_num_args (int p, int n);
extern int proto_insn_arg_is_tmp (int p, int n, int a);
extern int proto_insn_arg_opnd_or_tmp (int p, int n, int a);
extern int proto_insn_arg_immed (int p, int n, int a);
extern int proto_insn_arg_field (int p, int n, int a);


/* Coprocessors: Info about coprocessors.  */

extern int num_coprocs (void);
extern const char *coproc_name (int c);
extern int coproc_number (int c);


/* Interfaces: Info about external interfaces.  */

/* flags: keep these in sync with the internal header file!! */
#define XTENSA_INTERFACE_HAS_SIDE_EFFECT 0x00000001

extern int num_interfaces (void);
extern const char *interface_name (int intf);
extern int interface_num_bits (int intf);
extern uint32 interface_flags (int intf);
extern char interface_inout (int intf);
extern int8 interface_stage (int intf);


/* Interface classes: lists of "related" interfaces.  */

extern int num_interface_classes (void);
extern int interface_class_num_members (int cl);
extern const char *interface_class_member (int cl, int mbr);


/* Functional units: Info about shared functions.  */

extern int num_funcUnits (void);
extern const char *funcUnit_name (int f);
extern int funcUnit_num_copies (int f);

#endif /* XTENSA_ISA_INTERFACE_H */
