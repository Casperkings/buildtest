/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <algorithm>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#if defined(USE_OPENCL_DYNLIB) && defined(HAVE_UUID) && \
    !defined(USE_KERNEL_UUID)
#include <uuid/uuid.h>
#endif
#include "xrp_opencl_ns.h"
#include "mxpa_debug.h"
#include "host_interface.h"
#include "kernel.h"
#include "xt_device.h"
#include "xdyn_lib.h"

#ifdef USE_OPENCL_DYNLIB
// Device should generate a packaged dynamic library with the following name
static const char *lib_name = "device.pkg.lib";

// Loads contents of file into lib_buf and also returns the size of file
// in lib_buf_size.
//
// Returns 0 if successful, else returns -1
static int load_file(const char *filename, char **lib_buf, 
                     size_t *lib_buf_size) {
  FILE *fp;
  long size;
  int r = 0;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("load_file: Could not open %s\n", filename);
    return -1;
  }

  r = fseek(fp , 0, SEEK_END);
  if (r == -1) {
    printf("load_file: fseek of file %s failed\n", filename);
    return -1;
  }

  size = ftell(fp);
  if (size == -1) {
    printf("load_file: ftell of file %s failed\n", filename);
    return -1;
  }

  rewind(fp);

  char *buf = (char *)malloc(size);
  if (buf == NULL) {
    printf("load_file: Failed to malloc %ld bytes\n", size);
    return -1;
  }

  r = fread(buf, 1, size, fp);
  if (r != size) {
    printf("load_file: Loading file failed\n");
    return -1;
  }

  *lib_buf = buf;
  *lib_buf_size = size;

  fclose(fp);
  return 0;
}
#endif // USE_OPENCL_DYNLIB

__attribute__((unused))
static const char *get_uuid_string(const unsigned char *uuid) {
  static char s[128] = {0,};
  char *p = s;
  for (int i = 0; i < 16; ++i)
    p += sprintf(p, "%x ", uuid[i]);
  return s;
}

#if defined(USE_OPENCL_DYNLIB) && defined(HAVE_UUID) && defined(USE_KERNEL_UUID)
// Generate UUID from kernel. Returns 0 if successful, else -1
static int get_kernel_uuid(unsigned char *uuid) {
  static const unsigned NUM_UUIDS_CHARS = 36;

  unsigned char buf[NUM_UUIDS_CHARS+1] = {0,};

  FILE *fp = fopen("/proc/sys/kernel/random/uuid", "r");
  if (fp == NULL) {
    XOCL_DPRINTF("get_kernel_uuid: Could not get uuid from kernel\n");
    return -1;
  }

  size_t r = fread(buf, 1, NUM_UUIDS_CHARS, fp);
  if (r != NUM_UUIDS_CHARS) {
    XOCL_DPRINTF("get_kernel_uuid: Error reading uuid from kernel\n");
    return -1;
  }

  for (int i = 0, j = 0, k = 0; i < NUM_UUIDS_CHARS; ++i) {
    if (buf[i] >= 'a' && buf[i] <= 'f') {
      uuid[k] = (uuid[k] << 4) | ((buf[i] - 'a') + 0xa);
      ++j;
      k += (1 - (j & 0x1));
    } else if (buf[i] >= '0' && buf[i] <= '9') {
      uuid[k] = (uuid[k] << 4) | (buf[i] - '0');
      ++j;
      k += (1 - (j & 0x1));
    }
  }

  return 0;
}
#endif // defined(USE_OPENCL_DYNLIB) && defined(HAVE_UUID) && 
       // defined(USE_KERNEL_UUID)

