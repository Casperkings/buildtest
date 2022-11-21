/*
 * Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the.
 * copyrighted works and confidential proprietary information of Tensilica Inc..
 * They may not be modified, copied, reproduced, distributed, or disclosed to.
 * third parties in any manner, medium, or form, in whole or in part, without.
 * the prior written consent of Tensilica Inc..
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned chunk_t;
#define CHUNK_POW2_BYTES 2U
#define CHUNK_POW2_BITS (CHUNK_POW2_BYTES+3)
#define CHUNK_BITS (sizeof(chunk_t)<<3)
#define CHUNKS_IN_BITS(bits) (((bits)+CHUNK_BITS-1)>>CHUNK_POW2_BITS)
#define BIT_CHUNK(bits) ((bits) >> CHUNK_POW2_BITS)
#define CHUNK_BIT_MASK (CHUNK_BITS-1)
#define CHUNK_BIT_IDX(bits) ((bits) & CHUNK_BIT_MASK)
#define BITS_IN_CHUNKS(chunks) ((chunks) << CHUNK_POW2_BITS)

/* Vector */
typedef struct CVector_struct {
  void **v;
  unsigned size;
  unsigned max_size;
} CVector;

static CVector *
CVector_new(unsigned max_size) {
  CVector *v = (CVector *)malloc(sizeof(CVector));
  v->v = (void **)malloc(sizeof(void *) * max_size);
  v->max_size = max_size;
  v->size = 0;
  return v;
}

static void
CVector_delete(CVector *v) {
  free(v->v);
  free(v);
}

static void *
CVector_get(CVector *v, unsigned i) {
  if (i < v->size)
    return v->v[i];
  else
    return NULL;
}

static void
CVector_push_back(CVector *v, void *data) {
  if (v->size == v->max_size) {
    /* reallocate */
    void **tmp = v->v;
    v->v = (void **)malloc(sizeof(void *) * v->max_size * 2);
    memcpy(v->v, tmp, v->max_size * 4);
    v->max_size = v->max_size * 2;
    free(tmp);
  } 
  v->v[v->size] = data;
  v->size++;
}

/* END Vector */

/* ArithHelpers */
static void csa(chunk_t op_A, chunk_t op_B, chunk_t op_C,
		chunk_t *__restrict result_sum,
		chunk_t *__restrict result_carry) {
  *result_sum = op_A ^ op_B ^ op_C;
  *result_carry = (op_A & op_B) | (op_B & op_C) | (op_A & op_C);
}

#if 0
static void wide_csa(unsigned num_chunks,
		     const chunk_t *__restrict op_A,
		     const chunk_t *__restrict op_B,
		     const chunk_t *__restrict op_C,
		     chunk_t *__restrict result_sum,
		     chunk_t *__restrict result_carry) {
  unsigned i;
  for (i = 0; i < num_chunks; ++i) {
    csa(op_A[i], op_B[i], op_C[i], &result_sum[i], &result_carry[i]); 
  }
}

static void shift_left_inplace(unsigned num_chunks,
			       chunk_t *__restrict op_A) {
  int i;
  for (i = num_chunks-1; i > 0; --i) {
    unsigned val = (op_A[i-1] >> 31) | (op_A[i] << 1);
    op_A[i] = val;
    }
  op_A[0] <<= 1;
}
#endif

static void wide_shifted_csa(unsigned num_chunks,
			     const chunk_t *__restrict op_A,
			     const chunk_t *__restrict op_B,
			     const chunk_t *__restrict op_C,
			     chunk_t *__restrict result_sum,
			     chunk_t *__restrict result_carry) {
  
  int i = num_chunks-1;
  chunk_t carry;
  chunk_t last_carry;
  csa(op_A[i], op_B[i], op_C[i], &result_sum[i], &carry);
  last_carry = carry << 1;
  --i;
  for (; i >= 0; --i) {
      csa(op_A[i], op_B[i], op_C[i], &result_sum[i], &carry);
      result_carry[i+1] = last_carry | (carry>>31);
      last_carry = carry << 1;
  }
  result_carry[0] = last_carry;
}      


/* END ArithHelpers */


/* MulppMoveAction */
typedef struct MulppMoveAction_struct {
  // move mask of bits from input[a] to output[0,1]
  unsigned _from_row;
  unsigned _from_chunk;
  unsigned _mask;
  unsigned _to_row;
} MulppMoveAction;

static MulppMoveAction *
MulppMoveAction_new(unsigned from_row, 
		    unsigned from_chunk,
		    unsigned mask, 
		    unsigned to_row) {
  MulppMoveAction *action = (MulppMoveAction *)malloc(sizeof(MulppMoveAction));
  action->_from_row = from_row;
  action->_from_chunk = from_chunk;
  action->_mask = mask;
  action->_to_row = to_row;
  return action;
}

static void
MulppMoveAction_delete(MulppMoveAction *action) {
  free(action);
}

