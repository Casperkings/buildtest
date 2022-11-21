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

#include <string.h>
#if defined(HAVE_UUID)
#include <uuid/uuid.h>
#endif
#include "xdyn_lib.h"
#include "xdyn_lib_common.h"
/* On Xtensa host platforms */
#ifdef __XTENSA__
#include "xdyn_lib_sys.h"
#endif
#include "xdyn_lib_misc.h"

/* Load a dynamic library
 *
 * dev      : Pointer to XRP device
 * lib      : Pointer to library to be loaded
 * lib_size : Size of library in bytes
 * lib_uuid : UUID (16-bytes)
 * 
 * Returns XDYN_LIB_OK if successful, else returns one of
 * XDYN_LIB_LOAD_FAILED, XDYN_LIB_VERSION_MISMATCH, XDYN_LIB_UNKNOWN_CMD, 
 * or XDYN_LIB_ERR_INTERNAL
 */
xdyn_lib_status_t xdyn_lib_load(struct xrp_device *dev,
                                const char *lib, 
                                size_t lib_size, 
                                const unsigned char *lib_uuid) {
  xdyn_lib_status_t xdyn_lib_status = XDYN_LIB_OK;
  enum xrp_status xrp_status;

  // Create queue with the dynamic library handler namespace
  unsigned char dyn_lib_nsid[] = XDYN_LIB_XRP_CMD_NSID_INITIALIZER;
  struct xrp_queue *xrp_queue = 
                    xrp_create_ns_queue(dev, &dyn_lib_nsid, &xrp_status);
  XDYN_LIB_CHECK(xrp_status == XRP_STATUS_SUCCESS, XDYN_LIB_LOAD_FAILED,
                 "xdyn_lib_load: Failed to create queue\n");

  // Create XRP buffer in shared memory and copy the library
  struct xrp_buffer *xrp_buf = xrp_create_buffer(dev, lib_size, 
                                                 NULL, &xrp_status);
  XDYN_LIB_CHECK_STATUS(xrp_status == XRP_STATUS_SUCCESS, xdyn_lib_status,
                        XDYN_LIB_LOAD_FAILED, LABEL_FAIL1,
                        "xdyn_lib_load: Failed to create library buffer in "
                        "xrp shared memory\n");

  void *lib_buf = xrp_map_buffer(xrp_buf, 0, lib_size, 
                                 XRP_READ_WRITE, &xrp_status);
  XDYN_LIB_CHECK_STATUS(xrp_status == XRP_STATUS_SUCCESS, xdyn_lib_status,
                        XDYN_LIB_LOAD_FAILED, LABEL_FAIL2,
                        "xdyn_lib_load: Failed to map library to "
                        "xrp shared memory\n");

  memcpy(lib_buf, lib, lib_size);

  xrp_unmap_buffer(xrp_buf, lib_buf, &xrp_status);
  XDYN_LIB_CHECK_STATUS(xrp_status == XRP_STATUS_SUCCESS, xdyn_lib_status,
                        XDYN_LIB_LOAD_FAILED, LABEL_FAIL2,
                        "xdyn_lib_load: Failed to unmap xrp buffer\n");

  struct xrp_buffer_group *xrp_buf_grp = xrp_create_buffer_group(&xrp_status);
  XDYN_LIB_CHECK_STATUS(xrp_status == XRP_STATUS_SUCCESS, xdyn_lib_status,
                        XDYN_LIB_LOAD_FAILED, LABEL_FAIL2,
                        "xdyn_lib_load: Failed to create xrp buffer group\n");

  xrp_add_buffer_to_group(xrp_buf_grp, xrp_buf, XRP_READ_WRITE, &xrp_status);
  XDYN_LIB_CHECK_STATUS(xrp_status == XRP_STATUS_SUCCESS, xdyn_lib_status,
                        XDYN_LIB_LOAD_FAILED, LABEL_FAIL3,
                        "xdyn_lib_load: Failed to add xrp buffer to group\n");

  // Enqueue lib load command
  xdyn_lib_cmd_info_t cmd;
  cmd.version = XDYN_LIB_VERSION;
  cmd.id = XDYN_LIB_CMD_LOAD;
  cmd.size = lib_size;
  memcpy(cmd.uuid, lib_uuid, 16);

  XDYN_LIB_LOG("xdyn_lib_load: Loading dynamic libary from user buffer %p "
               " of size %d to device %p with id %s\n",
               lib, lib_size, dev, xdyn_lib_get_uuid_str(lib_uuid));

  int result = XDYN_LIB_ERR_INTERNAL;
  xrp_run_command_sync(xrp_queue, &cmd, sizeof(cmd), &result, sizeof(result),
                       xrp_buf_grp, &xrp_status);
  XDYN_LIB_CHECK_STATUS(result == XDYN_LIB_OK, xdyn_lib_status, 
                        result, LABEL_FAIL3,
                        "xdyn_lib_load: Failed to load dynamic library with "
                        "id %s\n", xdyn_lib_get_uuid_str(lib_uuid));

LABEL_FAIL3:
  xrp_release_buffer_group(xrp_buf_grp);

LABEL_FAIL2:
  xrp_release_buffer(xrp_buf);

LABEL_FAIL1:
  xrp_release_queue(xrp_queue);

  return xdyn_lib_status;
}

/* Unload a dynamic library
 *
 * dev      : Pointer to XRP device
 * lib_uuid : UUID (16-bytes) of the dynamic library
 * 
 * Returns XDYN_LIB_OK if successful, else returns one of
 * XDYN_LIB_UNLOAD_FAILED, XDYN_LIB_VERSION_MISMATCH, 
 * XDYN_LIB_UNKNOWN_CMD, or XDYN_LIB_ERR_INTERNAL
 */
xdyn_lib_status_t xdyn_lib_unload(struct xrp_device *dev,
                                  const unsigned char *lib_uuid) {
  xdyn_lib_status_t xdyn_lib_status = XDYN_LIB_OK;
  enum xrp_status xrp_status;

  // Create queue with the dynamic library handler namespace
  unsigned char dyn_lib_nsid[] = XDYN_LIB_XRP_CMD_NSID_INITIALIZER;
  struct xrp_queue *xrp_queue = 
                    xrp_create_ns_queue(dev, &dyn_lib_nsid, &xrp_status);
  XDYN_LIB_CHECK(xrp_status == XRP_STATUS_SUCCESS, XDYN_LIB_UNLOAD_FAILED,
                 "xdyn_lib_load: Failed to create queue\n");

  // Enqueue lib unload command
  xdyn_lib_cmd_info_t cmd;
  cmd.version = XDYN_LIB_VERSION;
  cmd.id = XDYN_LIB_CMD_UNLOAD;
  memcpy(cmd.uuid, lib_uuid, 16);

  XDYN_LIB_LOG("xdyn_lib_unload: Unloading dynamic libary from device %p "
               "with id %s\n", dev, xdyn_lib_get_uuid_str(lib_uuid));

  int result = XDYN_LIB_ERR_INTERNAL;
  xrp_run_command_sync(xrp_queue, &cmd, sizeof(cmd), &result, sizeof(result),
                       NULL, &xrp_status);
  XDYN_LIB_CHECK_STATUS(result == XDYN_LIB_OK, xdyn_lib_status, result,
                        LABEL_FAIL, "xdyn_lib_unload: Failed to unload "
                        "dynamic library with id %s\n", 
                        xdyn_lib_get_uuid_str(lib_uuid));

LABEL_FAIL:
  xrp_release_queue(xrp_queue);

  return xdyn_lib_status;
}
