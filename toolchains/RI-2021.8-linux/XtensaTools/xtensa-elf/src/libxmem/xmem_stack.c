#include "xmem_mgr.h"
#include "xmem_misc.h"

/* Stack manager usage statistics 
 *
 * mgr : stack manager
 *
 * Returns void
 */
__attribute__((unused)) static void
xmem_stack_print_stats(xmem_stack_mgr_t *mgr)
{
  XMEM_LOG("Stack buffer @ %p, size: %d, free bytes: %d, allocated bytes: %d, "
           "unused bytes: %d, percent overhead: %d\n",
           mgr->_buffer,
           mgr->_buffer_size,
           mgr->_buffer_end - mgr->_buffer_p + 1,
           mgr->_buffer_p - (uintptr_t)mgr->_buffer,
           mgr->_unused_bytes,
           (mgr->_unused_bytes*100)/mgr->_buffer_size);
}

/* Return stack manager unused bytes
 *
 * mgr : stack manager
 *
 * Returns stack manager unused bytes
 */
size_t xmem_stack_get_unused_bytes(xmem_stack_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_stack_get_unused_bytes: mgr is null\n");
  return mgr->_unused_bytes;
}

/* Return stack manager free bytes
 *
 * mgr : stack manager
 *
 * Returns stack manager free bytes
 */
size_t xmem_stack_get_free_bytes(xmem_stack_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_stack_get_free_bytes: mgr is null\n");
  return mgr->_buffer_end - mgr->_buffer_p + 1;
}

/* Return stack manager allocated bytes
 *
 * mgr : stack manager
 *
 * Returns stack manager allocated bytes
 */
size_t xmem_stack_get_allocated_bytes(xmem_stack_mgr_t *mgr) {
  XMEM_CHECK(mgr != NULL, SIZE_MAX,
             "xmem_stack_get_allocated_bytes: mgr is null\n");
  return mgr->_buffer_p - (uintptr_t)mgr->_buffer;
}

/* Initialize stack memory manager.
 *
 * mgr    : memory manager
 * buffer : user provided buffer for allocation
 * size   : size of the buffer
 *
 * Returns XMEM_OK if successful, else returns one of XMEM_ERR_INTERNAL
 * or XMEM_ERR_INVALID_ARGS.
 */