int xt_cl_device::initDspCores(const unsigned core_num) {
  int err = -1;
  xrp_status status = XRP_STATUS_SUCCESS;

  xrp_dsp_list_   = new xrp_device*[core_num];
  xrp_queue_list_ = new xrp_queue*[core_num];

#ifdef USE_OPENCL_DYNLIB
  int r;
  char *lib_buf = NULL;
  size_t size = 0;
  xdyn_lib_status_t xdlib_status;
#ifdef HAVE_UUID
#ifdef USE_KERNEL_UUID
  r = get_kernel_uuid(module_uuid_);
  if (r)
    return -1;
#else
  uuid_generate(module_uuid_);
#endif // USE_KERNEL_UUID
#else
  // Use a fixed namespace id
  unsigned char uuid[] = XRP_OPENCL_NSID_INITIALIZER;
  memcpy((void *)module_uuid_, (void *)uuid, sizeof(module_uuid_));
#endif // HAVE_UUID

  XOCL_DPRINTF("initDspCores: Using lib uuid: %s\n", 
               get_uuid_string(module_uuid_));

  for (unsigned core_id = 0; core_id < core_num; ++core_id) {
    HANDLE_XRP_FAIL(xrp_dsp_list_[core_id] = xrp_open_device(core_id,
                                                             &status));
    HANDLE_XRP_FAIL(xrp_queue_list_[core_id] = 
                              xrp_create_ns_queue(xrp_dsp_list_[core_id], 
                                                  &module_uuid_,
                                                  &status));
  }

  r = load_file(lib_name, &lib_buf, &size);
  if (r != 0) {
    XOCL_ERR("initDspCores: Failed to load library file %s\n", lib_name);
    goto FAILED;
  }

  // Load library on all cores
  for (unsigned id = 0; id < core_num; ++id) {
    xdlib_status = xdyn_lib_load(xrp_dsp_list_[id], lib_buf,
                                 size, module_uuid_);
    if (xdlib_status != XDYN_LIB_OK) {
      XOCL_ERR("initDspCores: Failed to load module with uuid %s on core %d, "
               "error = %d\n", get_uuid_string(module_uuid_), id, xdlib_status);
      goto FAILED;
    }
  }
#else
  // Use a fixed namespace id
  unsigned char uuid[] = XRP_OPENCL_NSID_INITIALIZER;
  memcpy((void *)module_uuid_, (void *)uuid, sizeof(module_uuid_));
  for (unsigned core_id = 0; core_id < core_num; ++core_id) {
    HANDLE_XRP_FAIL(xrp_dsp_list_[core_id] = xrp_open_device(core_id,
                                                             &status));
    HANDLE_XRP_FAIL(xrp_queue_list_[core_id] = 
                                   xrp_create_ns_queue(xrp_dsp_list_[core_id], 
                                                       &module_uuid_,
                                                       &status));
  }
#endif // USE_OPENCL_DYNLIB

  err = 0;

FAILED:
#ifdef USE_OPENCL_DYNLIB
  // Free space allocated for library
  if (lib_buf)
    free(lib_buf);
#endif
  // TODO: return error flag to notify user program.
  return err;
}

void xt_cl_device::releaseDspCores(unsigned core_num) {
  xrp_status status = XRP_STATUS_SUCCESS;
#ifdef USE_OPENCL_DYNLIB
  // Unload DSPs.
  xdyn_lib_status_t xdlib_status;
  for (unsigned id = 0; id < core_num; ++id) {
    xdlib_status = xdyn_lib_unload(xrp_dsp_list_[id], module_uuid_);
    if (xdlib_status != XDYN_LIB_OK) {
      XOCL_ERR("releaseDspCores: Failed to unload module with uuid %s on "
               "core %d\n", get_uuid_string(module_uuid_), id);
      return;
    }
  }
#endif // USE_OPENCL_DYNLIB
  if (syn_buf_) {
    xrp_release_buffer(syn_buf_);
    syn_buf_ = nullptr;
  }
  for (unsigned core_id = 0; core_id < core_num; ++core_id) {
    if (xrp_queue_list_)
      CHECK_XRP_STATUS(xrp_release_queue(xrp_queue_list_[core_id]));
    if (xrp_dsp_list_)
      CHECK_XRP_STATUS(xrp_release_device(xrp_dsp_list_[core_id]));
  }
  if (xrp_dsp_list_) {
    delete [] xrp_dsp_list_;
    xrp_dsp_list_ = NULL;
  }
  if (xrp_queue_list_) {
    delete [] xrp_queue_list_;
    xrp_queue_list_ = NULL;
  }
}

