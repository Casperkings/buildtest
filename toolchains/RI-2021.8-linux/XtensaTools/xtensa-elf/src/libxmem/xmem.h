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

#ifndef __XMEM_H__
#define __XMEM_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <xtensa/config/core-isa.h>

/*! Minimum size of a stack memory manager object in bytes. */
#define XMEM_STACK_MGR_SIZE            (24)

/*! Minimum size of a stack memory manager checkpoint object in bytes. */
#define XMEM_STACK_MGR_CHECKPOINT_SIZE (8)

/*! Minimum size of a heap memory manager object in bytes. */
#define XMEM_HEAP_MGR_SIZE             (88)

#ifndef __XMEM_MGR_H__
/* Override this by including <xtensa/system/xmp_subsystem.h> on an 
 * MP-subsystem, where XMP_MAX_DCACHE_LINESIZE is the maximum cache line
 * size across all cores in the subsystem 
 */
#ifndef XMP_MAX_DCACHE_LINESIZE
  #if XCHAL_DCACHE_SIZE>0
    #define XMEM_MAX_DCACHE_LINESIZE XCHAL_DCACHE_LINESIZE
  #else
    #define XMEM_MAX_DCACHE_LINESIZE 4
#endif
#else
  #define XMEM_MAX_DCACHE_LINESIZE XMP_MAX_DCACHE_LINESIZE
#endif

struct xmem_stack_mgr_struct {
  char _[((XMEM_STACK_MGR_SIZE+XMEM_MAX_DCACHE_LINESIZE-1)/XMEM_MAX_DCACHE_LINESIZE)*XMEM_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMEM_MAX_DCACHE_LINESIZE)));

struct xmem_heap_mgr_struct {
  char _[((XMEM_HEAP_MGR_SIZE+XMEM_MAX_DCACHE_LINESIZE-1)/XMEM_MAX_DCACHE_LINESIZE)*XMEM_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMEM_MAX_DCACHE_LINESIZE)));

struct xmem_stack_mgr_checkpoint_struct {
  char _[((XMEM_STACK_MGR_CHECKPOINT_SIZE+XMEM_MAX_DCACHE_LINESIZE-1)/XMEM_MAX_DCACHE_LINESIZE)*XMEM_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMEM_MAX_DCACHE_LINESIZE)));

#endif // __XMEM_MGR_H__

/*! Minimum size of a heap memory manager block in bytes */
#define XMEM_HEAP_BLOCK_STRUCT_SIZE       (16)

/*!
 * Macro to returns the number of bytes required to hold the header information 
 * for the heap memory manager. The header includes an array of block 
 * structures that are used to track the free and
 * allocated buffers in the pool and a bitvector to track 
 * available block structures.
 * The caller can use this macro to reserve bytes for the header that may be  
 * passed optionally to \ref xmem_heap_init.
 * The macro takes as parameter the number of blocks avaiable for the heap
 * manager to track the allocate/free buffers in the pool.
 * Use this macro to compute the header size on non cache-based subsystems.
 */
#define XMEM_HEAP_HEADER_SIZE(num_blocks) \
  (XMEM_HEAP_BLOCK_STRUCT_SIZE*(num_blocks)+(((num_blocks) + 31) >> 5) * 4)

/*!
 * Same as the macro \ref XMEM_HEAP_HEADER_SIZE but the computed size 
 * is aligned to the maximum data cache line size across all the cores 
 * in the subsystem. Use this macro to compute the header size on cache-based
 * subsystems.
 */
#define XMEM_HEAP_CACHE_ALIGNED_HEADER_SIZE(num_blocks) \
  (((XMEM_HEAP_HEADER_SIZE((num_blocks))+\
    XMEM_MAX_DCACHE_LINESIZE-1)/XMEM_MAX_DCACHE_LINESIZE)*\
    XMEM_MAX_DCACHE_LINESIZE)


/*! 
 * Type to represent the stack memory manager object. 
 * The minimum size of an object of this type is 
 * \ref XMEM_STACK_MGR_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xmem_stack_mgr_struct xmem_stack_mgr_t;

/*! 
 * Type to check point the state of a stack memory manager object. 
 * The minimum size of an object of this type is 
 * \ref XMEM_STACK_MGR_CHECKPOINT_SIZE. For a cached subsystem, the size is 
 * rounded aligned to the maximum dcache line size across all cores in the
 * subsystem. */
