// This example demonstrates the use of the L2 memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/xmem_bank.h>

// Function to query free space
static int query_free_space()
{
  xmem_bank_status_t status;
  void *start_free_space, *end_free_space;
  unsigned free_space;

  // Query max available contiguous free space
  free_space = xmem_l2_get_free_space(1, // align == 1
                                      &start_free_space, &end_free_space,
                                      &status);
  if (status != XMEM_BANK_OK) {
    printf("xmem_l2_get_free_space failed, status = %d\n", status);
    return -1;
  }
  printf("L2 has %d bytes free in range [%p, %p]\n",
         free_space, start_free_space, end_free_space);

  printf("Stats: free: %d, unused: %d, allocated: %d\n",
         xmem_l2_get_free_bytes(),
         xmem_l2_get_unused_bytes(),
         xmem_l2_get_allocated_bytes());

  return 0;
}

int main()
{
  xmem_bank_status_t status;
  void *start_free_space, *end_free_space;
  void *p;
  size_t rem;

  // Initialize the L2. 
  status = xmem_init_l2_mem();
  if (status != XMEM_BANK_OK) {
    printf("xmem_init_l2_mem failed, status = %d\n", status);
    return -1;
  }

  // Print free space 
  query_free_space();

  // Find free space
  rem = xmem_l2_get_free_space(1, // align
                               &start_free_space, &end_free_space, 
                               &status);
  if (status != XMEM_BANK_OK) {
    printf("xmem_l2_get_free_space, err = %d\n", status);
    return -1;
  }

  // Allocate all bytes of L2
  p = xmem_l2_alloc(rem, 1, &status);

  if (status != XMEM_BANK_OK) {
    printf("xmem_l2_alloc, status = %d\n", status);
    return -1;
  }

  // Print free space
  query_free_space();
  
  // Free allocated space
  status = xmem_l2_free( p);
  if (status != XMEM_BANK_OK) {
    printf("xmem_l2_free, status = %d\n", status);
    return -1;
  }

  // Print free space
  query_free_space();

  return 0;
}