bool xt_cl_device::isMyMemRes(xrp_buffer *buf) {
  return std::find(mem_res_list_.begin(),
                   mem_res_list_.end(), buf) != mem_res_list_.end();
}

void *xt_cl_device::mapBuf(buf_obj buf, unsigned offset, unsigned size,
                           xocl_mem_access_flag map_flag) {
  xrp_access_flags flag;
  switch (map_flag) {
    case MEM_READ:  flag = XRP_READ;       break;
    case MEM_WRITE: flag = XRP_WRITE;      break;
    default:        flag = XRP_READ_WRITE; break;
  }
  xrp_status status = XRP_STATUS_SUCCESS;
  void *virt_ptr;
  CHECK_XRP_STATUS(virt_ptr = xrp_map_buffer(buf, offset, size,
                                             flag, &status));
  return virt_ptr;
}

void xt_cl_device::unmapBuf(buf_obj buf, void *virt_ptr) {
  xrp_unmap_buffer(buf, virt_ptr, NULL);
}

cl_int xt_cl_device::allocMem(cl_mem mem, void *host_ptr) {
  XoclThread::Mutex::Mon mon(mt_);
  XOCL_DPRINTF("%s: %p size %zd\n", __func__, mem, mem->getSize());
  void *buf = nullptr;
  size_t size = mem->getSize();

  xrp_status status = XRP_STATUS_SUCCESS;
  // Note: Buffer can be shared between devices, so just create buffer on
  //       dsp0
  xrp_buffer *xrpbuf;
#ifdef XRP_USE_HOST_PTR
  XOCL_DPRINTF("%s: Creating xrpbuf %p, Using host_ptr: %p\n", __func__,
               xrpbuf, mem->getHostPtr());
  HANDLE_XRP_FAIL(xrpbuf = xrp_create_buffer(getXrpDspWithCoreID(0), size,
                                             mem->getHostPtr(), &status));
  if (mem->getFlags() & CL_MEM_COPY_HOST_PTR) {
    HANDLE_XRP_FAIL(buf = xrp_map_buffer(xrpbuf, 0, size,
                                         XRP_READ_WRITE, &status));
    memcpy(buf, host_ptr, size);
  }
#else
  HANDLE_XRP_FAIL(xrpbuf = xrp_create_buffer(getXrpDspWithCoreID(0), size,
                                             NULL, &status));
  if (host_ptr)
    HANDLE_XRP_FAIL(buf = xrp_map_buffer(xrpbuf, 0, size,
                                         XRP_READ_WRITE, &status));

  // Note! For xtensa device, we create cache buffer for host_ptr, and we will
  // synchronize with host_ptr when:
  //    clEnqueueMapBuffer, clEnqueueUnmapMemObject

  // flags can be 'CL_MEM_USE_HOST_PTR' or 'CL_MEM_COPY_HOST_PTR',
  // because we create cache for host_ptr, so we have to copy host_ptr for
  // either case.
  if (host_ptr) {
    memcpy(buf, host_ptr, size);
  }

  if (host_ptr)
    xrp_unmap_buffer(xrpbuf, buf, NULL);
#endif // XRP_USE_HOST_PTR

  mem->setBuf(xrpbuf);
  mem_res_list_.push_back(xrpbuf);
  return CL_SUCCESS;

FAILED:
  if (xrpbuf)
    xrp_release_buffer(xrpbuf);

  return CL_OUT_OF_RESOURCES;
}

void xt_cl_device::freeMem(cl_mem mem) {
  XoclThread::Mutex::Mon mon(mt_);
  XOCL_DPRINTF("%s: mem = %p, virt = %p, size = %zu\n", __func__, 
               mem, mem->getBuf(), mem->getSize());

  assert(!mem->isSubBuffer() && "Can't free sub memory.");
  assert(isMyMemRes(mem->getBuf()) &&
         "The device doesn't own this memory resource.");

  xrp_buffer *buf = mem->getBuf();
  xrp_release_buffer(buf);
  mem->setBuf(nullptr);
  mem_res_list_.erase(std::find(mem_res_list_.begin(),
                                mem_res_list_.end(), buf));
}

