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

#ifndef __XMEM_BANK_H__
#define __XMEM_BANK_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "xmem_bank_common.h"

/* Override this by including <xtensa/system/xmp_subsystem.h> on an 
 * MP-subsystem, where XMP_MAX_DCACHE_LINESIZE is the maximum cache line
 * size across all cores in the subsystem 
 */
#ifndef XMP_MAX_DCACHE_LINESIZE
  #if XCHAL_DCACHE_SIZE>0
    #define XMEM_BANK_MAX_DCACHE_LINESIZE XCHAL_DCACHE_LINESIZE
  #else
    #define XMEM_BANK_MAX_DCACHE_LINESIZE 4
#endif
#else
  #define XMEM_BANK_MAX_DCACHE_LINESIZE XMP_MAX_DCACHE_LINESIZE
#endif

/*! Minimum size of a memory manager checkpoint object in bytes. */
#define XMEM_BANK_CHECKPOINT_SIZE (8)

struct xmem_bank_checkpoint_struct {
  char _[((XMEM_BANK_CHECKPOINT_SIZE+XMEM_BANK_MAX_DCACHE_LINESIZE-1)/XMEM_BANK_MAX_DCACHE_LINESIZE)*XMEM_BANK_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMEM_BANK_MAX_DCACHE_LINESIZE)));

/*! 
 * Type to check point the state of a memory manager object. 
 * The minimum size of an object of this type is 
 * \ref XMEM_BANK_CHECKPOINT_SIZE. */
typedef struct xmem_bank_checkpoint_struct xmem_bank_checkpoint_t;

/*!
 * Top level routine to initialize banks in local memory. This discovers the
 * local memory banks in the processor and manages them. Needs to be called
 * prior to using any of the other bank allocation routines.
 *
 * \param  alloc_policy Can be one of XMEM_BANK_STACK_ALLOC or 
 *                      XMEM_BANK_HEAP_ALLOC.
 * \param  stack_size   Size of the stack to reserve in local memory. If there 
 *                      are multiple local memory banks the stack space is 
 *                      reserved at the end of of the highest addressed 
 *                      memory bank (note, stack grows downwards). If stack
 *                      cannot be reserved (based on the LSP) return 
 *                      XMEM_BANK_ERR_STACK_RESERVE_FAIL.
 *
 * \return              XMEM_BANK_OK if successful, else returns one of 
 *                      XMEM_BANK_ERR_CONFIG_UNSUPPORTED, 
 *                      XMEM_BANK_ERR_INIT_BANKS_FAIL, 
 *                      XMEM_BANK_ERR_STACK_RESERVE_FAIL,
 *                      XMEM_BANK_ERR_INIT_MGR.
 */
extern xmem_bank_status_t 
xmem_init_local_mem(xmem_bank_alloc_policy_t alloc_policy, 
                    unsigned stack_size);

/*!
 * Top level routine to initialize user supplied banks. Needs to be called
 * prior to using any of the other bank allocation routines.
 *
 * \param  alloc_policy Can be one of XMEM_BANK_STACK_ALLOC or
 *                      XMEM_BANK_HEAP_ALLOC.
 * \param  memory_banks User supplied array of memory banks. Each bank
 *                      is a contiguous area in memory with a start address
 *                      and size. The banks are expected to be sorted
 *                      in increasing order of their start address
 * \param num_banks     Number of memory banks
 *
 * \return              XMEM_BANK_OK if successful, else returns one of 
 *                      XMEM_BANK_ERR_INVALID_ARGS,
 *                      XMEM_BANK_ERR_INIT_BANKS_FAIL,
 *                      XMEM_BANK_ERR_INIT_MGR.
 */
extern xmem_bank_status_t 
xmem_init_banks(xmem_bank_alloc_policy_t alloc_policy, 
                xmem_bank_t *memory_banks, unsigned num_banks);

