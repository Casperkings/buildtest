// This example demonstrates the use of the stack memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/xmem.h>

#define POOL_SIZE (1024*10)

// Allocate from this pool
char mem_pool[POOL_SIZE];

// Function to query free space in the stack. 
static int query_free_space(xmem_stack_mgr_t *mgr)
{
  xmem_status_t status;
  void *start_free_space, *end_free_space;
  unsigned free_space;

  // Query remaining free space
  free_space = xmem_stack_get_free_space(mgr, 1, // align == 1
                                         &start_free_space, &end_free_space,
                                         &status);
  if (status != XMEM_OK) {
    printf("xmem_stack_get_free_space failed, status = %d\n", status);
    return -1;
  }
  printf("Memory has %d bytes free in range [%p, %p]\n",
         free_space, start_free_space, end_free_space);

  printf("Stats: free: %d, unused: %d, allocated: %d\n",
         xmem_stack_get_free_bytes(mgr),
         xmem_stack_get_unused_bytes(mgr),
         xmem_stack_get_allocated_bytes(mgr));

  return 0;
}

int main()
{
  xmem_stack_mgr_t mgr;
  xmem_stack_mgr_checkpoint_t cp;
  xmem_status_t status;
  int size = 1024;
  int align = 64;
  void *p = NULL;

  // Initialize stack manager with the user provided pool
  status = xmem_stack_init(&mgr, mem_pool, POOL_SIZE);
  if (status != XMEM_OK) {
    printf("xmem_stack_init failed, status = %d\n", status);
    return -1;
  }
  printf("Stack memory manager initialized using pool at %p of %d bytes\n",
          mem_pool, POOL_SIZE);

  // Query remaining free space
  query_free_space(&mgr);

  // Allocate from the memory pool
  p = xmem_stack_alloc(&mgr, size, align, &status);
  if (status != XMEM_OK) {
    printf("xmem_stack_alloc failed, status = %d\n", status);
    return -1;
  }
  printf("Allocated %d bytes with align %d at %p\n", size, align, p);

  // Query remaining free space
  query_free_space(&mgr);

  // Check point stack state
  printf("Check pointing memory manager state\n");
  status = xmem_stack_checkpoint_save(&mgr, &cp);
  if (status != XMEM_OK) {
    printf("xmem_stack_checkpoint_save failed, status = %d\n", status);
    return -1;
  }

  // Allocate again from the memory pool
  p = xmem_stack_alloc(&mgr, size, align, &status);
  if (status != XMEM_OK) {
    printf("xmem_stack_alloc failed, status = %d\n", status);
    return -1;
  }
  printf("Allocated %d bytes with align %d at %p\n", size, align, p);

  // Query remaining free space
  query_free_space(&mgr);

  // Restore checkpoint
  printf("Restoring memory manager state from check point\n");
  status = xmem_stack_checkpoint_restore(&mgr, &cp);
  if (status != XMEM_OK) {
    printf("xmem_stack_checkpoint_restore failed, status = %d\n", status);
    return -1;
  }

  // Query remaining free space
  query_free_space(&mgr);

  // Reset stack state
  printf("Resetting memory manager state\n");
  status = xmem_stack_reset(&mgr);
  if (status != XMEM_OK) {
    printf("xmem_stack_reset failed, status = %d\n", status);
    return -1;
  }

  // Query remaining free space
  query_free_space(&mgr);

  return 0;
}
