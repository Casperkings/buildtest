#include <xtensa/config/core-isa.h>
#include "xmem.h"
#include "xmem_bank.h"
#include "xlmem.h"
#include "xmem_misc.h"

/* Structure to manage L2 */
typedef struct {
  int              *_initialized;
  volatile int32_t *_lock;
  xmem_heap_mgr_t  *_hm;
} xmem_l2_mgr_t;

/* L2 configuration discovery */
#if XCHAL_HAVE_L2
extern uint32_t Xthal_L2ram_size;
extern char _memmap_mem_l2ram_max;
#endif

// Number of blocks for the heap manager
#define XMEM_BANK_HEAP_MGR_NUM_BLOCKS (64)

// Statically reserve L2 manager in core local memory
static xmem_l2_mgr_t xmem_l2_mgr XMEM_BANK_PREFERRED_BANK = {
#if XCHAL_HAVE_L2
  // Place at start of L2 memory and visible to all cores
  ._initialized = (void *)&_memmap_mem_l2ram_max,
  ._lock        = (void *)((uintptr_t)&_memmap_mem_l2ram_max + 4),
  ._hm          = (void *)((uintptr_t)&_memmap_mem_l2ram_max + 8)
#else
  ._initialized = NULL,
  ._lock        = NULL,
  ._hm          = NULL
#endif
};

/* Helper function to convert from xmem_status_t to xmem_bank_status_t */
static xmem_bank_status_t xmem_bank_get_status(xmem_status_t s) {
  if (s == XMEM_OK)
    return XMEM_BANK_OK;
  else if (s == XMEM_ERR_INVALID_ARGS)
    return XMEM_BANK_ERR_INVALID_ARGS;
  else if (s == XMEM_ERR_ALLOC_FAILED)
    return XMEM_BANK_ERR_ALLOC_FAILED;
  else if (s == XMEM_ERR_ILLEGAL_ALIGN)
    return XMEM_BANK_ERR_ILLEGAL_ALIGN;
  else if (s == XMEM_ERR_PTR_OUT_OF_BOUNDS)
    return XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS;
  else
    return XMEM_BANK_ERR_INTERNAL;
}

/* Initialize L2 with the heap memory manager.
 * 
 * Returns XMEM_BANK_OK if successful, else returns one of 
 * XMEM_BANK_ERR_INIT_BANKS_FAIL, XMEM_BANK_ERR_INIT_MGR, 
 * XMEM_BANK_ERR_CONFIG_UNSUPPORTED.
 */