static void 
MulppMoveAction_apply(const MulppMoveAction *action, 
		      chunk_t ** const src, chunk_t **dst) {
  unsigned val = src[action->_from_row][action->_from_chunk];
  dst[action->_to_row][action->_from_chunk] |= (val & action->_mask);
}

static int
MulppMoveAction_same_chunk_internal(const MulppMoveAction *action, 
				    unsigned from_row, unsigned from_chunk,
				    unsigned to_row) {
  return (from_row == action->_from_row) && 
    (from_chunk == action->_from_chunk) &&
    (to_row == action->_to_row);
}

static int 
MulppMoveAction_same_chunk(const MulppMoveAction *action, 
			    const MulppMoveAction *a) {
  return MulppMoveAction_same_chunk_internal(action, a->_from_row, 
					     a->_from_chunk, a->_to_row);
}

static void 
MulppMoveAction_merge_mask(MulppMoveAction *action, const MulppMoveAction *a) {
  //    assert(same_chunk(a));
    action->_mask |= a->_mask;
  } 


/* END MulppMoveAction */


/* MulppMoveSet */
typedef struct MulppMoveSet_struct {
  CVector *_move_results;
} MulppMoveSet;

static MulppMoveSet *
MulppMoveSet_new() {
  MulppMoveSet *moveset = (MulppMoveSet *)malloc(sizeof(MulppMoveSet));
  moveset->_move_results = CVector_new(16);
  return moveset;
}

static void
MulppMoveSet_delete(MulppMoveSet *moveset) {
  unsigned i;
  for (i = 0; i < moveset->_move_results->size; i++) {
    MulppMoveAction *action = (MulppMoveAction *)CVector_get(moveset->_move_results, i);
    MulppMoveAction_delete(action);
  }
  CVector_delete(moveset->_move_results);
  free(moveset);
}

static void 
MulppMoveSet_apply(const MulppMoveSet *moveset, 
		   chunk_t ** const src, chunk_t **dst) {
  unsigned move_count = moveset->_move_results->size;
  unsigned i;
  for (i = 0; i < move_count; ++i) {
    MulppMoveAction *action = (MulppMoveAction *)CVector_get(moveset->_move_results, i);
    MulppMoveAction_apply(action, src, dst);
  }
}

static void 
MulppMoveSet_add_move_action(MulppMoveSet *moveset, MulppMoveAction *a) {
  unsigned i;
  for (i = 0; i < moveset->_move_results->size; ++i) {
    MulppMoveAction *action = (MulppMoveAction *)CVector_get(moveset->_move_results, i);
    if (MulppMoveAction_same_chunk(action, a)) {
      MulppMoveAction_merge_mask(action, a);
      MulppMoveAction_delete(a);
      return;
    }
  }
  CVector_push_back(moveset->_move_results, a);
}


static void 
MulppMoveSet_add_move_bit(MulppMoveSet *moveset, unsigned from_row, 
			  unsigned from_bit, 
			  unsigned to_row) {
    unsigned from_chunk = BIT_CHUNK(from_bit);
    unsigned from_bit_mask = 1U << CHUNK_BIT_IDX(from_bit);
    MulppMoveAction *ma = MulppMoveAction_new(from_row, from_chunk, from_bit_mask, to_row);
    MulppMoveSet_add_move_action(moveset, ma);
}

/* END MulppMoveSet */


/* MulppCsaAction */
typedef struct MulppCsaAction_struct {
  unsigned _from_row;
  unsigned _num_chunks;
} MulppCsaAction;

static MulppCsaAction *
MulppCsaAction_new(unsigned from_row, unsigned num_chunks) {
  MulppCsaAction *action = (MulppCsaAction *)malloc(sizeof(MulppCsaAction));
  action->_from_row = from_row;
  action->_num_chunks = num_chunks;
  return action;
}

static void
MulppCsaAction_delete(MulppCsaAction *action) {
  free(action);
}

static void 
MulppCsaAction_apply(const MulppCsaAction *action, chunk_t ** const src,
		     chunk_t **sum_carry) { // sum is 0, carry is 1.
  wide_shifted_csa(action->_num_chunks, 
		   src[action->_from_row],
		   src[action->_from_row+1],
		   src[action->_from_row+2],
		   sum_carry[0], sum_carry[1]);
}

/* END MulppCsaAction */


/* MulppCsaReduction */
typedef struct MulppCsaReduction_struct {
  MulppCsaAction *_csa;
  MulppMoveSet *_move_set;  
} MulppCsaReduction;

static MulppCsaReduction *
MulppCsaReduction_new(MulppCsaAction *csa) {
  MulppCsaReduction *reduction = (MulppCsaReduction *)malloc(sizeof(MulppCsaReduction));
  reduction->_csa = MulppCsaAction_new(csa->_from_row, csa->_num_chunks);
  reduction->_move_set = MulppMoveSet_new();
  return reduction;
}

static void
MulppCsaReduction_delete(MulppCsaReduction *r) {
  MulppCsaAction_delete(r->_csa);
  MulppMoveSet_delete(r->_move_set);
  free(r);
}

