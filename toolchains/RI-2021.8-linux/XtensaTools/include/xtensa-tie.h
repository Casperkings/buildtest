/* Interface definition for the Xtensa TIE library.  */

/* Copyright (c) 2004-2012 Tensilica Inc.

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

#ifndef XTENSA_LIBTIE_H
#define XTENSA_LIBTIE_H

/* Version number: Set to the Xtensa version number.  This is intended
   to help support code that works with versions of this library from
   multiple Xtensa releases.  */
#define XTENSA_TIE_VERSION 1


#ifdef __cplusplus
extern "C" {
#if 0
} /* fix emacs autoindent */
#endif
#endif

/***************************************************************************/
/* This file defines the interface to the Xtensa TIE library. */


/***************************************************************************/
/* Opaque object types. */
   
/* Use opaque types to provide a simple C interface to the Xtensa TIE
   library. These opaque types are resolved through typecasts to internal
   C++ object types.
   
   In most cases the use of opaque iterator types can be abstracted away
   on the client side through xtie_foreach_... iterators. */

typedef struct xtensa_tie_opaque { int _; } *xtensa_tie;
typedef struct xtie_phase_opaque { int _; } *xtie_phase;
typedef struct xtie_writer_opaque { int _; } *xtie_writer;

typedef struct xtie_const_opaque { int _; } *xtie_const;
typedef struct xtie_cstub_swap_opaque { int _; } *xtie_cstub_swap;
typedef struct xtie_ctype_field_opaque { int _; } *xtie_ctype_field;
typedef struct xtie_ctype_opaque { int _; } *xtie_ctype;
typedef struct xtie_description_opaque { int _; } *xtie_description;
typedef struct xtie_encoding_opaque { int _; } *xtie_encoding;
typedef struct xtie_exception_opaque { int _; } *xtie_exception;
typedef struct xtie_field_opaque { int _; } *xtie_field;
typedef struct xtie_fielddef_opaque { int _; } *xtie_fielddef;
typedef struct xtie_format_opaque { int _; } *xtie_format;
typedef struct xtie_function_io_opaque { int _; } *xtie_function_io;
typedef struct xtie_function_opaque { int _; } *xtie_function;
typedef struct xtie_module_arg_opaque { int _; } *xtie_module_arg;
typedef struct xtie_module_opaque { int _; } *xtie_module;
typedef struct xtie_iclass_arg_opaque { int _; } *xtie_iclass_arg;
typedef struct xtie_iclass_opaque { int _; } *xtie_iclass;
typedef struct xtie_id_opaque { int _; } *xtie_id;
typedef struct xtie_imap_arg_opaque { int _; } *xtie_imap_arg;
typedef struct xtie_imap_opaque { int _; } *xtie_imap;
typedef struct xtie_imap_pattern_code_arg_opaque { int _; } *xtie_imap_pattern_code_arg;
typedef struct xtie_imap_pattern_code_opaque { int _; } *xtie_imap_pattern_code;
typedef struct xtie_imap_pattern_opaque { int _; } *xtie_imap_pattern;
typedef struct xtie_imap_pattern_temp_opaque { int _; } *xtie_imap_pattern_temp;
typedef struct xtie_immed_range_opaque { int _; } *xtie_immed_range;
typedef struct xtie_interface_opaque { int _; } *xtie_interface;
typedef struct xtie_length_opaque { int _; } *xtie_length;
typedef struct xtie_lookup_opaque { int _; } *xtie_lookup;
typedef struct xtie_opcode_opaque { int _; } *xtie_opcode;
typedef struct xtie_opcodedef_opaque { int _; } *xtie_opcodedef;
typedef struct xtie_operand_map_opaque { int _; } *xtie_operand_map;
typedef struct xtie_operand_opaque { int _; } *xtie_operand;
typedef struct xtie_operand_sem_opaque { int _; } *xtie_operand_sem;
typedef struct xtie_operation_arg_opaque { int _; } *xtie_operation_arg;
typedef struct xtie_operation_opaque { int _; } *xtie_operation;
typedef struct xtie_operation_si_opaque { int _; } *xtie_operation_si;
typedef struct xtie_operator_opaque { int _; } *xtie_operator;
typedef struct xtie_package_opaque { int _; } *xtie_package;
typedef struct xtie_property_opaque { int _; } *xtie_property;
typedef struct xtie_proto_opaque { int _; } *xtie_proto;
typedef struct xtie_queue_opaque { int _; } *xtie_queue;
typedef struct xtie_reference_opaque { int _; } *xtie_reference;
typedef struct xtie_regfile_opaque { int _; } *xtie_regfile;
typedef struct xtie_regfile_view_opaque { int _; } *xtie_regfile_view;
typedef struct xtie_regport_opaque { int _; } *xtie_regport;
typedef struct xtie_regport_view_opaque { int _; } *xtie_regport_view;
typedef struct xtie_regport_view_width_opaque { int _; } *xtie_regport_view_width;
typedef struct xtie_regport_view_stage_opaque { int _; } *xtie_regport_view_stage;
typedef struct xtie_schedule_opaque { int _; } *xtie_schedule;
typedef struct xtie_semantic_opaque { int _; } *xtie_semantic;
typedef struct xtie_signal_opaque { int _; } *xtie_signal;
typedef struct xtie_signals_opaque { int _; } *xtie_signals;
typedef struct xtie_slot_opaque { int _; } *xtie_slot;
typedef struct xtie_slotdef_opaque { int _; } *xtie_slotdef;
typedef struct xtie_state_opaque { int _; } *xtie_state;
typedef struct xtie_synopsis_opaque { int _; } *xtie_synopsis;
typedef struct xtie_table_opaque { int _; } *xtie_table;
typedef struct xtie_test_opaque { int _; } *xtie_test;
typedef struct xtie_tieport_opaque { int _; } *xtie_tieport;
typedef struct xtie_sreg_opaque { int _; } *xtie_sreg;
typedef struct xtie_ureg_opaque { int _; } *xtie_ureg;
typedef struct xtie_usedef_opaque { int _; } *xtie_usedef;
typedef struct xtie_vector_opaque { int _; } *xtie_vector;

typedef struct xtie_const_iter_opaque { int _; } *xtie_const_iter;
typedef struct xtie_cstub_swap_iter_opaque { int _; } *xtie_cstub_swap_iter;
typedef struct xtie_ctype_iter_opaque { int _; } *xtie_ctype_iter;
typedef struct xtie_description_iter_opaque { int _; } *xtie_description_iter;
typedef struct xtie_encoding_iter_opaque { int _; } *xtie_encoding_iter;
typedef struct xtie_exception_iter_opaque { int _; } *xtie_exception_iter;
typedef struct xtie_field_iter_opaque { int _; } *xtie_field_iter;
typedef struct xtie_fielddef_iter_opaque { int _; } *xtie_fielddef_iter;
typedef struct xtie_format_iter_opaque { int _; } *xtie_format_iter;
typedef struct xtie_function_io_iter_opaque { int _; } *xtie_function_io_iter;
typedef struct xtie_function_iter_opaque { int _; } *xtie_function_iter;
typedef struct xtie_module_iter_opaque { int _; } *xtie_module_iter;
typedef struct xtie_module_arg_iter_opaque { int _; } *xtie_module_arg_iter;
typedef struct xtie_iclass_arg_iter_opaque { int _; } *xtie_iclass_arg_iter;
typedef struct xtie_iclass_iter_opaque { int _; } *xtie_iclass_iter;
typedef struct xtie_id_iter_opaque { int _; } *xtie_id_iter;
typedef struct xtie_imap_arg_iter_opaque { int _; } *xtie_imap_arg_iter;
typedef struct xtie_imap_iter_opaque { int _; } *xtie_imap_iter;
typedef struct xtie_imap_pattern_code_arg_iter_opaque { int _; } *xtie_imap_pattern_code_arg_iter;
typedef struct xtie_imap_pattern_code_iter_opaque { int _; } *xtie_imap_pattern_code_iter;
typedef struct xtie_imap_pattern_temp_iter_opaque { int _; } *xtie_imap_pattern_temp_iter;
typedef struct xtie_immed_range_iter_opaque { int _; } *xtie_immed_range_iter;
typedef struct xtie_interface_iter_opaque { int _; } *xtie_interface_iter;
typedef struct xtie_length_iter_opaque { int _; } *xtie_length_iter;
typedef struct xtie_lookup_iter_opaque { int _; } *xtie_lookup_iter;
typedef struct xtie_opcode_iter_opaque { int _; } *xtie_opcode_iter;
typedef struct xtie_opcodedef_iter_opaque { int _; } *xtie_opcodedef_iter;
typedef struct xtie_operand_iter_opaque { int _; } *xtie_operand_iter;
typedef struct xtie_operand_map_iter_opaque { int _; } *xtie_operand_map_iter;
typedef struct xtie_operand_sem_iter_opaque { int _; } *xtie_operand_sem_iter;
typedef struct xtie_operation_arg_iter_opaque { int _; } *xtie_operation_arg_iter;
typedef struct xtie_operation_iter_opaque { int _; } *xtie_operation_iter;
typedef struct xtie_operation_si_iter_opaque { int _; } *xtie_operation_si_iter;
typedef struct xtie_operator_iter_opaque { int _; } *xtie_operator_iter;
typedef struct xtie_package_iter_opaque { int _; } *xtie_package_iter;
typedef struct xtie_property_iter_opaque { int _; } *xtie_property_iter;
typedef struct xtie_proto_iter_opaque { int _; } *xtie_proto_iter;
typedef struct xtie_queue_iter_opaque { int _; } *xtie_queue_iter;
typedef struct xtie_reference_iter_opaque { int _; } *xtie_reference_iter;
typedef struct xtie_regfile_iter_opaque { int _; } *xtie_regfile_iter;
typedef struct xtie_regfile_view_iter_opaque { int _; } *xtie_regfile_view_iter;
typedef struct xtie_regport_iter_opaque { int _; } *xtie_regport_iter;
typedef struct xtie_regport_view_iter_opaque { int _; } *xtie_regport_view_iter;
typedef struct xtie_regport_view_width_iter_opaque { int _; } *xtie_regport_view_width_iter;
typedef struct xtie_regport_view_stage_iter_opaque { int _; } *xtie_regport_view_stage_iter;
typedef struct xtie_schedule_iter_opaque { int _; } *xtie_schedule_iter;
typedef struct xtie_semantic_iter_opaque { int _; } *xtie_semantic_iter;
typedef struct xtie_slot_iter_opaque { int _; } *xtie_slot_iter;
typedef struct xtie_slotdef_iter_opaque { int _; } *xtie_slotdef_iter;
typedef struct xtie_state_iter_opaque { int _; } *xtie_state_iter;
typedef struct xtie_synopsis_iter_opaque { int _; } *xtie_synopsis_iter;
typedef struct xtie_table_iter_opaque { int _; } *xtie_table_iter;
typedef struct xtie_test_iter_opaque { int _; } *xtie_test_iter;
typedef struct xtie_tieport_iter_opaque { int _; } *xtie_tieport_iter;
typedef struct xtie_sreg_iter_opaque { int _; } *xtie_sreg_iter;
typedef struct xtie_ureg_iter_opaque { int _; } *xtie_ureg_iter;
typedef struct xtie_usedef_iter_opaque { int _; } *xtie_usedef_iter;
typedef struct xtie_vector_iter_opaque { int _; } *xtie_vector_iter;

typedef struct xtie_xml_attr_opaque { int _; } *xtie_xml_attr;
typedef struct xtie_xml_item_opaque { int _; } *xtie_xml_item;



/***************************************************************************/
/* Iterators.

   Sample use of iterators for phase level objects:
   
   xtie_phase xphase = xtie_get_post_rewrite_phase(xtie);
   xtie_foreach_semantic(xphase, xsem)
     {
       ...
       Use 'xsem'. 'xsem' is an implicitly defined xtie_semantic
       object in this scope.
       ...
     }
   end_xtie_foreach_semantic;

   Sample use of iterators for children objects:

   xtie_function xfunc = ...
   xtie_function_foreach_input(xfunc, xin)
     {
       ...
       Use 'xin'. 'xin' is an implicitly defined xtie_function_io
       object in this scope.
       ...
     }
   end_xtie_function_foreach_input;

   The easiest way to determine the correct type of the implicitly
   defined iterator object is to inspect the first argument of the
   xtie_foreach or xtie_foreach_internal macro call. For example,
   an iterator that is defined through a call to

   xtie_foreach_internal(id, ...

   would implicitly define an xtie_id object.

   It is OK (i.e. there are no side effects such as memory leaks)
   to continue or break out of an iterator loop. */

