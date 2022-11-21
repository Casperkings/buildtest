/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XT_DEVICE_H__
#define __XT_DEVICE_H__

#include "cl_internal_objects.h"
#include "host_interface.h"
#include "xrp_api.h"
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <vector>

// Reserve space for buffering kernel printfs
#define XOCL_PRINT_BUFFER_SIZE (1*1024*1024)

#define CHECK_XRP_STATUS(API)                                 \
  do {                                                        \
    (API);                                                    \
    if (status != XRP_STATUS_SUCCESS) {                       \
      fprintf(stderr, "xrp driver failed: %s:%d, err = %d\n", \
              __FILE__, __LINE__, status);                    \
    }                                                         \
  } while (0)

#define HANDLE_XRP_FAIL(API)                                  \
  do {                                                        \
    (API);                                                    \
    if (status != XRP_STATUS_SUCCESS) {                       \
      fprintf(stderr, "xrp driver failed: %s:%d, err = %d\n", \
              __FILE__, __LINE__, status);                    \
      goto FAILED;                                            \
    }                                                         \
  } while (0)

class xt_cl_device : public _cl_device_id {
public:
  static unsigned getNumDSP() { 
    return XOCL_NUM_DSPS;
  }

  bool isMyMemRes(xrp_buffer *mem);

  void *mapBuf(buf_obj buf, unsigned offset, unsigned size,
               xocl_mem_access_flag map_flag) override;

  void unmapBuf(buf_obj buf, void *virt_ptr) override;

  cl_int allocMem(cl_mem mem, void *host_ptr) override;
  void freeMem(cl_mem mem) override;
  void freeDeviceResources() override;
  bool isValidOffset(cl_mem mem, size_t offset) override;

  // return device pointer.
  buf_obj allocDevMem(cl_context ctx, size_t size) override;
  void freeDevMem(buf_obj dev_buf) override;
  void copyToDevMem(buf_obj dev_buf, const void *src, size_t size) override;

  // We don't do real allocation, instead, we just pass size to driver, and
  // driver will allocate it when executing kernel.
  // See: xtKernel::FeedLocalSizeToArg()
  void *allocDevLocalMem(cl_context ctx, size_t size) override {
    return nullptr;
  }

  void freeDevLocalMem(void *dev_ptr) override {}

  size_t getPrintfBufferSize() override { 
    return XOCL_PRINT_BUFFER_SIZE;
  }

  void createPrintBuffer() override;
  void createSyncBuffer() override;

  size_t getLocalMemSize() const override {
    return local_mem_size_;
  }

  size_t getDeviceAddressBits() const override {
    return sizeof(unsigned)* 8;
  }

  int createSubDevices(const cl_device_partition_property *properties,
                       cl_uint num_devices,
                       cl_device_id *out_devices,
                       cl_uint *num_devices_ret) override;

  cl_device_partition_property *getSubDeviceType() const override {
    return properties_;
  }

  void release() override {
    XoclThread::Mutex::Mon mon(mt_);
    if (ref_count_ < 1)
      return;
    --ref_count_;
    if (ref_count_ == 0) {
      if (is_sub_device_)
        delete this;
      else
        freeDeviceResources();
    }
  }

  void setSubDeviceType(const cl_device_partition_property* properties) {
    if (properties_) {
      free(properties_);
      properties_ = nullptr;
    }
    int device_num = 0;
    while (0 != properties[device_num])
      ++device_num;
    ++device_num;
    properties_ = (cl_device_partition_property*)
                  std::malloc(sizeof(cl_device_partition_property) * 
                              device_num);
    std::memcpy(properties_, properties, 
                sizeof(cl_device_partition_property) * device_num);
  }

  void setLocalMemSize(size_t size) { 
    local_mem_size_ = size; 
  }

  unsigned getProcCount() const override {
    return num_dsps_; 
  }

  virtual unsigned getMaxSubDeviceCount() const {
    return 0; 
  }

  buf_obj getDspPrintfBuffer(unsigned core_id) const {
    assert(core_id < xt_cl_device::getNumDSP() && "invalid core_id");
    return io_buf_[core_id];
  }

  buf_obj getDspSyncBuffer() const {
    return syn_buf_;
  }

  int initDspCores(unsigned core_num);
  void releaseDspCores(unsigned core_num);

  struct xrp_device *getXrpDspWithCoreID(unsigned core_id) const {
    assert(core_id < getNumDSP() && "invalid core_id");
    return xrp_dsp_list_[core_id];
  }

  struct xrp_queue *getXrpQueueWithCoreID(unsigned core_id) const {
    assert(core_id < getNumDSP() && "invalid core_id");
    return xrp_queue_list_[core_id];
  }

  xt_cl_device() : local_mem_size_(0) {
    pref_min_wg_num_ = 8;
    pref_wg_size_mul_ = 512;
    max_wg_size_ = 1024;
    io_buf_ = nullptr;
    syn_buf_ = nullptr;
    is_init_print_buf_ = false;
    is_init_sync_buf_ = false;
    is_sub_device_ = false;
    num_dsps_ = XOCL_NUM_DSPS;
    dsp_id_ = 0;
    properties_ = nullptr;
  }