static void
MulppCsaReduction_apply(MulppCsaReduction *r, chunk_t ** const src, 
			chunk_t **dst,
			chunk_t **sum_carry) {
  MulppCsaAction_apply(r->_csa, src, sum_carry);
  MulppMoveSet_apply(r->_move_set, sum_carry, dst);
}

static void 
MulppCsaReduction_add_move_bit(MulppCsaReduction *r, unsigned from_row, 
			       unsigned from_bit,
			       unsigned to_row) {
  MulppMoveSet_add_move_bit(r->_move_set, from_row, from_bit, to_row);
}

/* END MulppCsaReduction */

/* MulppReduction */
typedef struct MulppReduction_struct {
  MulppMoveSet *_orphans;
  CVector *_csa_reductions;
  unsigned _max_row;
  unsigned _max_row_bytes;
} MulppReduction;

static MulppReduction *
MulppReduction_new() {
  MulppReduction *reduction = (MulppReduction *)malloc(sizeof(MulppReduction));
  reduction->_max_row = 0;
  reduction->_orphans = MulppMoveSet_new();
  reduction->_csa_reductions = CVector_new(16);
  return reduction;
}

static void
MulppReduction_delete(MulppReduction *r) {
  unsigned i;
  for (i = 0; i < r->_csa_reductions->size; i++) {
    MulppCsaReduction *csar = (MulppCsaReduction *)CVector_get(r->_csa_reductions, i);
    MulppCsaReduction_delete(csar);
  }
  MulppMoveSet_delete(r->_orphans);
  CVector_delete(r->_csa_reductions);
  free(r);
}

static void 
MulppReduction_add_orphan_bit(MulppReduction *reduction, unsigned from_row, 
			      unsigned from_bit, unsigned to_row) {
  MulppMoveSet_add_move_bit(reduction->_orphans, from_row, from_bit, to_row);
}

static void
MulppReduction_add_csa_reduction(MulppReduction *reduction, 
				 MulppCsaReduction *r) {
  CVector_push_back(reduction->_csa_reductions, r);
}

static void 
MulppReduction_apply(const MulppReduction *r, chunk_t ** const src, 
		     chunk_t **dst, chunk_t **sum_carry) {
  unsigned csa_reductions_count;
  unsigned i;
  MulppMoveSet_apply(r->_orphans, src, dst);
  csa_reductions_count = r->_csa_reductions->size;
  for (i = 0; i < csa_reductions_count; ++i) {
    MulppCsaReduction *csar = (MulppCsaReduction *)CVector_get(r->_csa_reductions, i);
    MulppCsaReduction_apply(csar, src, dst, sum_carry);
  }
}

static void 
MulppReduction_set_max_row(MulppReduction *r, unsigned max_row,
			   unsigned num_chunks) {
  r->_max_row = max_row;
  r->_max_row_bytes = max_row*num_chunks*sizeof(chunk_t);
}

/* END MulppReduction */


/* MulppScratch */
typedef struct MulppScratch_struct {
  chunk_t **_work0;
  chunk_t **_work1;
  chunk_t **_sum_carry;

  chunk_t *_data; // this is an array of unsigned that should be cleared
                  // before use.
  unsigned _data_chunks;
  size_t _data_bytes;
  
  struct MulppScratch_struct *_next; // these form a single linked list
} MulppScratch;


static void  
MulppScratch_clear(MulppScratch *scratch) {
  memset(scratch->_data, 0, scratch->_data_bytes);
}

static MulppScratch *
MulppScratch_new(unsigned N, unsigned M, unsigned R) {
  unsigned M_bits = M;
  unsigned R_bits = R;
  unsigned initial_partials = (M_bits>>1)+1;
  // need space for the 3 used to compute csa
  unsigned num_partials = initial_partials+4; 
  unsigned partial_bits_pow2 = 1;
  unsigned R_chunks;
  unsigned i;

  MulppScratch *scratch = (MulppScratch *)malloc(sizeof(MulppScratch));
  scratch->_next = NULL;

  while ((1U<<partial_bits_pow2) < num_partials) 
    { partial_bits_pow2++; }
  num_partials = 1U<<partial_bits_pow2;
  
  R_chunks = CHUNKS_IN_BITS(R_bits);

  // This is all done here so that alloca can be used.

  //  unsigned work_chunks = R_chunks * num_partials;
  //  unsigned tmp_chunks = R_chunks;

  scratch->_work0 = (chunk_t **) malloc(sizeof (chunk_t *) * num_partials);
  scratch->_work1 = (chunk_t **) malloc(sizeof (chunk_t *) * num_partials);
  scratch->_sum_carry = (chunk_t **) malloc(sizeof (chunk_t *) * 2);
  
  scratch->_data_chunks = num_partials*2 + num_partials*R_chunks*2;
  scratch->_data_bytes = scratch->_data_chunks*sizeof(chunk_t);
  scratch->_data = (chunk_t *) malloc(sizeof (chunk_t) * scratch->_data_chunks);
  // clear the data
  MulppScratch_clear(scratch);
  
  for (i = 0; i < num_partials; ++i) {
    scratch->_work0[i] = &scratch->_data[0 + R_chunks*i];
    scratch->_work1[i] = &scratch->_data[R_chunks*num_partials + R_chunks*i];
  }
  scratch->_sum_carry[0] = &scratch->_data[2*R_chunks*num_partials];
  scratch->_sum_carry[1] = &scratch->_data[2*R_chunks*num_partials + R_chunks];
  return scratch;
}

