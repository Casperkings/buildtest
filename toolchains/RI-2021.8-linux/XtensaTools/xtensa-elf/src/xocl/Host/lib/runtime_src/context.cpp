/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "cl_internal_objects.h"
#include "mxpa_debug.h"
#include <algorithm>

_cl_context::_cl_context(cl_platform_id plat,
                         const cl_context_properties *properties,
                         const cl_device_id *device_id_list, size_t listSize)
  : plat_(plat) 
{
  assert(device_id_list && listSize && "Invalid device list.");
  device_list_ = (cl_device_id *)malloc(sizeof(cl_device_id) * listSize);
  memcpy(device_list_, device_id_list, sizeof(cl_device_id) * listSize);
  device_count_ = (cl_uint)listSize;
  context_properties_ = nullptr;
  context_properties_len_ = 0;
  if (properties) {
    int len = 0;
    cl_context_properties *p = const_cast<cl_context_properties *>(properties);
    while (p && ((int)(*p) != 0)) {
      len += 2;
      p += 2;
    }
    // end with 0
    len = (len + 1) * sizeof(cl_context_properties);
    context_properties_ = (cl_context_properties *)malloc(len);
    memcpy(context_properties_, properties, len);
    context_properties_len_ = len;
  }
  for(size_t i = 0; i < device_count_; ++i)
    device_list_[i]->retain();
}

_cl_context::~_cl_context() {
  plat_ = nullptr;
  if (context_properties_) {
    free(context_properties_);
    context_properties_ = nullptr;
  }
  destroyAllBuffers();
  for(size_t i = 0; i < device_count_; ++i)
    device_list_[i]->release();

  if (device_list_) {
    free(device_list_);
    device_list_ = nullptr;
  }
}

cl_event _cl_context::createUserEvent(cl_int *errcode_ret) {
  cl_event ret = _cl_event::create(cmdq_);
  if (ret) {
    if (errcode_ret != nullptr)
      *errcode_ret = CL_SUCCESS;
    ret->setUserEventStatus(CL_SUBMITTED);
    ret->setCommandType(CL_COMMAND_USER);
    return ret;
  } else {
    if (errcode_ret != nullptr)
      *errcode_ret = CL_OUT_OF_RESOURCES;
  }
  return nullptr;
}

cl_int _cl_context::createBuffer(cl_mem *out, cl_mem_flags flags,
                                 size_t size, void *host_ptr) {
  XoclThread::Mutex::Mon mon(mt_);
  *out = nullptr;
  _cl_mem *new_mem = new _cl_mem(this, flags, size, nullptr /*host_ptr*/,
                                 nullptr /*parent*/, 0 /*offset*/);
  if (!new_mem)
    return CL_OUT_OF_HOST_MEMORY;

  if (flags & CL_MEM_USE_HOST_PTR) {
    new_mem->host_ptr_ = host_ptr;
  }

  cl_int status = device_list_[0]->allocMem(new_mem, host_ptr);
  if (status != CL_SUCCESS) {
    delete new_mem;
    return status;
  }

  *out = new_mem;
  buffers_.push_back(new_mem);
  return CL_SUCCESS;
}

cl_int _cl_context::createSubBuffer(cl_mem *out, cl_mem parent,
                                    cl_mem_flags flags, size_t size, 
                                    size_t offset) {
  XoclThread::Mutex::Mon mon(mt_);
  *out = nullptr;
  _cl_mem *new_mem = new _cl_mem(this, flags, size,
                                 nullptr /*host_ptr*/, parent, offset);
  if (!new_mem)
    return CL_OUT_OF_HOST_MEMORY;

  if (!device_list_[0]->isValidOffset(parent, offset)) {
    delete new_mem;
    return CL_MISALIGNED_SUB_BUFFER_OFFSET;
  }

  if (parent->host_ptr_)
    new_mem->host_ptr_ = (void *)((char *)parent->host_ptr_ + offset);

  new_mem->setBuf(parent->getBuf());
  *out = new_mem;
  buffers_.push_back(new_mem);
  return CL_SUCCESS;
}

void _cl_context::destroyBuffer(cl_mem mem) {
  XoclThread::Mutex::Mon mon(mt_);
  std::vector<cl_mem>::iterator it = std::find(buffers_.begin(), 
                                               buffers_.end(), mem);
  if (it == buffers_.end()) {
    XOCL_DPRINTF("Warning! Try to destroy cl_mem object (%p) by "
                 "wrong context (%p)\n", mem, this);
    return;
  }

  if (!mem->isSubBuffer())
    device_list_[0]->freeMem(mem);

  buffers_.erase(it);
}

void _cl_context::destroyAllBuffers() {
  XoclThread::Mutex::Mon mon(mt_);
  for (unsigned i = 0; i < buffers_.size(); ++i) {
    cl_mem mem = buffers_[i];
    if (!mem->isSubBuffer())
      device_list_[0]->freeMem(mem);
    delete mem;
  }
  buffers_.clear();
}

bool _cl_context::isValidMemoryObject(cl_mem mem) const {
  return std::find(buffers_.begin(), buffers_.end(), mem) != buffers_.end();
}

buf_obj _cl_context::allocDevMem(size_t size) {
  return device_list_[0]->allocDevMem(this, size);
}

void _cl_context::freeDevMem(buf_obj dev_buf) {
  return device_list_[0]->freeDevMem(dev_buf);
}

void _cl_context::copyToDevMem(buf_obj dev_buf, const void *src, size_t size) {
  return device_list_[0]->copyToDevMem(dev_buf, src, size);
}

void *_cl_context::allocDevLocalMem(size_t size) {
  return device_list_[0]->allocDevLocalMem(this, size);
}

void _cl_context::freeDevLocalMem(void *dev_ptr) {
  return device_list_[0]->freeDevLocalMem(dev_ptr);
}
