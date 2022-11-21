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

/* Example to demonstrate use of dynamic library */

#include "xrp_api.h"
#include "xdyn_lib.h"
#include "xdyn_lib_callback_if.h"
#include <xt_mld_loader.h>

XTMLD_DATA_MEM_REQ(xtmld_load_normal, xtmld_load_require_localmem,
                   xtmld_load_prefer_l2mem, xtmld_load_normal);

// Pointers to table of external callback functions for generic utilities
xdyn_lib_utils_callback_if *utils_cb_if;

// Pointers to table of external callback functions for rtos functions
xdyn_lib_rtos_callback_if  *rtos_cb_if;

static void compute(int delay_count) {
  int i;
  for (i = 0; i < delay_count; i++)
    asm volatile ("_nop");
}

// XRP handler for this library
enum xrp_status run_command(void *context,
                            const void *in_data, size_t in_data_size,
                            void *out_data, size_t out_data_size,
                            struct xrp_buffer_group *buffer_group)
{
  (void)context;
  (void)in_data_size;
  (void)out_data_size;
  (void)buffer_group;

  unsigned in = *(const unsigned *)in_data;

  utils_cb_if->printf("%d: TID:%u PRI:%d run_command: got %x\n", 
                      XT_RSR_CCOUNT(), rtos_cb_if->rtos_thread_id(),
                      rtos_cb_if->rtos_get_thread_priority(), in);

  // Just increment the received and return as result
  *(int *)out_data = in + 1;

  compute(1000000);

  utils_cb_if->printf("%d: TID:%u PRI:%d run_command: done\n", 
                      XT_RSR_CCOUNT(), rtos_cb_if->rtos_thread_id(), 
                      rtos_cb_if->rtos_get_thread_priority());

  return XRP_STATUS_SUCCESS;
}

// Library init function. The context received is same as what was passed
// to the xdyn_lib_init function
void *lib_init(void *context)
{
  xdyn_lib_callback_if *cb_if = (xdyn_lib_callback_if *)context;

  // Register the callbacks to the generic utilties
  utils_cb_if = &(cb_if->utils_cb_if);

  // Register the callbacks to the rtos functions
  rtos_cb_if = &(cb_if->rtos_cb_if);

  utils_cb_if->printf("%d: TID:%u PRI:%d lib initialize...\n", 
                      XT_RSR_CCOUNT(), rtos_cb_if->rtos_thread_id(),
                      rtos_cb_if->rtos_get_thread_priority());

  // Returns pointer to the XRP handler for this library
  return run_command;
}

// Library finalization function
void lib_fini()
{
  utils_cb_if->printf("%d: TID:%u PRI:%d lib finalize...\n",
                      XT_RSR_CCOUNT(), rtos_cb_if->rtos_thread_id(),
                      rtos_cb_if->rtos_get_thread_priority());
}

// Library main entry point
void *_start(xdyn_lib_funcs_t func) {
  if (func == XDYN_LIB_INIT)
    return lib_init;
  else if (func == XDYN_LIB_FINI)
    return lib_fini;
  return 0;
}