typedef struct xmem_stack_mgr_checkpoint_struct xmem_stack_mgr_checkpoint_t;

/*! 
 * Type to represent the heap memory manager object. 
 * The minimum size of an object of this type is 
 * \ref XMEM_HEAP_MGR_SIZE. For a cached subsystem, the size is rounded 
 * and aligned to the maximum dcache line size across all cores in the 
 * subsystem. */
typedef struct xmem_heap_mgr_struct xmem_heap_mgr_t;

/*! 
 * Type used to represent the return value of the xmem API calls.
 */
typedef enum {
  /*! Memory allocation failed */
  XMEM_ERR_ALLOC_FAILED       = -100,

  /*! Illegal alignment value */
  XMEM_ERR_ILLEGAL_ALIGN      = -99,

  /*! Invalid arguments to API */
  XMEM_ERR_INVALID_ARGS       = -98,

  /*! Pointer is not contained in any of the banks */
  XMEM_ERR_PTR_OUT_OF_BOUNDS  = -97,

  /*! User provided pool does not have the required mimimum size */ 
  XMEM_ERR_POOL_SIZE          = -96,

  /*! Internal error */
  XMEM_ERR_INTERNAL           = -1,

  /*! No error */
  XMEM_OK                     = 0
} xmem_status_t;

/*! 
 * Type to qualify where to find the free space in the heap manager's pool
 */
typedef enum {
  /*! Find the largest available chunk of free space in the heap */
  XMEM_HEAP_MAX_FREE_SPACE   = 0,
  
  /*! Find the free space at the beginning of the pool */
  XMEM_HEAP_FREE_SPACE_START = 1,
  
  /*! Find the free space at the end of the pool */
  XMEM_HEAP_FREE_SPACE_END   = 2
} xmem_heap_free_space_qual_t;

/*! Initialize the stack memory manager.
 *
 * \param mgr    Pointer to stack memory manager object.
 * \param buffer User provided pool for allocation.
 * \param size   Size of the pool.
 * \return       XMEM_OK if successful, else returns XMEM_ERR_INTERNAL or
 *               XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t
xmem_stack_init(xmem_stack_mgr_t *mgr, void *buffer, size_t size);

/*! 
 * Allocate a buffer of given input size and alignment using the 
 * stack memory manager.
 * 
 * \param mgr    Pointer to the stack memory manager object.
 * \param size   Size of the buffer to allocate.
 * \param align  Requested alignment of the buffer. Must be a power of 2.
 * \param status Error status.
 * \return       Pointer to the allocated buffer. If the allocation succeeds,
 *               the \a status is set to XMEM_OK else one of 
 *               the following error codes
 *               - XMEM_ERR_ALLOC_FAILED
 *               - XMEM_ERR_ILLEGAL_ALIGN
 *               - XMEM_ERR_INVALID_ARGS
 *               is returned in \a status.
 */
extern void *
xmem_stack_alloc(xmem_stack_mgr_t *mgr,
                 size_t size,
                 uint32_t align,
                 xmem_status_t *status) __attribute__((malloc));

/*!
 * Return the free space in the stack memory after applying an alignment.
 * 
 * \param mgr              Pointer to the stack memory manager object
 * \param align            Requested alignment. Must be a power of 2.
 * \param start_free_space If non-null, returns the location of the start of
 *                         free space (after alignment is applied)
 * \param end_free_space   If non-null, returns end of allocatable space
 *
 * \return                 Amount of free space and sets status to XMEM_OK if 
 *                         successful, else returns one of 
 *                         XMEM_ERR_INVALID_ARGS, XMEM_ERR_ILLEGAL_ALIGN
 */
extern size_t
xmem_stack_get_free_space(xmem_stack_mgr_t *mgr, uint32_t align,
                          void **start_free_space, void **end_free_space,
                          xmem_status_t *status);

