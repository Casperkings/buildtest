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

#ifndef __XDYN_LIB_INTERNAL_H__
#define __XDYN_LIB_INTERNAL_H__

#include "xdyn_lib.h"
#include "xdyn_lib_common.h"

// Support a fixed number of dynamic libraries
#define XDYN_LIB_NUM 8

// Type defining the xrp namespace handler of the dynamic library 
typedef enum xrp_status
(*xdyn_lib_xrp_run_command_p)(void *, const void *, size_t, void *, size_t,
                              struct xrp_buffer_group *);

// Type defining the main entry point to the dynamic library. 
// Takes the requested function type as the parameter and returns a pointer 
// to the corresponding function defined in the library.
typedef void *(*xdyn_lib_start_p)(xdyn_lib_funcs_t);

// The init function takes a context that is passed via xdyn_lib_init. 
// It is called after the library is loaded and is expected to return a 
// handle to the xrp command namespace callback of the dynamic library.
typedef xdyn_lib_xrp_run_command_p (*xdyn_lib_init_p)(void *);

// The fini function is called at dynamic library unload time and does 
// any finalization required by the dynamic library.
typedef void (*xdyn_lib_fini_p)(void);

// Structure to record state of a dynamic library
typedef struct {
  xdyn_lib_start_p     start;
  xdyn_lib_init_p      init;
  xdyn_lib_fini_p      fini;
  void                *data_mem[XTMLD_MAX_DATA_SECTIONS];
  void                *code_mem[XTMLD_MAX_CODE_SECTIONS];
  xtmld_state_t        loader_state;
  xtmld_library_info_t lib_info;
  unsigned char        uuid[16];
  unsigned             ref_count;
} xdyn_lib_t;

// Structure defining generic context passed to the dynamic library loader
// XRP namespace callback
typedef struct {
  struct xrp_device *device;
  void *context;
} xdyn_lib_context_t;

#endif // __XDYN_LIB_INTERNAL_H__