/* Iterator helpers. Do not use directly. */
#define xtie_foreach_internal(OBJECT, obj, GET_ITER) \
  { \
    xtie_ ## OBJECT ## _iter _xit_; \
    for (_xit_ = GET_ITER; _xit_; _xit_ = xtie_ ## OBJECT ## _iter_step(_xit_)) \
      { \
        xtie_ ## OBJECT obj = xtie_ ## OBJECT ## _iter_get(_xit_);
#define end_xtie_foreach_internal(OBJECT) } }

#define xtie_foreach(OBJECT, phase, obj) \
  xtie_foreach_internal(OBJECT, obj, xtie_get_ ## OBJECT ## _iter(phase))
#define end_xtie_foreach(OBJECT) end_xtie_foreach_internal(OBJECT)


/***************************************************************************/
/* Error handling.  */

/* Error codes.  The code for the most recent error condition can be
   retrieved with the "errno" function.  For any result other than
   xtie_ok, an error message containing additional information
   about the problem can be retrieved using the "errmsg" function.
   The error messages are stored in an internal buffer, which should not
   be freed and may be overwritten by subsequent operations.  */

typedef enum xtie_status_enum
  {
    XTIE_OK = 0,
    XTIE_BUFFER_OVERFLOW,
    XTIE_INTERNAL_ERROR,
    XTIE_INVALID_MODULE,
    XTIE_INVALID_PHASE,
    XTIE_INVALID_NUMBER,
    XTIE_OUT_OF_MEMORY,
    XTIE_PARSE_ERROR
  } xtie_status;

extern xtie_status
xtie_errno (xtensa_tie tie);

extern char *
xtie_errmsg (xtensa_tie tie);


/***************************************************************************/
/* TIE information.  */

/* Load the TIE information from set of shared libraries.  The "dlls"
   parameter must be a null-terminated array of TIE DLL paths.  If
   successful, this returns a value which identifies the TIE for use in
   subsequent calls to the TIE library.  Otherwise, on error the return
   value is null, and if the "errno_p" and/or "errmsg_p" pointers are
   non-null, an error code and message will be stored through them.
   Multiple TIEs can be loaded to support heterogeneous multiprocessor
   systems.  */
extern xtensa_tie
xtie_init (char **dlls, xtie_status *errno_p, char **errmsg_p);


/* Deallocate an xtensa_tie structure.  */
extern void
xtie_free (xtensa_tie tie);


/***************************************************************************/
/* TIE phases. */

extern xtie_phase
xtie_get_post_parse_phase (xtensa_tie tie);

extern xtie_phase
xtie_get_post_rewrite_phase (xtensa_tie tie);

extern xtie_phase
xtie_get_compiler_phase (xtensa_tie tie);

extern xtie_phase
xtie_get_xinfo_phase (xtensa_tie tie);

extern int
xtie_phase_is_little_endian (xtie_phase phase);

extern int
xtie_phase_is_big_endian (xtie_phase phase);

extern int
xtie_phase_get_rstage (xtie_phase phase);

extern int
xtie_phase_get_estage (xtie_phase phase);

extern int
xtie_phase_get_mstage (xtie_phase phase);

extern int
xtie_phase_get_wstage (xtie_phase phase);


/***************************************************************************/
/* Writers */

/* Clients can initialize as many writers as they like. All data generated
   by the writer is allocated in its own memory pool. The writer must be
   freed in order to release the allocated memory. */

/* A writer to TIE. */
extern xtie_writer
xtie_tie_writer_init (xtie_phase phase);

extern void
xtie_writer_free (xtie_writer writer);

extern void
xtie_writer_set_indent (xtie_writer writer, int indent);


/***************************************************************************/
/* TIE XML predefined tags.
   NOTE: Keep this enum in-sync with the tag properties in xtensa-tie.cc. */
typedef enum xtie_tag_enum
  {
    XTIE_TAG_UNKNOWN = 0,
    XTIE_TAG_ADD,
    XTIE_TAG_ARG_IN,
    XTIE_TAG_ARG_INOUT,
    XTIE_TAG_ARG_LIST,
    XTIE_TAG_ARG_OUT,
    XTIE_TAG_ASSIGNMENT,
    XTIE_TAG_ASSM_NOTE,
    XTIE_TAG_BITWISE_AND,
    XTIE_TAG_BITWISE_NEGATION,
    XTIE_TAG_BITWISE_OR,
    XTIE_TAG_BITWISE_XNOR,
    XTIE_TAG_BITWISE_XOR,
    XTIE_TAG_CALL,
    XTIE_TAG_CODE,
    XTIE_TAG_CODES,
    XTIE_TAG_CODE_ARG,
    XTIE_TAG_CODE_ARGS,
    XTIE_TAG_COMPMOD,
    XTIE_TAG_COMPMOD_ARG,
    XTIE_TAG_COMPMOD_ARG_LIST,
    XTIE_TAG_CONCATENATION,
    XTIE_TAG_CONDITIONAL,
    XTIE_TAG_CONST,
    XTIE_TAG_COPROCESSOR,
    XTIE_TAG_CORE_SIGNAL,
    XTIE_TAG_CP_DEPENDS,
    XTIE_TAG_CSTUB_SWAP,
    XTIE_TAG_CTYPE,
    XTIE_TAG_CTYPE_FIELD,
    XTIE_TAG_CTYPE_FIELD_LIST,
    XTIE_TAG_DEF,
    XTIE_TAG_DESCRIPTION,
    XTIE_TAG_ENCODING,
    XTIE_TAG_ENDPACKAGE,
    XTIE_TAG_EQ,
    XTIE_TAG_EXCEPTION,
    XTIE_TAG_FIELD,
    XTIE_TAG_FIELDDEF,
    XTIE_TAG_FORMAT,
    XTIE_TAG_FUNCTION,
    XTIE_TAG_FUNCTION_IO,
    XTIE_TAG_GEQ,
    XTIE_TAG_GT,
    XTIE_TAG_ICLASS,
    XTIE_TAG_ID,
    XTIE_TAG_IF,
    XTIE_TAG_IMAP,
    XTIE_TAG_IMAP_PATTERN,
    XTIE_TAG_IMMED_RANGE,
    XTIE_TAG_IMPL_NOTE,
    XTIE_TAG_INT,
    XTIE_TAG_INTERFACE,
    XTIE_TAG_LENGTH,
    XTIE_TAG_LEQ,
    XTIE_TAG_LIST,
    XTIE_TAG_LOGICAL_AND,
    XTIE_TAG_LOGICAL_NEGATION,
    XTIE_TAG_LOGICAL_OR,
    XTIE_TAG_LOOKUP,
    XTIE_TAG_LT,
    XTIE_TAG_MODULE,
    XTIE_TAG_MODULE_ARG_LIST,
    XTIE_TAG_MODULE_STMT,
    XTIE_TAG_MULT,
    XTIE_TAG_NEQ,
    XTIE_TAG_OPCODE,
    XTIE_TAG_OPCODEDEF,
    XTIE_TAG_OPERAND,
    XTIE_TAG_OPERAND_MAP,
    XTIE_TAG_OPERAND_SEM,
    XTIE_TAG_OPERATION,
    XTIE_TAG_OPERATOR,
    XTIE_TAG_PACKAGE,
    XTIE_TAG_PRINT,
    XTIE_TAG_PRINT_ARG_LIST,
    XTIE_TAG_PROGRAM,
    XTIE_TAG_PROPERTY,
    XTIE_TAG_PROTO,
    XTIE_TAG_PROTO_ARGS,
    XTIE_TAG_PROTO_INOUT_ARG,
    XTIE_TAG_PROTO_IN_ARG,
    XTIE_TAG_PROTO_OUT_ARG,
    XTIE_TAG_PROTO_TEMPS,
    XTIE_TAG_PROTO_TEMP_ARG,
    XTIE_TAG_QUEUE,
    XTIE_TAG_REDUCTION_AND,
    XTIE_TAG_REDUCTION_NAND,
    XTIE_TAG_REDUCTION_NOR,
    XTIE_TAG_REDUCTION_OR,
    XTIE_TAG_REDUCTION_XNOR,
    XTIE_TAG_REDUCTION_XOR,
    XTIE_TAG_REFERENCE,
    XTIE_TAG_REGFILE,
    XTIE_TAG_REGFILE_VIEW,
    XTIE_TAG_REGFILE_VIEWS,
    XTIE_TAG_REGPORT,
    XTIE_TAG_REGPORT_VIEW,
    XTIE_TAG_REGPORT_VIEWS,
    XTIE_TAG_REPLICATION,
    XTIE_TAG_SCHEDULE,
    XTIE_TAG_SEMANTIC,
    XTIE_TAG_SHIFT_LEFT,
    XTIE_TAG_SHIFT_RIGHT,
    XTIE_TAG_SLOT,
    XTIE_TAG_SLOTDEF,
    XTIE_TAG_SLOT_OPCODES,
    XTIE_TAG_SREG,
    XTIE_TAG_STATE,
    XTIE_TAG_STATEMENTS,
    XTIE_TAG_STRING,
    XTIE_TAG_SUB,
    XTIE_TAG_SYNOPSIS,
    XTIE_TAG_TABLE,
    XTIE_TAG_TEST,
    XTIE_TAG_TIEPORT,
    XTIE_TAG_UREG,
    XTIE_TAG_USE,
    XTIE_TAG_USEDEF_LIST,
    XTIE_TAG_VECTOR,
    XTIE_TAG_WIRE,
    XTIE_TAG_XDOC,
    XTIE_TAG_XEMPTY,
    XTIE_TAG_XTEXT,
    XTIE_TAG_LAST /* this must be the last tag */
  } xtie_tag;


extern const char *
xtie_tag_get_str (xtie_tag tag);


/***************************************************************************/
/* TIE XML predefined attributes.
   NOTE: Keep this enum in-sync with the attribute properties in xtensa-tie.cc. */
typedef enum xtie_attr_enum
  {
    XTIE_ATTR_UNKNOWN = 0,
    XTIE_ATTR_AUTOADD,
    XTIE_ATTR_ENDIAN,
    XTIE_ATTR_FROM,
    XTIE_ATTR_GENERATED_SIGNAL,
    XTIE_ATTR_HIDDEN,
    XTIE_ATTR_HWUSE, 
    XTIE_ATTR_INTERNAL,
    XTIE_ATTR_ISSUSE, 
    XTIE_ATTR_LINE,
    XTIE_ATTR_MANGLED_NAME,
    XTIE_ATTR_NAME,
    XTIE_ATTR_PACKAGE,
    XTIE_ATTR_PARENT,
    XTIE_ATTR_SHARE,
    XTIE_ATTR_TCUSE, 
    XTIE_ATTR_TO,
    XTIE_ATTR_TYPE,
    XTIE_ATTR_WIDTH,
    XTIE_ATTR_VALUE,
    XTIE_ATTR_VALUE_INT,
    XTIE_ATTR_XTEXT,
    XTIE_ATTR_SHARED_FIELD,
    XTIE_ATTR_RSTAGE,
    XTIE_ATTR_ESTAGE,
    XTIE_ATTR_MSTAGE,
    XTIE_ATTR_WSTAGE,
    XTIE_ATTR_INSTBUF_WIDTH, 
    XTIE_ATTR_LAST /* this must be the last attr */
  } xtie_attr;


extern const char *
xtie_attr_get_str (xtie_attr attr);


/***************************************************************************/
/* TIE XML item. */


extern xtie_tag
xtie_xml_item_get_tag (xtie_xml_item xi);

extern const char *
xtie_xml_item_get_name (xtie_xml_item xi);

extern const char *
xtie_xml_item_get_attr_value (xtie_xml_item xi, xtie_attr attr);

extern int
xtie_xml_item_get_attr_int_value (xtie_xml_item xi, xtie_attr attr);

extern int
xtie_xml_item_is_text (xtie_xml_item xi);

extern const char *
xtie_xml_item_get_text (xtie_xml_item xi);

extern xtie_xml_item
xtie_xml_item_get_first_item (xtie_xml_item xi);

extern xtie_xml_item
xtie_xml_item_get_next (xtie_xml_item xi);

extern char *
xtie_xml_item_write (xtie_xml_item xi, xtie_writer xw);

extern xtie_xml_attr
xtie_xml_item_find_attr_by_name(xtie_xml_item xi, const char *name);

/***************************************************************************/
/* TIE signal direction.
   NOTE: Keep this enum in-sync with the direction properties in xtensa-tie.cc. */
typedef enum xtie_dir_enum
  {
    XTIE_DIR_UNKNOWN = 0,
    XTIE_DIR_IN,
    XTIE_DIR_OUT,
    XTIE_DIR_INOUT,
    XTIE_DIR_LAST /* this must be the last dir */
  } xtie_dir;


extern const char *
xtie_dir_get_str (xtie_dir attr);


/***************************************************************************/
/* TIE signal kind.
   NOTE: Keep this enum in-sync with the signal kind properties in xtensa-tie.cc. */
typedef enum xtie_signal_kind_enum
  {
    XTIE_SIG_UNKNOWN = 0,
    XTIE_SIG_BITKILL, 
    XTIE_SIG_DECODE,
    XTIE_SIG_EXCEPTION,
    XTIE_SIG_FIELD,
    XTIE_SIG_INTERFACE,
    XTIE_SIG_KILL,
    XTIE_SIG_OPER_ARG,
    XTIE_SIG_OPERAND,
    XTIE_SIG_STATE,
    XTIE_SIG_LAST /* this must be the last signal kind */
  } xtie_signal_kind;


extern const char *
xtie_signal_kind_get_str (xtie_signal_kind attr);


/***************************************************************************/
/* TIE signals. */

/* For some TIE objects (e.g. semantics, references and exceptions),
   clients may request a signal collection object. This object provides
   information about all signals (operands, states, interfaces, etc.)
   accessed by the TIE object. */

extern xtie_signal
xtie_signals_find_signal (xtie_signals sigs, const char *name);

extern int
xtie_signals_num_signals (xtie_signals sigs);

extern int
xtie_signals_num_decodes (xtie_signals sigs);

extern int
xtie_signals_num_exceptions (xtie_signals sigs);

extern int
xtie_signals_num_fields (xtie_signals sigs);

extern int
xtie_signals_num_interfaces (xtie_signals sigs);

extern int
xtie_signals_num_kills (xtie_signals sigs);

extern int
xtie_signals_num_bitkills (xtie_signals sigs);

extern int
xtie_signals_num_operands (xtie_signals sigs);

extern int
xtie_signals_num_states (xtie_signals sigs);

extern int
xtie_signals_num_oper_args (xtie_signals sigs);

extern xtie_signal
xtie_signals_get_signal (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_decode (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_exception (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_field (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_interface (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_kill (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_bitkill (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_operand (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_oper_arg (xtie_signals sigs, int i);

extern xtie_signal
xtie_signals_get_state (xtie_signals sigs, int i);

extern const char *
xtie_signal_get_name (xtie_signal sig);

extern int
xtie_signal_get_from_index (xtie_signal sig);

extern int
xtie_signal_get_to_index (xtie_signal sig);

extern xtie_dir
xtie_signal_get_dir (xtie_signal sig);

extern xtie_signal_kind
xtie_signal_get_kind (xtie_signal sig);

extern const char *
xtie_signal_get_type_name (xtie_signal sig);

extern int
xtie_signal_is_pointer (xtie_signal sig);

/* Return true if 'sig' is a TC-generated signal. */
extern int
xtie_signal_is_generated (xtie_signal sig);

extern xtie_signal
xtie_signal_get_kill_signal (xtie_signal sig);

extern xtie_signal
xtie_signal_get_bitkill_signal (xtie_signal sig);

extern xtie_signal
xtie_signal_get_killed_signal (xtie_signal sig);

#define xtie_signal_get_bitkilled_signal xtie_signal_get_killed_signal


/***************************************************************************/
/* TIE 'const'. */

extern const char *
xtie_const_get_value (xtie_const c);

extern xtie_const_iter
xtie_const_iter_step (xtie_const_iter iter);

extern xtie_const
xtie_const_iter_get (xtie_const_iter iter);


/***************************************************************************/
/* TIE 'cstub_swap'. */

extern xtie_cstub_swap
xtie_find_cstub_swap (xtie_phase phase, const char *name);

extern const char *
xtie_cstub_swap_get_name (xtie_cstub_swap swap);

extern const char *
xtie_cstub_swap_get_package (xtie_cstub_swap swap);

extern xtie_id_iter
xtie_cstub_swap_get_inst_iter (xtie_cstub_swap swap);

extern xtie_xml_item
xtie_cstub_swap_get_concatenation (xtie_cstub_swap swap);

extern const char *
xtie_cstub_swap_write (xtie_cstub_swap cstub_swap, xtie_writer xw);

extern xtie_cstub_swap_iter
xtie_get_cstub_swap_iter (xtie_phase phase);

extern xtie_cstub_swap_iter
xtie_cstub_swap_iter_step (xtie_cstub_swap_iter iter);

extern xtie_cstub_swap
xtie_cstub_swap_iter_get (xtie_cstub_swap_iter iter);

#define xtie_foreach_cstub_swap(phase, swap) xtie_foreach(cstub_swap, phase, swap)
#define end_xtie_foreach_cstub_swap end_xtie_foreach(cstub_swap)

#define xtie_cstub_swap_foreach_inst(swap, inst) \
  xtie_foreach_internal(id, inst, xtie_cstub_swap_get_inst_iter(swap))
#define end_xtie_cstub_swap_foreach_inst end_xtie_foreach_internal(id)


/***************************************************************************/
/* TIE 'ctype'. */

extern xtie_ctype
xtie_find_ctype (xtie_phase phase, const char *name);

/* The mangled name is the same as the libisa xtensa_ctype_get_name().  */
extern xtie_ctype
xtie_find_ctype_mangled (xtie_phase phase, const char *name);

extern const char *
xtie_ctype_get_name (xtie_ctype ctype);

extern const char *
xtie_ctype_get_mangled_name (xtie_ctype ctype);

extern const char *
xtie_ctype_get_package (xtie_ctype ctype);

extern int
xtie_ctype_get_width (xtie_ctype ctype);

extern int
xtie_ctype_get_alignment (xtie_ctype ctype);

extern const char *
xtie_ctype_get_regfile_view_name (xtie_ctype ctype);

extern int
xtie_ctype_is_default (xtie_ctype ctype);

extern int
xtie_ctype_is_proto_default (xtie_ctype ctype);

extern int
xtie_ctype_is_struct (xtie_ctype ctype);

extern int
xtie_ctype_num_fields (xtie_ctype ctype);

extern xtie_ctype_field
xtie_ctype_get_field (xtie_ctype ctype, unsigned int n);


extern const char *
xtie_ctype_write (xtie_ctype ctype, xtie_writer xw);

extern xtie_ctype_iter
xtie_get_ctype_iter (xtie_phase phase);

extern xtie_ctype_iter
xtie_ctype_iter_step (xtie_ctype_iter iter);

extern xtie_ctype
xtie_ctype_iter_get (xtie_ctype_iter iter);

#define xtie_foreach_ctype(phase, ct) xtie_foreach(ctype, phase, ct)
#define end_xtie_foreach_ctype end_xtie_foreach(ctype)


/***************************************************************************/
/* TIE 'ctype_field'. */


extern const char *
xtie_ctype_field_get_ctype_name(xtie_ctype_field fld);

extern const char *
xtie_ctype_field_get_name(xtie_ctype_field fld);

extern const char *
xtie_ctype_field_write(xtie_ctype_field fld);

/***************************************************************************/
/* TIE 'description'. */

extern xtie_description
xtie_find_description (xtie_phase phase, const char *name);

extern const char *
xtie_description_get_package (xtie_description description);

extern int
xtie_description_get_index (xtie_description description);

extern xtie_xml_item
xtie_description_get_expression (xtie_description description);

extern const char *
xtie_description_get_description (xtie_description description);

extern int
xtie_description_is_generated_name (xtie_description description);

extern int
xtie_description_is_generated_index (xtie_description description);

extern const char *
xtie_description_write (xtie_description description, xtie_writer xw);

extern xtie_description_iter
xtie_get_description_iter (xtie_phase phase);

extern xtie_description_iter
xtie_description_iter_step (xtie_description_iter iter);

extern xtie_description
xtie_description_iter_get (xtie_description_iter iter);

#define xtie_foreach_description(phase, q) xtie_foreach(description, phase, q)
#define end_xtie_foreach_description end_xtie_foreach(description)

/***************************************************************************/
/* TIE 'exception'. */

extern xtie_exception
xtie_find_exception (xtie_phase phase, const char *name);

extern const char *
xtie_exception_get_name (xtie_exception exc);

extern xtie_id_iter
xtie_exception_get_higher_priority_exception_iter (xtie_exception exc);

extern xtie_xml_item
xtie_exception_get_statements (xtie_exception exc);

extern xtie_signals
xtie_exception_get_signals (xtie_phase phase, xtie_exception exc);

extern const char *
xtie_exception_write (xtie_exception ctype, xtie_writer xw);

extern xtie_exception_iter
xtie_get_exception_iter (xtie_phase phase);

extern xtie_exception_iter
xtie_exception_iter_step (xtie_exception_iter iter);

extern xtie_exception
xtie_exception_iter_get (xtie_exception_iter iter);

#define xtie_foreach_exception(phase, exc) xtie_foreach(exception, phase, exc)
#define end_xtie_foreach_exception end_xtie_foreach(exception)

#define xtie_exception_foreach_higher_priority_exception(exc, exc_id) \
  xtie_foreach_internal(id, exc_id, xtie_exception_get_higher_priority_exception_iter(exc))
#define end_xtie_exception_foreach_higher_priority_exception end_xtie_foreach_internal(id)


/***************************************************************************/
/* TIE 'field'. */

extern xtie_field
xtie_find_field (xtie_phase phase, const char *name);

extern const char *
xtie_field_get_name (xtie_field field);

extern int
xtie_field_get_width (xtie_field field);

extern int
xtie_field_is_width_only (xtie_field field);

extern xtie_fielddef_iter
xtie_field_get_fielddef_iter (xtie_field field);

extern xtie_field_iter
xtie_get_field_iter (xtie_phase phase);

extern xtie_field_iter
xtie_field_iter_step (xtie_field_iter iter);

extern xtie_field
xtie_field_iter_get (xtie_field_iter iter);

#define xtie_foreach_field(phase, fld) xtie_foreach(field, phase, fld)
#define end_xtie_foreach_field end_xtie_foreach(field)

#define xtie_field_foreach_fielddef(fld, fd) \
  xtie_foreach_internal(fielddef, fd, xtie_field_get_fielddef_iter(fld))
#define end_xtie_field_foreach_fielddef end_xtie_foreach_internal(fielddef)


/***************************************************************************/
/* TIE 'fielddef'. */

extern const char *
xtie_fielddef_get_name (xtie_fielddef fd);

extern const char *
xtie_fielddef_get_package (xtie_fielddef fd);

extern int
xtie_fielddef_get_width (xtie_fielddef fd);

extern int
xtie_fielddef_is_width_only (xtie_fielddef fd);

extern const char *
xtie_fielddef_write (xtie_fielddef fielddef, xtie_writer xw);

/* Iterate through all subfields. Return an empty iterator for
   width-only fielddefs. */
extern xtie_id_iter
xtie_fielddef_get_subfield_iter (xtie_fielddef fd);

extern xtie_fielddef_iter
xtie_fielddef_iter_step (xtie_fielddef_iter iter);

extern xtie_fielddef
xtie_fielddef_iter_get (xtie_fielddef_iter iter);

#define xtie_fielddef_foreach_subfield(fd, subfield_id) \
  xtie_foreach_internal(id, subfield_id, xtie_fielddef_get_subfield_iter(fd))
#define end_xtie_fielddef_foreach_subfield end_xtie_foreach_internal(id)

/***************************************************************************/
/* TIE 'slotdef'. */

extern const char *
xtie_slotdef_get_name (xtie_slotdef sd);

extern const char *
xtie_slotdef_get_package (xtie_slotdef sd);

extern xtie_format
xtie_slotdef_get_format (xtie_slotdef sd);

extern xtie_slot
xtie_slotdef_get_slot (xtie_slotdef sd);

extern int
xtie_slotdef_get_index (xtie_slotdef sd);

extern int
xtie_slotdef_num_slotcomps (xtie_slotdef sd);

extern const char *
xtie_slotdef_write (xtie_slotdef sd, xtie_writer xw);

/* Iterate through all slotcomps. */
extern xtie_id_iter
xtie_slotdef_get_slotcomp_iter (xtie_slotdef sd);

extern xtie_slotdef_iter
xtie_slotdef_iter_step (xtie_slotdef_iter iter);

extern xtie_slotdef
xtie_slotdef_iter_get (xtie_slotdef_iter iter);

#define xtie_slotdef_foreach_slotcomp(sd, slotcomp_id) \
  xtie_foreach_internal(id, slotcomp_id, xtie_slotdef_get_slotcomp_iter(sd))
#define end_xtie_slotdef_foreach_slotcomp end_xtie_foreach_internal(id)


/***************************************************************************/
/* TIE 'format'. */

extern xtie_format
xtie_find_format (xtie_phase phase, const char *name);

extern const char *
xtie_format_get_name (xtie_format fmt);

extern const char *
xtie_format_get_package (xtie_format fmt);

extern xtie_length
xtie_format_get_length (xtie_format fmt);

/* Return the name of the 'length' object that associated with 'fmt'.
   If 'fmt' is defined through an explicit length value, return NULL. */
extern const char *
xtie_format_get_length_name (xtie_format fmt);

/* Return the bit size of 'fmt'. If 'fmt' is defined through a
   'length' object, return its width. Otherwise, return the explicit
   length value of 'fmt'. */
extern int
xtie_format_get_width (xtie_format fmt);

/* Return the number of bits in 'fmt' that can be used to encode
   slots and operations. That is, the width of 'fmt' without any
   bits required for format decoding. */
extern int
xtie_format_get_slotable_width (xtie_format fmt);

extern int
xtie_format_num_slots (xtie_format fmt);

extern xtie_id_iter
xtie_format_get_slot_iter (xtie_format fmt);

extern xtie_slotdef_iter
xtie_format_get_slotdef_iter (xtie_format fmt);

extern xtie_xml_item
xtie_format_get_decoder (xtie_format fmt);

extern const char *
xtie_format_write (xtie_format format, xtie_writer xw);

extern xtie_format_iter
xtie_get_format_iter (xtie_phase phase);

extern xtie_format_iter
xtie_format_iter_step (xtie_format_iter iter);

extern xtie_format
xtie_format_iter_get (xtie_format_iter iter);

#define xtie_foreach_format(phase, fmt) xtie_foreach(format, phase, fmt)
#define end_xtie_foreach_format end_xtie_foreach(format)

#define xtie_format_foreach_slot(fmt, slt) \
  xtie_foreach_internal(id, slt, xtie_format_get_slot_iter(fmt))
#define end_xtie_format_foreach_slot end_xtie_foreach_internal(id)

#define xtie_format_foreach_slotdef(fmt, slt) \
  xtie_foreach_internal(slotdef, slt, xtie_format_get_slotdef_iter(fmt))
#define end_xtie_format_foreach_slotdef end_xtie_foreach_internal(slotdef)

/***************************************************************************/
/* TIE 'function'. */

extern xtie_function
xtie_find_function (xtie_phase phase, const char *name);

extern const char *
xtie_function_get_name (xtie_function func);

extern const char *
xtie_function_get_package (xtie_function func);

extern xtie_function_io
xtie_function_get_output (xtie_function func);

extern xtie_function_io_iter
xtie_function_get_input_iter (xtie_function func);

extern int
xtie_function_is_shared (xtie_function func);

extern int
xtie_function_is_global_shared (xtie_function func);

extern int
xtie_function_is_slot_shared (xtie_function func);

extern xtie_xml_item
xtie_function_get_statements (xtie_function func);

extern const char *
xtie_function_io_get_name (xtie_function_io func_io);

extern int
xtie_function_io_get_from_index (xtie_function_io func_io);

extern int
xtie_function_io_get_to_index (xtie_function_io func_io);
  
extern const char *
xtie_function_write (xtie_function function, xtie_writer xw);

extern xtie_function_iter
xtie_get_function_iter (xtie_phase phase);

extern xtie_function_iter
xtie_function_iter_step (xtie_function_iter iter);

extern xtie_function
xtie_function_iter_get (xtie_function_iter iter);

extern xtie_function_io_iter
xtie_function_io_iter_step (xtie_function_io_iter iter);

extern xtie_function_io
xtie_function_io_iter_get (xtie_function_io_iter iter);

#define xtie_foreach_function(phase, func) xtie_foreach(function, phase, func)
#define end_xtie_foreach_function end_xtie_foreach(function)

#define xtie_function_foreach_input(func, io) \
  xtie_foreach_internal(function_io, io, xtie_function_get_input_iter(func))
#define end_xtie_function_foreach_input end_xtie_foreach_internal(function_io)


/***************************************************************************/
/* TIE 'module'. */

extern xtie_module
xtie_find_module (xtie_phase phase, const char *name);

extern const char *
xtie_module_get_name (xtie_module module);

extern const char *
xtie_module_get_package (xtie_module module);

extern xtie_module_arg_iter
xtie_module_get_arg_iter (xtie_module module);

/*
extern int
xtie_module_is_shared (xtie_module module);

extern int
xtie_module_is_global_shared (xtie_module module);

extern int
xtie_module_is_slot_shared (xtie_module module);
*/

extern xtie_xml_item
xtie_module_get_statements (xtie_module module);

extern const char *
xtie_module_arg_get_name (xtie_module_arg module_arg);

extern int
xtie_module_arg_get_from_index (xtie_module_arg module_arg);

extern int
xtie_module_arg_get_to_index (xtie_module_arg module_arg);

extern xtie_dir 
xtie_module_arg_get_dir (xtie_module_arg module_arg);
  
extern const char *
xtie_module_write (xtie_module module, xtie_writer xw);

extern xtie_module_iter
xtie_get_module_iter (xtie_phase phase);

extern xtie_module_iter
xtie_module_iter_step (xtie_module_iter iter);

extern xtie_module
xtie_module_iter_get (xtie_module_iter iter);

extern xtie_module_arg_iter
xtie_module_arg_iter_step (xtie_module_arg_iter iter);

extern xtie_module_arg
xtie_module_arg_iter_get (xtie_module_arg_iter iter);

#define xtie_foreach_module(phase, mod) xtie_foreach(module, phase, mod)
#define end_xtie_foreach_module end_xtie_foreach(module)

#define xtie_module_foreach_arg(mod, arg) \
  xtie_foreach_internal(module_arg, arg, xtie_module_get_arg_iter(mod))
#define end_xtie_module_foreach_arg end_xtie_foreach_internal(module_arg)

/***************************************************************************/
/* TIE 'iclass'. */

extern xtie_iclass
xtie_find_iclass (xtie_phase phase, const char *name);

extern const char *
xtie_iclass_get_name (xtie_iclass iclass);

extern const char *
xtie_iclass_get_package (xtie_iclass iclass);

/* Return true if 'iclass' contains virtual instructions that haven't
   been instantiated in any slots. */
extern int
xtie_iclass_is_virtual (xtie_phase xp, xtie_iclass iclass);

extern xtie_iclass_arg
xtie_iclass_get_operand (xtie_iclass iclass, unsigned int n);

extern xtie_iclass_arg
xtie_iclass_get_state (xtie_iclass iclass, unsigned int n);

extern xtie_iclass_arg
xtie_iclass_get_interface (xtie_iclass iclass, unsigned int n);

extern xtie_iclass_arg
xtie_iclass_get_exception (xtie_iclass iclass, unsigned int n);

extern int
xtie_iclass_num_instructions (xtie_iclass iclass);

extern int
xtie_iclass_num_operands (xtie_iclass iclass);

extern int
xtie_iclass_num_states (xtie_iclass iclass);

extern int
xtie_iclass_num_interfaces (xtie_iclass iclass);

extern int
xtie_iclass_num_exceptions (xtie_iclass iclass);

extern xtie_id_iter
xtie_iclass_get_inst_iter (xtie_iclass iclass);

extern xtie_iclass_arg_iter
xtie_iclass_get_operand_iter (xtie_iclass iclass);

extern xtie_iclass_arg_iter
xtie_iclass_get_state_iter (xtie_iclass iclass);

extern xtie_iclass_arg_iter
xtie_iclass_get_interface_iter (xtie_iclass iclass);

extern xtie_iclass_arg_iter
xtie_iclass_get_exception_iter (xtie_iclass iclass);

extern const char *
xtie_iclass_arg_get_name (xtie_iclass_arg arg);

extern xtie_dir
xtie_iclass_arg_get_dir (xtie_iclass_arg arg);

extern int
xtie_iclass_arg_get_width (xtie_phase phase, xtie_iclass_arg arg);

extern int
xtie_iclass_arg_is_shared_field (xtie_iclass_arg arg);

/* Return true if 'arg' is a TC-generated iclass argument. */
extern int
xtie_iclass_arg_is_generated_signal (xtie_iclass_arg arg);

extern const char *
xtie_iclass_write (xtie_iclass iclass, xtie_writer xw);

extern xtie_iclass_iter
xtie_get_iclass_iter (xtie_phase phase);

extern xtie_iclass_iter
xtie_iclass_iter_step (xtie_iclass_iter iter);

extern xtie_iclass
xtie_iclass_iter_get (xtie_iclass_iter iter);

extern xtie_iclass_arg_iter
xtie_iclass_arg_iter_step (xtie_iclass_arg_iter iter);

extern xtie_iclass_arg
xtie_iclass_arg_iter_get (xtie_iclass_arg_iter iter);

#define xtie_foreach_iclass(phase, ic) xtie_foreach(iclass, phase, ic)
#define end_xtie_foreach_iclass end_xtie_foreach(iclass)

#define xtie_iclass_foreach_inst(ic, inst) \
  xtie_foreach_internal(id, inst, xtie_iclass_get_inst_iter(ic))
#define end_xtie_iclass_foreach_inst end_xtie_foreach_internal(id)

#define xtie_iclass_foreach_operand(ic, opnd) \
  xtie_foreach_internal(iclass_arg, opnd, xtie_iclass_get_operand_iter(ic))
#define end_xtie_iclass_foreach_operand end_xtie_foreach_internal(iclass_arg)

#define xtie_iclass_foreach_state(ic, st) \
  xtie_foreach_internal(iclass_arg, st, xtie_iclass_get_state_iter(ic))
#define end_xtie_iclass_foreach_state end_xtie_foreach_internal(iclass_arg)

#define xtie_iclass_foreach_interface(ic, intf) \
  xtie_foreach_internal(iclass_arg, intf, xtie_iclass_get_interface_iter(ic))
#define end_xtie_iclass_foreach_interface end_xtie_foreach_internal(iclass_arg)

#define xtie_iclass_foreach_exception(ic, exc) \
  xtie_foreach_internal(iclass_arg, exc, xtie_iclass_get_exception_iter(ic))
#define end_xtie_iclass_foreach_exception end_xtie_foreach_internal(iclass_arg)


/***************************************************************************/
/* TIE 'id'. */

extern const char *
xtie_id_get_name (xtie_id id);

/* Return 0 if 'id' doesn't have 'from' and 'to' attributes,
   otherwise initialize the 'from' and the 'to' values. If the id
   has only a 'from' field, set the 'to' field to be the same as the
   'from'. This routine expects that 'id' has integer 'from' and
   'to' attributes. */
extern int
xtie_id_get_from_to (xtie_id id, int *from, int *to);

extern xtie_id_iter
xtie_id_iter_step (xtie_id_iter iter);

extern xtie_id
xtie_id_iter_get (xtie_id_iter iter);

/***************************************************************************/
/* TIE 'imap'. */

extern xtie_imap
xtie_find_imap (xtie_phase phase, const char *name);

extern const char *
xtie_imap_get_name (xtie_imap imap);

extern const char *
xtie_imap_get_package (xtie_imap imap);

extern const char *
xtie_imap_write (xtie_imap imap, xtie_writer xw);

extern xtie_imap_iter
xtie_get_imap_iter (xtie_phase phase);

extern xtie_imap_iter
xtie_imap_iter_step (xtie_imap_iter iter);

extern xtie_imap
xtie_imap_iter_get (xtie_imap_iter iter);

#define xtie_foreach_imap(phase, pr) xtie_foreach(imap, phase, pr)
#define end_xtie_foreach_imap end_xtie_foreach(imap)

extern xtie_imap_pattern
xtie_imap_get_impl(xtie_imap imap);

extern xtie_imap_pattern
xtie_imap_get_pattern(xtie_imap imap);

extern int
xtie_imap_num_args(xtie_imap imap);

extern xtie_imap_arg
xtie_imap_get_arg (xtie_imap imap, unsigned int n);

extern const char *
xtie_imap_arg_get_type_name(xtie_imap_arg arg);

extern const char *
xtie_imap_arg_get_name(xtie_imap_arg arg);

extern xtie_dir
xtie_imap_arg_get_dir(xtie_imap_arg arg);

extern int
xtie_imap_arg_is_pointer(xtie_imap_arg arg);

extern const char *
xtie_imap_pattern_temp_get_type_name(xtie_imap_pattern_temp temp);

extern const char *
xtie_imap_pattern_temp_get_name(xtie_imap_pattern_temp temp);

extern const char *
xtie_imap_pattern_code_get_name(xtie_imap_pattern_code code);

/* This function may return a constant */
extern const char *
xtie_imap_pattern_code_arg_get_name(xtie_imap_pattern_code_arg arg);

extern int
xtie_imap_pattern_code_arg_is_constant(xtie_imap_pattern_code_arg arg);

extern int
xtie_imap_pattern_code_arg_get_constant(xtie_imap_pattern_code_arg arg);

int
xtie_imap_pattern_num_temps (xtie_imap_pattern imap_pattern);

xtie_imap_pattern_temp
xtie_imap_pattern_get_temp (xtie_imap_pattern imap_pattern, unsigned int n);

int
xtie_imap_pattern_num_codes (xtie_imap_pattern imap_pattern);

xtie_imap_pattern_code
xtie_imap_pattern_get_code (xtie_imap_pattern imap_pattern, unsigned int n);

int
xtie_imap_pattern_code_num_args (xtie_imap_pattern_code imap_pattern_code);

xtie_imap_pattern_code_arg
xtie_imap_pattern_code_get_arg (xtie_imap_pattern_code imap_pattern_code, unsigned int n);

extern xtie_imap_arg_iter
xtie_imap_get_arg_iter (xtie_imap imap);

extern xtie_imap_arg_iter
xtie_imap_arg_iter_step (xtie_imap_arg_iter iter);

extern xtie_imap_arg
xtie_imap_arg_iter_get (xtie_imap_arg_iter iter);

extern xtie_imap_pattern_temp_iter
xtie_imap_pattern_get_temp_iter (xtie_imap_pattern pattern);

extern xtie_imap_pattern_temp_iter
xtie_imap_pattern_temp_iter_step (xtie_imap_pattern_temp_iter iter);

extern xtie_imap_pattern_temp
xtie_imap_pattern_temp_iter_get (xtie_imap_pattern_temp_iter iter);

extern xtie_imap_pattern_code_iter
xtie_imap_pattern_get_code_iter (xtie_imap_pattern pattern);

extern xtie_imap_pattern_code_iter
xtie_imap_pattern_code_iter_step (xtie_imap_pattern_code_iter iter);

extern xtie_imap_pattern_code
xtie_imap_pattern_code_iter_get (xtie_imap_pattern_code_iter iter);

extern xtie_imap_pattern_code_arg_iter
xtie_imap_pattern_code_get_arg_iter (xtie_imap_pattern_code code);

extern xtie_imap_pattern_code_arg_iter
xtie_imap_pattern_code_arg_iter_step (xtie_imap_pattern_code_arg_iter iter);

extern xtie_imap_pattern_code_arg
xtie_imap_pattern_code_arg_iter_get (xtie_imap_pattern_code_arg_iter iter);


/***************************************************************************/
/* TIE 'immed_range'. */

extern xtie_immed_range
xtie_find_immed_range (xtie_phase phase, const char *name);

extern const char *
xtie_immed_range_get_name (xtie_immed_range ir);

extern const char *
xtie_immed_range_get_package (xtie_immed_range ir);

extern int
xtie_immed_range_get_width (xtie_immed_range ir);

extern int
xtie_immed_range_get_low (xtie_immed_range ir);

extern int
xtie_immed_range_get_high (xtie_immed_range ir);

extern int
xtie_immed_range_get_step (xtie_immed_range ir);

extern const char *
xtie_immed_range_write (xtie_immed_range immed_range, xtie_writer xw);

extern xtie_immed_range_iter
xtie_get_immed_range_iter (xtie_phase phase);

extern xtie_immed_range_iter
xtie_immed_range_iter_step (xtie_immed_range_iter iter);

extern xtie_immed_range
xtie_immed_range_iter_get (xtie_immed_range_iter iter);

#define xtie_foreach_immed_range(phase, ir) xtie_foreach(immed_range, phase, ir)
#define end_xtie_foreach_immed_range end_xtie_foreach(immed_range)


/***************************************************************************/
/* TIE 'interface'. */

extern xtie_interface
xtie_find_interface (xtie_phase phase, const char *name);

extern const char *
xtie_interface_get_name (xtie_interface intf);

extern const char *
xtie_interface_get_package (xtie_interface intf);

extern int
xtie_interface_get_width (xtie_interface intf);

extern const char *
xtie_interface_get_to_name (xtie_interface intf);

extern int
xtie_interface_is_core (xtie_interface intf);

extern int
xtie_interface_is_external (xtie_interface intf);

extern int
xtie_interface_is_tie_internal (xtie_interface intf);

extern int
xtie_interface_is_internal (xtie_interface intf);

extern int
xtie_interface_is_hidden (xtie_interface intf);

extern int
xtie_interface_is_autoadd (xtie_interface intf);

extern int
xtie_interface_is_tcuse (xtie_interface intf);

extern int
xtie_interface_is_issuse (xtie_interface intf);

extern int
xtie_interface_is_hwuse (xtie_interface intf);

extern xtie_dir
xtie_interface_get_dir (xtie_interface intf);

extern const char *
xtie_interface_get_dir_str (xtie_interface intf);

extern int
xtie_interface_is_in (xtie_interface intf);

extern int
xtie_interface_is_out (xtie_interface intf);

extern int
xtie_interface_get_stage (xtie_interface intf);

extern const char *
xtie_interface_get_type (xtie_interface intf);

extern int
xtie_interface_is_data (xtie_interface intf);

extern int
xtie_interface_is_control (xtie_interface intf);

extern int
xtie_interface_is_registered (xtie_interface intf);

extern int
xtie_interface_get_buffer_depth (xtie_interface intf);

extern const char *
xtie_interface_get_default_value (xtie_interface intf);

extern xtie_xml_item
xtie_interface_get_stall_expr (xtie_interface intf);

extern const char *
xtie_interface_write (xtie_interface intf, xtie_writer xw);

extern xtie_interface_iter
xtie_get_interface_iter (xtie_phase phase);

extern xtie_interface_iter
xtie_interface_iter_step (xtie_interface_iter iter);

extern xtie_interface
xtie_interface_iter_get (xtie_interface_iter iter);

#define xtie_foreach_interface(phase, intf) xtie_foreach(interface, phase, intf)
#define end_xtie_foreach_interface end_xtie_foreach(interface)


/***************************************************************************/
/* TIE 'length'. */

extern xtie_length
xtie_find_length (xtie_phase phase, const char *name);

extern const char *
xtie_length_get_name (xtie_length len);

extern const char *
xtie_length_get_package (xtie_length len);

extern int
xtie_length_get_width (xtie_length len);

extern xtie_xml_item
xtie_length_get_decoder (xtie_length len);

/* Return decoder logic for the InstBuf byte containing op0. If 'b' is
   the byte containing op0, then ((b & mask) == code) would decode this
   length. */
extern void
xtie_length_get_op0_byte_decode (xtie_length len,
                                 unsigned char *mask, unsigned char *code);

extern const char *
xtie_length_write (xtie_length length, xtie_writer xw);

extern xtie_length_iter
xtie_get_length_iter (xtie_phase phase);

extern xtie_length_iter
xtie_length_iter_step (xtie_length_iter iter);

extern xtie_length
xtie_length_iter_get (xtie_length_iter iter);

#define xtie_foreach_length(phase, len) xtie_foreach(length, phase, len)
#define end_xtie_foreach_length end_xtie_foreach(length)


/***************************************************************************/
/* TIE 'lookup'. */

extern xtie_lookup
xtie_find_lookup (xtie_phase phase, const char *name);

extern const char *
xtie_lookup_get_name (xtie_lookup lookup);

extern const char *
xtie_lookup_get_package (xtie_lookup lookup);

extern int
xtie_lookup_get_out_width (xtie_lookup lookup);

extern int
xtie_lookup_get_out_stage (xtie_lookup lookup);

extern int
xtie_lookup_get_in_width (xtie_lookup lookup);

extern int
xtie_lookup_get_in_stage (xtie_lookup lookup);

extern int
xtie_lookup_has_rdy (xtie_lookup lookup);

extern const char *
xtie_lookup_write (xtie_lookup lookup, xtie_writer xw);

extern xtie_lookup_iter
xtie_get_lookup_iter (xtie_phase phase);

extern xtie_lookup_iter
xtie_lookup_iter_step (xtie_lookup_iter iter);

extern xtie_lookup
xtie_lookup_iter_get (xtie_lookup_iter iter);

#define xtie_foreach_lookup(phase, l) xtie_foreach(lookup, phase, l)
#define end_xtie_foreach_lookup end_xtie_foreach(lookup)


/***************************************************************************/
/* TIE 'opcode'. */

extern xtie_opcode
xtie_find_opcode (xtie_phase phase, const char *name);

extern const char *
xtie_opcode_get_name (xtie_opcode opcode);

/* If 'opcode' is an instruction opcode return its package (i.e., the
   package of its iclass or operation. Otherwise, return NULL. */
extern const char *
xtie_opcode_get_package (xtie_opcode opcode);

extern int
xtie_opcode_is_instruction (xtie_opcode opcode);

/* Return true if 'opcode' is a virtual opcode that is not instantiated
   in any slots. */
extern int
xtie_opcode_is_virtual (xtie_opcode opcode);

extern int
xtie_opcode_is_load (xtie_opcode opcode);

extern int
xtie_opcode_is_store (xtie_opcode opcode);

extern int
xtie_opcode_is_branch (xtie_opcode opcode);

/* Return the set of signals for an instruction 'opcode'. */
extern xtie_signals
xtie_opcode_get_signals (xtie_opcode opcode);

extern xtie_iclass
xtie_opcode_get_iclass (xtie_opcode opcode);

extern xtie_operation
xtie_opcode_get_operation (xtie_opcode opcode);

extern xtie_reference
xtie_opcode_get_reference (xtie_opcode opcode);

extern xtie_schedule
xtie_opcode_get_schedule (xtie_opcode opcode);

extern xtie_semantic
xtie_opcode_get_semantic (xtie_opcode opcode);

extern xtie_opcode_iter
xtie_get_opcode_iter (xtie_phase phase);

extern xtie_opcode_iter
xtie_opcode_iter_step (xtie_opcode_iter iter);

extern xtie_opcode
xtie_opcode_iter_get (xtie_opcode_iter iter);

#define xtie_foreach_opcode(phase, opc) xtie_foreach(opcode, phase, opc)
#define end_xtie_foreach_opcode end_xtie_foreach(opcode)


/***************************************************************************/
/* TIE 'opcodedef'. */

extern const char *
xtie_opcodedef_get_name (xtie_opcodedef od);

extern const char *
xtie_opcodedef_get_package (xtie_opcodedef od);

extern xtie_opcode
xtie_opcodedef_get_opcode (xtie_opcodedef od);

extern int
xtie_opcodedef_is_slot_only (xtie_opcodedef od);

extern const char *
xtie_opcodedef_write (xtie_opcodedef od, xtie_writer xw);

extern xtie_encoding_iter
xtie_opcodedef_get_encoding_iter (xtie_opcodedef od);

extern xtie_opcodedef_iter
xtie_opcodedef_iter_step (xtie_opcodedef_iter iter);

extern xtie_opcodedef
xtie_opcodedef_iter_get (xtie_opcodedef_iter iter);

extern const char *
xtie_encoding_get_name (xtie_encoding enc);

extern const char *
xtie_encoding_get_value (xtie_encoding enc);

extern xtie_encoding_iter
xtie_encoding_iter_step (xtie_encoding_iter iter);

extern xtie_encoding
xtie_encoding_iter_get (xtie_encoding_iter iter);

#define xtie_opcodedef_foreach_encoding(od, enc) \
  xtie_foreach_internal(encoding, enc, xtie_opcodedef_get_encoding_iter(od))
#define end_xtie_opcodedef_foreach_encoding end_xtie_foreach_internal(encoding)


/***************************************************************************/
/* TIE 'operand'. */

extern xtie_operand
xtie_find_operand (xtie_phase phase, const char *name);

extern const char *
xtie_operand_get_name (xtie_operand opnd);

extern const char *
xtie_operand_get_package (xtie_operand opnd);

extern const char *
xtie_operand_get_field_name (xtie_operand opnd);

extern int
xtie_operand_get_field_width (xtie_phase phase, xtie_operand opnd);

extern xtie_xml_item
xtie_operand_get_decode (xtie_operand opnd);

extern xtie_xml_item
xtie_operand_get_encode (xtie_operand opnd);

extern xtie_xml_item
xtie_operand_get_decode2 (xtie_operand opnd);

extern xtie_xml_item
xtie_operand_get_hardware_decode (xtie_operand opnd);

extern const char *
xtie_operand_get_operand_sem_name (xtie_operand opnd);

extern int
xtie_operand_is_implicit (xtie_operand opnd);

extern int
xtie_operand_is_label (xtie_operand opnd);

extern int
xtie_operand_is_regfile (xtie_phase phase, xtie_operand opnd);

extern int
xtie_operand_is_immed_range (xtie_phase phase, xtie_operand opnd);

extern int
xtie_operand_is_table (xtie_phase phase, xtie_operand opnd);

extern xtie_regfile
xtie_operand_get_regfile_or_view (xtie_phase phase, xtie_operand opnd,
                                  xtie_regfile_view *view);

extern xtie_immed_range
xtie_operand_get_immed_range (xtie_phase phase, xtie_operand opnd);

extern xtie_table
xtie_operand_get_table (xtie_phase phase, xtie_operand opnd);

extern xtie_xml_item
xtie_operand_get_statements (xtie_phase phase, xtie_operand opnd);

extern int
xtie_operand_get_width (xtie_phase phase, xtie_operand opnd);

extern const char *
xtie_operand_write (xtie_operand operand, xtie_writer xw);

extern xtie_operand_iter
xtie_get_operand_iter (xtie_phase phase);

extern xtie_operand_iter
xtie_operand_iter_step (xtie_operand_iter iter);

extern xtie_operand
xtie_operand_iter_get (xtie_operand_iter iter);

#define xtie_foreach_operand(phase, opnd) xtie_foreach(operand, phase, opnd)
#define end_xtie_foreach_operand end_xtie_foreach(operand)

/***************************************************************************/
/* TIE 'operand_map'. */

extern xtie_operand_map
xtie_find_operand_map (xtie_phase phase, const char *name);

extern const char *
xtie_operand_map_get_name (xtie_operand_map map);

extern const char *
xtie_operand_map_get_operand (xtie_operand_map map);

extern const char *
xtie_operand_map_get_package (xtie_operand_map map);

extern const char *
xtie_operand_map_get_operation (xtie_operand_map map);

extern const char *
xtie_operand_map_get_operation_arg (xtie_operand_map map);

extern xtie_operand_map_iter
xtie_get_operand_map_iter (xtie_phase phase);

extern xtie_operand_map_iter
xtie_operand_map_iter_step (xtie_operand_map_iter iter);

extern xtie_operand_map
xtie_operand_map_iter_get (xtie_operand_map_iter iter);

extern const char *
xtie_operand_map_write(xtie_operand_map map, xtie_writer xw);

#define xtie_foreach_operand_map(phase, map) xtie_foreach(operand_map, phase, map)
#define end_xtie_foreach_operand_map end_xtie_foreach(operand_map)

/***************************************************************************/
/* TIE 'operand_sem'. */

extern xtie_operand_sem
xtie_find_operand_sem (xtie_phase phase, const char *name);

extern const char *
xtie_operand_sem_get_name (xtie_operand_sem sem);

extern const char *
xtie_operand_sem_get_package (xtie_operand_sem sem);

extern const char *
xtie_operand_sem_get_type (xtie_operand_sem sem);

extern const char *
xtie_operand_sem_get_out_name (xtie_operand_sem sem);

extern int
xtie_operand_sem_get_out_width (xtie_operand_sem sem);

extern const char *
xtie_operand_sem_get_in_name (xtie_operand_sem sem);

extern int
xtie_operand_sem_get_in_width (xtie_operand_sem sem);

extern xtie_xml_item
xtie_operand_sem_get_decode (xtie_operand_sem sem);

extern xtie_xml_item
xtie_operand_sem_get_encode (xtie_operand_sem sem);

extern xtie_xml_item
xtie_operand_sem_get_statements (xtie_operand_sem sem);

extern int
xtie_operand_sem_has_hardware_decode (xtie_operand_sem sem);

extern int
xtie_operand_sem_is_immediate (xtie_operand_sem sem);

extern int
xtie_operand_sem_is_register (xtie_operand_sem sem);

extern xtie_regfile
xtie_operand_sem_get_regfile_or_view (xtie_phase phase, xtie_operand_sem sem,
				      xtie_regfile_view *view);
extern xtie_operand_sem_iter
xtie_get_operand_sem_iter (xtie_phase phase);

extern xtie_operand_sem_iter
xtie_operand_sem_iter_step (xtie_operand_sem_iter iter);

extern xtie_operand_sem
xtie_operand_sem_iter_get (xtie_operand_sem_iter iter);

extern const char *
xtie_operand_sem_write(xtie_operand_sem sem, xtie_writer xw);

#define xtie_foreach_operand_sem(phase, sem) xtie_foreach(operand_sem, phase, sem)
#define end_xtie_foreach_operand_sem end_xtie_foreach(operand_sem)

/***************************************************************************/
/* TIE 'operation'. */

extern xtie_operation
xtie_find_operation (xtie_phase phase, const char *name);

extern const char *
xtie_operation_get_name (xtie_operation oper);

extern const char *
xtie_operation_get_package (xtie_operation oper);

extern xtie_xml_item
xtie_operation_get_statements (xtie_operation oper);

extern xtie_operation_arg
xtie_operation_get_arg (xtie_operation oper, unsigned int n);

extern xtie_operation_si
xtie_operation_get_si (xtie_operation oper, unsigned int n);

extern xtie_operation_arg_iter
xtie_operation_get_arg_iter (xtie_operation oper);

extern xtie_operation_si_iter
xtie_operation_get_si_iter (xtie_operation oper);

extern const char *
xtie_operation_arg_get_type_name (xtie_operation_arg arg);

extern const char *
xtie_operation_arg_get_name (xtie_operation_arg arg);

extern xtie_dir
xtie_operation_arg_get_dir (xtie_operation_arg arg);

extern int
xtie_operation_arg_get_width (xtie_phase phase, xtie_operation_arg arg);

extern int
xtie_operation_arg_is_pointer (xtie_operation_arg arg);

extern const char *
xtie_operation_si_get_name (xtie_operation_si si);

extern xtie_dir
xtie_operation_si_get_dir (xtie_operation_si si);

extern int
xtie_operation_si_get_width (xtie_phase phase, xtie_operation_si si);

extern int
xtie_operation_si_is_state (xtie_phase phase, xtie_operation_si si);

extern int
xtie_operation_si_is_interface (xtie_phase phase, xtie_operation_si si);

extern xtie_signals
xtie_operation_get_signals (xtie_phase phase, xtie_operation oper);

extern const char *
xtie_operation_write (xtie_operation operation, xtie_writer xw);

extern xtie_operation_iter
xtie_get_operation_iter (xtie_phase phase);

extern xtie_operation_iter
xtie_operation_iter_step (xtie_operation_iter iter);

extern xtie_operation
xtie_operation_iter_get (xtie_operation_iter iter);

extern xtie_operation_arg_iter
xtie_operation_arg_iter_step (xtie_operation_arg_iter iter);

extern xtie_operation_arg
xtie_operation_arg_iter_get (xtie_operation_arg_iter iter);

extern xtie_operation_si_iter
xtie_operation_si_iter_step (xtie_operation_si_iter iter);

extern xtie_operation_si
xtie_operation_si_iter_get (xtie_operation_si_iter iter);

#define xtie_foreach_operation(phase, oper) xtie_foreach(operation, phase, oper)
#define end_xtie_foreach_operation end_xtie_foreach(operation)

#define xtie_operation_foreach_arg(oper, arg) \
  xtie_foreach_internal(operation_arg, arg, xtie_operation_get_arg_iter(oper))
#define end_xtie_operation_foreach_arg end_xtie_foreach_internal(operation_arg)

#define xtie_operation_foreach_si(oper, si) \
  xtie_foreach_internal(operation_si, si, xtie_operation_get_si_iter(oper))
#define end_xtie_operation_foreach_si end_xtie_foreach_internal(operation_si)

/***************************************************************************/
/* TIE 'operator'. */

extern const char *
xtie_operator_get_name (xtie_operator op);

extern const char *
xtie_operator_get_proto (xtie_operator op);

extern const char *
xtie_operator_get_package (xtie_operator op);

/* Only available in compiler library */
extern const char *
xtie_operator_get_mangled_proto (xtie_operator op);

extern xtie_operator_iter
xtie_get_operator_iter (xtie_phase phase);

extern xtie_operator_iter
xtie_operator_iter_step (xtie_operator_iter iter);

extern xtie_operator
xtie_operator_iter_get (xtie_operator_iter iter);

extern const char *
xtie_operator_write(xtie_operator op, xtie_writer xw);

#define xtie_foreach_operator(phase, oper) xtie_foreach(operator, phase, oper)
#define end_xtie_foreach_operator end_xtie_foreach(operator)

/***************************************************************************/
/* TIE 'package'. */

extern xtie_package
xtie_find_package (xtie_phase phase, const char *name);

extern const char *
xtie_package_get_name (xtie_package pkg);

extern const char *
xtie_package_get_description (xtie_package pkg);

extern const char *
xtie_package_get_prefix (xtie_package pkg);

extern const char *
xtie_package_get_parent_name (xtie_package pkg);

extern bool  
init_vector_intrinsics();

extern int
xtie_package_is_user (xtie_phase phase, xtie_package pkg);

extern const char *
xtie_package_write (xtie_package package, xtie_writer xw);

extern xtie_package_iter
xtie_get_package_iter (xtie_phase phase);

extern xtie_package_iter
xtie_package_iter_step (xtie_package_iter iter);

extern xtie_package
xtie_package_iter_get (xtie_package_iter iter);

#define xtie_foreach_package(phase, pkg) xtie_foreach(package, phase, pkg)
#define end_xtie_foreach_package end_xtie_foreach(package)


/***************************************************************************/
/* TIE 'property'. */

/* TIE predefined properties tags.
   NOTE: Keep this enum in-sync with the tag properties in xtensa-tie.cc. */
typedef enum xtie_property_type_enum
  {
    XTIE_PROPERTY_UNKNOWN = 0,
    XTIE_PROPERTY_CALLEE_SAVED, 
    XTIE_PROPERTY_COMMUTATIVE_INPUTS, 
    XTIE_PROPERTY_DISPLAY_FORMAT_NAME,
    XTIE_PROPERTY_DISPLAY_FORMAT_MAP, 
    XTIE_PROPERTY_ENNEWIMPL, 
    XTIE_PROPERTY_ENTIECOMP,
    XTIE_PROPERTY_EQUIVALENT_CTYPES,
    XTIE_PROPERTY_EQUIVALENT_OPS,
    XTIE_PROPERTY_GROUP,
    XTIE_PROPERTY_HEADER_INCLUDE,
    XTIE_PROPERTY_IMAP_NONEXACT, 
    XTIE_PROPERTY_FUSED_MADD_IMAP, 
    XTIE_PROPERTY_ISSUE_RATE,
    XTIE_PROPERTY_LOOKUP_MEMORY, 
    XTIE_PROPERTY_NONFLIX,
    XTIE_PROPERTY_OPCONFLICT_GROUP,
    XTIE_PROPERTY_OPERATION_CONFLICT,
    XTIE_PROPERTY_POST_UPDATE,    
    XTIE_PROPERTY_PRE_UPDATE,    
    XTIE_PROPERTY_PREDICATED_OP_FALSE,    
    XTIE_PROPERTY_PREDICATED_OP_TRUE,    
    XTIE_PROPERTY_PROMOTE_TYPE,    
    XTIE_PROPERTY_SEM_NO_RETIMING, 
    XTIE_PROPERTY_SHARED_SEMANTIC, 
    XTIE_PROPERTY_SIMD_CTYPE,
    XTIE_PROPERTY_SIMD_CTYPE_LITTLE_ENDIAN,
    XTIE_PROPERTY_SIMD_CTYPE_PRIORITY,
    XTIE_PROPERTY_SIMD_REMAINDER_LOOP_PREDICATE,
    XTIE_PROPERTY_SIMD_NZ_REMAINDER_LOOP_PREDICATE,
    XTIE_PROPERTY_SIMD_VSEL_ELEMENT_MEMORY_SIZE,
    XTIE_PROPERTY_SIMD_VSEL_IMMEDIATE_TABLES,
    XTIE_PROPERTY_SIMD_VSEL_TYPE_FACTOR,
    XTIE_PROPERTY_SIMD_VSHFL_ELEMENT_MEMORY_SIZE,
    XTIE_PROPERTY_SIMD_VSHFL_IMMEDIATE_TABLES,
    XTIE_PROPERTY_SIMD_VSHFL_TYPE_FACTOR,
    XTIE_PROPERTY_VECTOR_GATHER,
    XTIE_PROPERTY_VECTOR_SCATTER,
    XTIE_PROPERTY_SIMD_PROMO,
    XTIE_PROPERTY_SIMD_PROTO,
    XTIE_PROPERTY_SIMD_STATE,
    XTIE_PROPERTY_SIMD_REDUCTION,
    XTIE_PROPERTY_SIMD_ORDERED_REDUCTION,
    XTIE_PROPERTY_VECTOR_INTRINSIC,
    XTIE_PROPERTY_SPECIALIZED_OP,
    XTIE_PROPERTY_SPECIALIZED_OP_EQUIVALENT_STATE,
    XTIE_PROPERTY_SPECIALIZED_PIPE,
    XTIE_PROPERTY_USED_INSTR_ARG_BITS,
    XTIE_PROPERTY_SPLIT_PIPELINE,
    XTIE_PROPERTY_STATE_ALLOWS_OP_REORDERING,
    XTIE_PROPERTY_STRETCH_STREAM_GET,
    XTIE_PROPERTY_STRETCH_STREAM_PUT,
    XTIE_PROPERTY_STRETCH_VARIABLE_LS_SIZE,
    XTIE_PROPERTY_SW_INVISIBLE_STATE,
    XTIE_PROPERTY_XTENSA_CORE_OPERAND,
    XTIE_PROPERTY_XTENSA_CORE_STATE,
    XTIE_PROPERTY_CSTUB_ARG_POINTER,
    XTIE_PROPERTY_CSTUB_STATE_WIDTH,
    XTIE_PROPERTY_VARIABLE_DEF,
    XTIE_PROPERTY_VECTOR_INTRINSIC_SUPPORTS_UNALIGNED,
    XTIE_PROPERTY_BOOLEAN_SCALAR_CTYPE,
    XTIE_PROPERTY_VECTOR_INTRINSIC_RESULT_IS_LAST,
    XTIE_PROPERTY_PROTO_OVERRIDE,
    XTIE_PROPERTY_LAST /* this must be the last property type */
  } xtie_property_type;

extern const char *
xtie_property_type_get_str (xtie_property_type ptype);

extern const char *
xtie_property_get_name (xtie_property prop);

extern const char *
xtie_property_get_attribute (xtie_property prop);

extern const char *
xtie_property_get_package (xtie_property prop);

extern const char *
xtie_property_write (xtie_property prop, xtie_writer xw);

extern xtie_property_type
xtie_property_get_type (xtie_property prop);

extern const char *
xtie_property_get_type_id (xtie_property prop);

extern unsigned int
xtie_property_num_args (xtie_property prop);

extern const char *
xtie_property_get_later_arg_id (xtie_property prop, unsigned int n);

extern const char *
xtie_property_get_arg_id (xtie_property prop, unsigned int n);

extern int
xtie_property_get_arg_int (xtie_property prop, unsigned int n);

extern int
xtie_property_is_opcode_group (xtie_property prop);

extern xtie_property_iter
xtie_get_property_iter (xtie_phase phase);

extern xtie_property_iter
xtie_property_iter_step (xtie_property_iter iter);

extern xtie_property
xtie_property_iter_get (xtie_property_iter iter);

#define xtie_foreach_property(phase, prop) xtie_foreach(property, phase, prop)
#define end_xtie_foreach_property end_xtie_foreach(property)


/***************************************************************************/
/* TIE 'proto'. */

extern xtie_proto
xtie_find_proto (xtie_phase phase, const char *name);

extern xtie_proto
xtie_find_proto_mangled (xtie_phase phase, const char *name);

extern const char *
xtie_proto_get_name (xtie_proto proto);

extern const char *
xtie_proto_get_package (xtie_proto proto);

/* Return the non-mangled C-name for 'proto'. */
extern const char *
xtie_proto_get_cname (xtie_phase xp, xtie_proto proto);

/* Return true if 'proto' contains virtual instructions that haven't
   been instantiated in any slots. */
extern int
xtie_proto_is_virtual (xtie_phase xp, xtie_proto proto);

extern const char *
xtie_proto_get_mangled_name (xtie_proto proto);

extern const char *
xtie_proto_write (xtie_proto proto, xtie_writer xw);

extern xtie_proto_iter
xtie_get_proto_iter (xtie_phase phase);

extern xtie_proto_iter
xtie_proto_iter_step (xtie_proto_iter iter);

extern xtie_proto
xtie_proto_iter_get (xtie_proto_iter iter);

#define xtie_foreach_proto(phase, pr) xtie_foreach(proto, phase, pr)
#define end_xtie_foreach_proto end_xtie_foreach(proto)


/***************************************************************************/
/* TIE 'reference'. */

extern xtie_reference
xtie_find_reference (xtie_phase phase, const char *name);

extern const char *
xtie_reference_get_package (xtie_reference ref);

extern const char *
xtie_reference_get_name (xtie_reference ref);

extern xtie_id_iter
xtie_reference_get_inst_iter (xtie_reference ref);

extern xtie_xml_item
xtie_reference_get_statements (xtie_reference ref);

extern xtie_signals
xtie_reference_get_signals (xtie_phase phase, xtie_reference ref);

extern const char *
xtie_reference_write (xtie_reference reference, xtie_writer xw);

extern xtie_reference_iter
xtie_get_reference_iter (xtie_phase phase);

extern xtie_reference_iter
xtie_reference_iter_step (xtie_reference_iter iter);

extern xtie_reference
xtie_reference_iter_get (xtie_reference_iter iter);

#define xtie_foreach_reference(phase, ref) xtie_foreach(reference, phase, ref)
#define end_xtie_foreach_reference end_xtie_foreach(reference)

#define xtie_reference_foreach_inst(ref, inst) \
  xtie_foreach_internal(id, inst, xtie_reference_get_inst_iter(ref))
#define end_xtie_reference_foreach_inst end_xtie_foreach_internal(id)


/***************************************************************************/
/* TIE 'regfile'. */

extern xtie_regfile
xtie_find_regfile (xtie_phase phase, const char *name);

extern xtie_regfile
xtie_find_regfile_or_view (xtie_phase phase, const char *name, xtie_regfile_view *view);

extern const char *
xtie_regfile_get_name (xtie_regfile rf);

extern const char *
xtie_regfile_get_short_name (xtie_regfile rf);

extern const char *
xtie_regfile_get_package (xtie_regfile rf);

extern int
xtie_regfile_get_width (xtie_regfile rf);

extern int
xtie_regfile_get_depth (xtie_regfile rf);

extern int
xtie_regfile_get_factor (xtie_regfile rf);

extern int
xtie_regfile_is_ar (xtie_regfile rf);

extern int 
xtie_regfile_is_external(xtie_regfile rf);

extern xtie_regfile_view_iter
xtie_regfile_get_view_iter (xtie_regfile rf);

extern xtie_regport_view_iter
xtie_regfile_get_regport_view_iter (xtie_regfile rf);

extern const char *
xtie_regfile_view_get_name (xtie_regfile_view view);

/* Return the number of register file entries for 'view'. */
extern int
xtie_regfile_view_get_size (xtie_regfile_view view);

extern const char *
xtie_regfile_write (xtie_regfile regfile, xtie_writer xw);

extern xtie_regfile_iter
xtie_get_regfile_iter (xtie_phase phase);

extern xtie_regfile_iter
xtie_regfile_iter_step (xtie_regfile_iter iter);

extern xtie_regfile
xtie_regfile_iter_get (xtie_regfile_iter iter);

extern xtie_regfile_view_iter
xtie_regfile_view_iter_step (xtie_regfile_view_iter iter);

extern xtie_regfile_view
xtie_regfile_view_iter_get (xtie_regfile_view_iter iter);

extern const char *
xtie_regport_view_get_name (xtie_regport_view view);

extern const char *
xtie_regport_view_get_regfile_name (xtie_regport_view view);

extern int 
xtie_regport_view_is_read (xtie_regport_view view);

extern xtie_regport_view_iter
xtie_regport_view_iter_step (xtie_regport_view_iter iter);

extern xtie_regport_view
xtie_regport_view_iter_get (xtie_regport_view_iter iter);

extern xtie_regport_view_width_iter
xtie_regport_view_get_width_iter(xtie_regport_view view);

extern xtie_regport_view_width_iter
xtie_regport_view_width_iter_step (xtie_regport_view_width_iter iter);

extern xtie_regport_view_width
xtie_regport_view_width_iter_get (xtie_regport_view_width_iter iter);

extern int
xtie_regport_view_width_get_value (xtie_regport_view_width width);

extern xtie_regport_view_stage_iter
xtie_regport_view_get_stage_iter(xtie_regport_view view);

extern xtie_regport_view_stage_iter
xtie_regport_view_stage_iter_step (xtie_regport_view_stage_iter iter);

extern xtie_regport_view_stage
xtie_regport_view_stage_iter_get (xtie_regport_view_stage_iter iter);

extern int
xtie_regport_view_stage_get_value (xtie_regport_view_stage width);

#define xtie_foreach_regfile(phase, rf) xtie_foreach(regfile, phase, rf)
#define end_xtie_foreach_regfile end_xtie_foreach(regfile)

#define xtie_regfile_foreach_view(rf, view) \
  xtie_foreach_internal(regfile_view, view, xtie_regfile_get_view_iter(rf))
#define end_xtie_regfile_foreach_view end_xtie_foreach_internal(regfile_view)

#define xtie_regfile_foreach_regport_view(rf, view) \
  xtie_foreach_internal(regport_view, view, xtie_regfile_get_regport_view_iter(rf))
#define end_xtie_regfile_foreach_regport_view end_xtie_foreach_internal(view)

#define xtie_regport_view_foreach_width(view, width) \
  xtie_foreach_internal(regport_view_width, width, xtie_regport_view_get_width_iter(view))
#define end_xtie_regport_view_foreach_width end_xtie_foreach_internal(width)

#define xtie_regport_view_foreach_stage(view, stage) \
  xtie_foreach_internal(regport_view_stage, stage, xtie_regport_view_get_stage_iter(view))
#define end_xtie_regport_view_foreach_stage end_xtie_foreach_internal(stage)

/***************************************************************************/
/* TIE 'regport'. */

extern const char *
xtie_regport_get_name (xtie_regport rp);

extern const char *
xtie_regport_get_package (xtie_regport rp);

extern int
xtie_regport_get_read_number (xtie_regport rp);

extern int
xtie_regport_get_write_number (xtie_regport rp);

extern const char *
xtie_regport_write (xtie_regport rp, xtie_writer xw);

extern xtie_regport_iter
xtie_get_regport_iter (xtie_phase phase);

extern xtie_regport_iter
xtie_regport_iter_step (xtie_regport_iter iter);

extern xtie_regport
xtie_regport_iter_get (xtie_regport_iter iter);

#define xtie_foreach_regport(phase, rp) xtie_foreach(regport, phase, rp)
#define end_xtie_foreach_regport end_xtie_foreach(regport)


/***************************************************************************/
/* TIE 'schedule'. */

extern xtie_schedule
xtie_find_schedule (xtie_phase phase, const char *name);

extern const char *
xtie_schedule_get_name (xtie_schedule sched);

extern const char *
xtie_schedule_get_package (xtie_schedule sched);

/* Return true if 'sched' contains virtual instructions that haven't
   been instantiated in any slots. */
extern int
xtie_schedule_is_virtual (xtie_phase xp, xtie_schedule sched);

extern xtie_id_iter
xtie_schedule_get_inst_iter (xtie_schedule sched);

extern xtie_usedef_iter
xtie_schedule_get_usedef_iter (xtie_schedule sched);

extern xtie_schedule_iter
xtie_get_schedule_iter (xtie_phase phase);

extern const char *
xtie_usedef_get_name (xtie_usedef ud);

extern int
xtie_usedef_get_stage (xtie_usedef ud);

extern int
xtie_usedef_is_use (xtie_usedef ud);

extern int
xtie_usedef_is_def (xtie_usedef ud);

extern int
xtie_usedef_is_generated_signal (xtie_usedef ud);

extern const char *
xtie_schedule_write (xtie_schedule schedule, xtie_writer xw);

extern xtie_schedule_iter
xtie_schedule_iter_step (xtie_schedule_iter iter);

extern xtie_schedule
xtie_schedule_iter_get (xtie_schedule_iter iter);

extern xtie_usedef_iter
xtie_usedef_iter_step (xtie_usedef_iter iter);

extern xtie_usedef
xtie_usedef_iter_get (xtie_usedef_iter iter);

#define xtie_foreach_schedule(phase, sched) xtie_foreach(schedule, phase, sched)
#define end_xtie_foreach_schedule end_xtie_foreach(schedule)

#define xtie_schedule_foreach_inst(sched, inst) \
  xtie_foreach_internal(id, inst, xtie_schedule_get_inst_iter(sched))
#define end_xtie_schedule_foreach_inst end_xtie_foreach_internal(id)

#define xtie_schedule_foreach_usedef(sched, ud) \
  xtie_foreach_internal(usedef, ud, xtie_schedule_get_usedef_iter(sched))
#define end_xtie_schedule_foreach_usedef end_xtie_foreach_internal(usedef)


/***************************************************************************/
/* TIE 'semantic'. */

extern xtie_semantic
xtie_find_semantic (xtie_phase phase, const char *name);

extern const char *
xtie_semantic_get_name (xtie_semantic sem);

extern const char *
xtie_semantic_get_package (xtie_semantic sem);

extern xtie_id_iter
xtie_semantic_get_inst_iter (xtie_semantic sem);

extern xtie_xml_item
xtie_semantic_get_statements (xtie_semantic sem);

extern xtie_signals
xtie_semantic_get_signals (xtie_phase phase, xtie_semantic sem);

extern const char *
xtie_semantic_write (xtie_semantic semantic, xtie_writer xw);

extern xtie_semantic_iter
xtie_get_semantic_iter (xtie_phase phase);

extern xtie_semantic_iter
xtie_semantic_iter_step (xtie_semantic_iter iter);

extern xtie_semantic
xtie_semantic_iter_get (xtie_semantic_iter iter);

#define xtie_foreach_semantic(phase, sem) xtie_foreach(semantic, phase, sem)
#define end_xtie_foreach_semantic end_xtie_foreach(semantic)

#define xtie_semantic_foreach_inst(sem, inst) \
  xtie_foreach_internal(id, inst, xtie_semantic_get_inst_iter(sem))
#define end_xtie_semantic_foreach_inst end_xtie_foreach_internal(id)


/***************************************************************************/
/* TIE 'slot'. */

extern xtie_slot
xtie_find_slot (xtie_phase phase, const char *name);

extern const char *
xtie_slot_get_name (xtie_slot slot);

extern const char *
xtie_slot_get_package (xtie_slot slot);

/* Return the bit size of 'slot' or 0 if the slot doesn't have an
   explicit bit size. */
extern int
xtie_slot_get_width (xtie_slot slot);

/* Return the opcodedef for 'opcode' in 'slot'. */
extern xtie_opcodedef
xtie_slot_get_opcodedef (xtie_slot slot, xtie_opcode opcode);

extern xtie_fielddef_iter
xtie_slot_get_fielddef_iter (xtie_slot slot);

extern xtie_slotdef_iter
xtie_slot_get_slotdef_iter (xtie_slot slot);

extern xtie_opcodedef_iter
xtie_slot_get_opcodedef_iter (xtie_slot slot);

extern xtie_opcode_iter
xtie_slot_get_instruction_iter (xtie_slot slot);

/* Return 1 if 'slot' contains the instruction opcode 'inst'. */
extern int
xtie_slot_contains_instruction (xtie_slot slot, xtie_opcode inst);

/* Return 1 if 'slot' specifies the opcode group 'group_name' in a
   slot_opcodes statement. Note that non-post-parse TIE slots never
   specify any opcode groups because they are expanded by TC. */
extern int
xtie_slot_contains_opcode_group (xtie_slot slot, const char *group_name);

extern xtie_slot_iter
xtie_get_slot_iter (xtie_phase phase);

extern xtie_slot_iter
xtie_slot_iter_step (xtie_slot_iter iter);

extern xtie_slot
xtie_slot_iter_get (xtie_slot_iter iter);

#define xtie_foreach_slot(phase, slt) xtie_foreach(slot, phase, slt)
#define end_xtie_foreach_slot end_xtie_foreach(slot)

#define xtie_slot_foreach_fielddef(slt, fd) \
  xtie_foreach_internal(fielddef, fd, xtie_slot_get_fielddef_iter(slt))
#define end_xtie_slot_foreach_fielddef end_xtie_foreach_internal(fielddef)

#define xtie_slot_foreach_slotdef(slt, sd) \
  xtie_foreach_internal(slotdef, sd, xtie_slot_get_slotdef_iter(slt))
#define end_xtie_slot_foreach_slotdef end_xtie_foreach_internal(slotdef)

#define xtie_slot_foreach_opcodedef(slt, od) \
  xtie_foreach_internal(opcodedef, od, xtie_slot_get_opcodedef_iter(slt))
#define end_xtie_slot_foreach_opcodedef end_xtie_foreach_internal(opcodedef)

#define xtie_slot_foreach_instruction(slt, id) \
  xtie_foreach_internal(opcode, id, xtie_slot_get_instruction_iter(slt))
#define end_xtie_slot_foreach_instruction end_xtie_foreach_internal(opcode)


/***************************************************************************/
/* TIE 'state'. */

extern xtie_state
xtie_find_state (xtie_phase phase, const char *name);

extern const char *
xtie_state_get_name (xtie_state state);

extern const char *
xtie_state_get_package (xtie_state state);

extern int
xtie_state_get_width (xtie_state state);

extern const char *
xtie_state_get_init_value (xtie_state state);

extern int
xtie_state_is_export (xtie_state state);

extern int
xtie_state_is_shared_or (xtie_state state);

extern int
xtie_state_is_automap (xtie_state state);

extern int
xtie_state_get_export_stage (xtie_state state);

extern const char *
xtie_state_write (xtie_state state, xtie_writer xw);

extern xtie_state_iter
xtie_get_state_iter (xtie_phase phase);

extern xtie_state_iter
xtie_state_iter_step (xtie_state_iter iter);

extern xtie_state
xtie_state_iter_get (xtie_state_iter iter);

#define xtie_foreach_state(phase, st) xtie_foreach(state, phase, st)
#define end_xtie_foreach_state end_xtie_foreach(state)

/***************************************************************************/
/* TIE 'queue'. */

extern xtie_queue
xtie_find_queue (xtie_phase phase, const char *name);

extern const char *
xtie_queue_get_name (xtie_queue queue);

extern const char *
xtie_queue_get_package (xtie_queue queue);

extern int
xtie_queue_get_width (xtie_queue queue);

extern xtie_dir
xtie_queue_get_dir (xtie_queue queue);

extern int
xtie_queue_get_stage (xtie_queue queue);

extern const char *
xtie_queue_get_type (xtie_queue queue);

extern int
xtie_queue_is_in (xtie_queue queue);

extern int
xtie_queue_is_out (xtie_queue queue);

extern const char *
xtie_queue_write (xtie_queue queue, xtie_writer xw);

extern xtie_queue_iter
xtie_get_queue_iter (xtie_phase phase);

extern xtie_queue_iter
xtie_queue_iter_step (xtie_queue_iter iter);

extern xtie_queue
xtie_queue_iter_get (xtie_queue_iter iter);

#define xtie_foreach_queue(phase, q) xtie_foreach(queue, phase, q)
#define end_xtie_foreach_queue end_xtie_foreach(queue)


/***************************************************************************/
/* TIE 'synopsis'. */

extern xtie_synopsis
xtie_find_synopsis (xtie_phase phase, const char *name);

extern const char *
xtie_synopsis_get_package (xtie_synopsis synopsis);

extern int
xtie_synopsis_get_index (xtie_synopsis synopsis);

extern xtie_xml_item
xtie_synopsis_get_expression (xtie_synopsis synopsis);

extern const char *
xtie_synopsis_get_synopsis (xtie_synopsis synopsis);

extern int
xtie_synopsis_is_generated_name (xtie_synopsis synopsis);

extern int
xtie_synopsis_is_generated_index (xtie_synopsis synopsis);

extern const char *
xtie_synopsis_write (xtie_synopsis synopsis, xtie_writer xw);

extern xtie_synopsis_iter
xtie_get_synopsis_iter (xtie_phase phase);

extern xtie_synopsis_iter
xtie_synopsis_iter_step (xtie_synopsis_iter iter);

extern xtie_synopsis
xtie_synopsis_iter_get (xtie_synopsis_iter iter);

#define xtie_foreach_synopsis(phase, q) xtie_foreach(synopsis, phase, q)
#define end_xtie_foreach_synopsis end_xtie_foreach(synopsis)

/***************************************************************************/
/* TIE 'table'. */

extern xtie_table
xtie_find_table (xtie_phase phase, const char *name);

extern const char *
xtie_table_get_name (xtie_table table);

extern const char *
xtie_table_get_package (xtie_table table);

extern int
xtie_table_get_width (xtie_table table);

extern int
xtie_table_get_depth (xtie_table table);

extern unsigned int
xtie_table_get_value_width (xtie_table table, unsigned int n);

extern int
xtie_table_get_value (xtie_table table, unsigned int n);

extern const char *
xtie_table_get_value_string (xtie_table table, unsigned int n);

extern const char *
xtie_table_write (xtie_table table, xtie_writer xw);

extern xtie_table_iter
xtie_get_table_iter (xtie_phase phase);

extern xtie_table_iter
xtie_table_iter_step (xtie_table_iter iter);

extern xtie_table
xtie_table_iter_get (xtie_table_iter iter);

#define xtie_foreach_table(phase, tab) xtie_foreach(table, phase, tab)
#define end_xtie_foreach_table end_xtie_foreach(table)


/***************************************************************************/
/* TIE 'test'. */

extern xtie_test
xtie_find_test (xtie_phase phase, const char *name);

extern const char *
xtie_test_get_name (xtie_test test);

extern const char *
xtie_test_get_instruction (xtie_test test);

extern xtie_id_iter
xtie_test_get_input_iter (xtie_test test);

extern xtie_id_iter
xtie_test_get_output_iter (xtie_test test);

extern xtie_vector_iter
xtie_test_get_vector_iter (xtie_test test);

extern const char *
xtie_test_write (xtie_test test, xtie_writer xw);

extern xtie_test_iter
xtie_get_test_iter (xtie_phase phase);

extern xtie_test_iter
xtie_test_iter_step (xtie_test_iter iter);

extern xtie_test
xtie_test_iter_get (xtie_test_iter iter);

extern int
xtie_vector_get_count (xtie_vector vec);

extern const char *
xtie_vector_get_user_function (xtie_vector vec);

extern xtie_const_iter
xtie_vector_get_input_value_iter (xtie_vector vec);

extern xtie_const_iter
xtie_vector_get_output_value_iter (xtie_vector vec);

extern xtie_vector_iter
xtie_vector_iter_step (xtie_vector_iter iter);

extern xtie_vector
xtie_vector_iter_get (xtie_vector_iter iter);

#define xtie_foreach_test(phase, tst) xtie_foreach(test, phase, tst)
#define end_xtie_foreach_test end_xtie_foreach(test)

#define xtie_test_foreach_input(test, io) \
  xtie_foreach_internal(id, io, xtie_test_get_input_iter(test))
#define end_xtie_test_foreach_input end_xtie_foreach_internal(id)

#define xtie_test_foreach_output(test, io) \
  xtie_foreach_internal(id, io, xtie_test_get_output_iter(test))
#define end_xtie_test_foreach_output end_xtie_foreach_internal(id)

#define xtie_test_foreach_vector(test, vec) \
  xtie_foreach_internal(vector, vec, xtie_test_get_vector_iter(test))
#define end_xtie_test_foreach_vector end_xtie_foreach_internal(vec)

#define xtie_vector_foreach_input_value(vec, iov) \
  xtie_foreach_internal(const, iov, xtie_vector_get_input_value_iter(vec))
#define end_xtie_vector_foreach_input_value end_xtie_foreach_internal(const)

#define xtie_vector_foreach_output_value(vec, iov) \
  xtie_foreach_internal(const, iov, xtie_vector_get_output_value_iter(vec))
#define end_xtie_vector_foreach_output_value end_xtie_foreach_internal(const)


/***************************************************************************/
/* TIE 'tieport'. */

extern xtie_tieport
xtie_find_tieport (xtie_phase phase, const char *name);

extern const char *
xtie_tieport_get_name (xtie_tieport tieport);

extern const char *
xtie_tieport_get_package (xtie_tieport tieport);

extern int
xtie_tieport_get_width (xtie_tieport tieport);

extern xtie_dir
xtie_tieport_get_dir (xtie_tieport tieport);

extern int
xtie_tieport_get_stage (xtie_tieport tieport);

extern int
xtie_tieport_is_in (xtie_tieport tieport);

extern int
xtie_tieport_is_out (xtie_tieport tieport);

extern const char *
xtie_tieport_write (xtie_tieport tieport, xtie_writer xw);

extern xtie_tieport_iter
xtie_get_tieport_iter (xtie_phase phase);

extern xtie_tieport_iter
xtie_tieport_iter_step (xtie_tieport_iter iter);

extern xtie_tieport
xtie_tieport_iter_get (xtie_tieport_iter iter);

#define xtie_foreach_tieport(phase, q) xtie_foreach(tieport, phase, q)
#define end_xtie_foreach_tieport end_xtie_foreach(tieport)


/***************************************************************************/
/* TIE 'system_register'. */

extern xtie_sreg
xtie_find_sreg (xtie_phase phase, const char *name);

extern const char *
xtie_sreg_get_name (xtie_sreg sreg);

extern const char *
xtie_sreg_get_package (xtie_sreg sreg);

extern int
xtie_sreg_get_index (xtie_sreg sreg);

extern xtie_xml_item
xtie_sreg_get_expression (xtie_sreg sreg);

extern int
xtie_sreg_is_generated_name (xtie_sreg sreg);

extern int
xtie_sreg_is_generated_index (xtie_sreg sreg);

extern const char *
xtie_sreg_write (xtie_sreg sreg, xtie_writer xw);

extern xtie_sreg_iter
xtie_get_sreg_iter (xtie_phase phase);

extern xtie_sreg_iter
xtie_sreg_iter_step (xtie_sreg_iter iter);

extern xtie_sreg
xtie_sreg_iter_get (xtie_sreg_iter iter);

#define xtie_foreach_sreg(phase, q) xtie_foreach(sreg, phase, q)
#define end_xtie_foreach_sreg end_xtie_foreach(sreg)


/***************************************************************************/
/* TIE 'user_register'. */

extern xtie_ureg
xtie_find_ureg (xtie_phase phase, const char *name);

extern const char *
xtie_ureg_get_name (xtie_ureg ureg);

extern const char *
xtie_ureg_get_package (xtie_ureg ureg);

extern int
xtie_ureg_get_index (xtie_ureg ureg);

extern xtie_xml_item
xtie_ureg_get_expression (xtie_ureg ureg);

extern int
xtie_ureg_is_generated_name (xtie_ureg ureg);

extern int
xtie_ureg_is_generated_index (xtie_ureg ureg);

extern const char *
xtie_ureg_write (xtie_ureg ureg, xtie_writer xw);

extern xtie_ureg_iter
xtie_get_ureg_iter (xtie_phase phase);

extern xtie_ureg_iter
xtie_ureg_iter_step (xtie_ureg_iter iter);

extern xtie_ureg
xtie_ureg_iter_get (xtie_ureg_iter iter);

#define xtie_foreach_ureg(phase, q) xtie_foreach(ureg, phase, q)
#define end_xtie_foreach_ureg end_xtie_foreach(ureg)


#ifdef __cplusplus
#if 0
{ /* fix emacs autoindent */
#endif
}
#endif
#endif /* XTENSA_LIBTIE_H */