static void 
MulppScratch_delete(MulppScratch *scratch) {
  free(scratch->_work0);
  free(scratch->_work1);
  free(scratch->_sum_carry);
  free(scratch->_data);
  free(scratch);
}
 
static MulppScratch *
MulppScratch_replace_next(MulppScratch *scratch, MulppScratch *n) {
  MulppScratch *t = scratch->_next;
  scratch->_next = n;
  return t;
}

/* END MulppScratch */

/* MulppTable */
typedef struct MulppTable_struct {
  unsigned _N_bits; // _N is a macro on solaris
  unsigned _M;
  unsigned _R;
  unsigned _num_chunks;
  CVector *_reductions;

  // we keep a list of these
  MulppScratch *_unused_scratch;
  
} MulppTable;

static MulppTable *
MulppTable_new(unsigned N_bits, unsigned M, unsigned R) {
  MulppTable *table = (MulppTable *)malloc(sizeof(MulppTable));
  table->_N_bits = N_bits;
  table->_M = M;
  table->_R = R;
  table->_num_chunks = CHUNKS_IN_BITS(R);
  table->_reductions = CVector_new(16);
  table->_unused_scratch = NULL;
  return table;
}

static void
MulppTable_delete(MulppTable *table) {
  unsigned i;
  for (i = 0; i < table->_reductions->size; i++) {
    MulppReduction *reduction = (MulppReduction *)CVector_get(table->_reductions, i);
    MulppReduction_delete(reduction);
  }
  CVector_delete(table->_reductions);
  if (table->_unused_scratch) {
    MulppScratch_delete(table->_unused_scratch);
  }
  free(table);
}

static MulppScratch *
MulppTable_pop_scratch(MulppTable *table) {
  MulppScratch *t = table->_unused_scratch;
  MulppScratch *next = 
    MulppScratch_replace_next(table->_unused_scratch, NULL);
  table->_unused_scratch = next;
  return t;
}

static void 
MulppTable_push_scratch(MulppTable *table, MulppScratch *scratch) {
  MulppScratch_replace_next(scratch, table->_unused_scratch);
  table->_unused_scratch = scratch;
}

static MulppScratch *
MulppTable_alloc_scratch(MulppTable *table) {
  if (table->_unused_scratch) {
    MulppScratch *next = MulppTable_pop_scratch(table);
    MulppScratch_clear(next);
    return next;
  }
  return MulppScratch_new(table->_N_bits, table->_M, table->_R);
}

static void 
MulppTable_free_scratch(MulppTable *table, MulppScratch *scratch) {
  MulppTable_push_scratch(table, scratch);
}

static void 
MulppTable_add_reduction(MulppTable *table, MulppReduction *r) {
  CVector_push_back(table->_reductions, r);
}

static chunk_t **
MulppTable_compute_partials(MulppTable *table, 
			    chunk_t **src,
			    chunk_t **dst,
			    chunk_t **sum_carry) {
  // src should have valid bits,
  // dst should be zeros.
  // src and dst should be allocated contiguously so they
  // a single memcopy can clear them.
  // sum_carry can have garbage
  chunk_t **current = src;
  chunk_t **next = dst;
  chunk_t **tmp;
  unsigned reductions_size = table->_reductions->size;
  unsigned i;
  for (i = 0; i < reductions_size; ++i) {
    const MulppReduction *reduction = (const MulppReduction *)CVector_get(table->_reductions, i);
    MulppReduction_apply(reduction, current, next, sum_carry);
    tmp = current; current = next; next = tmp;
    memset(next[0], 0, reduction->_max_row * table->_num_chunks *
	   sizeof(chunk_t));
  }
  return current;
}

static int
MulppTable_matches(const MulppTable *table, unsigned N_bits, 
		   unsigned M, unsigned R) {
  return (N_bits == table->_N_bits) && (M == table->_M) && (R == table->_R);
}


/* END MulppTable */

/* MulppHelper */
typedef struct MulppHelper_struct {
  CVector *_tables;
} MulppHelper;

static MulppHelper *
MulppHelper_new() {
  MulppHelper *helper = (MulppHelper *)malloc(sizeof(MulppHelper));
  helper->_tables = CVector_new(16);
  return helper;
}

static void
MulppHelper_delete(MulppHelper *helper) {
  unsigned i;
  for (i = 0; i < helper->_tables->size; i++) {
    MulppTable *table = (MulppTable *)CVector_get(helper->_tables, i);
    MulppTable_delete(table);
  }
  CVector_delete(helper->_tables);
  free(helper);
}

