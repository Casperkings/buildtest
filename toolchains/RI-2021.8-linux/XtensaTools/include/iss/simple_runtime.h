/* Interface for runtime accessible functions. */

/* Copyright (c) 2003, 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#ifndef SIMPLE_RUNTIME_H
#define SIMPLE_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

/* This structure is designed to allow easy access to the states
   from per-instruction semantics.  It includes tracing info because
   some tracing, like TIE print, needs to be performed inside the semantic
   for efficiency. */

typedef void (*tprint_fn)(void *, unsigned, unsigned, unsigned, ...);
typedef void (*tsim_mulpp_inplace)(unsigned *op_N, unsigned N_bits,
				   unsigned *op_M, unsigned M_bits,
				   unsigned *partial0, unsigned R_bits,
				   unsigned *partial1, int is_signed,
				   int negate);
typedef void (*aligned_load_fn)(void *,
				unsigned vaddress, unsigned size,
				unsigned ring, unsigned *dst /*out */);
typedef void (*aligned_store_check_fn)(void *,
				       unsigned vaddress, unsigned size,
				       /* unsigned disables, */
				       unsigned ring,
				       unsigned *kill_out);
typedef void (*aligned_store_fn)(void *,
				 unsigned vaddress, unsigned size,
				 const unsigned *src, 
				 const unsigned *disables,
				 unsigned ring,
				 unsigned *kill_out);
typedef void (*get_prid_fn)(void *, unsigned *prid);

struct simple_callbacks {
  tprint_fn tie_print;
  aligned_load_fn xtms_aligned_load;
  aligned_store_check_fn xtms_aligned_store_check;
  aligned_store_fn xtms_aligned_store;
  get_prid_fn xtms_get_prid;
  tsim_mulpp_inplace mulpp_inplace;
};

struct simple_states {
  unsigned **states;
  unsigned **regfiles;
  struct simple_callbacks callbacks;
  void *callback_data;  /* a TurboCore for simulation */
};

void tsim_arith_wide_mulpp_inplace(unsigned *op_N, unsigned N_bits,
				   unsigned *op_M, unsigned M_bits,
				   unsigned *partial0, unsigned R_bits,
				   unsigned *partial1, 
				   int is_signed,
				   int negate);

#ifdef __cplusplus
}
#endif


#endif /* SIMPLE_RUNTIME_H */
