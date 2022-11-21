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

#ifndef __XMEM_BANK_COMMON_H__
#define __XMEM_BANK_COMMON_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Describe different memory allocation policies.
 */
typedef enum {
  /*! Use a stack-like memory management scheme with sequential allocate/free */
  XMEM_BANK_STACK_ALLOC   = 1,

  /*! Use a heap-like memory management scheme with flexible allocate/free */
  XMEM_BANK_HEAP_ALLOC    = 2
} xmem_bank_alloc_policy_t;

/*!
 * This is used by the xmem bank APIs to represent different error conditions.
 */
typedef enum {
  /*! Invalid arguments to the api functions */
  XMEM_BANK_ERR_INVALID_ARGS       = -100,

  /*! Failed to initialize the memory bank information */
  XMEM_BANK_ERR_INIT_BANKS_FAIL    = -99,

  /*! The target configuration is not supported */
  XMEM_BANK_ERR_CONFIG_UNSUPPORTED = -98,

  /*! Failed to initialize the memory bank manager */
  XMEM_BANK_ERR_INIT_MGR           = -97,

  /*! Failed to allocate memory from the memory bank manager */
  XMEM_BANK_ERR_ALLOC_FAILED       = -96,

  /*! Illegal alignment request */
  XMEM_BANK_ERR_ILLEGAL_ALIGN      = -95,

  /*! Pointer is not contained in any of the banks */
  XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS  = -94,

  /*! Allocation policy not supported */
  XMEM_BANK_ERR_UNSUPPORTED_ALLOC  = -93,

  /*! Banks not initialized */
  XMEM_BANK_ERR_UNINITIALIZED      = -92,

  /*! Failed to reserve stack in local memory */
  XMEM_BANK_ERR_STACK_RESERVE_FAIL = -91,

  /*! Internal error */
  XMEM_BANK_ERR_INTERNAL           = -1,

  /*! No Error */
  XMEM_BANK_OK                     = 0
} xmem_bank_status_t;

/*! Structure to represent a memory bank */
typedef struct {
  void     *_start; // Start of free space
  uint32_t  _size;  // Size of free memory in this bank
} xmem_bank_t;

#ifdef __cplusplus
}
#endif

#endif // __XMEM_BANK_COMMON_H__

