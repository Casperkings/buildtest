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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <xtensa/config/system.h>
#if XSHAL_CLIB == XTHAL_CLIB_NEWLIB
#include <malloc.h>
#endif
#include <xt_mld_loader.h>
#include <xtensa/xmem_bank.h>
#include "xdyn_lib_dev.h"
#include "xdyn_lib_sys.h"
#include "xdyn_lib_misc.h"

// Create a array of dynamically loaded libraries
static xdyn_lib_t dynamic_libs[XDYN_LIB_NUM];

// Create a context for passing to dynamic library callbacks
static xdyn_lib_context_t xdl_context_t;

// Record memory available in banks/L2 after initialization
static unsigned bank_free_space[2];
#if XCHAL_HAVE_L2
static unsigned l2_free_space;
#endif

#ifdef XDYN_LIB_DEBUG
static const char *section_name(const char* name, int i)
{
  static char buffer[10];
  if (i)
    sprintf(buffer, ".%1d.%s", i, name);
  else
    sprintf(buffer, ".%s", name);
  return buffer;
}

static const char *data_section_name(int i, xtmld_mem_require_t req)
{
  if (req == xtmld_load_require_dram0) {
    return ".dram0.data";
  } else if (req == xtmld_load_require_dram1) {
    return ".dram1.data";
  }
  return section_name("data", i);
}

static const char* mem_req_string(xtmld_mem_require_t req)
{
  switch (req) {
  case xtmld_load_normal:
    return "normal";
  case xtmld_load_prefer_localmem:
    return "prefer local memory";
  case xtmld_load_prefer_l2mem:
    return "prefer L2 memory";
  case xtmld_load_require_localmem:
    return "require local memory";
  case xtmld_load_require_dram0:
    return "require dataram0";
  case xtmld_load_require_dram1:
    return "require dataram1";
  default:
    return "???";
    break;
  }
}
#endif

// Find a dynamic library entry from the list of dynamic libraries.
// If uuid is provided, find dynamic library entry with matching uuid. If
// no match is found return a free dynamic library entry.
// Returns pointer to dynamic library state if successful, else returns NULL.
static xdyn_lib_t *find_dyn_lib(const unsigned char *uuid) 
{
  int fs = -1;
  for (int i = 0; i < XDYN_LIB_NUM; ++i) {
    if (uuid) {
      if (!memcmp(uuid, &dynamic_libs[i].uuid, 16))
        return &dynamic_libs[i];
    }
    if (fs == -1 && dynamic_libs[i].ref_count == 0)
      fs = i;
  }
  return (fs != -1) ? &dynamic_libs[fs] : NULL;
}
  
// Free allocate code/data sections of the dynamic library
static void free_data_code_sections(xdyn_lib_t *dlib)
{
  for (unsigned i = 0; i < dlib->lib_info.number_of_code_sections; i++) {
    if (dlib->lib_info.code_section_size[i] && dlib->code_mem[i])
      free(dlib->code_mem[i]);
  }
  for (unsigned i = 0; i < dlib->lib_info.number_of_data_sections; i++) {
    if (!dlib->lib_info.data_section_size[i])
      continue;
    if (!dlib->data_mem[i])
      continue;
    if ((dlib->lib_info.data_section_mem_req[i] == 
         xtmld_load_require_localmem) ||
        (dlib->lib_info.data_section_mem_req[i] == 
         xtmld_load_require_dram0)) {
      XDYN_LIB_LOG("free_data_code_sections: Freeing %p from local mem bank0\n",
                   dlib->data_mem[i]);
      xmem_bank_free(0, dlib->data_mem[i]);
    } else if (dlib->lib_info.data_section_mem_req[i] == 
               xtmld_load_require_dram1) {
      XDYN_LIB_LOG("free_data_code_sections: Freeing %p from local mem bank1\n",
                   dlib->data_mem[i]);
      xmem_bank_free(1, dlib->data_mem[i]);
#if XCHAL_HAVE_L2
    } else if (dlib->lib_info.data_section_mem_req[i] == 
               xtmld_load_prefer_l2mem &&
               xmem_l2_check_bounds(dlib->data_mem[i]) == XMEM_BANK_OK) {
      XDYN_LIB_LOG("free_data_code_sections: Freeing %p from L2 mem\n",
                   dlib->data_mem[i]);
      xmem_l2_free(dlib->data_mem[i]);
#endif
    } else {
      XDYN_LIB_LOG("free_data_code_sections: Freeing %p from heap\n",
                   dlib->data_mem[i]);
      free(dlib->data_mem[i]);
    }
  }
}

