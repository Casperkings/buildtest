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
#include <stdlib.h>
#include <stdint.h>
#include <xtensa/hal.h>
#include <xtensa/system/xmp_subsystem.h>
#include "xmp_log.h"

extern const uintptr_t xmp_mmio_addrs_xipc_intr[];

static int 
is_mpu_entry_ok(struct xthal_MPU_entry mpu_entry)
{
  unsigned v = XTHAL_MPU_ENTRY_GET_MEMORY_TYPE(mpu_entry) & 0x1fe;
  // See Table 4-129 (Memory Protection Unit Option Memory Type)
  // of ISA reference manual.
  if (v == 0x1e || // non-cacheable, sharable
      v == 0x6  || // Device shareable, non-interruptible 
      v == 0xe)    // Device shareable, interruptible
    return 1;
  
  return 0;
}

void xmp_set_mpu_attrs_region_uncached(void *addr, size_t size)
{
  int err;
  err = xthal_mpu_set_region_attribute(addr, size,
                                       XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS,
                                        (XTHAL_MEM_NON_CACHEABLE    |
                                         XTHAL_MEM_SYSTEM_SHAREABLE |
                                         XTHAL_MEM_BUFFERABLE),
                                       0);
  if (err != XTHAL_SUCCESS) {
    xmp_log("Error setting region attribute on [%x,%x], err %d\n",
            (void *)addr, (void *)(addr+size), err);
    exit(-1);
  }
}

void xmp_set_mpu_attrs()
{
  int i, err;

#ifdef XCHAL_HAVE_DATARAM0
  // Marks the global address space for dataram0 for all cores as shared 
  // non-cached
  uintptr_t global_base_addr_dataram0[] =
                             {XMP_PROC_ARRAY_P(GLOBAL_BASE_ADDR_DATARAM0)};
  uint32_t dataram0_size[] = {XMP_PROC_ARRAY_P(DATARAM0_SIZE)};
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    int is_fg;
    struct xthal_MPU_entry mpu_entry =
      xthal_get_entry_for_address((void *)global_base_addr_dataram0[i], &is_fg);
    if (is_mpu_entry_ok(mpu_entry))
      continue;
    err = xthal_mpu_set_region_attribute(
                             (void *)global_base_addr_dataram0[i],
                             dataram0_size[i],
                             XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS,
                             (XTHAL_MEM_NON_CACHEABLE    |
                              XTHAL_MEM_SYSTEM_SHAREABLE |
                              XTHAL_MEM_BUFFERABLE),
                             0);
    if (err != XTHAL_SUCCESS) {
      xmp_log("Error setting region attribute on [%x,%x), err %d\n",
              (void *)global_base_addr_dataram0[i],
              (void *)(global_base_addr_dataram0[i]+ dataram0_size[i]),
              err);
      exit(-1);
    }
  }
#endif

#ifdef XCHAL_HAVE_DATARAM1
  // Marks the global address space for dataram1 for all cores as shared 
  // non-cached
  uintptr_t global_base_addr_dataram1[] =
                             {XMP_PROC_ARRAY_P(GLOBAL_BASE_ADDR_DATARAM1)};
  uint32_t dataram1_size[] = {XMP_PROC_ARRAY_P(DATARAM1_SIZE)};
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    int is_fg;
    struct xthal_MPU_entry mpu_entry =
      xthal_get_entry_for_address((void *)global_base_addr_dataram1[i], &is_fg);
    if (is_mpu_entry_ok(mpu_entry))
      continue;
    err = xthal_mpu_set_region_attribute(
                             (void *)global_base_addr_dataram1[i],
                             dataram1_size[i],
                             XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS,
                             (XTHAL_MEM_NON_CACHEABLE    |
                              XTHAL_MEM_SYSTEM_SHAREABLE |
                              XTHAL_MEM_BUFFERABLE),
                             0);
    if (err != XTHAL_SUCCESS) {
      xmp_log("Error setting region attribute on [%x,%x), err %d\n",
              (void *)global_base_addr_dataram1[i],
              (void *)(global_base_addr_dataram1[i]+ dataram1_size[i]),
              err);
      exit(-1);
    }
  }
#endif

  // Marks the global address space for mmio registers for all cores as shared 
  // non-cached
  uint32_t has_xipc_interrupts[] = {XMP_PROC_ARRAY_P(HAS_XIPC_INTR)};
  for (i = 0; i < XMP_NUM_PROCS; i++) {
    if (!has_xipc_interrupts[i])
      continue;
    int is_fg;
    struct xthal_MPU_entry mpu_entry =
      xthal_get_entry_for_address((void *)xmp_mmio_addrs_xipc_intr[i], &is_fg);
    if (is_mpu_entry_ok(mpu_entry))
      continue;
    uint32_t mmio_size = 64;
    err = xthal_mpu_set_region_attribute(
                             (void *)xmp_mmio_addrs_xipc_intr[i],
                             mmio_size,
                             XTHAL_MPU_USE_EXISTING_ACCESS_RIGHTS,
                             (XTHAL_MEM_NON_CACHEABLE    |
                              XTHAL_MEM_SYSTEM_SHAREABLE |
                              XTHAL_MEM_BUFFERABLE),
                             0);
    if (err != XTHAL_SUCCESS) {
      xmp_log("Error setting region attribute on [%x,%x), err %d\n",
              (void *)xmp_mmio_addrs_xipc_intr[i],
              (void *)(xmp_mmio_addrs_xipc_intr[i] + mmio_size),
              err);
      exit(-1);
    }
  }
}