/*!
 * Top level routine to initialize user supplied banks in the caller
 * thread context. Needs to be called prior to using any of the other 
 * bank allocation routines.
 *
 * \param  alloc_policy Can be one of XMEM_BANK_STACK_ALLOC or
 *                      XMEM_BANK_HEAP_ALLOC.
 * \param  memory_banks User supplied array of memory banks. Each bank
 *                      is a contiguous area in memory with a start address
 *                      and size. The banks are expected to be sorted
 *                      in increasing order of their start address
 * \param num_banks     Number of memory banks
 *
 * \return              XMEM_BANK_OK if successful, else returns one of 
 *                      XMEM_BANK_ERR_INVALID_ARGS,
 *                      XMEM_BANK_ERR_INIT_BANKS_FAIL,
 *                      XMEM_BANK_ERR_INIT_MGR.
 */
extern xmem_bank_status_t 
xmem_thread_init_banks(xmem_bank_alloc_policy_t alloc_policy, 
                       xmem_bank_t *memory_banks, unsigned num_banks);

/*!
 *  Reset calling thread's bank manager 
 */
extern void xmem_thread_reset_banks();

/*!
 * Allocate \a size bytes from \a bank with alignment \a align.
 *
 * \param bank   Local memory bank number. If -1, allocation can span
 *               bank boundaries, if the banks are contigous.
 * \param size   Size of data to allocate. Has to be non-zero.
 * \param align  Requested alignment of allocated data.
 * \param status If not NULL, status is set to XMEM_BANK_OK if successful, 
 *               or one of
 *               XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_ILLEGAL_ALIGN, 
 *               XMEM_BANK_ERR_ALLOC_FAILED, XMEM_BANK_ERR_UNINITIALIZED,
 *               otherwise.
 * 
 * \return       Pointer to allocated memory if successful.
 */
extern void *
xmem_bank_alloc(int bank, size_t size, uint32_t align, 
                xmem_bank_status_t *status) __attribute__((malloc));

/*!
 * Allocate \a size bytes from \a bank with alignment \a align. If
 * the requsted memory is not available, the allocation blocks until
 * space is available from a corresponding call to xmem_bank_free_signal
 * from another thread or if \a timeout expires. A \timeout of 0 implies 
 * no timeout.
 *
 * \param bank    Local memory bank number. If -1, allocation can span
 *                bank boundaries, if the banks are contigous.
 * \param size    Size of data to allocate. Has to be non-zero.
 * \param align   Requested alignment of allocated data.
 * \param timeout Wait until timeout cycles
 * \param status  If not NULL, status is set to XMEM_BANK_OK if successful, 
 *                or one of
 *                XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_ILLEGAL_ALIGN, 
 *                XMEM_BANK_ERR_ALLOC_FAILED, XMEM_BANK_ERR_UNINITIALIZED,
 *                otherwise.
 * 
 * \return       Pointer to allocated memory if successful.
 */
extern void *
xmem_bank_alloc_wait(int bank, size_t size, uint32_t align, uint64_t timeout,
                     xmem_bank_status_t *status) __attribute__((malloc));

/*!
 * Returns the amount of free space in \a bank after taking into account the
 * alignment \align. 
 * 
 * \param bank             Local memory bank number.
 * \param align            Requested alignment to apply.
 * \param start_free_space If non-null, returns the location of the start of
 *                         free space (after alignment is applied).
 * \param end_free_space   If non-null, returns end of allocatable space.
 *
 * \return                 The free space and sets status to XMEM_BANK_OK 
 *                         if successful, else returns one of 
 *                         XMEM_BANK_ERR_INVALID_ARGS, 
 *                         XMEM_BANK_ERR_ILLEGAL_ALIGN,
 *                         XMEM_BANK_ERR_UNINITIALIZED.
 */
extern size_t
xmem_bank_get_free_space(unsigned bank, uint32_t align, 
                         void **start_free_space, void **end_free_space,
                         xmem_bank_status_t *status);