// Allocate code/data sections from dynamic library. Any local mem data
// is allocated using the local memory manager.
// Returns 0 if succesful, else returns -1.
static int allocate_data_code_sections(xdyn_lib_t *dlib)
{
  int err = 0;

  // Allocate memory for code
  XDYN_LIB_LOG("allocate_data_code_sections:  %16s %8s %8s  "
               "load preference\n", "code section", "size", "address");
  for (unsigned i = 0; i < dlib->lib_info.number_of_code_sections; ++i) {
    // All code is loaded to system memory. No support yet for IRAM
    XDYN_LIB_ASSERT(dlib->lib_info.code_section_mem_req[i] == xtmld_load_normal,
                    "allocate_data_code_sections: "
                    "Code needs to be in system memory");
    if (dlib->lib_info.code_section_size[i]) {
      dlib->code_mem[i] = malloc(dlib->lib_info.code_section_size[i]);
      XDYN_LIB_CHECK_STATUS(dlib->code_mem[i], err, -1, fail,
                            "allocate_data_code_sections: failed to "
                            "allocate %d bytes in heap memory\n", 
                            dlib->lib_info.data_section_size[i]);
    } else {
      dlib->code_mem[i] = 0;
    }
#ifdef XDYN_LIB_DEBUG
    const char *mrs = mem_req_string(dlib->lib_info.code_section_mem_req[i]);
    const char *sn = section_name("text", i);
    XDYN_LIB_LOG("allocate_data_code_sections: %16s 0x%08x 0x%08x %s\n", sn,
                 dlib->lib_info.code_section_size[i],
                (unsigned)dlib->code_mem[i], mrs);
#endif
  }

  // Allocate memory for data
  XDYN_LIB_LOG("allocate_data_code_sections:  %16s %8s %8s  "
               "load preference\n", "data section", "size", "address");
  for (unsigned i = 0; i < dlib->lib_info.number_of_data_sections; i++) {
    if (dlib->lib_info.data_section_size[i]) {
      if ((dlib->lib_info.data_section_mem_req[i] == 
            xtmld_load_require_localmem) ||
          (dlib->lib_info.data_section_mem_req[i] == 
           xtmld_load_require_dram0) ||
          (dlib->lib_info.data_section_mem_req[i] == 
           xtmld_load_require_dram1))
      {
        xmem_bank_status_t s;
        // Pick the bank based on the section load requirement
#if XCHAL_NUM_DATARAM
        void *bank0_sfs, *bank0_efs;
        xmem_bank_get_free_space(0, 1, &bank0_sfs, &bank0_efs, &s);
        int dram0_bank =
          (((uintptr_t)bank0_sfs >= XCHAL_DATARAM0_PADDR) &&
           ((uintptr_t)bank0_efs < (XCHAL_DATARAM0_PADDR+XCHAL_DATARAM0_SIZE)))
          ? 0 : 1;
#else
        int dram0_bank = 0;
#endif
#if XCHAL_NUM_DATARAM > 1
        void *bank1_sfs, *bank1_efs;
        xmem_bank_get_free_space(1, 1, &bank1_sfs, &bank1_efs, &s);
        int dram1_bank =
          (((uintptr_t)bank1_sfs >= XCHAL_DATARAM1_PADDR) &&
           ((uintptr_t)bank1_efs < (XCHAL_DATARAM1_PADDR+XCHAL_DATARAM1_SIZE)))
          ? 1 : 0;
#else
        int dram1_bank = 0;
#endif
        int bank = ((dlib->lib_info.data_section_mem_req[i] == 
                     xtmld_load_require_localmem) ||
                    (dlib->lib_info.data_section_mem_req[i] ==
                     xtmld_load_require_dram0)) ? dram0_bank : dram1_bank;

        // Aligned to machine load/store width
        dlib->data_mem[i] = 
            xmem_bank_alloc(bank, dlib->lib_info.data_section_size[i], 
                            XCHAL_DATA_WIDTH, &s);
        XDYN_LIB_CHECK_STATUS(s == XMEM_BANK_OK, err, -1, fail,
                              "allocate_data_code_sections: failed to "
                              "allocate %d bytes in local memory\n", 
                              dlib->lib_info.data_section_size[i]);
        XDYN_LIB_LOG("allocate_data_code_sections: allocating %d data in "
                     "local mem at %p\n", dlib->lib_info.data_section_size[i], 
                     dlib->data_mem[i]);
#if XCHAL_HAVE_L2
      } else if (dlib->lib_info.data_section_mem_req[i] == 
                 xtmld_load_prefer_l2mem) {
        xmem_bank_status_t s;
        size_t sz = xmem_l2_get_free_bytes();
        // Check if L2 is initialzied
        if (sz == SIZE_MAX) {
          // Allocate from heap with machine load/store width alignment
          dlib->data_mem[i] = memalign(XCHAL_DATA_WIDTH,
                                       dlib->lib_info.data_section_size[i]);
          XDYN_LIB_LOG("allocate_data_code_sections: allocating %d data in "
                       "heap at %p\n", dlib->lib_info.data_section_size[i], 
                       dlib->data_mem[i]);
        } else {
          dlib->data_mem[i] = xmem_l2_alloc(dlib->lib_info.data_section_size[i],
                                            XCHAL_DATA_WIDTH, &s);
          XDYN_LIB_LOG("allocate_data_code_sections: allocating %d data in "
                       "L2 mem at %p\n", dlib->lib_info.data_section_size[i], 
                       dlib->data_mem[i]);
        }
        XDYN_LIB_CHECK_STATUS(dlib->data_mem[i], err, -1, fail,
                              "allocate_data_code_sections: failed to "
                              "allocate %d bytes in L2/heap memory\n", 
                              dlib->lib_info.data_section_size[i]);
#endif
      } else {
        // Allocate from heap with machine load/store width alignment
        dlib->data_mem[i] = memalign(XCHAL_DATA_WIDTH,
                                     dlib->lib_info.data_section_size[i]);
        XDYN_LIB_CHECK_STATUS(dlib->data_mem[i], err, -1, fail,
                              "allocate_data_code_sections: failed to "
                              "allocate %d bytes in heap memory\n", 
                              dlib->lib_info.data_section_size[i]);
        XDYN_LIB_LOG("allocate_data_code_sections: allocating %d data in "
                     "heap at %p\n", dlib->lib_info.data_section_size[i], 
                     dlib->data_mem[i]);
      }
    } else {
      dlib->data_mem[i] = 0;
    }
#ifdef XDYN_LIB_DEBUG
    const char *mrs = mem_req_string(dlib->lib_info.data_section_mem_req[i]);
    const char *sn = data_section_name(i, 
                                       dlib->lib_info.data_section_mem_req[i]);
    XDYN_LIB_LOG("allocate_data_code_sections: %16s 0x%08x  0x%08x  %s\n", sn,
                 dlib->lib_info.data_section_size[i], 
                 (unsigned)dlib->data_mem[i], mrs);
#endif
  }

fail:
  return err;
}