void xt_cl_device::freeDeviceResources() {
  XoclThread::Mutex::Mon mon(mt_);

  XOCL_DPRINTF("%s\n", __func__);

  for (xrp_buffer *buf : mem_res_list_)
    xrp_release_buffer(buf);

  mem_res_list_.clear();

  // Free buffer which for PRINTF
  // if is a subdevice, can not free io_buf_
  if (io_buf_ != nullptr && !is_sub_device_) {
    for (unsigned i = 0; i < getNumDSP(); ++i)
      xrp_release_buffer(io_buf_[i]);
    delete [] io_buf_;
    io_buf_ = nullptr;
  }
}

bool xt_cl_device::isValidOffset(cl_mem mem, size_t offset) {
  // FIXME: TODO!
  // (char *)mem->host_acc_dev_ptr_ + offset is aligned on cacheline size?
  return true;
}

buf_obj xt_cl_device::allocDevMem(cl_context ctx, size_t size) {
  XoclThread::Mutex::Mon mon(mt_);
  xrp_status status = XRP_STATUS_SUCCESS;
  // Note: Buffer can be shared between devices, so just create buffer on
  //       dsp0
  xrp_buffer *buf;
  HANDLE_XRP_FAIL(buf = xrp_create_buffer(getXrpDspWithCoreID(0),
                                          size,
                                          NULL,
                                          &status));
  mem_res_list_.push_back(buf);
  return buf;

FAILED:
  return nullptr;
}

void xt_cl_device::freeDevMem(buf_obj dev_buf) {
  XoclThread::Mutex::Mon mon(mt_);
  if (!isMyMemRes(dev_buf)) {
    XOCL_ERR("Can not free %p, it is not device pointer.\n", dev_buf);
    return;
  }
  xrp_release_buffer(dev_buf);
  mem_res_list_.erase(std::find(mem_res_list_.begin(),
                                mem_res_list_.end(), dev_buf));
}

void xt_cl_device::copyToDevMem(buf_obj dev_buf, const void *src, size_t size) {
  if (!isMyMemRes(dev_buf)) {
    XOCL_ERR("%p is not device pointer.\n", dev_buf);
    return;
  }

  void *dst = xrp_map_buffer(dev_buf, 0, size, XRP_WRITE, NULL);
  if (!dst) {
    XOCL_ERR("Failed copy to dev memory %p, src %p size %zd\n",
             dev_buf, src, size);
    return;
  }

  memcpy(dst, src, size);

  // Makes latest buffer content visible to device
  xrp_unmap_buffer(dev_buf, dst, NULL);
}

void xt_cl_device::createSyncBuffer() {
  if (is_init_sync_buf_)
    return;
  // Create sync buffer in core0 and pass to all cores.
  xrp_status status = XRP_STATUS_SUCCESS;
  // Allocate #procs * device cache line size
  unsigned size = getNumDSP() * XOCL_MAX_DEVICE_CACHE_LINE_SIZE;
  CHECK_XRP_STATUS(syn_buf_ = xrp_create_buffer(getXrpDspWithCoreID(0), size,
                                                NULL, &status));
  // Zero initialize
  void *buf = nullptr;
  CHECK_XRP_STATUS(buf = xrp_map_buffer(syn_buf_, 0, size,
                                        XRP_READ_WRITE, &status));
  memset(buf, 0, size);
  xrp_unmap_buffer(syn_buf_, buf, NULL);

  is_init_sync_buf_ = true;
}

void xt_cl_device::createPrintBuffer() {
  // Create buffer for IO
  if (!mxpa::RegisteredKernels::isCallPrintfFun() || is_init_print_buf_)
    return;
  io_buf_ = new xrp_buffer*[getNumDSP()];
  for (unsigned dspid = 0; dspid < getNumDSP(); ++dspid) {
    xrp_status status = XRP_STATUS_SUCCESS;
    CHECK_XRP_STATUS(
      io_buf_[dspid] = xrp_create_buffer(getXrpDspWithCoreID(dspid),
                                         getPrintfBufferSize(),
                                         NULL,
                                         &status));
  }
  is_init_print_buf_ = true;
}

