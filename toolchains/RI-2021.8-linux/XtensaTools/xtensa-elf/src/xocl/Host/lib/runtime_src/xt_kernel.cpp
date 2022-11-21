/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <cstring> 
#include "kernel.h"
#include "xt_device.h"
#include "host_interface.h"

using namespace mxpa;

class xtKernel : public Kernel {
public:
  xtKernel(mxpa::KernelRegistry *kr, cl_context ctx);
  xtKernel(mxpa::Kernel* k);
  ~xtKernel();

  // Returns access range lower bound based on specified 
  // argument and batch number.
  size_t argumentLowerBound(unsigned arg_no, size_t batch) const override {
    assert(false && "not impl.");
    return 0;
  }

  // Returns access range upper bound based on specified argument 
  // and aatch number.
  size_t argumentUpperBound(unsigned arg_no, size_t batch) const override {
    assert(false && "not impl.");
    return 0;
  }

  // todo: we should already knew expected value of *isBuf in compilation time
  //       (do it in ArgumentDirection() or something else)
  const void *getArgument(unsigned arg_no, bool *isBuf = NULL) const override {
    if (isBuf) 
      *isBuf = false;
    return nullptr;
  }

  void invokeBatchByRange(size_t, size_t) override {
    assert(false && "not impl.");
  }

  void invokeGroupByID(size_t batch, size_t proc_num) override;

  void invokeGroupByRange(size_t proc_num,
                          buf_group shared_buf_group,
                          bool use_printf) override;

  int passParameters2Device(size_t use_num_cores,
                            buf_group shared_buf_group,
                            unsigned long long id) override;

  cl_int init();
  unsigned getLaunchCmdSize() const;

private:
  xocl_launch_kernel_cmd *launch_cmd_;
  xrp_buffer_group *xrp_buf_gp_;
};

xtKernel::xtKernel(mxpa::KernelRegistry *kr, cl_context ctx)
  : Kernel(kr, ctx), launch_cmd_(nullptr), xrp_buf_gp_(nullptr) {}

xtKernel::xtKernel(mxpa::Kernel* k)
  : Kernel(k), launch_cmd_(nullptr), xrp_buf_gp_(nullptr) {}

xtKernel::~xtKernel() {
  if (launch_cmd_) {
    std::free(launch_cmd_);
    launch_cmd_ = nullptr;
  }
  kernel_registry_ = nullptr;
  if (xrp_buf_gp_) {
    xrp_release_buffer_group(xrp_buf_gp_);
    xrp_buf_gp_ = nullptr;
  }
}

cl_int xtKernel::init() {
  // init launch_cmd_
  // we allocate one more space for arguments, but it's ok.
  unsigned cmd_size = getLaunchCmdSize();
  launch_cmd_ = (xocl_launch_kernel_cmd *)std::malloc(cmd_size);

  memset(launch_cmd_, 0, cmd_size);
  launch_cmd_->num_args = num_args_;

  // init arg_size_ with 0
  args_size_.resize(num_args_, 0);
  return CL_SUCCESS;
}

unsigned xtKernel::getLaunchCmdSize() const {
  return sizeof(xocl_launch_kernel_cmd) + sizeof(xocl_krnl_arg) * num_args_;
}

int xtKernel::passParameters2Device(size_t use_num_cores,
                                    buf_group shared_buf_group,
                                    unsigned long long id) {
  assert(use_num_cores <= xt_cl_device::getNumDSP());
  launch_cmd_->kernel_id = RegisteredKernels::getKernelId(kernel_registry_);
  launch_cmd_->proc_num = use_num_cores;

  std::memcpy(launch_cmd_->global_size, global_size_, sizeof(global_size_));
  std::memcpy(launch_cmd_->local_size, local_size_, sizeof(local_size_));
  std::memcpy(launch_cmd_->global_offset, global_offset_, 
              sizeof(global_offset_));
  launch_cmd_->work_dim = work_dim_;
  launch_cmd_->id = id;

  xrp_status status = XRP_STATUS_SUCCESS;
  if (!shared_buf_group) {
    if (xrp_buf_gp_)
      xrp_release_buffer_group(xrp_buf_gp_);

    CHECK_XRP_STATUS(xrp_buf_gp_ = xrp_create_buffer_group(&status));
  }

  // use shared buffer group if it is not null.
  buf_group buf_gp = shared_buf_group ? shared_buf_group : xrp_buf_gp_;

  for (unsigned i = 0; i < num_args_; ++i) {
    launch_cmd_->arguments[i].buf_id = XOCL_NULL_PTR_BUF;
    launch_cmd_->arguments[i].buf_size = args_size_[i];
    launch_cmd_->arguments[i].offset = 0;

    OCL_ARG_T arg_ty = args_type_[i];
    if (arg_ty != OCL_ARG_T_MEM && arg_ty != OCL_ARG_T_PRIVATE)
      continue;

    // arg is not an null pointer
    if (arguments_[i]) {
      xrp_buffer *xbuf;
      if (arg_ty == OCL_ARG_T_MEM) {
        _cl_mem *mem = (_cl_mem *)arguments_[i];
        xbuf = mem->getBuf();
        launch_cmd_->arguments[i].buf_size = mem->getSize();
        launch_cmd_->arguments[i].offset = mem->getOffset();
      } else
        xbuf = (xrp_buffer *)arguments_[i];

      xrp_status status = XRP_STATUS_SUCCESS;
      unsigned buf_id;
      CHECK_XRP_STATUS(
        buf_id = (unsigned)xrp_add_buffer_to_group(buf_gp, xbuf,
                                                   XRP_READ_WRITE, &status));
      launch_cmd_->arguments[i].buf_id = buf_id;
    }
  }

  launch_cmd_->CMD = XOCL_CMD_SET_KERNEL;
  xt_cl_device *xtdev = (xt_cl_device *)getCLKrnlObj()->getBindedDevice();
  unsigned start_id = xtdev->getDspID();
  unsigned end_id = start_id + use_num_cores;
  launch_cmd_->master_proc_id = start_id;

  xtdev->dev_lock();

  int cl_status = CL_SUCCESS;
  for (unsigned i = start_id; i < end_id; ++i) {
    int result;
    launch_cmd_->proc_id = i-start_id;
    CHECK_XRP_STATUS(xrp_run_command_sync(xtdev->getXrpQueueWithCoreID(i),
                                          launch_cmd_,
                                          getLaunchCmdSize(),
                                          &result,
                                          sizeof(result),
                                          buf_gp,
                                          &status));
    if (result == CL_OUT_OF_RESOURCES) {
      cl_status = result;
      break;
    }
  }

  xtdev->dev_unlock();

  return cl_status;
}