static int is_swap(unsigned N, unsigned M) {
  return (N < M);
}

static int 
MulppHelper_find_table_index(MulppHelper *helper, unsigned N, 
			     unsigned M, unsigned R) {
  unsigned i;
  if (is_swap(N, M)) return -1;
  for (i = 0; i < helper->_tables->size; ++i) {
    MulppTable *t = (MulppTable *)CVector_get(helper->_tables, i);
    if (MulppTable_matches(t, N, M, R)) {
      return i;
    }
  }
  return -1;
}

static MulppTable *
MulppHelper_get_table(MulppHelper *helper, unsigned N, 
		      unsigned M, unsigned R) {
  int i = MulppHelper_find_table_index(helper, N, M, R);
  MulppTable *table;
  if (i == -1) return NULL;
  table = (MulppTable *)CVector_get(helper->_tables, i);
  return table;
}

static MulppTable *
MulppHelper_build_table(MulppHelper *helper, unsigned N, 
			unsigned M, unsigned R) {

  MulppTable *t = MulppTable_new(N, M, R);

  /* This is very tricky because we have to pay a lot of attention to
     the bits.  */
  /* Booth encoding. Read 3 bits of M, 2 at a time.  at bit a
     If 000, nothing
     If 001  +( N << a*2)
     If 010  +( N << a*2)
     If 011  +(2N << a*2)
     If 100  -(2N << a*2)
     If 101  - (N << a*2)
     If 110  - (N << a*2)
     If 111  nothing
     
     When subtracting, add the inverted N and set the carry to 1.
     i.e. 
          -(2N << a*2) ==  ((~2N) << a*2) + 1<< a*2
          -( N << a*2) ==  ((~ N) << a*2) + 1<< a*2
     After booth encoding, we have bits/2+1 terms
     and bits/2+1 carry ins
     
     Initially these are set up
     sign, sign, p_0_n+1... p_0_6 p_0_5 p_0_4 p_0_3 p_0_2 p_0_1 p_0_0
     sign, sign, p_1_n+1... p_1_2 p_1_3 p_1_2 p_1_1 p_1_0 ..... cin_0
     sign, sign, p_1_n+1... p_2_0 p_2_1 p_2_0 ..... cin_1 ..... .....
     sign, sign, p_1_n+1... p_3_0 ..... cin_2 ..... ..... ..... .....

     The general idea is to compute the sum for this column and the
     carry for the next bit and put them in the right column. Repeat
     until finished.  However, only compute the sums if we have
     3 bits or we have 2 bits and a previous column has 3 bits.
     At the end of this round, put the newly computed bits after
     the ones remaining.  Do this until there are not more than
     2 bits in each column.

     Note that when the previous column did not generate a carry,
     (stack size == 1 or 2 and no previous column had 3)
     we can not propagate the carry.
     When the size of all stacks is 2 or 1, we are done.  return the results
     as two partial products
  */
  
  /*
    We need two rows for tmp sum and carry
    Max rows for multiplying M bits: (m/2)+1+1 initially.  We could keep
    these in a circular buffer.  otherwise we need ~3x this size
    We will use a power of 2 bits in a circular buffer to make the wrap-around
    function simple.
  */
  
  unsigned M_bits, R_bits, initial_partials, num_partials, partial_bits_pow2;
  unsigned R_chunks, *work0_column_bits, *work1_column_bits, i, csa;
  unsigned *current_column_bits, current_max_bits, *next_column_bits;

  M_bits = M;
  R_bits = R;

  /* get the next power of 2 */
  initial_partials = (M_bits>>1)+1;
  // need space for the 3 used to compute csa
  num_partials = initial_partials+4; 
  partial_bits_pow2 = 1;
  while ((1U<<partial_bits_pow2) < num_partials)
    { partial_bits_pow2++; }
  num_partials = 1U<<partial_bits_pow2;
  
  R_chunks = CHUNKS_IN_BITS(R_bits);

  work0_column_bits = (unsigned *)malloc(sizeof(unsigned) * R_bits);
  work1_column_bits = (unsigned *)malloc(sizeof(unsigned) * R_bits);
  for (i = 0; i < R_bits; ++i) {
    work0_column_bits[i] = 0;
    work1_column_bits[i] = 0;
  }
    
  /* This should be the max number of csas.  We pad it */

  /* set the current ones */
  current_column_bits = work0_column_bits;
  current_max_bits = 0;

  next_column_bits = work1_column_bits;
  
  // compute the number of bits in each column.

  for (i = 0; i < R_bits; ++i) {
    unsigned num_bits = 0;
    if (i > ((initial_partials-1)<<1)) {
      num_bits = initial_partials;
    } else {
      /* at bit 0, we have 1 bit + 1 carry,
	 at bit 1, we have 1 bit
	 at bit 2, we have 2 bits + 1 carry
      */
      num_bits = ( i >> 1 )+1 + ((i+1)& 0x1);
    }
    if (current_max_bits < num_bits) {
      current_max_bits = num_bits;
    }
    current_column_bits[i] = num_bits;
  }
  
  while (current_max_bits > 2) {
    MulppReduction *reduction = MulppReduction_new();
    /* move bits that do not compute carry, sum to the alternate */

    unsigned next_max_bits = 0;
    
    unsigned num_csas = 0;
    for (i = 0; i < R_bits; ++i) {
      unsigned nrows = current_column_bits[i];
      unsigned groups = nrows/3;
      unsigned rems;
      if (groups > num_csas) {
	num_csas = groups;
      }
      rems = nrows % 3;
      next_column_bits[i] = 0;
      if (rems == 1 || (rems == 2 && num_csas == groups)) {
	unsigned base = groups*3;
	unsigned j;
	for (j = 0; j < rems; ++j) {
	  MulppReduction_add_orphan_bit(reduction, base+j, i, j);
	}
	current_column_bits[i] -= rems;
	next_column_bits[i] = rems;
	if (next_column_bits[i] > next_max_bits) {
	  next_max_bits = next_column_bits[i];
	}
      }
    }
    
    for (csa = 0; csa < num_csas; ++csa) {
      unsigned base = csa *3;
      MulppCsaAction *csa_action = MulppCsaAction_new(base, R_chunks);
      MulppCsaReduction *csa_reduction = MulppCsaReduction_new(csa_action);
      MulppCsaAction_delete(csa_action);
      unsigned j;
      // now compute the moves from the sum/carry to the result.
      
      // Note the carry bits have already been shifted left by one bit.
      for (j = 0; j < R_bits; ++j) {
	/* TBD: stop after we reach the first set of 3. */
	/* We go in reverse order.  Do not add the carry bit if the
	   previous column has 0 bits, Do not add the sum if this
	   column has 0 bits */
	unsigned i = R_bits-1-j;
	unsigned row = current_column_bits[i];
	int this_has_sum = (row > base);
	int prev_has_carry = (i != 0 && (current_column_bits[i-1] > base));
	
	if (this_has_sum) {
	  unsigned sum_row = 0;
	  unsigned next_row = next_column_bits[i];
	  MulppCsaReduction_add_move_bit(csa_reduction, sum_row, i, next_row);
	  next_column_bits[i]++;
	}
	if (prev_has_carry) {
	  unsigned carry_row = 1;
	  unsigned next_row = next_column_bits[i];
	  MulppCsaReduction_add_move_bit(csa_reduction, carry_row, i, next_row);
	  next_column_bits[i]++;
	}
	if (next_column_bits[i] > next_max_bits) {
	  next_max_bits = next_column_bits[i];
	}
      }
      MulppReduction_add_csa_reduction(reduction, csa_reduction);
    }
    if (next_max_bits > 2) {
      // number of rows to clear to be ready for the next
      // round
      MulppReduction_set_max_row(reduction, (num_csas+1)*3, R_chunks);
    }
 
    /* swap current_ and next_ */
    {
      unsigned *tmp = current_column_bits;
      current_column_bits = next_column_bits;
      next_column_bits = tmp;
    }
    {
      unsigned tmp = current_max_bits;
      current_max_bits = next_max_bits;
      next_max_bits = tmp;
    }

    // add this one to the data structure
    MulppTable_add_reduction(t, reduction);
  }
  
  free(work0_column_bits);
  free(work1_column_bits);

  return t;
}


