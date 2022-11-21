#include "xmem_mgr.h"
#include "xmem_misc.h"
#include <string.h>

#ifdef XMEM_DEBUG
static void
xmem_heap_print_bitvec(const char *msg, const uint32_t *bitvec, 
                       uint32_t num_bits)
{
  int i;
  char b[256];
  sprintf(b, "%s: (%d-bits) ", msg, num_bits);
  int num_words = (num_bits+31)>>5;
  xmem_log("%s", b);
  for (i = num_words-1; i >= 0; i--) {
    xt_printf("0x%x,", bitvec[i]);
  }
  xt_printf("\n");
}

static char *
xmem_heap_print_block_suffix(xmem_heap_block_t *block, xmem_heap_mgr_t *mgr)
{
  if (block == &mgr->_free_list_head)
    return "fh";
  else if (block == &mgr->_alloc_list_head)
    return "ah";
  else if (block == &mgr->_free_list_tail)
    return "ft";
  else
    return "";
}

static void
xmem_heap_print_block(char *m, xmem_heap_block_t *block, xmem_heap_mgr_t *mgr)
{
  char *ct = xmem_heap_print_block_suffix(block, mgr);
  if (block->_next_block) {
    char *nt = xmem_heap_print_block_suffix(block->_next_block, mgr);
    xmem_log("%s : block@%p (%s) %p:%p, next_block (%s) %p:%p, size: %d\n",
             m, block, ct, block->_buffer, block->_aligned_buffer,
             nt, block->_next_block->_buffer,
             block->_next_block->_aligned_buffer,
             block->_block_size);
  } else {
    xmem_log("%s : block@%p (%s) %p:%p, size: %d\n",
             m, block, ct, block->_buffer, block->_aligned_buffer,
             block->_block_size);
  }
}

static void
xmem_heap_print_free_list(xmem_heap_mgr_t *mgr)
{
  xmem_heap_block_t *block;
  XMEM_LOG("Dumping free list\n");
  for (block = &mgr->_free_list_head; block != NULL; block = block->_next_block)
    xmem_heap_print_block(" free_list_block", block, mgr);
}

static void
xmem_heap_print_alloc_list(xmem_heap_mgr_t *mgr)
{
  xmem_heap_block_t *block;
  XMEM_LOG("Dumping alloc list\n");
  for (block = &mgr->_alloc_list_head; block != NULL;
       block = block->_next_block)
    xmem_heap_print_block(" alloc_list_block", block, mgr);
}
#endif

#ifdef XMEM_VERIFY
void 
xmem_heap_verify(xmem_heap_mgr_t *mgr)
{
  xmem_heap_block_t *block;
  int free_size = 0, alloc_size = 0;
  uintptr_t prev_block_addr = 0;
  int num_blocks = 0;

  for (block = mgr->_free_list_head._next_block;
       block != &mgr->_free_list_tail;
       block = block->_next_block)
  {
    if ((uintptr_t)block->_buffer < (uintptr_t)mgr->_buffer ||
        ((uintptr_t)block->_buffer >=
                   ((uintptr_t)mgr->_buffer + (uintptr_t)mgr->_buffer_size))) {
      XMEM_ABORT("xmem_heap_verify: Block %p in free list not contained "
                 "within buffer\n", block, mgr->_buffer);
    }
    if ((uintptr_t)block->_aligned_buffer < (uintptr_t)mgr->_buffer ||
        ((uintptr_t)block->_aligned_buffer >=
                   ((uintptr_t)mgr->_buffer + (uintptr_t)mgr->_buffer_size))) {
      XMEM_ABORT("xmem_heap_verify: Block %p in free list not contained "
                 "within buffer\n", block, mgr->_buffer);
    }
    if ((uintptr_t)block->_buffer < prev_block_addr) {
#ifdef XMEM_DEBUG
      xmem_heap_print_free_list(mgr);
#endif
      XMEM_ABORT("xmem_heap_verify: "
                 "Free list not in sorted order of block addr\n");
    }
    if (block->_next_block != &mgr->_free_list_tail) {
      if ((uintptr_t)block->_buffer + block->_block_size >
          (uintptr_t)block->_next_block->_buffer) {
#ifdef XMEM_DEBUG
        xmem_heap_print_block("block: ", block, mgr);
#endif
        XMEM_ABORT("xmem_heap_verify: Free list blocks overlap\n");
      }
    }
    uint32_t idx = ((uintptr_t)block - (uintptr_t)&mgr->_blocks[0]) >>
                   XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2;
    uint32_t lzc =
      xmem_find_leading_zero_one_count(mgr->_block_free_bitvec,
                                       mgr->_num_blocks, idx, 0);
    if (lzc < 1) {
      XMEM_ABORT("xmem_heap_verify: Expecting bit at idx %d to be 0\n", idx);
    }

    prev_block_addr = (uintptr_t)block->_buffer;
    free_size += block->_block_size;
    num_blocks++;
  }
  if (free_size != mgr->_free_bytes) {
    XMEM_ABORT("xmem_heap_verify: Expecting free_size %d to be equal to "
               "free_bytes %d\n", free_size, mgr->_free_bytes);
  }

  for (block = mgr->_alloc_list_head._next_block; block != 0;
       block = block->_next_block)
  {
    if ((uintptr_t)block->_buffer < (uintptr_t)mgr->_buffer ||
        ((uintptr_t)block->_buffer >=
                   ((uintptr_t)mgr->_buffer + (uintptr_t)mgr->_buffer_size))) {
      XMEM_ABORT("xmem_heap_verify: Block %p in alloc list not contained "
                 "within buffer\n", block, mgr->_buffer);
    }
    if ((uintptr_t)block->_aligned_buffer < (uintptr_t)mgr->_buffer ||
        ((uintptr_t)block->_aligned_buffer >=
                   ((uintptr_t)mgr->_buffer + (uintptr_t)mgr->_buffer_size))) {
      XMEM_ABORT("xmem_heap_verify: Block %p in alloc list not contained "
                 "within buffer\n", block, mgr->_buffer);
    }
    uint32_t idx = ((uintptr_t)block - (uintptr_t)&mgr->_blocks[0]) >>
                   XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2;
    uint32_t lzc =
      xmem_find_leading_zero_one_count(mgr->_block_free_bitvec,
                                       mgr->_num_blocks, idx, 0);
    if (lzc < 1) {
      XMEM_ABORT("xmem_heap_verify: Expecting bit at idx %d to be 0\n", idx);
    }
    alloc_size += block->_block_size;
    num_blocks++;
  }
  if (alloc_size != mgr->_allocated_bytes) {
    XMEM_ABORT("xmem_heap_verify: Expecting alloc_size %d to be equal to "
               "allocated_bytes %d\n", alloc_size, mgr->_allocated_bytes);
  }

  if ((mgr->_free_bytes + mgr->_allocated_bytes) != mgr->_buffer_size) {
    XMEM_ABORT("xmem_heap_verify: Mismatch in allocated/free bytes, "
               "free_bytes: %d allocated_bytes %d buffer_size %d\n", 
               mgr->_free_bytes, mgr->_allocated_bytes, mgr->_buffer_size);
  }

  uint32_t pop_count = xmem_pop_count(mgr->_block_free_bitvec,
                                      mgr->_num_blocks,
                                      0xffffffff);
  if (num_blocks != pop_count) {
    XMEM_ABORT("xmem_heap_verify: Mismatch. num_blocks %d, pop_count %d\n",
               num_blocks, pop_count);
  }
}
#endif

