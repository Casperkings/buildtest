/* Copyright (c) 2020 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <xtensa/config/core-isa.h>
#include "xlmem.h"
#include "xmem_bank_common.h"
#include "xmem_misc.h"

/*
 * Local memory configuration discovery from Linker Support Package (LSP) and 
 * core configuration.
 */

// Symbols provided by LSP 

#if XCHAL_NUM_DATARAM > 0
// Start of free space in DRAM0 (end of statically allocated data)
extern char _memmap_mem_dram0_max;
// End address of DRAM0
extern char _memmap_mem_dram0_end;
#endif

#if XCHAL_NUM_DATARAM > 1
// Start of free space in DRAM1 (end of statically allocated data)
extern char _memmap_mem_dram1_max;
// End address of DRAM1 
extern char _memmap_mem_dram1_end;
#endif

// Start of stack
extern void __stack;

/* Initializes the memory banks with start address and size of each local memory
 * bank
 *
 * banks      : Pointer to array of xmem_bank_t structures. Local memory bank 
 *              information is returned through this.
 * num_banks  : Number of elements in the banks array. Will be set to the
 *              actual number of local memory banks.
 * stack_size : Stack space to reserve
 * 
 * Returns XMEM_BANK_OK if successful, else XMEM_BANK_ERR_INVALID_ARGS,
 * XMEM_BANK_ERR_CONFIG_UNSUPPORTED, XMEM_BANK_ERR_STACK_RESERVE_FAIL,
 * or XMEM_BANK_ERR_INIT_BANKS_FAIL.
 */
xmem_bank_status_t xlmem_init_banks(xmem_bank_t *banks, unsigned *num_banks,
                                    unsigned stack_size)
{
  xmem_bank_t xlmem_banks[XCHAL_NUM_DATARAM];

  XMEM_CHECK(XCHAL_NUM_DATARAM, XMEM_BANK_ERR_CONFIG_UNSUPPORTED,
             "xlmem_init_banks: No local memory banks present in config\n");

  XMEM_CHECK(XCHAL_NUM_DATARAM <= 2, XMEM_BANK_ERR_CONFIG_UNSUPPORTED,
             "xlmem_init_banks: Config has %d local memory banks, but only a "
             "max of 2 local memory banks are supported\n", XCHAL_NUM_DATARAM);

  XMEM_CHECK(banks != NULL, XMEM_BANK_ERR_INVALID_ARGS,
             "xlmem_init_banks: banks is null\n");

  XMEM_CHECK(num_banks != NULL, XMEM_BANK_ERR_INVALID_ARGS,
             "xlmem_init_banks: num_banks is null\n");

#if XCHAL_NUM_DATARAM
  unsigned count = 0;
  // Start of free space is end of statically allocated data in the bank
  // Remaining is free space
  xlmem_banks[count]._start = &_memmap_mem_dram0_max;
  xlmem_banks[count]._size  = &_memmap_mem_dram0_end - &_memmap_mem_dram0_max;
  ++count;
#endif

#if XCHAL_NUM_DATARAM > 1
  xlmem_banks[count]._start = &_memmap_mem_dram1_max;
  xlmem_banks[count]._size  = &_memmap_mem_dram1_end - &_memmap_mem_dram1_max;

  // Order the banks by the start address
  if ((uintptr_t)xlmem_banks[count]._start < 
      (uintptr_t)xlmem_banks[count - 1]._start) {
    xmem_bank_t tmp = xlmem_banks[count - 1];
    xlmem_banks[count - 1] = xlmem_banks[count];
    xlmem_banks[count] = tmp;
  }
  ++count;
#endif

  // Find the bank containing the stack and adjust bank size to exclude the
  // space reserved for stack
  uintptr_t stack_end = (uintptr_t)&__stack - stack_size;
  int allocated_stack = 0;
  int i;
  for (i = 0; i < XCHAL_NUM_DATARAM; ++i) {
    if ((uintptr_t)xlmem_banks[i]._start <= stack_end &&
        (uintptr_t)xlmem_banks[i]._start + xlmem_banks[i]._size >= 
          (uintptr_t)&__stack) {
      xlmem_banks[i]._size = stack_end - (uintptr_t)xlmem_banks[i]._start;
      allocated_stack = 1;
      break;
    }
  }

  xmem_bank_status_t xbs = XMEM_BANK_OK;

  if (stack_size != 0 && !allocated_stack) {
    XMEM_LOG("xlmem_init_banks: Could not allocate stack of size %d in any "
             "of the banks\n", stack_size);
    xbs = XMEM_BANK_ERR_STACK_RESERVE_FAIL;
  }

#if XMEM_DEBUG
  XMEM_LOG("xlmem_init_banks: Initial local memory bank info:\n");
  for (i = 0; i < XCHAL_NUM_DATARAM; ++i) {
    XMEM_LOG("Bank %d, start = %p, end = %p, size = %u\n", i,
             xlmem_banks[i]._start, 
             (void *)((uintptr_t)xlmem_banks[i]._start+xlmem_banks[i]._size-1),
             xlmem_banks[i]._size);
  }
#endif

  // Copy back the bank info
  int n = *num_banks < XCHAL_NUM_DATARAM ? *num_banks : XCHAL_NUM_DATARAM;
  memcpy(banks, xlmem_banks, n*sizeof(xmem_bank_t));
  *num_banks = XCHAL_NUM_DATARAM;

  return xbs;
}