xmem_status_t
xmem_stack_init(xmem_stack_mgr_t *mgr, void *buffer, size_t size)
{
  // Internal consistency checks
  XMEM_CHECK(sizeof(xmem_stack_mgr_t) == XMEM_STACK_MGR_SIZE,
             XMEM_ERR_INTERNAL,
             "xmem_stack_init: Sizeof xmem_stack_mgr_t expected to be %d, "
             "but got %d\n", XMEM_STACK_MGR_SIZE, sizeof(xmem_stack_mgr_t));

  XMEM_CHECK(sizeof(xmem_stack_mgr_checkpoint_t) == 
             XMEM_STACK_MGR_CHECKPOINT_SIZE,
             XMEM_ERR_INTERNAL,
             "xmem_stack_init: Sizeof xmem_stack_mgr_checkpoint_t expected "
             "to be %d, but got %d\n", XMEM_STACK_MGR_CHECKPOINT_SIZE, 
             sizeof(xmem_stack_mgr_checkpoint_t));

  XMEM_CHECK(buffer != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_init: buffer is null\n");

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_init: mgr is null\n");

  mgr->_lock             = NULL;
  mgr->_buffer           = buffer;
  mgr->_buffer_size      = size;
  mgr->_buffer_p         = (uintptr_t)buffer;
  mgr->_buffer_end       = (uintptr_t)buffer + size - 1;
  mgr->_unused_bytes     = 0;

  XMEM_LOG("xmem_stack_init: Initializing manager %p, buffer %p, size %d\n", 
           mgr, buffer, size);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  return XMEM_OK;
}

/* Set lock object
 *
 * mgr  : stack memory manager
 * lock : lock object provided buffer for allocation
 *
 * Returns XMEM_OK, if successful, else returns XMEM_ERR_INVALID_ARGS
 */
xmem_status_t
xmem_stack_set_lock(xmem_stack_mgr_t *mgr, volatile void *lock) 
{
  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_mgr_set_lock: mgr is null\n");

  mgr->_lock = lock;
  return XMEM_OK;
}
  
/* Allocate a buffer of requested size and alignment from the stack memory
 * manager
 *
 * mgr    : memory manager
 * size   : size of the buffer
 * align  : alignment of the buffer
 * status : error code if any is returned
 *
 * Returns a pointer to the allocated buffer and sets status to XMEM_OK, 
 * else returns NULL and sets status to one of XMEM_ERR_ALLOC_FAILED,
 * XMEM_ERR_ILLEGAL_ALIGN, or XMEM_ERR_INVALID_ARGS.
 */
void *
xmem_stack_alloc(xmem_stack_mgr_t *mgr, size_t size, uint32_t align,
                 xmem_status_t *status)
{
  uintptr_t r = 0;
  xmem_status_t err = XMEM_OK;

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  XMEM_CHECK_STATUS(mgr != NULL, err, XMEM_ERR_INVALID_ARGS, fail,
                    "xmem_stack_alloc: mgr is null\n");

  /* Expect alignment to be a power-of-2 */
  XMEM_CHECK_STATUS(align && !(align & (align-1)), err, 
                    XMEM_ERR_ILLEGAL_ALIGN, fail,
                    "xmem_stack_alloc: alignment should be a power of 2\n");

  size_t unused_bytes = mgr->_unused_bytes;
  uintptr_t buffer_p = mgr->_buffer_p;
  uintptr_t buffer_end = mgr->_buffer_end;

  /* Round up to align bytes */
  uint32_t p = xmem_find_msbit(align);
  uintptr_t new_buffer_p = ((buffer_p + align - 1) >> p) << p;

  XMEM_LOG("xmem_stack_alloc: before alloc: buffer_p: %p buffer_end: %p, "
           "new_buffer_p: %p, unused_bytes: %d, align: %d, size: %d, "
           "rem size: %d\n", mgr->_buffer_p, mgr->_buffer_end, new_buffer_p,
           mgr->_unused_bytes, align, size, (buffer_end-buffer_p+1));

  /* Check if the requested size + aligned buffer pointer exceeds the end of
   * buffer space. If yes, the allocation fails, else returns the buffer
   * at the alignment */
  if (new_buffer_p <= buffer_end && (size + new_buffer_p <= (buffer_end+1))) {
    r = new_buffer_p;
    mgr->_buffer_p = new_buffer_p + size;
    unused_bytes += (new_buffer_p - buffer_p);
    mgr->_unused_bytes = unused_bytes;
#ifdef XMEM_VERIFY
    if (mgr->_buffer_p > (mgr->_buffer_end+1)) {
      XMEM_ABORT("xmem_stack_alloc: buffer_p: %p exceeds buffer_end %p\n", 
                 mgr->_buffer_p, mgr->_buffer_end+1);
    }
#endif
    XMEM_LOG("xmem_stack_alloc: after alloc: buffer_p: %p buffer_end: %p, "
             "unused_bytes: %d, align: %d, size: %d, rem size: %d\n", 
             mgr->_buffer_p, mgr->_buffer_end, mgr->_unused_bytes, 
             align, size, (buffer_end-mgr->_buffer_p+1));
  } else {
#pragma frequency_hint NEVER
    XMEM_LOG("xmem_stack_alloc: Could not allocate %d bytes\n", size);
    err = XMEM_ERR_ALLOC_FAILED;
    goto fail;
  }

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

fail:
  if (status)
    *status = err;

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return (void *)r;
}

/* Return the free space in a stack memory after applying an alignment.
 * 
 * mgr              The stack memory manager object
 * align            The requested alignment
 * start_free_space If non-null, returns the location of the start of 
 *                  free space (after alignment is applied)
 * end_free_space   If non-null, returns end of allocatable space
 *
 * Return the free space and sets status to XMEM_OK if successful, else
 * returns one of XMEM_ERR_INVALID_ARGS, XMEM_ERR_ILLEGAL_ALIGN.
 */
size_t
xmem_stack_get_free_space(xmem_stack_mgr_t *mgr, uint32_t align,
                          void **start_free_space, void **end_free_space,
                          xmem_status_t *status)
{
  size_t r = 0;
  uintptr_t s_fs = 0, e_fs = 0;
  xmem_status_t err = XMEM_OK;

  XMEM_CHECK_STATUS(mgr != NULL, err, XMEM_ERR_INVALID_ARGS, fail,
                    "xmem_stack_get_free_space: mgr is null\n");

  /* Expect alignment to be a power-of-2 */
  XMEM_CHECK_STATUS(align && !(align & (align-1)), err, 
                    XMEM_ERR_ILLEGAL_ALIGN, fail,
                    "xmem_stack_get_free_space: alignment should be a "
                    "power of 2\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  uintptr_t buffer_p = mgr->_buffer_p;
  uintptr_t buffer_end = mgr->_buffer_end;
  /* Round up to align bytes */
  uint32_t p = xmem_find_msbit(align);
  uintptr_t new_buffer_p = ((buffer_p + align - 1) >> p) << p;
  if (new_buffer_p <= buffer_end) {
    r = buffer_end - new_buffer_p + 1;
    s_fs = new_buffer_p;
    e_fs = buffer_end;
  }

  XMEM_LOG("xmem_stack_get_free_space: r: %d, start: %p, end: %p\n",
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

/* Saves the stack manager object state
 *
 * mgr    Pointer to the stack memory manager object
 * mgr_cp Pointer to stack manager checkpoint object
 * 
 * Returns XMEM_OK if successful, else returns XMEM_ERR_INVALID_ARGS
 */
xmem_status_t xmem_stack_checkpoint_save(xmem_stack_mgr_t *mgr,
                                         xmem_stack_mgr_checkpoint_t *mgr_cp)
{
  xmem_status_t err = XMEM_OK;

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
  xthal_dcache_region_invalidate((void *)mgr_cp, 
                                 sizeof(xmem_stack_mgr_checkpoint_t));
#endif

  XMEM_CHECK_STATUS(mgr != NULL, err, XMEM_ERR_INVALID_ARGS, fail,
                    "xmem_stack_checkpoint_save: mgr is null\n");

  XMEM_CHECK_STATUS(mgr_cp != NULL, err, XMEM_ERR_INVALID_ARGS, fail,
                    "xmem_stack_checkpoint_save: mgr_cp is null\n");

  mgr_cp->_buffer_p = mgr->_buffer_p;
  mgr_cp->_unused_bytes = mgr->_unused_bytes;

  XMEM_LOG("xmem_stack_checkpoint_save: buffer_p: %p, unused_bytes: %d\n",
           mgr_cp->_buffer_p, mgr_cp->_unused_bytes);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr_cp, 
                                sizeof(xmem_stack_mgr_checkpoint_t));
#endif

fail:
  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return err;
}

/* Restores the stack manager object state
 *
 * mgr    Pointer to the stack memory manager object
 * mgr_cp Pointer to stack manager checkpoint object
 * 
 * Returns XMEM_OK if successful, else returns XMEM_ERR_INVALID_ARGS
 */
xmem_status_t xmem_stack_checkpoint_restore(xmem_stack_mgr_t *mgr,
                                            xmem_stack_mgr_checkpoint_t *mgr_cp)
{
  xmem_status_t err = XMEM_OK;

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_checkpoint_restore: mgr is null\n");

  XMEM_CHECK(mgr_cp != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_checkpoint_restore: mgr_cp is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
  xthal_dcache_region_invalidate((void *)mgr_cp, 
                                 sizeof(xmem_stack_mgr_checkpoint_t));
#endif

  mgr->_buffer_p = mgr_cp->_buffer_p;
  mgr->_unused_bytes = mgr_cp->_unused_bytes;

  XMEM_LOG("xmem_stack_checkpoint_restore: buffer_p: %p, unused_bytes: %d\n",
           mgr_cp->_buffer_p, mgr_cp->_unused_bytes);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return err;
}

/* Reset the stack manager state
 *
 * mgr : Pointer to stack manager object
 *
 * Returns XMEM_OK if successful, else XMEM_ERR_INVALID_ARGS
 */
xmem_status_t xmem_stack_reset(xmem_stack_mgr_t *mgr)
{
  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_reset: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  mgr->_buffer_p     = (uintptr_t)mgr->_buffer;
  mgr->_unused_bytes = 0;

  XMEM_LOG("xmem_stack_reset buffer %p\n", mgr->_buffer_p);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_writeback((void *)mgr, sizeof(xmem_stack_mgr_t));
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
xmem_status_t xmem_stack_check_bounds(xmem_stack_mgr_t *mgr, void *p)
{
  xmem_status_t err = XMEM_OK;

  XMEM_CHECK(mgr != NULL, XMEM_ERR_INVALID_ARGS,
             "xmem_stack_check_bounds: mgr is null\n");

  xmem_disable_preemption();
  xmem_lock(mgr->_lock);

#if XCHAL_DCACHE_SIZE > 0 && !XCHAL_DCACHE_IS_COHERENT
  xthal_dcache_region_invalidate((void *)mgr, sizeof(xmem_stack_mgr_t));
#endif

  if ((uintptr_t)p < (uintptr_t)mgr->_buffer || (uintptr_t)p >= mgr->_buffer_p)
    err = XMEM_ERR_PTR_OUT_OF_BOUNDS;

  xmem_unlock(mgr->_lock);
  xmem_enable_preemption();

  return err;
}