  xt_cl_device(xt_cl_device* parent_dev) :
    local_mem_size_(parent_dev->local_mem_size_),
    io_buf_(parent_dev->io_buf_),
    syn_buf_(parent_dev->syn_buf_),
    is_init_print_buf_(parent_dev->is_init_print_buf_),
    is_init_sync_buf_(parent_dev->is_init_sync_buf_),
    xrp_dsp_list_(parent_dev->xrp_dsp_list_),
    xrp_queue_list_(parent_dev->xrp_queue_list_),
    is_sub_device_(true) 
  {
    pref_wg_size_mul_ = parent_dev->pref_wg_size_mul_;
    max_wg_size_ = parent_dev->max_wg_size_;
    pref_min_wg_num_ = parent_dev->pref_min_wg_num_;
    properties_ = nullptr;
  }

  virtual ~xt_cl_device() {
    freeDeviceResources();
    if (!is_sub_device_) {
      releaseDspCores(getNumDSP());
    }
    if (properties_) {
      free(properties_);
      properties_ = NULL;
    }
  }

  int launchKernelsOnDevice(size_t use_num_cores,
                            void *shared_buf_group,
                            bool use_printf,
                            unsigned long long id) override {
    assert(use_num_cores <= getNumDSP());
    buf_group buf_gp = (buf_group)shared_buf_group;
    unsigned cmd_size = sizeof(xocl_launch_kernel_cmd);
    xocl_launch_kernel_cmd* launch_cmd = 
                            (xocl_launch_kernel_cmd *)std::malloc(cmd_size);
    std::memset(launch_cmd, 0, cmd_size);
    xrp_status status = XRP_STATUS_SUCCESS;
    launch_cmd->CMD = XOCL_CMD_EXE_KERNEL;
    // use shared buffer group if it is not null.
    buf_obj synbuf = getDspSyncBuffer();
    unsigned syn_id;
    CHECK_XRP_STATUS(syn_id = (unsigned)
                              xrp_add_buffer_to_group(buf_gp, synbuf,
                                                      XRP_READ_WRITE, &status));
    launch_cmd->syn_buf_id = syn_id;
    launch_cmd->syn_buf_size = getNumDSP()*XOCL_MAX_DEVICE_CACHE_LINE_SIZE;
    launch_cmd->id = id;

    unsigned start_id = getDspID();
    unsigned end_id = start_id + use_num_cores;

    xrp_event **events = (xrp_event **)
                         std::malloc(sizeof(xrp_event *)*(end_id-start_id));
    int *ret_val = (int *)std::malloc(sizeof(int) * (end_id-start_id));

    launch_cmd->master_proc_id = start_id;
    launch_cmd->proc_num = use_num_cores;

    dev_lock();

    for (unsigned i = start_id; i < end_id; ++i) {
      if (use_printf) {
        buf_obj pbuf = getDspPrintfBuffer(i);
        unsigned buf_id;
        CHECK_XRP_STATUS(
            buf_id = (unsigned)
                     xrp_add_buffer_to_group(buf_gp, pbuf,
                                             XRP_READ_WRITE, &status));
        launch_cmd->printf_buf_id = buf_id;
        launch_cmd->printf_buf_size = getPrintfBufferSize();
      }

      launch_cmd->proc_id = i - start_id;

      CHECK_XRP_STATUS(xrp_enqueue_command(getXrpQueueWithCoreID(i),
                                           launch_cmd,
                                           cmd_size,
                                           &ret_val[i-start_id],
                                           sizeof(int),
                                           buf_gp,
                                           &events[i-start_id],
                                           &status));
    }

    dev_unlock();

    for (unsigned i = start_id; i < end_id; ++i) {
      CHECK_XRP_STATUS(xrp_wait(events[i-start_id], &status));
      CHECK_XRP_STATUS(xrp_release_event(events[i-start_id]));
    }

    int cl_status = CL_SUCCESS;
    for (unsigned i = start_id; i < end_id; ++i) {
      if (ret_val[i-start_id] == CL_OUT_OF_RESOURCES) {
        cl_status = CL_OUT_OF_RESOURCES;
        break;
      }
    }

    if (use_printf) {
      char *msg;
      for (unsigned i = start_id; i < end_id; ++i) {
        buf_obj pbuf = getDspPrintfBuffer(i);
        CHECK_XRP_STATUS(msg = (char *)mapBuf(pbuf, 0, getPrintfBufferSize(),
                                              MEM_READ));
        fprintf(stdout, "%s", msg);
        unmapBuf(pbuf, msg);
      }
    }
    free(events);
    free(ret_val);
    free(launch_cmd);
    return cl_status;
  }

  void setDspNum(unsigned num_dsps) { 
    num_dsps_ = num_dsps; 
  }

  void setDspID(unsigned dsp_id) { 
    dsp_id_ = dsp_id; 
  }

  unsigned getDspID() { 
    return dsp_id_; 
  }

  static xt_cl_device* getXtDevice() {
    if (xt_dev_ == NULL)
      xt_dev_ = new xt_cl_device();
    return xt_dev_;
  }

private:
  std::vector<xrp_buffer *> mem_res_list_;
  size_t local_mem_size_;
  xrp_buffer **io_buf_;
  xrp_buffer *syn_buf_;
  bool is_init_print_buf_;
  bool is_init_sync_buf_;
  struct xrp_device **xrp_dsp_list_; // maybe more then 1 DSP cores
  struct xrp_queue **xrp_queue_list_;
  unsigned char module_uuid_[16];
  unsigned num_dsps_;
  unsigned dsp_id_;
  unsigned is_sub_device_;
  cl_device_partition_property *properties_;
  static xt_cl_device *xt_dev_;
};

#endif // __XT_DEVICE_H__
