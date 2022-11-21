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
#include <stdint.h>
#include <stddef.h>
#include "xipc_addr.h"
#include "xipc_misc.h"
#include "xipc_common.h"

/* Externally defined in the subsystem */
extern const xipc_pid_t xmp_xipc_num_procs;
extern const unsigned xmp_xipc_max_addr_descs_per_proc;
extern xipc_proc_addr_desc_t xmp_xipc_proc_addr_descs[];

/* Function for checking if address is on this proc's local address space
 * The sizes are assumed to be in bytes */
static XIPC_INLINE xipc_bool_t
xipc_address_is_local(uintptr_t a, uintptr_t local_addr, size_t size)
{
  if (a >= local_addr && a < (local_addr + size))
    return XIPC_TRUE;
  return XIPC_FALSE;
}

/* Function for converting addresses that are on this proc's local address
 * space to the subsystem address space. The sizes are
 * assumed to be in bytes */
static XIPC_INLINE uintptr_t
xipc_address_translate(uintptr_t a, uintptr_t local_addr, 
                       uintptr_t subsys_addr, size_t size)
{
  if (a >= local_addr && a < (local_addr + size)) {
    uintptr_t v = (a - local_addr) + subsys_addr;
    return v;
  }
  return 0xffffffff;
}
  
/* Function to translate an address 'addr' from proc's local address
 * space to the subsystem address space */
void *
xipc_get_sys_addr(void *addr, xipc_pid_t proc_id)
{
  if (proc_id >= xmp_xipc_num_procs)
    return 0;
  xipc_proc_addr_desc_t *desc = xmp_xipc_proc_addr_descs +
                                proc_id * xmp_xipc_max_addr_descs_per_proc;
  while (desc->_size != 0xffffffff) {
    uintptr_t new_addr;
    new_addr = xipc_address_translate((uintptr_t)addr,
                                      desc->_local_addr,
                                      desc->_subsys_addr,
                                      desc->_size);
    if (new_addr != 0xffffffff)
      return (void *)new_addr;
    ++desc;
  }
  return addr;
}

/* Function to check if an address 'addr' is in proc's local address space */
xipc_bool_t
xipc_is_local_address(void *addr, xipc_pid_t proc_id)
{
  if (proc_id >= xmp_xipc_num_procs)
    return XIPC_FALSE;
  xipc_proc_addr_desc_t *desc = xmp_xipc_proc_addr_descs +
                                proc_id * xmp_xipc_max_addr_descs_per_proc;
  while (desc->_size != 0xffffffff) {
    if (xipc_address_is_local((uintptr_t)addr, desc->_local_addr, desc->_size))
      return XIPC_TRUE;
    ++desc;
  }
  return XIPC_FALSE;
}
