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

#ifndef __XDYN_LIB_H__
#define __XDYN_LIB_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "xrp_api.h"

/*! Type used to represent the return value of the xdyn lib API calls */
typedef enum {
  /*! Failed to load library */
  XDYN_LIB_LOAD_FAILED      = -100,

  /*! Failed to unload library */
  XDYN_LIB_UNLOAD_FAILED    = -99,

  /*! Loader version mismatch */
  XDYN_LIB_VERSION_MISMATCH = -98,

  /*! Unrecognized command */
  XDYN_LIB_UNKNOWN_CMD      = -97,

  /*! Library initialization failed */
  XDYN_LIB_INIT_FAILED      = -96,

  /*! Internal error */
  XDYN_LIB_ERR_INTERNAL     = -1,

  /*! No error */
  XDYN_LIB_OK = 0
} xdyn_lib_status_t;

/*! Type defining functionality provided by the dynamic library */
typedef enum {
  /*! Dynamic library load time initialization function */
  XDYN_LIB_INIT = 0,

  /*! Dynamic library unload time finalization function */
  XDYN_LIB_FINI = 1,
} xdyn_lib_funcs_t;

/*! Load dynamic library to a device
 *
 * \param dev      Pointer to XRP device
 * \param lib      Pointer to library to be loaded
 * \param lib_size Size of the library in bytes
 * \param lib_uuid UUID (16-bytes) of the library
 * \return         XDYN_LIB_OK if successful, else returns one of 
 *                 XDYN_LIB_LOAD_FAILED, XDYN_LIB_VERSION_MISMATCH,
 *                 XDYN_LIB_UNKNOWN_CMD, or XDYN_LIB_ERR_INTERNAL
 */
extern xdyn_lib_status_t 
xdyn_lib_load(struct xrp_device *dev, const char *lib, size_t lib_size,
              const unsigned char *lib_uuid);

/*! Unload dynamic library from a device
 *
 * \param dev      Pointer to XRP device
 * \param lib_uuid UUID (16-bytes) of the library
 * \return         XDYN_LIB_OK if successful, else returns one of 
 *                 XDYN_LIB_UNLOAD_FAILED, XDYN_LIB_VERSION_MISMATCH,
 *                 XDYN_LIB_UNKNOWN_CMD, or XDYN_LIB_ERR_INTERNAL
 */
extern xdyn_lib_status_t 
xdyn_lib_unload(struct xrp_device *dev, const unsigned char *lib_uuid);

/*! Initialize dynamic library. To be invoked from the device side
 *
 * \param dev     Pointer to XRP device
 * \param context Generic context. The context is passed to the libraries
 *                init function
 * \return        XDYN_LIB_OK if successful, else returns XDYN_LIB_INIT_FAILED
 */
extern xdyn_lib_status_t
xdyn_lib_init(struct xrp_device *device, void *context);

#ifdef __cplusplus
}
#endif

#endif // __XDYN_LIB_H__