/* Heap manager usage statistics
 *
 * mgr : heap manager
 *
 * Returns void
 */
__attribute__((unused)) static void
xmem_heap_print_stats(xmem_heap_mgr_t *mgr)
{
  XMEM_LOG("Heap buffer @ %p, size: %d, free bytes: %d, "
           "allocated bytes: %d, unused bytes: %d, percent overhead: %d\n",
           mgr->_buffer, mgr->_buffer_size, mgr->_free_bytes,
           mgr->_allocated_bytes, mgr->_unused_bytes,
           (mgr->_unused_bytes*100)/mgr->_buffer_size);
}

/* Return heap manager unused bytes
 *
 * mgr : heap manager
 *
 * Returns heap manager unused bytes
 */
size_t xmem_heap_get_unused_bytes(xmem_heap_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_heap_get_unused_bytes: mgr is null\n");
  return mgr->_unused_bytes;
}

/* Return heap manager free bytes
 *
 * mgr : heap manager
 *
 * Returns heap manager free bytes
 */
size_t xmem_heap_get_free_bytes(xmem_heap_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_heap_get_free_bytes: mgr is null\n");
  return mgr->_free_bytes;
}

/* Return heap manager allocated bytes
 *
 * mgr : heap manager
 *
 * Returns heap manager allocated bytes
 */
size_t xmem_heap_get_allocated_bytes(xmem_heap_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_heap_get_allocated_bytes: mgr is null\n");
  return mgr->_allocated_bytes;
}

/* Initialize the heap manager
 *
 * mgr        : memory manager
 * buffer     : user provided pool for allocation
 * size       : size of the pool
 * num_blocks : number of blocks available for allocation/free
 * header     : header buffer used to store the block information.
 *
 * Returns XMEM_OK if successful, else returns one of XMEM_ERR_INTERNAL
 * or XMEM_ERR_INVALID_ARGS.
 */
