// This example demonstrates the use of the bank memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/xmem_bank.h>

// Function to query free space in the banks. 
static int query_free_space(int bank)
{
  xmem_bank_status_t status;
  void *start_free_space, *end_free_space;
  unsigned free_space;

  // Query max available contiguous free space
  free_space = xmem_bank_get_free_space(bank, 1, // align == 1
                                        &start_free_space, &end_free_space,
                                        &status);
  if (status != XMEM_BANK_OK) {
    printf("xmem_bank_get_free_space failed, status = %d\n", status);
    return -1;
  }
  printf("Bank %d has %d bytes free in range [%p, %p]\n",
         bank, free_space, start_free_space, end_free_space);

  printf("Stats: free: %d, unused: %d, allocated: %d\n",
         xmem_bank_get_free_bytes(bank),
         xmem_bank_get_unused_bytes(bank),
         xmem_bank_get_allocated_bytes(bank));

  return 0;
}

int main()
{
  xmem_bank_status_t status;
  void *start_free_space, *end_free_space;
  void *p;
  size_t rem;

  // Initialize the banks and manage them using heap allocation policy
  // Reserve 16-Kb for stack space. Note, only if stack is present in
  // local memories would this get reserved (eg: using sim-stacklocal LSP)
  status = xmem_init_local_mem(XMEM_BANK_HEAP_ALLOC, 16*1024);
  if (status != XMEM_BANK_OK && status != XMEM_BANK_ERR_STACK_RESERVE_FAIL) {
    printf("xmem_init_local_mem failed, status = %d\n", status);
    return -1;
  }

  // Print free space in both banks
  query_free_space(0);
  if (xmem_bank_get_num_banks() > 1) {
    query_free_space(1);
  }

  // Find free space in bank-0
  rem = xmem_bank_get_free_space(0, 1, // align
                                 &start_free_space, &end_free_space, 
                                 &status);
  if (status != XMEM_BANK_OK) {
    printf("xmem_bank_get_free_space, err = %d\n", status);
    return -1;
  }

  if (xmem_bank_get_num_banks() > 1) {
    // Allocate all bytes from bank-0 + 5 more bytes from bank-1
    p = xmem_bank_alloc(-1, rem+5, 1, &status);
  } else {
    // Allocate all bytes of bank-0
    p = xmem_bank_alloc(-1, rem, 1, &status);
  }
  if (status != XMEM_BANK_OK) {
    printf("xmem_bank_alloc, status = %d\n", status);
    return -1;
  }

  // Print free space in both banks
  query_free_space(0);
  if (xmem_bank_get_num_banks() > 1) {
    query_free_space(1);
  }
  
  // Free allocated space
  status = xmem_bank_free(-1, p);
  if (status != XMEM_BANK_OK) {
    printf("xmem_bank_free, status = %d\n", status);
    return -1;
  }

  // Print free space in both banks
  query_free_space(0);
  if (xmem_bank_get_num_banks() > 1) {
    query_free_space(1);
  }

  return 0;
}