// XRP namespace handler for loading/unloading dynamic libraries.
// If successful, returns a 0 else returns a -1 in out_data.
static enum xrp_status 
xdyn_lib_cmd_run(void *xrp_context, const void *in_data, 
                 size_t in_data_size, void *out_data, 
                 size_t out_data_size, struct xrp_buffer_group *buffer_group)
{
  (void)out_data_size;
  (void)in_data_size;

  enum xrp_status status;
  int err = XDYN_LIB_OK;
  int r = 0;
  xtmld_result_code_t code;

  // Extract the device and the generic context to be passed to
  // the dynamic library's init function
  struct xrp_device *device = ((xdyn_lib_context_t *)xrp_context)->device;
  void *context = ((xdyn_lib_context_t *)xrp_context)->context;

  xdyn_lib_cmd_info_t *cmd = (xdyn_lib_cmd_info_t *)in_data;

  XDYN_LIB_CHECK_STATUS(cmd->version == XDYN_LIB_VERSION, err, 
                        XDYN_LIB_VERSION_MISMATCH, LABEL_FAIL5,
                        "xdyn_lib_cmd_run: XDYN_LIB_VERSION mismatch, "
                        "expected %d got %d\n", XDYN_LIB_VERSION, cmd->version);
                 
  switch (cmd->id) {
  case XDYN_LIB_CMD_LOAD: {

    // Prevent context-switch
    xdyn_lib_disable_preemption();

    XDYN_LIB_LOG("xdyn_lib_cmd_run: XDYN_LIB_CMD_LOAD: Loading library "
                 "of size %d with uuid %s\n", cmd->size,
                 xdyn_lib_get_uuid_str(cmd->uuid));

    // Load the dynamic library
    struct xrp_buffer *lib_xrp_buf = 
                       xrp_get_buffer_from_group(buffer_group, 0, &status);
    XDYN_LIB_CHECK_STATUS(status == XRP_STATUS_SUCCESS, err, 
                          XDYN_LIB_LOAD_FAILED, LABEL_FAIL5,
                          "xdyn_lib_cmd_run: Failed to get xrp library "
                          "buffer from buffer group: %p\n", buffer_group);

    XDYN_LIB_LOG("xdyn_lib_cmd_run: dynamic library size: %d bytes\n", 
                 cmd->size);

    void *lib = xrp_map_buffer(lib_xrp_buf, 0, cmd->size, XRP_READ, &status);
    XDYN_LIB_CHECK_STATUS(status == XRP_STATUS_SUCCESS, err, 
                          XDYN_LIB_LOAD_FAILED, LABEL_FAIL4,
                          "xdyn_lib_cmd_run: Failed to map dynamic library "
                          "from xrp buffer: %p\n", lib_xrp_buf);

    xdyn_lib_t *dyn_lib = find_dyn_lib(cmd->uuid);
    if (dyn_lib && dyn_lib->ref_count) {
      XDYN_LIB_LOG("xdyn_lib_cmd_run: Found previously loaded "
                   " dynamic library with ref count %d\n", dyn_lib->ref_count);
      err = XDYN_LIB_OK;
      goto LABEL_SUCCESS;
    }

    // Found a free dynamic library slot
    XDYN_LIB_CHECK_STATUS(dyn_lib, err, XDYN_LIB_LOAD_FAILED, LABEL_FAIL3,
                          "xdyn_lib_cmd_run: Cannot allocate "
                          "dynamic library\n");
    XDYN_LIB_ASSERT(dyn_lib->ref_count == 0, 
                    "xdyn_lib_cmd_run: Expecting library ref count to be 0\n");
   
    // Initialize the dynamic library
    xtmld_packaged_lib_t *p_pi_library = (xtmld_packaged_lib_t *)lib;
    code = xtmld_library_info(p_pi_library, &dyn_lib->lib_info);
    XDYN_LIB_CHECK_STATUS(code == xtmld_success, err, XDYN_LIB_LOAD_FAILED,
                          LABEL_FAIL2, "xdyn_lib_cmd_run: xtmld_library_info "
                          "failed with error: %s\n", xtmld_error_string(code));

    // Allocate code/data sections
    r = allocate_data_code_sections(dyn_lib);
    XDYN_LIB_CHECK_STATUS(r == 0, err, XDYN_LIB_LOAD_FAILED, LABEL_FAIL2,
                          "xdyn_lib_cmd_run: Failed to allocate "
                          "code/data sections for dynamic lobrary\n");

    // Load the library
    code = xtmld_load(p_pi_library, dyn_lib->code_mem, dyn_lib->data_mem,
                      &dyn_lib->loader_state, 
                      (xtmld_start_fn_ptr *)&dyn_lib->start);
    XDYN_LIB_CHECK_STATUS(code == xtmld_success && dyn_lib->start != NULL, 
                          err, XDYN_LIB_LOAD_FAILED, LABEL_FAIL2,
                          "xdyn_lib_cmd_run: xtmld_load failed: %s\n", 
                          xtmld_error_string(code));
    XDYN_LIB_LOG("xdyn_lib_cmd_run: xtmld_load() succeeded, start_fn = %p\n", 
                 dyn_lib->start);

    // Get the init and fini functions from the dynamic library
    dyn_lib->init = dyn_lib->start(XDYN_LIB_INIT);
    dyn_lib->fini = dyn_lib->start(XDYN_LIB_FINI);
    XDYN_LIB_CHECK_STATUS(dyn_lib->init != NULL && dyn_lib->fini != NULL,
                          err, XDYN_LIB_LOAD_FAILED, LABEL_FAIL1,
                          "xdyn_lib_cmd_run: Failed to get "
                          "init/fini functions from dynamic library\n");

    // Initialize the dynamic library. Pass the generic context.
    // Get the xrp namespace handler of the library
    xdyn_lib_xrp_run_command_p xrp_run_command = dyn_lib->init(context);
    XDYN_LIB_CHECK_STATUS(xrp_run_command, err, XDYN_LIB_LOAD_FAILED, 
                          LABEL_FAIL0, "xdyn_lib_cmd_run: Failed to get "
                          "xrp command handle from dynamic library\n");

    // Register the handler for this library
    xrp_device_register_namespace(device, cmd->uuid,
                                  xrp_run_command, NULL, &status);
    XDYN_LIB_CHECK_STATUS(status == XRP_STATUS_SUCCESS, err,
                          XDYN_LIB_LOAD_FAILED, LABEL_FAIL0,
                          "xdyn_lib_cmd_run: Could not register dynamic "
                          "library to xrp namespace\n");

    // Init the uuid and reference count
    memcpy(dyn_lib->uuid, cmd->uuid, sizeof(cmd->uuid));
    dyn_lib->ref_count = 1;

LABEL_FAIL0:
    // De-init the library on failure
    if (err != XDYN_LIB_OK)
      dyn_lib->fini();
  
LABEL_FAIL1:
    // Unload dynamic library on failure
    if (err != XDYN_LIB_OK)
      xtmld_unload(&dyn_lib->loader_state);

LABEL_FAIL2:
    // Free allocated coded/data sections on failure
    if (err != XDYN_LIB_OK) {
      free_data_code_sections(dyn_lib);
      memset(dyn_lib, 0, sizeof(xdyn_lib_t));
    }

LABEL_SUCCESS:
LABEL_FAIL3:
    xrp_unmap_buffer(lib_xrp_buf, lib, &status);

LABEL_FAIL4:
    xrp_release_buffer(lib_xrp_buf);

LABEL_FAIL5:
    *(int *)out_data = err;

    // Enable context-switch
    xdyn_lib_enable_preemption();
    
    return XRP_STATUS_SUCCESS;
  }

  case XDYN_LIB_CMD_UNLOAD: {
    // Prevent context-switch
    xdyn_lib_disable_preemption();

    XDYN_LIB_LOG("xdyn_lib_cmd_run: XDYN_LIB_CMD_UNLOAD: Unloading library "
                 "with uuid:%s\n", xdyn_lib_get_uuid_str(cmd->uuid));

    // Find the dynamic library to unload
    xdyn_lib_t *dyn_lib = find_dyn_lib(cmd->uuid);
    XDYN_LIB_CHECK_STATUS(dyn_lib, err, XDYN_LIB_UNLOAD_FAILED, LABEL_FAIL6,
                          "xdyn_lib_cmd_run: Could not find library\n");
    XDYN_LIB_ASSERT(dyn_lib->ref_count != 0, "xdyn_lib_cmd_run: "
                    "Found dynamic library with 0 reference count\n");

    // If last reference, unload
    if (dyn_lib->ref_count == 1) {
      // Unregister handler from namespace
      xrp_device_unregister_namespace(device, cmd->uuid, NULL);

      // Perform any finalization of the dynamic library
      dyn_lib->fini();

      // Unload and free code/data sections allocated for the library
      xtmld_result_code_t code = xtmld_unload(&dyn_lib->loader_state);
      XDYN_LIB_ASSERT(code == xtmld_success, 
                      "xdyn_lib_cmd_run: xtmld_unload() failed\n");

      XDYN_LIB_LOG("xdyn_lib_cmd_run: xtmld_unload() succeeded.\n");

      free_data_code_sections(dyn_lib);
      memset(dyn_lib, 0, sizeof(xdyn_lib_t));
    } else {
      XDYN_LIB_LOG("xdyn_lib_cmd_run: Found existing dynamic library with "
                   " ref count %d\n", dyn_lib->ref_count);
      dyn_lib->ref_count--;
    }

LABEL_FAIL6:
    *(int *)out_data = err;

    // Enable context-switch
    xdyn_lib_enable_preemption();

    return XRP_STATUS_SUCCESS;
  }
  default: {
    XDYN_LIB_LOG("xdyn_lib_cmd_run: Unknown cmd %d\n", cmd->id);
    *((int *)out_data) = XDYN_LIB_UNKNOWN_CMD;
    return XRP_STATUS_SUCCESS;
  }
  }
}