void xtKernel::invokeGroupByRange(size_t use_num_cores,
                                  buf_group shared_buf_group,
                                  bool use_printf) {
  assert(use_num_cores <= xt_cl_device::getNumDSP());
  xrp_status status = XRP_STATUS_SUCCESS;
  launch_cmd_->CMD = XOCL_CMD_EXE_KERNEL;
  xt_cl_device *xtdev = (xt_cl_device *)getCLKrnlObj()->getBindedDevice();

  // use shared buffer group if it is not null.
  buf_group buf_gp = shared_buf_group ? shared_buf_group : xrp_buf_gp_;
  buf_obj synbuf = xtdev->getDspSyncBuffer();
  unsigned syn_id;
  CHECK_XRP_STATUS(syn_id = (unsigned)xrp_add_buffer_to_group(buf_gp, synbuf,
                                                              XRP_READ_WRITE, 
                                                              &status));
  launch_cmd_->syn_buf_id = syn_id;
  launch_cmd_->syn_buf_size = xt_cl_device::getNumDSP() *
                              XOCL_MAX_DEVICE_CACHE_LINE_SIZE;

  unsigned start_id = xtdev->getDspID();
  unsigned end_id = start_id + use_num_cores;

  xrp_event **events = (xrp_event **)
                       malloc(sizeof(xrp_event *)*(end_id-start_id));

  launch_cmd_->master_proc_id = start_id;
  launch_cmd_->proc_num = use_num_cores;

  for (unsigned i = start_id; i < end_id; ++i) {
    if (use_printf) {
      buf_obj pbuf = xtdev->getDspPrintfBuffer(i);
      unsigned buf_id;
      CHECK_XRP_STATUS(
        buf_id = (unsigned)xrp_add_buffer_to_group(buf_gp, pbuf,
                                                   XRP_READ_WRITE, &status));
      launch_cmd_->printf_buf_id = buf_id;
      launch_cmd_->printf_buf_size = xtdev->getPrintfBufferSize();
    }

    launch_cmd_->proc_id = i-start_id;
    CHECK_XRP_STATUS(xrp_enqueue_command(xtdev->getXrpQueueWithCoreID(i),
                                         launch_cmd_,
                                         getLaunchCmdSize(),
                                         NULL,
                                         0,
                                         buf_gp,
                                         &events[i-start_id],
                                         &status));

  }

  for (unsigned i = start_id; i < end_id; ++i)
    CHECK_XRP_STATUS(xrp_wait(events[i-start_id], &status));

  if (xrp_buf_gp_) {
    xrp_release_buffer_group(xrp_buf_gp_);
    xrp_buf_gp_ = nullptr;
  }

  if (use_printf) {
    char *msg;
    for (unsigned i = start_id; i < end_id; ++i) {
      buf_obj pbuf = xtdev->getDspPrintfBuffer(i);
      CHECK_XRP_STATUS(
        msg = (char *)xtdev->mapBuf(pbuf, 0, xtdev->getPrintfBufferSize(),
                                    MEM_READ));
      fprintf(stdout, "%s", msg);
      xtdev->unmapBuf(pbuf, msg);
    }
  }

  free(events);
}

void xtKernel::invokeGroupByID(size_t batch, size_t proc_num) {
  assert(false && "Unreachable.");
}

namespace mxpa {

buf_group createSharedBufGroup() {
  xrp_status status = XRP_STATUS_SUCCESS;
  buf_group bufgrp;
  CHECK_XRP_STATUS(bufgrp = xrp_create_buffer_group(&status));
  return bufgrp;
}

void releaseSharedBufGroup(buf_group bufgrp) {
  xrp_release_buffer_group(bufgrp);
}

Kernel *xtCreateDeviceKernel(KernelRegistry *kern_reg, cl_context ctx, 
                             Kernel* k) {
  xtKernel *kern = nullptr;
  if (k) {
    (void)kern_reg;
    (void)ctx;
    kern = new xtKernel(k);
  } else {
    (void)k;
    kern = new xtKernel(kern_reg, ctx);
  }
  if (kern && kern->init() != CL_SUCCESS) {
    delete kern;
    return nullptr;
  }
  return kern;
}

mxpa::KernelCreator CreateDeviceKernel = xtCreateDeviceKernel;

} // mxpa namespace