/*!
 * Saves the stack manager object state.
 *
 * \param mgr    Pointer to the stack memory manager object.
 * \param mgr_cp Pointer to stack manager checkpoint object.
 * 
 * \return       Returns XMEM_OK if successful, else returns 
 *               XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t 
xmem_stack_checkpoint_save(xmem_stack_mgr_t *mgr, 
                           xmem_stack_mgr_checkpoint_t *mgr_cp);

/*!
 * Restores the stack manager object state.
 *
 * \param mgr    Pointer to the stack memory manager object.
 * \param mgr_cp Pointer to stack manager checkpoint object.
 * 
 * \return       Returns XMEM_OK if successful, else returns 
 *               XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t 
xmem_stack_checkpoint_restore(xmem_stack_mgr_t *mgr, 
                              xmem_stack_mgr_checkpoint_t *mgr_cp);

/*!
 * Set the lock object on the stack memory manager.
 *
 * \param mgr  Pointer to the stack memory manager object.
 * \param lock Pointer to the lock object.
 *
 * \return     XMEM_OK if successful, else returns XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t
xmem_stack_set_lock(xmem_stack_mgr_t *mgr, volatile void *lock);

/*!
 * Set the lock object on the heap memory manager.
 *
 * \param mgr  Pointer to the heap memory manager object.
 * \param lock Pointer to the lock object.
 *
 * \return     XMEM_OK if successful, else returns XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t
xmem_heap_set_lock(xmem_heap_mgr_t *mgr, volatile void *lock);

/*!
 * Reset the stack memory manager state.
 *
 * \param mgr Pointer to stack manager object.
 *
 * \return    XMEM_OK if successful, else XMEM_ERR_INVALID_ARGS.
 */
extern xmem_status_t 
xmem_stack_reset(xmem_stack_mgr_t *mgr);

/*!
 * Check if pointer is in bounds of the allocated space 
 * of the stack manager memory pool.
 *
 * \param mgr Poiner to stack manager object.
 * \param p   Pointer to check for bounds.
 *
 * \return    XMEM_OK if pointer is in bounds, else returns one of
 *            XMEM_ERR_INVALID_ARGS or XMEM_ERR_PTR_OUT_OF_BOUNDS.
 */
extern xmem_status_t 
xmem_stack_check_bounds(xmem_stack_mgr_t *mgr, void *p);

/*! 
 * Initialize the heap memory manager.
 *
 * \param mgr        Pointer to heap memory manager object.
 * \param buffer     User provided pool for allocation.
 * \param size       Size of the pool.
 * \param num_blocks Number of blocks available to track the allocation/free
 *                   status of the user buffers. This is used only if the
 *                   \a header is not NULL, else a default of 32 blocks are 
 *                   reserved for tracking the user allocations.
 * \param header     Optional buffer used to store the header information.
 *                   required by the heap3 memory manager.
 *                   If NULL, the headers are allocated out of the user 
 *                   provided pool. 
 *
 * \return           XMEM_OK if successful, else returns one of 
 *                   - XMEM_ERR_POOL_SIZE 
 *                   - XMEM_ERR_INTERNAL
 */
extern xmem_status_t
xmem_heap_init(xmem_heap_mgr_t *mgr,
               void *buffer,
               size_t size,
               uint32_t num_blocks,
               void *header);

/*! 
 * Allocate a buffer of given input size and alignment using the 
 * heap memory manager.
 * 
 * \param mgr    Pointer to the heap memory manager object.
 * \param size   Size of the buffer to allocate. Has to be non-zero.
 * \param align  Requested alignment of the buffer. Needs to be a power of 2.
 * \param status Error status.
 *
 * \return       Pointer to the allocated buffer. If the allocation succeeds,
 *               the \a status is set to XMEM_OK else one of 
 *               the following error codes
 *               - XMEM_ERR_ALLOC_FAILED
 *               - XMEM_ERR_ILLEGAL_ALIGN
 *               - XMEM_ERR_INVALID_ARGS
 *               is returned in \a status.
 */
extern void *
xmem_heap_alloc(xmem_heap_mgr_t *mgr,
                size_t size,
                uint32_t align,
                xmem_status_t *status) __attribute__((malloc));