xmem_bank_status_t 
xmem_init_l2_mem()
{
#if !XCHAL_HAVE_L2
  return XMEM_BANK_ERR_CONFIG_UNSUPPORTED;
#else
  // Start at the end of statically allocated data in L2
  void *start = (void *)&_memmap_mem_l2ram_max;

  // The size is to be discovered at runtime since it can vary 
  // based on what fraction is cache vs. RAM. Account for any statically
  // allocated data.
  uint32_t size = Xthal_L2ram_size - 
                  ((uintptr_t)&_memmap_mem_l2ram_max - 
                   (uintptr_t)XCHAL_L2RAM_RESET_PADDR);

  unsigned heap_mgr_header_size =
           XMEM_HEAP_HEADER_SIZE(XMEM_BANK_HEAP_MGR_NUM_BLOCKS);

  XMEM_CHECK(size > sizeof(xmem_heap_mgr_t)+heap_mgr_header_size+8,
             XMEM_BANK_ERR_INIT_BANKS_FAIL,
             "xmem_init_l2_mem: Expecting > %d bytes in L2\n",
             sizeof(xmem_heap_mgr_t)+heap_mgr_header_size);

  XMEM_LOG("xmem_init_l2_mem: Start = %p, reserved = %d, size = %d bytes\n", 
           start, ((uintptr_t)&_memmap_mem_l2ram_max - 
                   (uintptr_t)XCHAL_L2RAM_RESET_PADDR), size);

  // Setup space for xmem_l2_mgr._initialized, ._lock, and heap manager
  start = (void *)((uintptr_t)start + 8 + sizeof(xmem_heap_mgr_t));
  size -= (8 + sizeof(xmem_heap_mgr_t));

  // Setup base of the heap memory manager headers. Follow the heap manager
  char *xmem_l2_heap_mgr_header = (char *)start;

  XMEM_LOG("xmem_init_l2_mem: 4 bytes of xmem_l2_mgr._initialized at %p, "
           " 4 bytes of xmem_l2_mgr._lock at %p\n ",
           xmem_l2_mgr._initialized, xmem_l2_mgr._lock);

  XMEM_LOG("xmem_init_l2_mem: Allocating %d bytes for L2 heap "
           "managers at %p\n", sizeof(xmem_heap_mgr_t), xmem_l2_mgr._hm);

  XMEM_LOG("xmem_init_l2_mem: Allocating %d bytes for L2 heap "
           "manager headers at %p\n", heap_mgr_header_size,
           xmem_l2_heap_mgr_header);

  // Adjust size and start of L2 to account for manager and header
  start = (void *)((uintptr_t)start + heap_mgr_header_size);
  size -= heap_mgr_header_size;

  // Align to load/store width
  uint32_t align = XCHAL_DATA_WIDTH;
  uint32_t p = xmem_find_msbit(align);
  uintptr_t aligned_start = (((uintptr_t)start + align - 1) >> p) << p;
    
  XMEM_LOG("xmem_init_l2_mem: Reserving %d bytes in L2 for "
           "aligning to data width of %d bytes\n", 
           aligned_start - (uintptr_t)start, XCHAL_DATA_WIDTH);

  size -= (aligned_start - (uintptr_t)start);
  start = (void *)aligned_start;

  // Initialize heap memory manager
  XMEM_CHECK(xmem_heap_init(xmem_l2_mgr._hm, start, size,
                            XMEM_BANK_HEAP_MGR_NUM_BLOCKS,
                            xmem_l2_heap_mgr_header) == XMEM_OK, 
             XMEM_BANK_ERR_INIT_MGR, "xmem_init_l2_mem: "
             "Error initializing heap memory manager for L2\n");

  // Initialize lock
  *xmem_l2_mgr._lock = 0;
  xmem_heap_set_lock(xmem_l2_mgr._hm, xmem_l2_mgr._lock);

  // L2 is now initialized
  *xmem_l2_mgr._initialized = 1;

#pragma flush_memory

  return XMEM_BANK_OK;
#endif
}

/* Helper function to allocate from the heap memory manager */
static XMEM_INLINE void *
xmem_l2_heap_do_alloc(xmem_heap_mgr_t *mgr, size_t size, 
                      uint32_t align, xmem_bank_status_t *status)
{
  xmem_status_t xs;
  void *p = xmem_heap_alloc(mgr, size, align, &xs);
  xmem_bank_status_t xbs = xmem_bank_get_status(xs);
  if (status)
    *status = xbs;
  return p;
}

/* Allocate memory from L2 of given size and alignment. 
 *
 * size   : Size to allocate
 * align  : Requested alignment
 * status : Optional. If successful set to XMEM_OK, else set to one of
 *          XMEM_BANK_ERR_ILLEGAL_ALIGN, XMEM_BANK_ERR_ALLOC_FAILED,
 *          XMEM_BANK_ERR_UNINITIALIZED
 */
void *xmem_l2_alloc(size_t size, uint32_t align, xmem_bank_status_t *status)
{
  void *p = NULL;
  xmem_bank_status_t xbs;

  XMEM_CHECK_STATUS(*xmem_l2_mgr._initialized, xbs, XMEM_BANK_ERR_UNINITIALIZED,
                    fail, "xmem_l2_alloc: L2 not initialized\n");

  /* Expect alignment to be a power-of-2 */
  XMEM_CHECK_STATUS(align && !(align & (align-1)),
                    xbs, XMEM_BANK_ERR_ILLEGAL_ALIGN, fail,
                    "xmem_l2_alloc: alignment should be a power of 2\n");

  p = xmem_l2_heap_do_alloc(xmem_l2_mgr._hm, size, align, &xbs);

fail:
  if (status)
    *status = xbs;

  XMEM_LOG("xmem_l2_alloc: Allocating %d bytes, align %d, from L2 at %p\n",
           size, align, p);

  return p;
}

