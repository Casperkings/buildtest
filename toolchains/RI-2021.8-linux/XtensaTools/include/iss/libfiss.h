
/* Copyright (c) 2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.
*/

/* NOTE THIS FILE MUST BE COMPILABLE BY A C COMPILER AS WELL */

#ifndef LIBFISS_H
#define LIBFISS_H

#if defined __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

#define LIBFISS_VERSION 2

/* TODO: might need to remove this limitation since we can have upto 32/64
 * pipeline stages. One idea: instead of building a 2D array that's
 * stage_functions[opcode][MAX_POSSIBLE_PARTIAL_STAGES], where we don't know
 * either index at simulator-build time (but only at config build time), we
 * could index into it as a single dimensional array, and compute the second
 * dimension manually at sim time.
 *
 */

#define MAX_POSSIBLE_PARTIAL_STAGES 32

// TODO: Same as above: don't hardcode this? # of exceptions changes?
#define MAX_POSSIBLE_EXCEPTIONS 64


#define SEMANTIC_STAGES 7

typedef int (libfiss_dll_semstage_function_raw)(unsigned int * __restrict operands, struct simple_states *xstate__);
typedef libfiss_dll_semstage_function_raw *libfiss_dll_semstage_function;
typedef libfiss_dll_semstage_function libfiss_stage_fn_table[MAX_POSSIBLE_PARTIAL_STAGES];
typedef libfiss_dll_semstage_function libfiss_semantic_fn_table[SEMANTIC_STAGES];

typedef unsigned exceptions_idxs_t[MAX_POSSIBLE_EXCEPTIONS];

typedef struct {
  unsigned opcode_count;
  unsigned global_stage_bits;
  unsigned max_slot_operands;
  unsigned max_exception_operands;
} libfiss_config_metadata_t;

#define STATELOAD_SEMANTIC_FN_INDEX        0
#define REGLOAD_SEMANTIC_FN_INDEX          1
#define MEMLOAD_SEMANTIC_FN_INDEX          2
#define MEMSTORE_SEMANTIC_FN_INDEX         3
#define MEMSTORE_CHECK_SEMANTIC_FN_INDEX   4
#define WRITEBACK_SEMANTIC_FN_INDEX        5
#define OPCODE_COMPLETE_SEMANTIC_FN_INDEX  6


/* TODO: also, remove NativeCodeFn from op_sem's semantics if possible. Check
 * to see if any of NativeCodeFn's metadata is needed.
 */

#endif