/*!
 * Saves the bank allocation state. Only supported for 
 * XMEM_BANK_STACK_ALLOC policy.
 *
 * \param bank Local memory bank number.
 * \param s    Pointer to object where the bank allocation state is saved to.
 * 
 * \return     Returns XMEM_BANK_OK if successful, else returns 
 *             one of XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *             XMEM_BANK_ERR_INVALID_ARGS, or XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_bank_checkpoint_save(unsigned bank, xmem_bank_checkpoint_t *s);

/*!
 * Restores the bank allocation state. Only supported for XMEM_BANK_STACK_ALLOC
 * policy.
 *
 * \param bank   Local memory bank number.
 * \param s      Pointer to object where the bank allocation state is restored 
 *               from.
 * 
 * \return       Returns XMEM_BANK_OK if successful, else returns 
 *               one of XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *               XMEM_BANK_ERR_INVALID_ARGS, or XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_bank_checkpoint_restore(unsigned bank, xmem_bank_checkpoint_t *s);

/*!
 * Check if pointer is in bounds of the bank.
 *
 * \param bank Local memory bank number.
 * \param p    Pointer to check for bounds.
 *
 * \return     Return XMEM_BANK_OK if in bounds, else returns one of
 *             XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS,
 *             or XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_bank_check_bounds(unsigned bank, void *p);

/*!
 * Reset bank allocation state.
 *
 * \param bank Local memory bank number.
 *
 * \return     Return XMEM_BANK_OK if in bounds, else returns
 *             XMEM_BANK_ERR_INVALID_ARGS or XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_bank_reset(unsigned bank);

/*!
 * Frees memory allocated by the heap memory manager. Only supported for
 * XMEM_BANK_HEAP_ALLOC policy. Signals any thread waiting for space to be
 * available using a corresponding call to \a xmem_bank_alloc_wait.
 *
 * \param bank Local memory bank number.
 * \param p    Pointer to be freed.
 *
 * \return     Return XMEM_BANK_OK if successful, else returns one of
 *             XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *             or XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_bank_free_signal(int bank, void *p);

/*!
 * Frees memory allocated by the heap memory manager. Only supported for
 * XMEM_BANK_HEAP_ALLOC policy.
 *
 * \param bank Local memory bank number.
 * \param p    Pointer to be freed.
 *
 * \return     Return XMEM_BANK_OK if successful, else returns one of
 *             XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *             or XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_bank_free(int bank, void *p);

/*!
 * Frees and clears memory allocated by the heap memory manager. 
 * Only supported for XMEM_BANK_HEAP_ALLOC policy.
 *
 * \param bank Local memory bank number.
 * \param p    Pointer to be freed.
 *
 * \return     Return XMEM_BANK_OK if successful, else returns one of
 *             XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *             or XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_bank_free_with_clear(int bank, void *p);

/*!
 * Frees and clears memory allocated by the heap memory manager. Only supported 
 * for XMEM_BANK_HEAP_ALLOC policy. Signals any thread waiting for space to be
 * available using a corresponding call to \a xmem_bank_alloc_wait.
 *
 * \param bank Local memory bank number.
 * \param p    Pointer to be freed.
 *
 * \return     Return XMEM_BANK_OK if successful, else returns one of
 *             XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
 *             or XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_bank_free_with_clear_signal(int bank, void *p);

/*! 
 * Returns the number of memory banks
 *
 * \return Number of memory banks
 */
extern uint32_t xmem_bank_get_num_banks();

/*!
 * Returns the bank allocation policy
 *
 * \return The bank allocation policy
 */
extern xmem_bank_alloc_policy_t xmem_bank_get_alloc_policy();

/*!
 * Returns the free bytes in the bank
 *
 * \param bank Local memory bank number.
 *
 * \return     Returns the free bytes in the bank
 */
extern ssize_t
xmem_bank_get_free_bytes(unsigned bank);

/*!
 * Returns the used bytes in the bank
 *
 * \param bank Local memory bank number.
 *
 * \return     Returns the used bytes in the bank
 */
extern ssize_t
xmem_bank_get_unused_bytes(unsigned bank);

/*!
 * Returns the allocated bytes in the bank
 *
 * \param bank Local memory bank number.
 *
 * \return     Returns the allocated bytes in the bank
 */
