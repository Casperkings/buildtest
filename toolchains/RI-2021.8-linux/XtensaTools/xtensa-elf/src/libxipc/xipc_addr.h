/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
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
#ifndef __XIPC_ADDR_H__
#define __XIPC_ADDR_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "xipc_common.h"

/*! Data structure defining the local address, global address space, and size. 
 */
struct xipc_proc_addr_desc_struct {
  uintptr_t _local_addr;
  uintptr_t _subsys_addr;
  size_t    _size;
};

/*! Data structure defining the local address, global address space and size. */
typedef struct xipc_proc_addr_desc_struct xipc_proc_addr_desc_t;

/*! 
 * This function translates the address \a addr in a processor's local 
 * address space to the global address space. The MP-subsystem builder maps 
 * the core's local address space into distinct regions in the global 
 * address space.  To allow other cores to access a buffer in a core's 
 * local address spacei (eg: local memories), the core uses this function to 
 * translate the local address to the global address space.
 *
 * \param addr    Address in \a proc_id's local address space.
 * \param proc_id Map address addr to \a proc_id's global address space.
 * \return        Translated address in the global address space.
 */
extern void *xipc_get_sys_addr(void *addr, xipc_pid_t proc_id);

/*! 
 * This function checks if the address \a addr is in a processor's local
 * address space.
 *
 * \param addr    Address to check if in target \a proc_id's local address
 *                space.
 * \param proc_id Target processor id.
 * \return        True if address is in target processor's local address space
 *                else returns false.
 */
extern xipc_bool_t xipc_is_local_address(void *addr, xipc_pid_t proc_id);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_ADDR_H__ */
