/* Interface for dense VM state. */

/* Copyright (c) 2003, 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */
#ifndef SIMPLE_VM_STATE_H
#define SIMPLE_VM_STATE_H

/* struct simple_vm_data MUST be defined before compiling this.
   The runtime compile will use the actual state.  The static compile
   will have an empty structure.  This just makes it easier for
   us to generate code that compiles directly.  */

#define XTENSA_VM_ARCH_REG 16

#define TSIM_RCODE_OK 0
#define TSIM_RCODE_SIMCALL 2
#define TSIM_RCODE_BRANCH 1
#define TSIM_RCODE_WOVER4 3
#define TSIM_RCODE_WOVER8 4
#define TSIM_RCODE_WOVER12 5
#define TSIM_RCODE_TIMER 6
#define TSIM_RCODE_ADDRESSING 7
#define TSIM_RCODE_MEMSPLIT 8
#define TSIM_RCODE_STORE_EXC 9
#define TSIM_RCODE_LOAD_EXC 10
#define TSIM_RCODE_ILLEGAL_INSN 11
#define TSIM_RCODE_EXCM 12
#define TSIM_RCODE_PRIVINST 13
#define TSIM_RCODE_RETW_N0 14
#define TSIM_RCODE_RETW_NM 15
#define TSIM_RCODE_RETW_WOE 16
#define TSIM_RCODE_UPDATING_STORE 17
#define TSIM_RCODE_RET 18
#define TSIM_RCODE_CALL 19
#define TSIM_RCODE_ALLOCA 20
// simulator requires a replay of this instruction.
#define TSIM_RCODE_SIM_REPLAY 21
/* Fast simulator took some exception.  Do a slot replay. */
#define TSIM_RCODE_EXCEPTION 22
#define TSIM_RCODE_WOVERFLOW 23
#define TSIM_RCODE_INTERRUPT 24
#define TSIM_RCODE_PIF       25
#define TSIM_RCODE_MMU       26
#define TSIM_RCODE_RFE       27
#define TSIM_RCODE_SIMERROR  28
#define TSIM_RCODE_SIMEXIT   29
#define TSIM_RCODE_SIMINT    30
#define TSIM_RCODE_SLOWDOWN  31
struct raw_mem_block;
struct fast_memory_map;
struct fast_cache;
struct simple_vm_data;


#define SPECIAL_STATE_NONE 0
#define SPECIAL_STATE_WAITI 1
#define SPECIAL_STATE_AUTOFILL_0 2
#define SPECIAL_STATE_AUTOFILL_1D 3
#define SPECIAL_STATE_AUTOFILL_1I 4
#define SPECIAL_STATE_QUEUE_WAIT 5
#define SPECIAL_STATE_STORE_WAIT 6
#define SPECIAL_STATE_LOAD_WAIT 7
#define SPECIAL_STATE_SWITCH_WAIT 8
#define SPECIAL_STATE_LOOKUP_WAIT 9
#define SPECIAL_STATE_CACHE_FLUSH 10
#define SPECIAL_STATE_IFETCH_WAIT 11
#define SPECIAL_STATE_EXTW_WAIT 12
#define SPECIAL_STATE_OCD_HALT 13
#define SPECIAL_STATE_DEBUG_INT 14
#define SPECIAL_STATE_MEM_SPIN 15
#define SPECIAL_STATE_RCW_STORE_WAIT 16
#define SPECIAL_STATE_EXT_REGFILE_WAIT 17

struct simple_states;

struct simple_vm {

  /* 64 bit counters.  gcc is awful with unsigned long long increment
     Thus we only keep part of the icount/ccount here
     actual_ccount = ccount + (orig_countdown-countdown)
     actual_icount = icount + (orig_countdown-countdown-non_insn_count) */
  unsigned long long icount; /* instructions completed */
  unsigned long long ccount; /* cycles simulated */

  struct fast_memory_map *virt_mem_map;
  struct simple_states *xstate__; /* another copy of machine info */
  void *machine_config; /* constant machine configuration */

  unsigned countdown;        /* max cycles that can be simulated in turbo */
  unsigned pc;
  unsigned nextPC;
  
  /* state computed from DBREAKC to avoid dbreak checks everywhere */
  unsigned dbreak_load_enable;
  unsigned dbreak_store_enable;

  unsigned special_state; /* when non-zero, system is in waiti, 
			     autofill, or queue wait state. */
  unsigned global_stall;  /* when non-zero, system is in global stall */
  unsigned backoff_level; /* when stalling, keep track of the number
			     of spin cycles. Beyond some threshhold
			     consider using an event-driven stall */

  unsigned AR_base; /* usually zero, but can be moved up to
                       reduce the register computation. */

  unsigned waiti_count; /* queue stalls simulated */
  unsigned qstall_count; /* queue stalls simulated */
  unsigned lsstall_count; /* load store stalls simulated */
  unsigned lookupstall_count; /* lookup stalls simulated */
  unsigned extregfilestall_count; /* external register file stalls simulated */
  unsigned branchtaken_count; /* branches taken (not loops) */
  unsigned underflow_count; /* underflows */
  unsigned overflow_count; /* overflows */
  unsigned exception_count; /* other exceptions */

