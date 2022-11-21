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

#ifndef __XDYN_LIB_COMMON_H__
#define __XDYN_LIB_COMMON_H__

#include "xdyn_lib.h"

// Dynamic library loader version
#define XDYN_LIB_VERSION 1

// Type used to represent the kind of the dynamic library commands
typedef enum {
  XDYN_LIB_CMD_LOAD   = 1,
  XDYN_LIB_CMD_UNLOAD = 2
} xdyn_lib_cmd_t;

// XRP namespace id for processing dynamic library loader commands
#define XDYN_LIB_XRP_CMD_NSID_INITIALIZER \
  {0xd4, 0x51, 0xac, 0xd1, 0x35, 0x0f, 0x4b, 0x9b, \
   0x90, 0xae, 0xc8, 0x71, 0x35, 0xb6, 0x5f, 0x52}

// Command structure passed to the dynamic loader
typedef struct {
  unsigned version;       // Version number
  xdyn_lib_cmd_t id;      // Command type
  unsigned size;          // Size in bytes of the dynamic library
  unsigned char uuid[16]; // UUID of the dynamic library
} xdyn_lib_cmd_info_t;

#endif // __XDYN_LIB_COMMON_H__