static MulppTable *
MulppHelper_retrieve_table(MulppHelper *helper, unsigned N, 
			   unsigned M, unsigned R) {
  MulppTable *t;
  if (is_swap(N, M)) return NULL;
  t = MulppHelper_get_table(helper, N, M, R);
  if (t) return t;
  t = MulppHelper_build_table(helper, N, M, R);
  CVector_push_back(helper->_tables, t);
  return t;
}

/* End MulppTable */

static unsigned 
arith_get_bitval(const chunk_t *__restrict op_N,
		 unsigned bit) {
  return ((op_N[BIT_CHUNK(bit)] >> CHUNK_BIT_IDX(bit)) & 0x1);
}

static void 
arith_set_bitval(chunk_t *__restrict op_N,
		 unsigned bit,
		 unsigned bitval) {
  chunk_t value = op_N[BIT_CHUNK(bit)];
  if (bitval) {
    value |= (1 << CHUNK_BIT_IDX(bit));
  } else {
    value &= ~(1 << CHUNK_BIT_IDX(bit));
  }
  op_N[BIT_CHUNK(bit)] = value;
}

/* 0xfffffff  zero-extended from bit 4 => 0x0000000f */
static void 
arith_wide_extend_inplace(chunk_t N_chunks,
			  unsigned extend_from_bit,
			  unsigned bitval,
			  chunk_t *__restrict op_N)
{
  //  assert(extend_from_bit <= BITS_IN_CHUNKS(N_chunks));
  unsigned mask, bit_idx, chunk;
  if (extend_from_bit == BITS_IN_CHUNKS(N_chunks))
    return;
  mask = bitval ? 0xffffffff : 0;
  bit_idx = CHUNK_BIT_IDX(extend_from_bit+1);
  chunk = BIT_CHUNK(extend_from_bit+1);
  if (bit_idx != 0) {
    /* first one needs to insert the sign or zero */
    unsigned high_mask = 0xffffffff << bit_idx;
    if (bitval) {
      op_N[chunk] |= high_mask;
    } else {
      op_N[chunk] &= ~high_mask;
    }
    chunk++;
  }
  
  for (; chunk < N_chunks; ++chunk) {
    op_N[chunk] = mask;
  }
}