xmem_status_t 
xmem_heap_init(xmem_heap_mgr_t *mgr, void *buffer, size_t size,
               uint32_t num_blocks, void *header) 
{
  /* Check if manager block size is internally consistent */

  XMEM_CHECK(sizeof(xmem_heap_block_t) == XMEM_HEAP_BLOCK_STRUCT_SIZE, 
             XMEM_ERR_INTERNAL,
             "xmem_heap_init: Sizeof xmem_heap_block_t expected to be %d, "
             "but got %d\n", XMEM_HEAP_BLOCK_STRUCT_SIZE, 
             sizeof(xmem_heap_block_t));

  XMEM_CHECK(!(XMEM_HEAP_BLOCK_STRUCT_SIZE & (XMEM_HEAP_BLOCK_STRUCT_SIZE-1)),
             XMEM_ERR_INTERNAL,
             "xmem_heap_init: XMEM_HEAP_BLOCK_STRUCT_SIZE (%d) is not a "
             "power of 2\n", XMEM_HEAP_BLOCK_STRUCT_SIZE);

  XMEM_CHECK(XMEM_HEAP_BLOCK_STRUCT_SIZE ==
            (1 << XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2),
            XMEM_ERR_INTERNAL,
            "xmem_heap_init: XMEM_HEAP_BLOCK_STRUCT_SIZE (%d) and "
            "XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2 (%d) not consistent\n", 
            XMEM_HEAP_BLOCK_STRUCT_SIZE, XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2);

  XMEM_CHECK(sizeof(xmem_heap_mgr_t) == XMEM_HEAP_MGR_SIZE,
             XMEM_ERR_INTERNAL,
             "xmem_heap_init: Sizeof xmem_heap_mgr_t expected to be %d, "
             "but got %d\n", XMEM_HEAP_MGR_SIZE, sizeof(xmem_heap_mgr_t));

  XMEM_CHECK(buffer != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_init: buffer is null\n");

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_init: mgr is null\n");

  XMEM_LOG("xmem_heap_init: Initializing manager: %p, buffer: %p, size: %d "
           "num_blocks: %d, header: %p\n", mgr, buffer, 
           size, num_blocks, header);

  mgr->_lock = NULL;
  mgr->_buffer = buffer;
  mgr->_buffer_size = size;

  /* If there is an externally provided header, the blocks and the bitvector
   * are allocated from the header, else they are from the user provided pool */
  if (header) {
    mgr->_num_blocks = num_blocks;
    mgr->_blocks = (xmem_heap_block_t *)header;
    mgr->_has_external_header = 1;
  } else {
    mgr->_num_blocks = XMEM_HEAP_DEFAULT_NUM_BLOCKS;
    mgr->_blocks = (xmem_heap_block_t *)buffer;
    mgr->_has_external_header = 0;
  }
  uint32_t num_words_in_block_bitvec = (mgr->_num_blocks + 31) >> 5;
  mgr->_block_free_bitvec = 
    (uint32_t *)((uintptr_t)mgr->_blocks + 
                 sizeof(xmem_heap_block_t) * mgr->_num_blocks);
  mgr->_header_size = sizeof(xmem_heap_block_t)*mgr->_num_blocks +
                      num_words_in_block_bitvec*4;

   /* Check if there is the minimum required buffer size and number of blocks */
  uint32_t min_size = !header ? mgr->_header_size + XMEM_HEAP_MIN_ALLOC_SIZE
                              : XMEM_HEAP_MIN_ALLOC_SIZE;
   
  XMEM_CHECK(size >= min_size, XMEM_ERR_POOL_SIZE,
             "xmem_heap_init: Pool has %d bytes. Need at atleast %d "
             "bytes for pool\n", size, min_size);

  XMEM_CHECK(mgr->_num_blocks, XMEM_ERR_INVALID_ARGS,
             "xmem_heap_init: num_blocks cannot be 0\n");

  /* Initialize free and allocated list */
  mgr->_free_list_head._block_size = 0;
  mgr->_free_list_head._buffer = 0;
  mgr->_free_list_head._aligned_buffer = 0;
  mgr->_free_list_tail._block_size = 0xffffffff;
  mgr->_free_list_tail._buffer = (void *)0xffffffff;
  mgr->_free_list_tail._aligned_buffer = (void *)0xffffffff;

  mgr->_alloc_list_head._block_size = 0;
  mgr->_alloc_list_head._buffer = 0;
  mgr->_alloc_list_head._aligned_buffer = 0;

  mgr->_allocated_bytes = 0;

  if (header) {
    /* Initialize the first block with the start address of the buffer and
     * size as the whole buffer and add to the free list */
    mgr->_blocks[0]._block_size = size;
    mgr->_blocks[0]._buffer = buffer;
    mgr->_blocks[0]._aligned_buffer = buffer;

    mgr->_free_list_head._next_block = &mgr->_blocks[0];
    mgr->_free_list_tail._next_block = 0;
    mgr->_blocks[0]._next_block = &mgr->_free_list_tail;

    mgr->_alloc_list_head._next_block = 0;

    /* Set the first block, which is the whole buffer free block 
     * as in use (set to 0) and rest as available (set to 1) */
    mgr->_block_free_bitvec[0] = 0xfffffffe;

    /* Initialize the rest as available */
    int i;
    for (i = 1; i < (mgr->_num_blocks + 31) >> 5; ++i) {
      mgr->_block_free_bitvec[i] = 0xffffffff;
    }

    mgr->_free_bytes = size;
    mgr->_unused_bytes = 0;
  } else {
    /* The header (array of xmem_heap_block_t and the bitvector) is placed at 
     * the start of buf. Data allocation begins after that. Initialize the first
     * block with the header address (start of buf) and header size. */
    mgr->_blocks[0]._block_size = mgr->_header_size;
    mgr->_blocks[0]._buffer = buffer;
    mgr->_blocks[0]._aligned_buffer = buffer;

    mgr->_alloc_list_head._next_block = &mgr->_blocks[0];
    mgr->_blocks[0]._next_block = 0;

    /* Initialize the second block with the address of the area in buffer 
     * following the xmem_heap_block_t array and bitvector that is available for
     * user allocation. Size is set as the buffer size minus the space required
     * for the block array and bitvector */
    mgr->_blocks[1]._block_size = size - mgr->_blocks[0]._block_size;
    mgr->_blocks[1]._buffer =  (void *)((uintptr_t)mgr->_blocks[0]._buffer +
                                         mgr->_blocks[0]._block_size);
    mgr->_blocks[1]._aligned_buffer = mgr->_blocks[1]._buffer;

    mgr->_free_list_head._next_block = &mgr->_blocks[1];
    mgr->_free_list_tail._next_block = 0;
    mgr->_blocks[1]._next_block = &mgr->_free_list_tail;

    /* Set the first two blocks (the block that holds the header and the
     * block for the rest of the buffer) as in use (set to 0) and rest as 
     * available (set to 1) */
    mgr->_block_free_bitvec[0] = 0xfffffffc;

    /* Initialize the rest as available */
    int i;
    for (i = 1; i < (mgr->_num_blocks + 31) >> 5; ++i) {
      mgr->_block_free_bitvec[i] = 0xffffffff;
    }

    mgr->_free_bytes = mgr->_blocks[1]._block_size;
    mgr->_unused_bytes = mgr->_blocks[0]._block_size;
    mgr->_allocated_bytes = mgr->_unused_bytes;
  }

#ifdef XMEM_DEBUG
  xmem_heap_print_free_list(mgr);
  xmem_heap_print_alloc_list(mgr);
  xmem_heap_print_bitvec("bitvec", mgr->_block_free_bitvec, mgr->_num_blocks);
#endif

  XMEM_LOG("xmem_heap_init: Original buffer: %p, blocks: %p "
           "block_free_bivec: %p allocatable buffer: %p "
           "allocatable buffer size: %d allocated_bytes: %d free_bytes: %d "
           "unused_bytes: %d\n", buffer, mgr->_blocks, mgr->_block_free_bitvec,
           mgr->_free_list_head._next_block->_buffer, 
           mgr->_free_list_head._next_block->_block_size, 
           mgr->_allocated_bytes, mgr->_free_bytes, mgr->_unused_bytes);
#ifdef XMEM_DEBUG
  xmem_print_bitvec("xmem_heap_init: block_free_bivec", 
                    mgr->_block_free_bitvec, mgr->_num_blocks);
#endif

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_writeback(mgr->_blocks, mgr->_header_size);
#endif

  return XMEM_OK;
}

/* Set lock object
 *
 * mgr  : heap memory manager
 * lock : lock object provided buffer for allocation
 *
 * Returns XMEM_OK, if successful, else returns XMEM_ERR_INVALID_ARGS
 */
xmem_status_t
xmem_heap_set_lock(xmem_heap_mgr_t *mgr, volatile void *lock) {
  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_set_lock: mgr is null\n");
  mgr->_lock = lock;
  return XMEM_OK;
}

/* Compute the size required based on the alignment. If the block address is
 * not aligned, extra space is alloted to align the address to the requested
 * alignment.
 *
 * block : block from which allocation is done
 * size  : size to be allocated
 * align : requested alignment
 *
 * Returns the size after alignment
 */
static XMEM_INLINE size_t
xmem_heap_compute_new_size(xmem_heap_block_t *block, size_t size, 
                           uint32_t align)
{
  /* Align the block buffer address to the specified alignment */
  uint32_t p = xmem_find_msbit(align);
  uintptr_t aligned_block = (uintptr_t)block->_buffer;
  aligned_block = ((aligned_block + align - 1) >> p) << p;
  /* compute extra space required for alignment */
  size_t extra_size = aligned_block - (uintptr_t)block->_buffer ;
  size_t new_size = size + extra_size;
  XMEM_LOG("xmem_heap_compute_new_size: block buffer: %p "
           "aligned_block buffer: %p extra_size: %u new_size %u\n", 
           block->_buffer, (void *)aligned_block, extra_size, new_size);
  return new_size;
}

/* Allocate buffer from the heap manager of specified size and alignment. 
 * Alignment has to be a power-of-2 and size has to be non-zero
 *
 * mgr    : memory manager
 * size   : size of buffer to allocate
 * align  : alignment of the requested buffer
 * status : error code if any is returned.
 *
 * Returns a pointer to the allocated buffer and sets status to XMEM_OK, 
 * else returns NULL and sets status to one of XMEM_ERR_ALLOC_FAILED,
 * XMEM_ERR_ILLEGAL_ALIGN, or XMEM_ERR_INVALID_ARGS.
 */
void *
xmem_heap_alloc(xmem_heap_mgr_t *mgr, size_t size, uint32_t align,
                xmem_status_t *status)
{
  void *r = NULL;
  xmem_status_t err = XMEM_OK;

  XMEM_LOG("xmem_heap_alloc: Requesting %d bytes at align %d\n", size, align);

  XMEM_CHECK_STATUS(mgr != NULL, err, XMEM_ERR_INVALID_ARGS, outer_fail,
                    "xmem_heap_alloc: mgr is null\n");

  /* Expect alignment to be a power-of-2 */
  XMEM_CHECK_STATUS(align && !(align & (align-1)),
                    err, XMEM_ERR_ILLEGAL_ALIGN, outer_fail,
                    "xmem_heap_alloc: alignment should be a power of 2\n");

  /* Size has to be > 0 */
  XMEM_CHECK_STATUS(size > 0, err, XMEM_ERR_INVALID_ARGS, outer_fail,
                    "xmem_heap_alloc: size should be > 0\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_invalidate(mgr->_blocks, mgr->_header_size);
#endif

  xmem_heap_block_t *prev_block = &mgr->_free_list_head;
  xmem_heap_block_t *curr_block = mgr->_free_list_head._next_block;

#ifdef XMEM_DEBUG
  xmem_heap_print_block("xmem_heap_alloc: prev_block", prev_block, mgr);
  xmem_heap_print_block("xmem_heap_alloc: curr_block", curr_block, mgr);
#endif

  /* Compute required size based on alignment */
  uint32_t new_size = xmem_heap_compute_new_size(curr_block, size, align);

  /* Check for the first available block in the free list that satisfies
   * the required size */
  while (curr_block->_block_size < new_size) {
    prev_block = curr_block;
    curr_block = curr_block->_next_block;
#ifdef XMEM_DEBUG
    xmem_heap_print_block("xmem_heap_alloc: prev_block", prev_block, mgr);
    xmem_heap_print_block("xmem_heap_alloc: curr_block", curr_block, mgr);
#endif
    new_size = xmem_heap_compute_new_size(curr_block, size, align);
  }

  XMEM_CHECK_STATUS(curr_block != &mgr->_free_list_tail,
                    err, XMEM_ERR_ALLOC_FAILED, fail,
                    "Failed to allocate %d bytes with alignment %d. "
                    "Heap has %d free bytes and %d allocated bytes\n", 
                    (int32_t)size, (int32_t)align, (int32_t)mgr->_free_bytes, 
                    (int32_t)mgr->_allocated_bytes);

  /* Found the block */
  curr_block->_aligned_buffer = (void *)((uintptr_t)curr_block->_buffer +
                                         new_size - size);
  /* The aligned buffer address gets returned to the user */
  r = curr_block->_aligned_buffer;
  mgr->_unused_bytes += (new_size - size);

  /* Find a free block to hold the remaining of the current block */
  uint32_t avail_buf_idx =
           xmem_find_leading_zero_one_count(mgr->_block_free_bitvec,
                                            mgr->_num_blocks, 0, 0);

  /* If the remaining space in the block is < than the minimum alloc size,
   * just allocate the whole block to this request, else split the block 
   * and add the remaining to the free list, if there are free blocks
   * available in the header */
  if ((curr_block->_block_size - new_size) > XMEM_HEAP_MIN_ALLOC_SIZE &&
      avail_buf_idx < mgr->_num_blocks) {
    XMEM_LOG("xmem_heap_alloc: avail buf idx (leading zero count) is %d\n",
             avail_buf_idx);
    /* Mark the new created block as allocated */
    xmem_toggle_bitvec(mgr->_block_free_bitvec, mgr->_num_blocks,
                       avail_buf_idx, 1);
    xmem_heap_block_t *new_block = &mgr->_blocks[avail_buf_idx];
    new_block->_block_size = curr_block->_block_size - new_size;
    new_block->_buffer = (void *)((uintptr_t)curr_block->_buffer+new_size);
    new_block->_aligned_buffer = new_block->_buffer;
    curr_block->_block_size = new_size;
    /* Add to free list */
    new_block->_next_block = curr_block->_next_block;
    prev_block->_next_block = new_block;
#ifdef XMEM_DEBUG
    xmem_heap_print_block("add_block_to_free_list", new_block, mgr);
#endif
  } else {
#pragma frequency_hint NEVER
    /* Run out of block headers. Cannot split further.
     * Allocate the whole block to this request.
     * TODO: Do reallocate and expand the block header if the block buffer
     *       is allocated from user pool */
    if (avail_buf_idx >= mgr->_num_blocks) {
#pragma frequency_hint NEVER
      XMEM_LOG("No available blocks. Failed to allocate %d bytes with "
               "alignment %d. Heap has %d free bytes and %d "
               "allocated bytes %d\n",
               (int32_t)size, (int32_t)align, (int32_t)mgr->_free_bytes,
               (int32_t)mgr->_allocated_bytes);
    }
    new_size = curr_block->_block_size;
    prev_block->_next_block = curr_block->_next_block;
  }

  /* Add the allocated block to beginning of the allocated list */
  curr_block->_next_block = mgr->_alloc_list_head._next_block;
  mgr->_alloc_list_head._next_block = curr_block;
  mgr->_free_bytes -= curr_block->_block_size;
  mgr->_allocated_bytes += curr_block->_block_size;
  XMEM_LOG("xmem_heap_alloc: Allocated at %p %d bytes, "
           "(requested %d bytes) at alignment %d. Heap has free bytes %d, "
           "allocated bytes %d\n", r, new_size, size, align, 
           mgr->_free_bytes, mgr->_allocated_bytes);

#ifdef XMEM_DEBUG
  xmem_heap_print_free_list(mgr);
  xmem_heap_print_alloc_list(mgr);
  xmem_heap_print_bitvec("bitvec", mgr->_block_free_bitvec, mgr->_num_blocks);
#endif

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_writeback(mgr->_blocks, mgr->_header_size);
#endif

#ifdef XMEM_VERIFY
  xmem_heap_verify(mgr);
#endif

#ifdef XMEM_VERIFY
  if ((uintptr_t)r != 0 && ((uintptr_t)r < (uintptr_t)mgr->_buffer ||
      ((uintptr_t)r >= ((uintptr_t)mgr->_buffer +
                        (uintptr_t)mgr->_buffer_size)))) {
    XMEM_ABORT("Allocated ptr %p not contained within buffer\n", r);
  }
#endif

fail:
  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

outer_fail:
  if (status)
    *status = err;

  return r;
}

/* Add a block to the free list that is sorted based on address. Coalesce
 * with the previous or next block if they happen to be contiguous.
 *
 * new_block : block to be added to free list
 * mgr       : memory manager
 */
static XMEM_INLINE void
xmem_heap_add_block_to_free_list(xmem_heap_block_t *new_block,
                                 xmem_heap_mgr_t *mgr)
{
  xmem_heap_block_t *block;
  /* Find block whose next free block's buffer address is >= buffer address of 
   * the new block to be added */
  for (block = &mgr->_free_list_head;
       (uintptr_t)block->_next_block->_buffer < (uintptr_t)new_block->_buffer;
       block = block->_next_block)
    ;
  int merge_with_prev = 0;
  int merge_with_next = 0;
  /* Check if the previous block can be merged with the new block */
  if (block != &mgr->_free_list_head) {
    if ((uintptr_t)block->_buffer + block->_block_size ==
        (uintptr_t)new_block->_buffer) {
      XMEM_LOG("Merging prev block with new_block\n");
#ifdef XMEM_DEBUG
      xmem_heap_print_block("prev block", block, mgr);
      xmem_heap_print_block("new block", new_block, mgr);
#endif
      block->_block_size += new_block->_block_size;
      uint32_t new_block_bv_idx =
        ((uintptr_t)new_block - (uintptr_t)&mgr->_blocks[0]) >>
          XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2;
      xmem_toggle_bitvec(mgr->_block_free_bitvec, mgr->_num_blocks,
                         new_block_bv_idx, 1);
      new_block = block;
      merge_with_prev = 1;
    }
  }

  /* Check if new_block (or new_block that is merged with previous from above)
   * can be merged with the next */
  if (block->_next_block != &mgr->_free_list_tail) {
    if ((uintptr_t)new_block->_buffer + new_block->_block_size ==
        (uintptr_t)block->_next_block->_buffer) {
      XMEM_LOG("Merging new block with next block\n");
#ifdef XMEM_DEBUG
      xmem_heap_print_block("new block", new_block, mgr);
      xmem_heap_print_block("next block", block->_next_block, mgr);
#endif
      uint32_t next_block_bv_idx =
        ((uintptr_t)block->_next_block - (uintptr_t)&mgr->_blocks[0]) >>
          XMEM_HEAP_BLOCK_STRUCT_SIZE_LOG2;
      xmem_toggle_bitvec(mgr->_block_free_bitvec, mgr->_num_blocks,
                         next_block_bv_idx, 1);
      new_block->_block_size += block->_next_block->_block_size;
      new_block->_next_block = block->_next_block->_next_block;
      if (merge_with_prev == 0) {
        block->_next_block = new_block;
      }
      merge_with_next = 1;
    }
  }

  if (merge_with_prev == 0 && merge_with_next == 0) {
    new_block->_next_block = block->_next_block;
    block->_next_block = new_block;
  }
}

/* Free buffer allocated from the heap manager and optionally clear freed memory
 *
 * mgr   : memory manager
 * p     : buffer to free
 * clear : clear freed memory
 *
 * Returns XMEM_OK if successful, else returns one of XMEM_ERR_PTR_OUT_OF_BOUNDS
 * or XMEM_ERR_INVALID_ARGS
 */
static XMEM_INLINE xmem_status_t
xmem_heap_free_internal(xmem_heap_mgr_t *mgr, void *p, int clear)
{
  xmem_status_t err = XMEM_OK;

  XMEM_LOG("xmem_heap_free_internal: Requesting freeing of %p\n", p);

  XMEM_CHECK(p != NULL, XMEM_ERR_PTR_OUT_OF_BOUNDS, 
             "xmem_heap_free_internal: Could not find p %p in heap\n", p);

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_free_internal: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_invalidate(mgr->_blocks, mgr->_header_size);
#endif

  xmem_heap_block_t *block;
  xmem_heap_block_t *prev_block;

  /* Check the allocated list to find the matching block that holds p */
  for (block = mgr->_alloc_list_head._next_block,
       prev_block = &mgr->_alloc_list_head;
       block != 0;
       prev_block = block, block = block->_next_block) {
    if (block->_aligned_buffer == p)
      break;
  }

  XMEM_CHECK_STATUS(block, err, XMEM_ERR_PTR_OUT_OF_BOUNDS, fail,
             "xmem_heap_free_internal: Could not find p %p in heap\n", p);

  /* Remove from allocated list and add to free list */
  prev_block->_next_block = block->_next_block;
  mgr->_free_bytes += block->_block_size;
  mgr->_allocated_bytes -= block->_block_size;
  mgr->_unused_bytes -= ((uintptr_t)block->_buffer -
                         (uintptr_t)block->_aligned_buffer);
  if (clear) {
    XMEM_LOG("Free with clear\n");
    memset(block->_buffer, 0, block->_block_size);
  }

  xmem_heap_add_block_to_free_list(block, mgr);

  XMEM_LOG("Free bytes %d, allocated bytes %d\n",
           mgr->_free_bytes, mgr->_allocated_bytes);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_writeback(mgr->_blocks, mgr->_header_size);
#endif

#ifdef XMEM_DEBUG
  xmem_heap_print_free_list(mgr);
  xmem_heap_print_alloc_list(mgr);
  xmem_heap_print_bitvec("bitvec", mgr->_block_free_bitvec, mgr->_num_blocks);
#endif

#ifdef XMEM_VERIFY
  xmem_heap_verify(mgr);
#endif

fail:
  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return err;
}

/* Free buffer allocated from the heap manager
 *
 * mgr : memory manager
 * p   : buffer to free
 *
 * Returns XMEM_OK if successful, else returns one of XMEM_ERR_PTR_OUT_OF_BOUNDS
 * or XMEM_ERR_INVALID_ARGS
 */
xmem_status_t
xmem_heap_free(xmem_heap_mgr_t *mgr, void *p)
{
  return xmem_heap_free_internal(mgr, p, 0);
}

/* Free buffer allocated from the heap manager and clear freed memory
 *
 * mgr : memory manager
 * p   : buffer to free
 *
 * Returns XMEM_OK if successful, else returns one of XMEM_ERR_PTR_OUT_OF_BOUNDS
 * or XMEM_ERR_INVALID_ARGS
 */
xmem_status_t
xmem_heap_free_with_clear(xmem_heap_mgr_t *mgr, void *p)
{
  return xmem_heap_free_internal(mgr, p, 1);
}

/* Return the usable size of the block after applying the alignment */
static XMEM_INLINE uint32_t 
xmem_heap_block_size(xmem_heap_block_t *block, uint32_t align)
{
  /* Align the block buffer address to the specified alignment */
  uint32_t p = xmem_find_msbit(align);
  uintptr_t aligned_block = (uintptr_t)block->_buffer;
  aligned_block = ((aligned_block + align - 1) >> p) << p;
  /* compute extra space required for alignment */
  size_t extra_size = aligned_block - (uintptr_t)block->_buffer ;
  return block->_block_size - extra_size;
}

/* Return the max contiguously avaiable free space in a heap memory after 
 * applying an alignment.
 * 
 * mgr              The heap memory manager object
 * align            The requested alignment
 * free_space_qual  Return the free space at start, end, or the max available
 * start_free_space If non-null, returns the location of the start of 
 *                  max free space (after alignment is applied)
 * end_free_space   If non-null, returns end of allocatable space
 *
 * Return the max free space and sets status to XMEM_OK if successful, else
 * returns one of XMEM_ERR_INVALID_ARGS, XMEM_ERR_ILLEGAL_ALIGN.
 */
size_t
xmem_heap_get_free_space(xmem_heap_mgr_t *mgr, uint32_t align, 
                         xmem_heap_free_space_qual_t free_space_qual,
                         void **start_free_space, void **end_free_space,
                         xmem_status_t *status)
{
  size_t r = 0;
  uintptr_t s_fs = 0, e_fs = 0;
  xmem_status_t err = XMEM_OK;

  XMEM_LOG("xmem_heap_get_free_space: Requesting free space at %d\n",
           free_space_qual);

  XMEM_CHECK_STATUS(mgr != NULL, err, XMEM_ERR_INVALID_ARGS, fail,
                    "xmem_heap_get_free_space: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_invalidate(mgr->_blocks, mgr->_header_size);
#endif

  xmem_heap_block_t *curr_block = mgr->_free_list_head._next_block;
  xmem_heap_block_t *prev_block = NULL;

#ifdef XMEM_DEBUG
  xmem_heap_print_block("xmem_heap_get_free_space: curr_block", 
                        curr_block, mgr);
#endif

  /* Compute block size based on alignment */
  size_t new_size = xmem_heap_block_size(curr_block, align);

  if (free_space_qual == XMEM_HEAP_FREE_SPACE_START && 
      curr_block != &mgr->_free_list_tail) {
    r = new_size;
    s_fs = (uintptr_t)curr_block->_buffer + curr_block->_block_size - new_size;
    e_fs = (uintptr_t)curr_block->_buffer + curr_block->_block_size - 1;
  } else {
    /* Check for the max sized block in the free list */
    while (curr_block != &mgr->_free_list_tail) {
      if (new_size > r) {
        r = new_size;
        s_fs = (uintptr_t)curr_block->_buffer + 
                          curr_block->_block_size - new_size;
        e_fs = (uintptr_t)curr_block->_buffer +
                          curr_block->_block_size - 1;
      }
      prev_block = curr_block;
      curr_block = curr_block->_next_block;
#ifdef XMEM_DEBUG
      xmem_heap_print_block("xmem_heap_get_free_space: curr_block", 
                            curr_block, mgr);
#endif
      new_size = xmem_heap_block_size(curr_block, align);
    }

    /* If end is set, return the size/start/end of free space at the end
     * of the manager's buffer pool. Note, the last buffer in the free list
     * is always at the highest address since the free list is sorted by
     * block address
     */
    if (free_space_qual == XMEM_HEAP_FREE_SPACE_END && prev_block) {
      uintptr_t end_of_prev_block = (uintptr_t)prev_block->_buffer +
                                    prev_block->_block_size - 1;
      if (end_of_prev_block == (uintptr_t)mgr->_buffer + 
                                          mgr->_buffer_size - 1) {
        r = xmem_heap_block_size(prev_block, align);
        s_fs = (uintptr_t)prev_block->_buffer + 
                          prev_block->_block_size - r;
        e_fs = (uintptr_t)prev_block->_buffer +
                          prev_block->_block_size - 1;
      } else {
        r = 0;
        s_fs = e_fs = 0;
      }
    }
  }

  XMEM_LOG("xmem_heap_get_free_space: r: %d, start: %p, end: %p\n",
           r, s_fs, e_fs);

  if (start_free_space)
    *start_free_space = (void *)s_fs;
  if (end_free_space)
    *end_free_space = (void *)e_fs;

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

fail:
  if (status)
    *status = err;

  return r;
}

/* Reset the heap manager state
 *
 * mgr : Pointer to heap manager object
 *
 * Returns XMEM_OK if successful, else XMEM_ERR_INVALID_ARGS
 */
xmem_status_t xmem_heap_reset(xmem_heap_mgr_t *mgr)
{
  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_reset: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_heap_mgr_t));
#endif

  /* Initialize free and allocated list */
  mgr->_free_list_head._block_size = 0;
  mgr->_free_list_head._buffer = 0;
  mgr->_free_list_head._aligned_buffer = 0;
  mgr->_free_list_tail._block_size = 0xffffffff;
  mgr->_free_list_tail._buffer = (void *)0xffffffff;
  mgr->_free_list_tail._aligned_buffer = (void *)0xffffffff;

  mgr->_alloc_list_head._block_size = 0;
  mgr->_alloc_list_head._buffer = 0;
  mgr->_alloc_list_head._aligned_buffer = 0;

  mgr->_allocated_bytes = 0;

  if (mgr->_has_external_header) {
    /* Initialize the first block with the start address of the buffer and
     * size as the whole buffer and add to the free list */
    mgr->_blocks[0]._block_size = mgr->_buffer_size;
    mgr->_blocks[0]._buffer = mgr->_buffer;
    mgr->_blocks[0]._aligned_buffer = mgr->_buffer;

    mgr->_free_list_head._next_block = &mgr->_blocks[0];
    mgr->_free_list_tail._next_block = 0;
    mgr->_blocks[0]._next_block = &mgr->_free_list_tail;

    mgr->_alloc_list_head._next_block = 0;

    /* Set the first block as in use (set to 0) and rest as 
     * available (set to 1) */
    mgr->_block_free_bitvec[0] = 0xfffffffe;

    /* Initialize the rest as available */
    int i;
    for (i = 1; i < (mgr->_num_blocks + 31) >> 5; ++i) {
      mgr->_block_free_bitvec[i] = 0xffffffff;
    }

    mgr->_free_bytes = mgr->_buffer_size;
    mgr->_unused_bytes = 0;
  } else {
    /* The header (array of xmem_heap_block_t and the bitvector) is placed at 
     * the start of buf. Data allocation begins after that. Initialize the first
     * block with the header address (start of buf) and header size. */
    mgr->_blocks[0]._block_size = mgr->_header_size;
    mgr->_blocks[0]._buffer = mgr->_buffer;
    mgr->_blocks[0]._aligned_buffer = mgr->_buffer;

    mgr->_alloc_list_head._next_block = &mgr->_blocks[0];
    mgr->_blocks[0]._next_block = 0;

    /* Initialize the second block with the address of the area in buffer 
     * following the xmem_heap_block_t array and bitvector that is available for
     * user allocation. Size is set as the buffer size minus the space required
     * for the block array and bitvector */
    mgr->_blocks[1]._block_size = mgr->_buffer_size - 
                                  mgr->_blocks[0]._block_size;
    mgr->_blocks[1]._buffer =  (void *)((uintptr_t)mgr->_blocks[0]._buffer +
                                         mgr->_blocks[0]._block_size);
    mgr->_blocks[1]._aligned_buffer = mgr->_blocks[1]._buffer;

    mgr->_free_list_head._next_block = &mgr->_blocks[1];
    mgr->_free_list_tail._next_block = 0;
    mgr->_blocks[1]._next_block = &mgr->_free_list_tail;

    /* Set the first two blocks as in use (set to 0) and rest as 
     * available (set to 1) */
    mgr->_block_free_bitvec[0] = 0xfffffffc;

    /* Initialize the rest as available */
    int i;
    for (i = 2; i < (mgr->_num_blocks + 31) >> 5; ++i) {
      mgr->_block_free_bitvec[i] = 0xffffffff;
    }

    mgr->_free_bytes = mgr->_blocks[1]._block_size;
    mgr->_unused_bytes = mgr->_blocks[0]._block_size;
    mgr->_allocated_bytes = mgr->_unused_bytes;
  }

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_heap_mgr_t));
  if (mgr->_has_external_header)
    xthal_dcache_region_writeback(mgr->_blocks, mgr->_header_size);
#endif

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return XMEM_OK;
}

/* Check if pointer is in bounds of the allocated space of the manager
 *
 * mgr : Pointer to manager object
 * p   : Pointer to check for bounds
 *
 * Returns XMEM_OK if in bounds, else returns one of XMEM_ERR_INVALID_ARGS or
 * XMEM_ERR_PTR_OUT_OF_BOUNDS
 */
xmem_status_t xmem_heap_check_bounds(xmem_heap_mgr_t *mgr, void *p)
{
  xmem_status_t err = XMEM_OK;

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS, 
             "xmem_heap_check_bounds: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_heap_mgr_t));
#endif

  /* Check the allocated list to find if p lies within the block */
  int in_bounds = 0;
  xmem_heap_block_t *block;
  for (block = mgr->_alloc_list_head._next_block;
       block != 0; block = block->_next_block) {
    if ((uintptr_t)p >= (uintptr_t)block->_buffer &&
        (uintptr_t)p < ((uintptr_t)block->_buffer + block->_block_size)) {
      in_bounds = 1;
      break;
    }
  }

  if (!in_bounds)
    err = XMEM_ERR_PTR_OUT_OF_BOUNDS;

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return err;
}
