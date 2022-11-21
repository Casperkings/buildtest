/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "cl_internal_objects.h"
#include "mxpa_common.h"

_cl_mem::_cl_mem(cl_context ctx, cl_mem_flags flags, size_t size,
                 void *ptr, cl_mem parent, size_t offset)
  : context_(ctx), flags_(flags), size_(size), offset_(offset),
    parent_mem_(parent), buf_(nullptr), host_ptr_(ptr), phy_ptr_(nullptr) {
  context_->retain();
}

_cl_mem::~_cl_mem() {
  while (!pfn_notify_stack_.empty()) {
    notify_func fn = pfn_notify_stack_.top();
    void *data = user_data_stack_.top();
    fn(this, data);
    pfn_notify_stack_.pop();
    user_data_stack_.pop();
  }

  context_->destroyBuffer(this);

  context_->release();
}

cl_int _cl_mem::setMemObjectDestructorCallback(notify_func pfn_notify,
                                               void *user_data) {
  if (NULL == pfn_notify)
    return CL_INVALID_VALUE;
  pfn_notify_stack_.push(pfn_notify);
  user_data_stack_.push(user_data);
  return CL_SUCCESS;
}

void *_cl_mem::mapBuf(unsigned offset, unsigned size,
                      xocl_mem_access_flag map_flag) {
  // buffer might be sub-buffer, so add buffer->getOffset().
  return context_->getDeviceList()[0]->mapBuf(getBuf(), getOffset() + offset,
                                              size, map_flag);
}

void _cl_mem::unmapBuf(void *virt_ptr) {
  context_->getDeviceList()[0]->unmapBuf(getBuf(), virt_ptr);
}

cl_int _cl_mem::getInfo(cl_mem_info param_name, size_t param_value_size,
                        void *param_value, size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_MEM_TYPE:
    return setParam<cl_mem_object_type>(param_value, CL_MEM_OBJECT_BUFFER,
                                        param_value_size,
                                        param_value_size_ret);
  case CL_MEM_FLAGS:
    return setParam<cl_mem_flags>(param_value, flags_,
                                  param_value_size, param_value_size_ret);
  case CL_MEM_SIZE:
    return setParam<size_t>(param_value, size_,
                            param_value_size, param_value_size_ret);
  case CL_MEM_HOST_PTR:
    return setParam<void *>(param_value, host_ptr_,
                            param_value_size, param_value_size_ret);
  case CL_MEM_MAP_COUNT:
    return setParam<cl_uint>(param_value, 0,
                             param_value_size, param_value_size_ret);
  case CL_MEM_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, (cl_uint)ref_count_,
                             param_value_size, param_value_size_ret);
  case CL_MEM_CONTEXT:
    return setParam<cl_context>(param_value, this->getContext(),
                                param_value_size, param_value_size_ret);
  case CL_MEM_ASSOCIATED_MEMOBJECT:
    return setParam<cl_mem>(param_value, parent_mem_,
                            param_value_size, param_value_size_ret);
  case CL_MEM_OFFSET:
    return setParam<size_t>(param_value, offset_,
                            param_value_size, param_value_size_ret);
  default:
    return CL_INVALID_VALUE;
  }
}