/* copy full chunks */
static void 
arith_wide_copy(chunk_t *__restrict dest,
		const chunk_t *__restrict src,
		unsigned N_bits)
{
  unsigned num_chunks = CHUNKS_IN_BITS(N_bits);
  unsigned chunk;
  for (chunk = 0; chunk < num_chunks; ++chunk) {
    dest[chunk] = src[chunk];
  }
}

/* Words are ordered little endian.
   Within a word, use the host endian */
/* carry_in must be 0 or 1 */
/* 0x0000ffff sign extended from bit 15 is 0xffffffff */
static void 
arith_insert_shifted_extended_bits(chunk_t *__restrict result,
				   unsigned R_bits,
				   unsigned start_bit,
				   const chunk_t *__restrict op_N,
				   unsigned N_bits,
				   unsigned is_signed,
				   unsigned negate) {
  
  /* start at the first chunk */
  unsigned N_chunks = CHUNKS_IN_BITS(N_bits);
  unsigned start_idx = CHUNK_BIT_IDX(start_bit);
  unsigned start_R_chunk = BIT_CHUNK(start_bit);
  unsigned R_chunks = CHUNKS_IN_BITS(R_bits);
  unsigned chunk;
  unsigned N_sign = is_signed ? arith_get_bitval(op_N, N_bits-1) : 0;
  if (negate) N_sign = (1-N_sign);
 
  if (start_idx == 0) {
    for (chunk = 0; 
	 chunk < N_chunks && (start_R_chunk + chunk < R_chunks);
	 ++chunk) {
      unsigned value = op_N[chunk];
      if (negate) value = ~value;
      result[start_R_chunk+chunk] = value;
    }
  } else {
    for (chunk = 0; chunk < N_chunks; ++chunk) {
      unsigned to_insert = op_N[chunk];
      chunk_t value;
      if (negate) to_insert = ~to_insert;
      
      /* first half goes into */
      if (start_R_chunk + chunk >= R_chunks) 
	break;
      value = result[start_R_chunk+chunk];
      /* TBD: this will overwrite bits above this value */
      value = value & ((1U<<start_idx)-1);
      value = value | (to_insert<<start_idx);
      result[start_R_chunk+chunk] = value;

      /* second half, just insert */
      if ((start_R_chunk + chunk + 1) >= R_chunks) 
	break;
      /* TBD: this should not just insert. It should check for
	 The end */
      result[start_R_chunk+chunk+1] = (to_insert >>(32-start_idx));
    }
  }
  /* Finally, sign extend in place */
  if ((start_bit + N_bits) < BITS_IN_CHUNKS(R_chunks)) {
    arith_wide_extend_inplace(R_chunks, start_bit + N_bits-1,
			      N_sign, result);
  }

  if (CHUNK_BIT_IDX(R_bits) != 0 && N_sign != 0) {
    /* Set the end to zero */
    arith_wide_extend_inplace(R_chunks, R_bits-1, 0, result);
  }
}    

static MulppHelper * get_mulpp_helper();
static void set_mulpp_helper(MulppHelper *helper);

static signed char bootharray[8] = { 0, 1, 1, 2, -2, -1, -1, 0 };


