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

#ifndef __XLMEM_H__
#define __XLMEM_H__

#include <stdint.h>
#include "xmem_bank_common.h"

/*! 
 * Initialize the memory bank structure for each of the available local 
 * memory banks in the config. The banks are ordered in increasing order of 
 * their start addresses.
 * 
 * \param banks      Pointer to array of xmem_bank_t structures. 
 *                   Local memory bank information is returned through this.
 * \param num_banks  Number of elements in the banks array. Will be set to the
 *                   actual number of local memory banks.
 * \param stack_size Stack size to reserve. The stack is reserved in the 
 *                   highest addressed bank.
 *
 * \return           XMEM_BANK_OK if successful, else returns one of
 *                   - XMEM_BANK_ERR_INVALID_ARGS,
 *                   - XMEM_BANK_ERR_STACK_RESERVE_FAIL,
 *                   - XMEM_BANK_ERR_CONFIG_UNSUPPORTED, or 
 *                   - XMEM_BANK_ERR_INIT_BANKS_FAIL.
 */
extern xmem_bank_status_t xlmem_init_banks(xmem_bank_t *banks, 
                                           unsigned *num_banks,
                                           unsigned stack_size);

#endif // __XLMEM_H__