/* Initialize dynamic library. To be invoked from the device side
 *
 * dev     : Pointer to XRP device
 * context : Generic context
 *
 * Returns XDYN_LIB_OK if successful, else returns XDYN_LIB_INIT_FAILED
 */
xdyn_lib_status_t xdyn_lib_init(struct xrp_device *device, void *context)
{
  enum xrp_status status;

  XDYN_LIB_CHECK(xmem_bank_get_alloc_policy() == XMEM_BANK_HEAP_ALLOC,
                 XDYN_LIB_INIT_FAILED, 
                 "xdyn_lib_init: Only heap allocation supported\n");

  // Initialize the array of dynamic library states
  for (int i = 0; i < XDYN_LIB_NUM; ++i)
    memset(&dynamic_libs[i], 0, sizeof(xdyn_lib_t));

  // Register namespace handler with XRP to process dynamic library
  // load/unload commands
  unsigned char dyn_lib_nsid[] = XDYN_LIB_XRP_CMD_NSID_INITIALIZER;

  // Set up the context to be passed to the dynamic library XRP handler
  xdl_context_t.device = device;
  xdl_context_t.context = context;

  // Pass the device to the namespace handler context
  xrp_device_register_namespace(device, &dyn_lib_nsid, xdyn_lib_cmd_run, 
                                (void *)&xdl_context_t, &status);
  XDYN_LIB_CHECK(status == XRP_STATUS_SUCCESS, XDYN_LIB_INIT_FAILED,
                 "xdyn_lib_init: Failed to register base "
                 "dynamic library namespace\n");

  // Record the available memory in banks for all the dynamic libraries
  if (xmem_bank_get_num_banks())
    bank_free_space[0] = xmem_bank_get_free_bytes(0);
  if (xmem_bank_get_num_banks() > 1)
    bank_free_space[1] = xmem_bank_get_free_bytes(1);

#if XCHAL_HAVE_L2
  l2_free_space = xmem_l2_get_free_bytes();
#endif

  XDYN_LIB_LOG("xdyn_lib_init: Done\n");

  return XDYN_LIB_OK;
}