static void 
tcalc_arith_wide_mulpp_inplace(unsigned *op_N, unsigned N_bits,
			     unsigned *op_M, unsigned M_bits,
			     unsigned *partial0, unsigned R_bits,
			     unsigned *partial1, 
			     int is_signed,
			     int negate) {
  MulppTable *mulpp_table;
  unsigned initial_partials, M_sign, i;
  chunk_t **work0, **work1, **tmp_sum, **current_work, **next_work, **result;
  MulppScratch *mscratch;

  if (N_bits < M_bits) {
    /* do booth on smaller operand */
    tcalc_arith_wide_mulpp_inplace(op_M, M_bits, op_N, N_bits,
				   partial0, R_bits, partial1,
				   is_signed, negate);
    return;
  }
  
  if (get_mulpp_helper() == NULL) {
    set_mulpp_helper(MulppHelper_new());
  }
  
  mulpp_table =
    MulppHelper_retrieve_table(get_mulpp_helper(), N_bits, M_bits, R_bits);

  /* This is very tricky because we have to pay a lot of attention to
     the bits.  */
  /* Booth encoding. Read 3 bits of M, 2 at a time.  at bit a
     If 000, nothing
     If 001  +( N << a*2)
     If 010  +( N << a*2)
     If 011  +(2N << a*2)
     If 100  -(2N << a*2)
     If 101  - (N << a*2)
     If 110  - (N << a*2)
     If 111  nothing
     
     When subtracting, add the inverted N and set the carry to 1.
     i.e. 
          -(2N << a*2) ==  ((~2N) << a*2) + 1<< a*2
          -( N << a*2) ==  ((~ N) << a*2) + 1<< a*2
     After booth encoding, we have bits/2+1 terms
     and bits/2+1 carry ins
     
     Initially these are set up
     sign, sign, p_0_n+1... p_0_6 p_0_5 p_0_4 p_0_3 p_0_2 p_0_1 p_0_0
     sign, sign, p_1_n+1... p_1_2 p_1_3 p_1_2 p_1_1 p_1_0 ..... cin_0
     sign, sign, p_1_n+1... p_2_0 p_2_1 p_2_0 ..... cin_1 ..... .....
     sign, sign, p_1_n+1... p_3_0 ..... cin_2 ..... ..... ..... .....

     The general idea is to compute the sum for this column and the
     carry for the next bit and put them in the right column. Repeat
     until finished.  However, only compute the sums if we have
     3 bits or we have 2 bits and a previous column has 3 bits.
     At the end of this round, put the newly computed bits after
     the ones remaining.  Do this until there are not more than
     2 bits in each column.

     Note that when the previous column did not generate a carry,
     (stack size == 1 or 2 and no previous column had 3)
     we can not propagate the carry.
     When the size of all stacks is 2 or 1, we are done.  return the results
     as two partial products
  */
  
  /*
    We need two rows for tmp sum and carry
    Max rows for multiplying M bits: (m/2)+1+1 initially.  We could keep
    these in a circular buffer.  otherwise we need ~3x this size
    We will use a power of 2 bits in a circular buffer to make the wrap-around
    function simple.
  */
  
  /* get the next power of 2 */
  initial_partials = (M_bits>>1)+1;

  mscratch = MulppTable_alloc_scratch(mulpp_table);
  work0 = mscratch->_work0;
  work1 = mscratch->_work1;
  tmp_sum = mscratch->_sum_carry;

    
  /* This should be the max number of csas.  We pad it */

  /* set the current ones */
  current_work = work0;

  next_work = work1;

  /* First generate the partial products and insert them
     sign extended into the partials array.
  */
  M_sign = is_signed ? arith_get_bitval(op_M, M_bits-1) : 0;
  for (i = 0; i < initial_partials; ++i) {
    /* booth encode is 0, 1, 1, 2, -2, -1, -1, 0 */
    unsigned booth_bit2 = (((i<<1)+1) < M_bits) ? 
      arith_get_bitval(op_M, (i<<1)+1) : M_sign;
    unsigned booth_bit1 = (((i<<1)) < M_bits) ? 
      arith_get_bitval(op_M, (i<<1)) : M_sign;
    unsigned booth_bit0 = (i == 0) ? 0 : 
      arith_get_bitval(op_M, (i<<1)-1);
    unsigned booth_bits = (booth_bit2<<2) | (booth_bit1<<1) | booth_bit0;
    signed char booth_val = bootharray[booth_bits];
    unsigned carry = 0;
  
    if (negate) {
      booth_val = -booth_val;
    }
    switch(booth_val) {
    case 1:
      arith_insert_shifted_extended_bits(current_work[i], R_bits,
					 i<<1, 
					 op_N, N_bits, is_signed, 0);
      break;
    case 2:
      arith_insert_shifted_extended_bits(current_work[i], R_bits,
					 (i<<1) + 1, 
					 op_N, N_bits, is_signed, 0);
      break;
    case -1:
      arith_insert_shifted_extended_bits(current_work[i], R_bits,
					 (i<<1), 
					 op_N, N_bits, is_signed, 1);
      carry = 1;
      break;
    case -2:
      arith_insert_shifted_extended_bits(current_work[i], R_bits,
					 (i<<1)+1, 
					 op_N, N_bits, is_signed, 1);
      /* The bottom bit is also 1. */
      arith_set_bitval(current_work[i], (i<<1), 1);
      carry = 1;
      break;
    case 0:
    default:
      /* No work, or carry */
      break;
    }
    
    if (carry) {
      /* Because this carry is put into the next partial product, the
	 insert_shifted_extended_bits must insert, not copy bits */
      arith_set_bitval(current_work[i+1], (i<<1), carry);
    }
  }

  // next compute the partials
  result =
    MulppTable_compute_partials(mulpp_table, current_work, next_work, tmp_sum);
  arith_wide_copy(partial0, result[1], R_bits);
  arith_wide_copy(partial1, result[0], R_bits);

  MulppTable_free_scratch(mulpp_table, mscratch);

}
  
