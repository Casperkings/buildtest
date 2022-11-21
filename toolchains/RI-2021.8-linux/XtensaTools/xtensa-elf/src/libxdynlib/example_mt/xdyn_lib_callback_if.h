/* Copyright (c) 2021 Cadence Design Systems, Inc.
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

#ifndef __XDYN_LIB_CALLBACK_IF_H__
#define __XDYN_LIB_CALLBACK_IF_H__

/* 
 * Define function pointers for external functions callable from the
 * dynamic library
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xtensa/sim.h>
#include <xtensa/idma.h>
#include <xtensa/xmem_bank.h>
#include "xrp_api.h"

/* iDMA functions */
typedef idma_status_t (*idma_init_loop_p)(idma_buffer_t *,
                                          idma_type_t,
                                          int32_t,
                                          void *,
                                          idma_callback_fn);
typedef idma_status_t (*idma_add_desc_p)(idma_buffer_t *,
                                         void *,
                                         void *,
                                         size_t,
                                         uint32_t);
typedef idma_status_t (*idma_add_2d_desc_p)(idma_buffer_t *,
                                            void *,
                                            void *,
                                            size_t,
                                            uint32_t,
                                            uint32_t,
                                            uint32_t,
                                            uint32_t);
typedef idma_status_t (*idma_copy_2d_desc_p)(void *,
                                             void *,
                                             size_t,
                                             uint32_t,
                                             uint32_t,
                                             uint32_t,
                                             uint32_t);
typedef int32_t (*idma_schedule_desc_p)(uint32_t);
typedef idma_status_t (*idma_sleep_p)();
typedef int32_t (*idma_buffer_status_p)();
typedef int32_t (*idma_desc_done_p)(int32_t);
typedef idma_hw_error_t (*idma_buffer_check_errors_p)();

/* XRP functions */
typedef struct xrp_buffer *(*xrp_get_buffer_from_group_p)(
                            struct xrp_buffer_group *,
                            size_t,
                            enum xrp_status *);
typedef void (*xrp_release_buffer_p)(struct xrp_buffer *);
typedef void *(*xrp_map_buffer_p)(struct xrp_buffer *, 
                                  size_t, 
                                  size_t,
                                  enum xrp_access_flags, 
                                  enum xrp_status *);
typedef void (*xrp_unmap_buffer_p)(struct xrp_buffer *, 
                                   void *,
                                   enum xrp_status *);

/* XMEM bank functions */
typedef void *
(*xmem_bank_alloc_p)(int, 
                     size_t, 
                     uint32_t,
                     xmem_bank_status_t *);
typedef xmem_bank_status_t
(*xmem_bank_free_p)(int bank, void *p);
typedef size_t 
(*xmem_bank_get_free_space_p)(unsigned, 
                              uint32_t, 
                              void **,
                              void **,
                              xmem_bank_status_t *);
typedef xmem_bank_status_t 
(*xmem_bank_checkpoint_save_p)(unsigned, 
                               xmem_bank_checkpoint_t *);
typedef xmem_bank_status_t
(*xmem_bank_checkpoint_restore_p)(unsigned,
                                  xmem_bank_checkpoint_t *);
typedef xmem_bank_alloc_policy_t (*xmem_bank_get_alloc_policy_p)(void);
typedef uint32_t (*xmem_bank_get_num_banks_p)(void);

/* Generic utilities */
typedef void (*xt_iss_profile_disable_p)(void);
typedef void (*xt_iss_profile_enable_p)(void);
typedef int  (*printf_p)(const char *, ...);
typedef int  (*snprintf_p)(char *, size_t, const char *, ...);
typedef int  (*vsnprintf_p)(char *, size_t, const char *, va_list);
typedef void (*abort_p)(void);
typedef void *(*malloc_p)(size_t);
typedef void *(*realloc_p)(void *, size_t);
typedef void (*free_p)(void *);

/* RTOS functions */
typedef uint32_t (*rtos_enable_preemption_p)(void);
typedef uint32_t (*rtos_disable_preemption_p)(void);
typedef uint32_t (*rtos_get_thread_priority_p)(void);
typedef uint32_t (*rtos_thread_id_p)(void);

/* iDMA routines */
typedef struct {
  idma_init_loop_p           idma_init_loop;
  idma_add_desc_p            idma_add_desc;
  idma_add_2d_desc_p         idma_add_2d_desc;
  idma_copy_2d_desc_p        idma_copy_2d_desc;
  idma_schedule_desc_p       idma_schedule_desc;
  idma_sleep_p               idma_sleep;
  idma_buffer_status_p       idma_buffer_status;
  idma_desc_done_p           idma_desc_done;
  idma_buffer_check_errors_p idma_buffer_check_errors;
} xdyn_lib_dma_callback_if;

/* XRP routines */
typedef struct {
  xrp_get_buffer_from_group_p xrp_get_buffer_from_group;
  xrp_release_buffer_p        xrp_release_buffer;
  xrp_map_buffer_p            xrp_map_buffer;
  xrp_unmap_buffer_p          xrp_unmap_buffer;
} xdyn_lib_xrp_callback_if;

/* Local memory bank allocation routines */
typedef struct {
  xmem_bank_alloc_p              xmem_bank_alloc;
  xmem_bank_free_p               xmem_bank_free;
  xmem_bank_get_free_space_p     xmem_bank_get_free_space;
  xmem_bank_checkpoint_save_p    xmem_bank_checkpoint_save;
  xmem_bank_checkpoint_restore_p xmem_bank_checkpoint_restore;
  xmem_bank_get_alloc_policy_p   xmem_bank_get_alloc_policy;
  xmem_bank_get_num_banks_p      xmem_bank_get_num_banks;
} xdyn_lib_xmem_bank_callback_if;

/* Generic utilities */
typedef struct {
  xt_iss_profile_enable_p  xt_iss_profile_enable;
  xt_iss_profile_disable_p xt_iss_profile_disable;
  printf_p                 printf;
  snprintf_p               snprintf;
  vsnprintf_p              vsnprintf;
  abort_p                  abort;
  malloc_p                 malloc;
  realloc_p                realloc;
  free_p                   free;
} xdyn_lib_utils_callback_if;

/* RTOS functions */
typedef struct {
  rtos_enable_preemption_p   rtos_enable_preemption;
  rtos_disable_preemption_p  rtos_disable_preemption;
  rtos_get_thread_priority_p rtos_get_thread_priority;
  rtos_thread_id_p           rtos_thread_id;
} xdyn_lib_rtos_callback_if;

/* Type defining table of external functions  */
typedef struct {
  xdyn_lib_dma_callback_if       dma_cb_if;
  xdyn_lib_xrp_callback_if       xrp_cb_if;
  xdyn_lib_xmem_bank_callback_if xmem_bank_cb_if;
  xdyn_lib_utils_callback_if     utils_cb_if;
  xdyn_lib_rtos_callback_if      rtos_cb_if;
} xdyn_lib_callback_if;

#endif // __XDYN_LIB_CALLBACK_IF_H__