extern ssize_t
xmem_bank_get_allocated_bytes(unsigned bank);


/*!
 * Checks if the banks are contiguous
 *
 * \return 1 if the banks are contiguous, else returns 0
 */
extern int 
xmem_banks_contiguous();

/*!
 * Top level routine to initialize L2 memory. This discovers the
 * available L2 memory space and manages them. Needs to be called
 * prior to using any of the other L2 allocation routines.
 * For a MP system, only one of the cores should be allowed
 * to operate on the L2 using the below set of functions.
 *
 * \return XMEM_BANK_OK if successful, else returns one of 
 *         XMEM_BANK_ERR_CONFIG_UNSUPPORTED, 
 *         XMEM_BANK_ERR_INIT_BANKS_FAIL, 
 *         XMEM_BANK_ERR_INIT_MGR.
 */
extern xmem_bank_status_t 
xmem_init_l2_mem();

/*!
 * Allocate \a size bytes from L2 with alignment \a align.
 *
 * \param size   Size of data to allocate.
 * \param align  Requested alignment of allocated data.
 * \param status If not NULL, status is set to XMEM_BANK_OK if successful,
 *               or one of XMEM_BANK_ERR_ILLEGAL_ALIGN, 
 *               XMEM_BANK_ERR_ALLOC_FAILED, XMEM_BANK_ERR_UNINITIALIZED
 *               otherwise.
 * 
 * \return       Pointer to allocated memory if successful.
 */
extern void *
xmem_l2_alloc(size_t size, uint32_t align, 
              xmem_bank_status_t *status) __attribute__((malloc));

/*!
 * Returns the amount of free space in L2 after taking into account the
 * alignment \align. 
 * 
 * \param align            Requested alignment to apply.
 * \param start_free_space If non-null, returns the location of the start of
 *                         free space (after alignment is applied).
 * \param end_free_space   If non-null, returns end of allocatable space.
 *
 * \return                 The free space and sets status to XMEM_BANK_OK 
 *                         if successful, else returns one of 
 *                         XMEM_BANK_ERR_UNINITIALIZED or 
 *                         XMEM_BANK_ERR_ILLEGAL_ALIGN
 */
extern size_t
xmem_l2_get_free_space(uint32_t align, void **start_free_space, 
                       void **end_free_space, xmem_bank_status_t *status);

/*!
 * Check if pointer is in bounds of the L2
 *
 * \param p    Pointer to check for bounds.
 *
 * \return     Return XMEM_BANK_OK if in bounds, else returns one of
 *             XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_l2_check_bounds(void *p);

/*!
 * Reset L2 allocation state.
 *
 * \param bank Local memory bank number.
 *
 * \return     Return XMEM_BANK_OK if in bounds, else returns
 *             XMEM_BANK_ERR_UNINITIALIZED
 */
extern xmem_bank_status_t 
xmem_l2_reset();

/*!
 * Frees memory allocated by the heap memory manager. 
 *
 * \param p Pointer to be freed.
 *
 * \return  Return XMEM_BANK_OK if successful, else returns one of
 *          XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_l2_free(void *p);

/*!
 * Frees and clears memory allocated by the heap memory manager. 
 *
 * \param p Pointer to be freed.
 *
 * \return  Return XMEM_BANK_OK if successful, else returns one of
 *          XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS or XMEM_BANK_ERR_UNINITIALIZED.
 */
extern xmem_bank_status_t 
xmem_l2_free_with_clear(void *p);

/*!
 * Returns the free bytes in L2
 *
 * \return Returns the free bytes in L2
 */
extern ssize_t
xmem_l2_get_free_bytes();

/*!
 * Returns the unsed bytes in L2
 *
 * \return Returns the unsed bytes in L2
 */
extern ssize_t
xmem_l2_get_unused_bytes();

/*!
 * Returns the allocated bytes in L2
 *
 * \return Returns the allocated bytes in L2
 */
extern ssize_t
xmem_l2_get_allocated_bytes();

#ifdef __cplusplus
}
#endif

#endif // __XMEM_BANK_H__ 
