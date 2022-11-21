/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "cl_internal_objects.h"

_cl_event::_cl_event(cl_command_queue cmd_queue):
    OBJ(),
    cmd_q_(cmd_queue),
    event_status_(CL_COMPLETE),
    command_type_(CL_COMMAND_NDRANGE_KERNEL),
    queue_time_(0), start_time_(0), submit_time_(0), end_time_(0), is_set_(0),
    user_stop_(0),
    evt_(XoclThread::Event::create(false)),
    mt_(XoclThread::Mutex::create()) 
  {
    is_create_ = 1;
    wait_usrevt_num_ = 0;
    user_data_queue_ = new std::queue<void *>();
    pfn_notify_queue_ = new std::queue<NOTIFY_FUNC>();
    cmd_q_->insertAssociatedEvent(this);
  }

_cl_event::~_cl_event() {
  cmd_q_->eraseAssociatedEvent(this);

  delete pfn_notify_queue_;
  delete user_data_queue_;

  if (evt_ != nullptr) {
    evt_->release();
    evt_ = nullptr;
  }

  if (mt_ != nullptr) {
    mt_->release();
    mt_ = nullptr;
  }

  pfn_notify_queue_ = nullptr;
  user_data_queue_ = nullptr;
}

cl_int _cl_event::setUserEventStatus(cl_int execution_status) {
  event_status_ = execution_status;
  cl_int ret = CL_SUCCESS;

  if (pfn_notify_queue_ != nullptr && user_data_queue_ != nullptr) {
    XoclThread::Mutex::Mon mon(mt_);

    while (!pfn_notify_queue_->empty()) {
      if (execution_status < 0)
        return ret;
      NOTIFY_FUNC fn = pfn_notify_queue_->front();
      void *user_data = user_data_queue_->front();
      fn(this, this->event_status_, user_data);
      pfn_notify_queue_->pop();
      user_data_queue_->pop();
      this->release();
    }

    if (0 > execution_status) {
      user_stop_ = 1;
      evt_->set();
    }

    if (CL_COMPLETE == execution_status)
      evt_->set();
    else if ((CL_COMPLETE != execution_status) && (execution_status >= 0))
      ret = CL_INVALID_VALUE;
  }

  return ret;
}

_cl_event *_cl_event::create(cl_command_queue cmd_queue) {
  return new _cl_event(cmd_queue);
}

cl_int  _cl_event::getProfilingInfo(cl_profiling_info param_name,
                                    size_t param_value_size,
                                    void *param_value,
                                    size_t *param_value_size_ret) {
  cl_int ret_code = CL_SUCCESS;

  if (param_name != CL_PROFILING_COMMAND_QUEUED &&
      param_name != CL_PROFILING_COMMAND_SUBMIT &&
      param_name != CL_PROFILING_COMMAND_START &&
      param_name != CL_PROFILING_COMMAND_END)
    ret_code = CL_INVALID_VALUE;
  else if (param_value_size < sizeof(cl_ulong))
    ret_code = CL_INVALID_VALUE;
  else if (param_value) {
    // will extend for get the real information.
    if (param_name == CL_PROFILING_COMMAND_QUEUED)
      *((cl_ulong *)param_value) = getQueue_time();
    else if (param_name == CL_PROFILING_COMMAND_SUBMIT)
      *((cl_ulong *)param_value) = getSubmit_time();
    else if (param_name == CL_PROFILING_COMMAND_START)
      *((cl_ulong *)param_value) = getStart_time();
    else if (param_name == CL_PROFILING_COMMAND_END)
      *((cl_ulong *)param_value) = getEnd_time();
  }

  if (param_value_size_ret != nullptr)
    *param_value_size_ret = sizeof(cl_ulong);

  if ((command_type_ == CL_COMMAND_USER) || (event_status_ == CL_SUBMITTED))
    ret_code = CL_PROFILING_INFO_NOT_AVAILABLE;

  if ((0 == param_value_size) && (nullptr == param_value))
    ret_code = CL_SUCCESS;

  return ret_code;
}

cl_int _cl_event::getInfo(cl_event_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_EVENT_COMMAND_EXECUTION_STATUS: {
    if (param_value_size_ret != nullptr)
      *param_value_size_ret = sizeof(cl_int);
    if (param_value != nullptr)
      *((cl_int *)param_value) = this->event_status_;
    return CL_SUCCESS;
  }
  case CL_EVENT_COMMAND_QUEUE:{
    if (param_value_size_ret != nullptr)
      *param_value_size_ret = sizeof(cl_command_queue);
    if (param_value != nullptr) {
      if (command_type_ == CL_COMMAND_USER)
        *((cl_command_queue *)param_value) = nullptr;
      else
        *((cl_command_queue *)param_value) = cmd_q_;
    }
    return CL_SUCCESS;
  }
  case CL_EVENT_COMMAND_TYPE: {
    if (param_value_size_ret != nullptr)
      *param_value_size_ret = sizeof(cl_command_type);
    if (param_value != nullptr)
      *((cl_command_type *)param_value) = this->command_type_;
    return CL_SUCCESS;
  }
  case CL_EVENT_REFERENCE_COUNT: {
    if (param_value_size_ret != nullptr)
      *param_value_size_ret = sizeof(cl_uint);
    if (param_value != nullptr)
      *((cl_uint *)param_value) = ref_count_;
    return CL_SUCCESS;
  }
  case CL_EVENT_CONTEXT: {
    if (param_value_size_ret != nullptr)
      *param_value_size_ret = sizeof(cl_context);
    if (param_value != nullptr)
      *((cl_context *)param_value) = cmd_q_->getContext();
    return CL_SUCCESS;
  }
  }
  return CL_INVALID_VALUE;
}

cl_int _cl_event :: setEventCallback(cl_int command_exec_callback_type,
                                     void (CL_CALLBACK *pfn_event_notify)(
                                             cl_event event,
                                             cl_int event_command_exec_status,
                                             void *user_data),
                                     void *user_data) {
  XoclThread::Mutex::Mon mon(mt_);

  if (CL_COMPLETE == event_status_) {
    pfn_event_notify(this, event_status_, user_data);
  } else {
    this->retain();
    pfn_notify_queue_->push(pfn_event_notify);
    user_data_queue_->push(user_data);
  }
  return CL_SUCCESS;
}