int 
xt_cl_device::createSubDevices(const cl_device_partition_property *properties,
                               cl_uint num_devices, cl_device_id *out_devices, 
                               cl_uint *num_devices_ret) {
  if (properties[0] == CL_DEVICE_PARTITION_EQUALLY &&
      properties[1] > (int)num_dsps_)
    return CL_OUT_OF_RESOURCES;

  if (properties[0] == CL_DEVICE_PARTITION_BY_COUNTS) {
    int device_num = 0;
    unsigned device_sum = 0;
    while (CL_DEVICE_PARTITION_BY_COUNTS_LIST_END != properties[++device_num])
      device_sum += properties[device_num];
    if (device_sum > num_dsps_)
      return CL_OUT_OF_RESOURCES;
  }

  if (properties[0] == CL_DEVICE_PARTITION_EQUALLY) {
    int device_num = num_dsps_/properties[1] + 
                     (num_dsps_ % properties[1] ? 1 : 0);
    if (num_devices_ret)
      *num_devices_ret = device_num;
    if (out_devices) {
      for (int i = 0; i < device_num; ++i) {
        xt_cl_device *sub_dev = new xt_cl_device(this);
        if (i < device_num-1)
          sub_dev->setDspNum(properties[1]);
        else
          sub_dev->setDspNum(num_dsps_ - (properties[1] * (device_num - 1)));
        sub_dev->setDspID(i*properties[1]);
        sub_dev->setSubDeviceType(properties);
        out_devices[i] = sub_dev;
      }
    }
  } else if (properties[0] == CL_DEVICE_PARTITION_BY_COUNTS) {
    int device_num = 0;
    while (CL_DEVICE_PARTITION_BY_COUNTS_LIST_END != properties[++device_num]) {
      if (out_devices) {
        xt_cl_device *sub_dev = new xt_cl_device(this);
        sub_dev->setDspNum(properties[device_num]);
        sub_dev->setDspID(
            device_num == 1 
              ? 0 
              : (((xt_cl_device *)out_devices[device_num - 2])->getDspID() +
                 properties[device_num - 1])
        );
        sub_dev->setSubDeviceType(properties);
        out_devices[device_num - 1] = sub_dev;
      }
    }
    if (num_devices_ret)
      *num_devices_ret = device_num - 1;
  } else {
    return CL_DEVICE_PARTITION_FAILED;
  }

  return 0;
}

xt_cl_device *xt_cl_device::xt_dev_ = nullptr;

cl_device_id initDevice() {
  XOCL_DPRINTF("initDevice: ...\n");
  xt_cl_device *xtDevice = xt_cl_device::getXtDevice();
  int err = xtDevice->initDspCores(xt_cl_device::getNumDSP());
  if (err) {
    XOCL_ERR("initDevice: Error initializing DSPs\n");
    return nullptr;
  }
  xtDevice->createSyncBuffer();

  // Query available local memory size
  {
    xrp_status status = XRP_STATUS_SUCCESS;
    unsigned localMemSize;
    xocl_generic_cmd cmd;
    cmd.CMD = XOCL_CMD_QUERY_LOCAL_MEM_SIZE;
    CHECK_XRP_STATUS(xrp_run_command_sync(xtDevice->getXrpQueueWithCoreID(0),
                                          &cmd, sizeof(cmd),
                                          &localMemSize, sizeof(localMemSize),
                                          NULL, &status));
    xtDevice->setLocalMemSize(localMemSize);
    XOCL_DPRINTF("initDevice: Local Memory Size %d\n", localMemSize);
  }
  return xtDevice;
}

void unInitDevice() {
  XOCL_DPRINTF("unInitDevice:...\n");
  delete xt_cl_device::getXtDevice();
#ifdef XRP_SIM
  xrp_exit();
#endif
}