/*!
 * Free an allocated buffer using the heap memory manager.
 *
 * \param mgr Pointer to the heap memory manager object.
 * \param p   Pointer to buffer to be freed.
 *
 * \return    Returns XMEM_OK if successful, else one of the following error
 *            codes
 *            - XMEM_ERR_INVALID_ARGS
 *            - XMEM_ERR_PTR_OUT_OF_BOUNDS
 */
extern xmem_status_t
xmem_heap_free(xmem_heap_mgr_t *mgr, void *p);

/*!
 * Free and clear an allocated buffer using the heap memory manager
 *
 * \param mgr Pointer to the heap memory manager object.
 * \param p   Pointer to buffer to be freed.
 *
 * \return    Returns XMEM_OK if successful, else one of the following error
 *            codes
 *            - XMEM_ERR_INVALID_ARGS
 *            - XMEM_ERR_PTR_OUT_OF_BOUNDS
 */
extern xmem_status_t
xmem_heap_free_with_clear(xmem_heap_mgr_t *mgr, void *p);

/*!
 * Return the max available contiguous free space in the heap memory 
 * after applying an alignment.
 * 
 * \param mgr              Pointer to the heap memory manager object
 * \param align            Requested alignment
 * \param free_space_qual  Return the free space at start, end, or the 
 *                         max available
 * \param start_free_space If non-null, returns the location of the start of
 *                         free space (after alignment is applied)
 * \param end_free_space   If non-null, returns end of allocatable space
 *
 * \return                 Amount of free space and sets status to XMEM_OK if 
 *                         successful, else returns one of 
 *                         XMEM_ERR_INVALID_ARGS, XMEM_ERR_ILLEGAL_ALIGN
 */
extern size_t
xmem_heap_get_free_space(xmem_heap_mgr_t *mgr, uint32_t align, 
                         xmem_heap_free_space_qual_t free_space_qual,
                         void **start_free_space, void **end_free_space,
                         xmem_status_t *status);

/*!
 * Reset the heap manager state
 *
 * \param mgr Pointer to the heap manager object
 *
 * \return    XMEM_OK if successful, else XMEM_ERR_INVALID_ARGS
 */
extern xmem_status_t 
xmem_heap_reset(xmem_heap_mgr_t *mgr);

/*!
 * Check if pointer is in bounds of the allocated space 
 * of the heap manager memory pool.
 *
 * \param mgr Poiner to heap manager object.
 * \param p   Pointer to check for bounds.
 *
 * \return    XMEM_OK if pointer is in bounds, else returns one of
 *            XMEM_ERR_INVALID_ARGS or XMEM_ERR_PTR_OUT_OF_BOUNDS.
 */
extern xmem_status_t 
xmem_heap_check_bounds(xmem_heap_mgr_t *mgr, void *p);

/*!
 * Return stack manager unused bytes
 *
 * \param mgr Pointer to stack manager object.
 *
 * \return void
 */
extern size_t 
xmem_stack_get_unused_bytes(xmem_stack_mgr_t *mgr);

/*!
 * Return stack manager free bytes
 *
 * \param mgr Pointer to stack manager object.
 *
 * \return void
 */
extern size_t 
xmem_stack_get_free_bytes(xmem_stack_mgr_t *mgr);

/*!
 * Return stack manager allocated bytes
 *
 * \param mgr Pointer to stack manager object.
 *
 * \return void
 */
extern size_t 
xmem_stack_get_allocated_bytes(xmem_stack_mgr_t *mgr);

/*!
 * Return heap manager unused bytes
 *
 * \param mgr Pointer to heap manager object.
 *
 * \return void
 */
extern size_t 
xmem_heap_get_unused_bytes(xmem_heap_mgr_t *mgr);

/*!
 * Return heap manager free bytes
 *
 * \param mgr Pointer to heap manager object.
 *
 * \return void
 */
extern size_t 
xmem_heap_get_free_bytes(xmem_heap_mgr_t *mgr);

/*!
 * Return heap manager allocated bytes
 *
 * \param mgr Pointer to heap manager object.
 *
 * \return void
 */
extern size_t 
xmem_heap_get_allocated_bytes(xmem_heap_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif // __XMEM_H__ 
