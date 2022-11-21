// This example demonstrates the use of the heap memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/xmem.h>

#define POOL_SIZE  (1024*10)
#define NUM_BLOCKS (32)

// Allocate from this pool
char mem_pool[POOL_SIZE];

// Heap header
char heap_header[XMEM_HEAP_HEADER_SIZE(NUM_BLOCKS)];

// Function to query free space in the heap.
static int query_free_space(xmem_heap_mgr_t *mgr)
{
  xmem_status_t status;
  void *start_free_space, *end_free_space;
  unsigned free_space;

  // Query max available contiguous free space
  free_space = xmem_heap_get_free_space(mgr, 1, // align == 1
                                        XMEM_HEAP_MAX_FREE_SPACE,
                                        &start_free_space, &end_free_space,
                                        &status);
  if (status != XMEM_OK) {
    printf("xmem_heap_get_free_space failed, status = %d\n", status);
    return -1;
  }
  printf("Memory has %d bytes free in range [%p, %p]\n",
         free_space, start_free_space, end_free_space);

  printf("Stats: free: %d, unused: %d, allocated: %d\n",
         xmem_heap_get_free_bytes(mgr),
         xmem_heap_get_unused_bytes(mgr),
         xmem_heap_get_allocated_bytes(mgr));

  return 0;
}

int main()
{
  xmem_heap_mgr_t mgr;

  xmem_status_t status;
  int size = 1024;
  int align = 64;
  void *p = NULL;

  // Initialize heap manager with the user provided pool
  status = xmem_heap_init(&mgr, mem_pool, POOL_SIZE, NUM_BLOCKS, heap_header);
  if (status != XMEM_OK) {
    printf("xmem_heap_init failed, status = %d\n", status);
    return -1;
  }
  printf("Heap memory manager initialized using pool at %p of %d bytes\n",
          mem_pool, POOL_SIZE);

  // Query remaining free space
  query_free_space(&mgr);

  // Allocate from the memory pool
  p = xmem_heap_alloc(&mgr, size, align, &status);
  if (status != XMEM_OK) {
    printf("xmem_heap_alloc failed, status = %d\n", status);
    return -1;
  }
  printf("Allocated %d bytes with align %d at %p\n", size, align, p);

  // Query remaining free space
  query_free_space(&mgr);

  // Free allocated space
  printf("Freeing memory at %p\n", p);
  status = xmem_heap_free(&mgr, p);
  if (status != XMEM_OK) {
    printf("xmem_heap_free failed, status = %d\n", status);
    return -1;
  }

  // Query remaining free space
  query_free_space(&mgr);

  // Allocate again from the memory pool
  p = xmem_heap_alloc(&mgr, size, align, &status);
  if (status != XMEM_OK) {
    printf("xmem_heap_alloc failed, status = %d\n", status);
    return -1;
  }
  printf("Allocated %d bytes with align %d at %p\n", size, align, p);

  // Query remaining free space
  query_free_space(&mgr);

  // Reset heap state
  printf("Resetting memory manager state\n");
  status = xmem_heap_reset(&mgr);
  if (status != XMEM_OK) {
    printf("xmem_heap_reset failed, status = %d\n", status);
    return -1;
  }

  // Query remaining free space
  query_free_space(&mgr);

  return 0;
}