  /* These are used for finding the actual icount/ccount when running in turbo
   */
  unsigned non_insn_count;   /* number of non-instruction cycles in turbo */
  unsigned orig_countdown;   /* number of non-instruction cycles in turbo */
  unsigned callee_icount;  /* number of instructions in called routines */

  /* These counts are used to avoid re-reading instructions
     when executed.  When using an instruction cache, for non-iram
     accesses, an ihi or iii must be executed before a changed instruction
     is definitely visible.   Otherwise, just an isync is required. */
  unsigned isync_count;
  unsigned ii_count;

  unsigned external_mod;

  unsigned DIDR;             /* Debug in data register */

  /* 2 long longs, 3 pointers, 24 ints + 1 struct/int */

  /* When pointers + ints + struct/int are not a multipl of 8 bytes
   * in Windows, uncomment __fill_dummy for alignment.
   * unsigned __fill_dummy;
   */

  unsigned ibreak_enable;
  unsigned interpret_insn_count; /* Number of instructions executed in interpret mode from compiled code */

  struct simple_vm_data state; /* 1 int */
};



#ifdef __cplusplus
extern "C" {
#endif

unsigned tsim_a_to_ar(const struct simple_vm *data,
		      unsigned regno);

/* return the number of physical ar registers */
unsigned tsim_ar_entries(const struct simple_vm *data_);

/* same as tsim_a_to_ar, but returns the index number in
   AR that holds the value instead of the architectural "AR"
   register number */
unsigned tsim_a_to_aridx(const struct simple_vm *data,
			  unsigned regno);
unsigned tsim_ar_to_aridx(const struct simple_vm *data,
			  unsigned regno);
/* takes rotation info account */
unsigned tsim_ar_val(const struct simple_vm *data,
		     unsigned ar_regno);

unsigned tsim_a_val(const struct simple_vm *data,
		    unsigned a_regno);

void tsim_set_a_val(struct simple_vm *data,
		    unsigned a_regno,
		    unsigned val);

void tsim_set_ar_val(struct simple_vm *data,
		     unsigned ar_regno,
		     unsigned val);


/* rotate the window so that registers 0 ..max_regnum
   can be accessed without masking the high bits */

/* return non-zero if unable to complete the request */
int tsim_mem_load(struct simple_vm *data,
		  unsigned *dst, unsigned vaddress, unsigned size,
		  unsigned ring);
int tsim_mem_store_check(struct simple_vm *data,
			 unsigned vaddress, unsigned size, unsigned ring);
int tsim_mem_store(struct simple_vm *data,
		   const unsigned *src, unsigned vaddress, unsigned size,
		   unsigned ring);

  int tsim_interrupt_check(const struct simple_vm *data);

  void tsim_set_timer_interrupt(const struct simple_vm *data,
                                const unsigned timer);

  unsigned tsim_read_intrrequest(const struct simple_vm *data);

  void tsim_write_intrquery(const struct simple_vm *data,
                            const unsigned query);

  int tsim_windowunderflow(struct simple_vm *data,
			   unsigned callinc);
  int tsim_windowoverflow(struct simple_vm *data,
			  unsigned max_reg);
  int tsim_call(struct simple_vm *data);
  int tsim_simcall(struct simple_vm *data);
  int tsim_interpret_single_fast_insn(struct simple_vm *data);
  int tsim_interpret_single_insn(struct simple_vm *data);


  int tsim_prepare_input_queue(struct simple_vm *data, unsigned queue_id,
			       unsigned *queue_data);
  int tsim_prepare_output_queue(struct simple_vm *data, unsigned queue_id);
  void tsim_commit_input_queue(struct simple_vm *data, unsigned queue_id);
  void tsim_commit_output_queue(struct simple_vm *data, unsigned queue_id,
				const unsigned *queue_data);
  int tsim_perform_lookup(struct simple_vm *data, unsigned lookup_id,
			  const unsigned *out_data,
			  unsigned *in_data);

  /* These are the canonical callbacks.  They will dispatch to the
     appropriate client functions */
  void tclient_before_fetch_callback(struct simple_vm *data, unsigned len);
  void tclient_before_commit_callback(struct simple_vm *data);
  void tclient_before_branch_commit_callback(struct simple_vm *data,
					     unsigned target_pc,
					     int brkind);

  void tclient_after_mem_load_callback(struct simple_vm *data,
					unsigned address, unsigned size,
					const unsigned *value_p,
					const unsigned *disables);
  void tclient_after_mem_store_callback(struct simple_vm *data,
					unsigned address, unsigned size,
					const unsigned *value_p,
					const unsigned *disables);
  void tclient_reg_load_callback(struct simple_vm *data,
					unsigned rf, unsigned entry);
  void tclient_after_reg_store_callback(struct simple_vm *data,
					unsigned rf, unsigned entry);
  void tclient_after_state_store_callback(struct simple_vm *data,
						   unsigned state);


#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_VM_STATE_H */