/* Return the amount of free space in L2 after taking into account the
 * alignment. 
 * 
 * align            The requested alignment
 * start_free_space If non-null, returns the location of the start of 
 *                  free space (after alignment is applied)
 * end_free_space   If non-null, returns end of allocatable space
 *
 * Return the free space and sets status to XMEM_BANK_OK if successful, else
 * returns one of XMEM_BANK_ERR_UNINITIALIZED or XMEM_BANK_ERR_ILLEGAL_ALIGN.
 */
size_t
xmem_l2_get_free_space(uint32_t align, void **start_free_space, 
                       void **end_free_space, xmem_bank_status_t *status)
{
  xmem_bank_status_t xbs;
  size_t r = 0;

  XMEM_CHECK_STATUS(*xmem_l2_mgr._initialized, xbs, XMEM_BANK_ERR_UNINITIALIZED,
                    fail, "xmem_l2_get_free_space: L2 not initialized\n");

  xmem_status_t xs;
  r = xmem_heap_get_free_space(xmem_l2_mgr._hm, align, 
                               XMEM_HEAP_MAX_FREE_SPACE,
                               start_free_space, end_free_space, &xs);
  xbs = xmem_bank_get_status(xs);

fail:
  if (status)
    *status = xbs;

  XMEM_LOG("xmem_l2_get_free_space: Start %p, end %p, size %d bytes\n",
           start_free_space ? *start_free_space : 0,
           end_free_space   ? *end_free_space : 0,
           r);

  return r;
}

/* Reset the L2 allocation state 
 *
 * Returns XMEM_BANK_OK if successful, else returns XMEM_BANK_ERR_UNINITIALIZED
 */
xmem_bank_status_t 
xmem_l2_reset()
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_l2_reset: L2 not initialized\n");
  
  xmem_status_t err = xmem_heap_reset(xmem_l2_mgr._hm);

  return xmem_bank_get_status(err);
}

/* Check if pointer is in bounds of the manager
 *
 * p : Pointer to check for bounds
 *
 * Returns XMEM_BANK_OK if in bounds, else returns one of 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t 
xmem_l2_check_bounds(void *p)
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_l2_check_bounds: L2 not initialized\n");

  xmem_status_t err = xmem_heap_check_bounds(xmem_l2_mgr._hm, p);

  return xmem_bank_get_status(err);
}

/* Free memory allocated via the heap manager
 *
 * p : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_l2_free(void *p)
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_l2_free: L2 not initialized\n");

  xmem_status_t err = xmem_heap_free(xmem_l2_mgr._hm, p);

  XMEM_LOG("xmem_l2_free: Freeing %p from L2\n", p);

  return xmem_bank_get_status(err);
}

/* Free and clears memory allocated via the heap manager
 *
 * p : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_l2_free_with_clear(void *p)
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_l2_free_with_clear: L2 not initialized\n");

  xmem_status_t err = xmem_heap_free_with_clear(xmem_l2_mgr._hm, p);

  XMEM_LOG("xmem_l2_free_with_clear: Freeing %p from L2\n", p);

  return xmem_bank_get_status(err);
}

/* Returns the free bytes in L2 */
ssize_t
xmem_l2_get_free_bytes()
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, -1,
             "xmem_l2_get_free_bytes: L2 not initialized\n");

  return xmem_heap_get_free_bytes(xmem_l2_mgr._hm);
}

/* Returns the allocated bytes in L2 */
ssize_t
xmem_l2_get_allocated_bytes()
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, -1,
             "xmem_l2_get_allocated_bytes: L2 not initialized\n");

  return xmem_heap_get_allocated_bytes(xmem_l2_mgr._hm);
}

/* Returns the unused bytes in L2 */
ssize_t
xmem_l2_get_unused_bytes()
{
  XMEM_CHECK(*xmem_l2_mgr._initialized, -1,
             "xmem_l2_get_unused_bytes: L2 not initialized\n");

  return xmem_heap_get_unused_bytes(xmem_l2_mgr._hm);
}
