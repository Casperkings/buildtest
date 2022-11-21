/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <cstring>
#include "cl_internal_objects.h"
#include "mxpa_common.h"
#include "mxpa_debug.h"

static unsigned command_queue_uniq_id = 0;

void _cl_command_queue::asyncWaitEvent(void *data) {
  EventTask *task = (EventTask *)data;
  _cl_event *evt = (_cl_event *) task->evt;
  evt->innerEvent()->wait();
  evt->release();
  XoclThread::FlexMemPool *p = task->mem_pool;
  p->free(task);
}

void _cl_command_queue::doEqueueWaitForEvents(cl_uint num_events,
                                              const cl_event *event_list) {
  std::vector<cl_command_queue> cmdqs;
  for (cl_uint i = 0; i < num_events; ++i)
    cmdqs.push_back(event_list[i]->getCommandQueue());

  sort(cmdqs.begin(), cmdqs.end());
  cmdqs.erase(std::unique(cmdqs.begin(), cmdqs.end()), cmdqs.end());

  // Launch commandqueue->run()
  for (unsigned i = 0; i < cmdqs.size(); ++i)
    cmdqs[i]->run();

  for (cl_uint i = 0; i < num_events; ++i) {
    cl_event evt = event_list[i];
    evt->retain();
    EventTask *task = mem_pool_->alloc<EventTask>();
    task->mem_pool = mem_pool_;
    task->evt = evt;
    sched_->enqueueTask(asyncWaitEvent, (void *)task, nullptr);
  }
}

_cl_command_queue::_cl_command_queue(cl_context c, cl_device_id dev,
                                     cl_command_queue_properties properties) :
  context_(c), dev_(dev), properties_(properties),
  mem_pool_(XoclThread::FlexMemPool::create()), cl_status_(CL_SUCCESS) 
{
  sched_ = new XoclThread::AsyncQueueRunner();
  sched_->startRunner();
  use_printf_ = false;
  shared_buf_group_ = nullptr;
  context_->retain();
  clearNdrangedKernels();
  associatedEvents_.clear();
  id_ = command_queue_uniq_id++;
}

_cl_command_queue::~_cl_command_queue() {
  sched_->stopRunner();
  delete sched_;
  mem_pool_->release();
  mem_pool_ = nullptr;
  clear();
  context_->release();
}

void _cl_command_queue::asyncMapBuffer(void *data) {
  MapBufferTask *task = (MapBufferTask *)data;
  _cl_event *evt = task->evt;
  if (evt != nullptr)
    evt->setSubmit_time();
  // Synchronize cached buffer to host_ptr buffer if we are using
  // CL_MEM_USE_HOST_PTR.
#ifndef XRP_USE_HOST_PTR
  if (task->buffer->getHostPtr()) {
    const void *dev_host_ptr = task->buffer->mapBuf(0, task->buffer->getSize(),
                                                    MEM_READ);
    std::memcpy(task->mapped_ptr, dev_host_ptr, task->buffer->getSize());
    task->buffer->unmapBuf((void *)dev_host_ptr);
  }
#endif
  task->buffer->release();
  if (evt != nullptr) {
    evt->setUserEventStatus(CL_COMPLETE);
    evt->setEnd_time();
    evt->release();
  }
  XoclThread::FlexMemPool *p = task->mem_pool;
  p->free(task);
}

void *_cl_command_queue::enqueueMapBuffer(cl_mem buffer,
                                          cl_bool blocking_map,
                                          cl_map_flags map_flags,
                                          size_t offset,
                                          size_t size,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event *event_wait_list,
                                          cl_event *event,
                                          cl_int *errcode_ret) {
  run();

  cl_int ret = CL_SUCCESS;

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  _cl_event *evt = nullptr;

  if ((event != nullptr) || blocking_map) {
    evt = _cl_event::create(this);
    evt->retain();
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    if (event)
      *event = evt;
  }

  buffer->retain();
  MapBufferTask *task = mem_pool_->alloc<MapBufferTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->evt = evt;
  void* mapped = nullptr;
#ifdef XRP_USE_HOST_PTR
  buffer->getPhyPtr() = buffer->mapBuf(0, buffer->getSize(), MEM_READ_WRITE);
  mapped = (void *)((char *)(buffer->getPhyPtr()) + offset);
  task->mapped_ptr = buffer->getPhyPtr();
  XOCL_DPRINTF("Mapping xrp buffer %p to %p\n", 
               buffer->getBuf(), buffer->getPhyPtr());
#else
  if (void *host_ptr = buffer->getHostPtr()) {
    task->mapped_ptr = host_ptr;
    mapped = (void *)((char *)host_ptr + offset);
  } else {
    buffer->getPhyPtr() = buffer->mapBuf(0, buffer->getSize(), MEM_READ_WRITE);
    mapped = (void *)((char *)(buffer->getPhyPtr()) + offset);
    task->mapped_ptr = buffer->getPhyPtr();
  }
#endif

  sched_->enqueueTask(asyncMapBuffer, (void *)task, nullptr);

  if (blocking_map) {
    evt->innerEvent()->wait();
    if (event == nullptr)
      evt->release();
  }

  if (errcode_ret)
    *errcode_ret = ret;

  return mapped;
}

void  _cl_command_queue::asyncUnMapBuffer(void *data) {
  UnMapBufferTask *task = (UnMapBufferTask *)data;
  _cl_event *evt = task->evt;
  if (evt != nullptr)
    evt->setSubmit_time();
#ifdef XRP_USE_HOST_PTR
  task->buffer->unmapBuf(task->mapped_ptr);
#else
  if (task->buffer->getHostPtr()) {
     void *dev_host_ptr = task->buffer->mapBuf(0, task->buffer->getSize(), 
                                               MEM_WRITE);
    memcpy(dev_host_ptr, task->mapped_ptr, task->buffer->getSize());
    task->buffer->unmapBuf(dev_host_ptr);
  } else {
    task->buffer->unmapBuf(task->mapped_ptr);
  }
#endif
  task->buffer->release();
  if (evt) {
    evt->setUserEventStatus(CL_COMPLETE);
    evt->setEnd_time();
    evt->release();
  }
  XoclThread::FlexMemPool *p = task->mem_pool;
  p->free(task);
}

cl_int  _cl_command_queue::enqueueUnmapMemObject(cl_mem buffer,
                                                 void *mapped_ptr,
                                                 cl_uint num_events,
                                                 const cl_event *wait_events,
                                                 cl_event *outevent) {
  run();

  if (wait_events != nullptr)
    doEqueueWaitForEvents(num_events, wait_events);

  _cl_event *evt = nullptr;
  if (outevent) {
    evt = _cl_event::create(this);
    evt->retain();
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    *outevent = evt;
  }

  buffer->retain();
  UnMapBufferTask *task = mem_pool_->alloc<UnMapBufferTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
#ifdef XRP_USE_HOST_PTR
  task->mapped_ptr = buffer->getPhyPtr();
#else
  if (void *host_ptr = buffer->getHostPtr())
    task->mapped_ptr = host_ptr;
  else if (void *phy_ptr = buffer->getPhyPtr())
    task->mapped_ptr = phy_ptr;
  else
    assert(false && "can not reach here!");
#endif
  task->evt = evt;
  sched_->enqueueTask(asyncUnMapBuffer, (void *)task, nullptr);

  return CL_SUCCESS;
}

void _cl_command_queue::ndRangeKernel(void *data) {
  NDRangeTask *task = (NDRangeTask *)data;
  cl_uint flag = 0;
  for (int kk = 0; kk < task->usrevt_size; ++kk) {
    cl_event tm = task->usrevt_vec[kk];
    if (tm->getExeStatus())
      flag = 1;
  }

  cl_command_queue cmdq = task->cmdq;
  size_t nProcs = cmdq->getDevice()->getProcCount();
  if (task->kernel->isCallPrintfFun())
    cmdq->use_printf_ = true;
  if (cmdq->shared_buf_group_ == nullptr)
    cmdq->shared_buf_group_ = mxpa::createSharedBufGroup();
  //printf("pid: %llx cmdq_id: %x\n", task->pid, cmdq->id_);
  unsigned long long id = (task->pid << 8) | (cmdq->id_ & 0xff);
  int cl_status = task->kernel->passParameters2Device(nProcs, 
                                                      cmdq->shared_buf_group_, 
                                                      id);
  if (cl_status != CL_SUCCESS) {
    if (task->evt != nullptr)
      task->evt->setUserEventStatus(cl_status);
    goto fail;
  }
    
  task->cmdq->insertNdrangedKernels(task->kernel, task->evt);

  if (task->evt != nullptr && flag)
    task->evt->setExeStatus(1);

fail:
  XoclThread::FlexMemPool *p = task->mem_pool;
  p->free(task);
}

cl_int  _cl_command_queue::enqueueWaitForEvents(cl_uint num_events,
                                                const cl_event *event_list) {
  cl_int ret_code = CL_SUCCESS;
  if ((num_events > 2048) || (num_events == 0))
    ret_code = CL_INVALID_VALUE;
  else if (event_list == nullptr)
    ret_code = CL_INVALID_VALUE;
  else {
    doEqueueWaitForEvents(num_events, event_list);
    ret_code = CL_SUCCESS;
  }
  return ret_code;
}

void _cl_command_queue::runKernels(void *data) {
  CommandQueueTask *task = (CommandQueueTask *)data;
  cl_command_queue cmdq = task->cmdq;
  if (cmdq->getNdrangedKernels().size() == 0) {
    XoclThread::FlexMemPool *p = task->mem_pool;
    p->free(task);
    return;
  }

  // Set the status of events as START
  cmdq->setKernelEventAsSubmit();

  size_t nProcs = cmdq->getDevice()->getProcCount();
  if (cmdq->shared_buf_group_ == nullptr)
    cmdq->shared_buf_group_ = mxpa::createSharedBufGroup();
  //printf("pid: %x cmdq_id: %x\n", task->pid, cmdq->id_);
  unsigned long long id = (task->pid << 8) | (cmdq->id_ & 0xff);
  int cl_status = 
          cmdq->getDevice()->launchKernelsOnDevice(nProcs,
                                                   cmdq->shared_buf_group_,
                                                   cmdq->use_printf_,
                                                   id);
  cmdq->cl_status_ = cl_status;
  cmdq->use_printf_ = false;

  if (cmdq->shared_buf_group_) {
    mxpa::releaseSharedBufGroup(cmdq->shared_buf_group_);
    cmdq->shared_buf_group_ = nullptr;
  }

  cmdq->releaseNdrangedKernels();

  XoclThread::FlexMemPool *p = task->mem_pool;
  p->free(task);
}

cl_int _cl_command_queue::enqueueNDRangeKernel(mxpa::Kernel *kernel,
                                              cl_uint num_events_in_wait_list,
                                              const cl_event *event_wait_list,
                                              cl_event *event) {
  _cl_event *evt = nullptr;
  if (event != nullptr) {
    evt =  _cl_event::create(this);
    evt->retain();
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    evt->setWaitUsrEvtNum(num_events_in_wait_list);
    *event = evt;
  }

  mxpa::Kernel *krnl = createKernel(kernel);
  krnl->getCLKrnlObj()->setBindedDevice(getDevice());
  for (auto it : krnl->getMemArgs())
    it.second->retain();

  NDRangeTask *task = mem_pool_->alloc<NDRangeTask>();
  task->mem_pool = mem_pool_;
  task->cmdq = this;
  task->kernel = krnl;
  task->evt  = evt;
  task->usrevt_size = 0;
  task->usrevt_vec = nullptr;
  task->pid = XoclThread::getProcessID();
  if (event_wait_list != nullptr) {
    task->usrevt_size = (int)num_events_in_wait_list;
    task->usrevt_vec = event_wait_list;
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);
  }

  sched_->enqueueTask(ndRangeKernel, (void *)task, nullptr);
  return CL_SUCCESS;
}


void _cl_command_queue::asyncReadBufferRect(void *data) {
  ReadBufferRectTask *t = (ReadBufferRectTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  const char *src = (char *)t->buffer->mapBuf(0, t->buffer->getSize(), 
                                              MEM_READ);
  size_t *buffer_origin = t->buffer_origin;
  size_t *host_origin   = t->host_origin;
  size_t *region        = t->region;
  size_t buffer_row_pitch   = t->buffer_row_pitch;
  size_t buffer_slice_pitch = t->buffer_slice_pitch;
  size_t host_row_pitch     = t->host_row_pitch;
  size_t host_slice_pitch   = t->host_slice_pitch;
  void *ptr = t->ptr;

  size_t buffer_offset = buffer_origin[2] * buffer_slice_pitch + 
                         buffer_origin[1] * buffer_row_pitch + 
                         buffer_origin[0];
  size_t host_offset = host_origin[2] * host_slice_pitch + 
                       host_origin[1] * host_row_pitch + 
                       host_origin[0];

  // Makes latest src content visible to cpu.
  char *dst = (char *)ptr;

  for (size_t depth = 0; depth < region[2]; ++depth) {
    for (size_t height = 0; height < region[1]; ++height) {
      void *dst_line = (void *)(dst + host_offset +
                                depth * host_slice_pitch +
                                height * host_row_pitch);
      void *src_line = (void *)(src + buffer_offset +
                                depth * buffer_slice_pitch +
                                height * buffer_row_pitch);
      std::memcpy(dst_line, src_line, region[0]);
    }
  }

  t->buffer->unmapBuf((void *)src);

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  t->buffer->release();
  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue::enqueueReadBufferRect(cl_mem buffer,
                                                cl_bool blocking_read,
                                                const size_t buffer_origin[3],
                                                const size_t host_origin[3],
                                                const size_t region[3],
                                                size_t buffer_row_pitch,
                                                size_t buffer_slice_pitch,
                                                size_t host_row_pitch,
                                                size_t host_slice_pitch,
                                                void *ptr,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event *event_wait_list,
                                                cl_event *event) {
  run();

  _cl_event *evt = nullptr;
  if ((event != nullptr) || blocking_read) {
    evt = _cl_event::create(this);
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    evt->retain();
    if (event != nullptr)
      *event = evt;
  }

  ReadBufferRectTask *task = mem_pool_->alloc<ReadBufferRectTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->buffer_origin[0] = buffer_origin[0];
  task->buffer_origin[1] = buffer_origin[1];
  task->buffer_origin[2] = buffer_origin[2];
  task->host_origin[0] = host_origin[0];
  task->host_origin[1] = host_origin[1];
  task->host_origin[2] = host_origin[2];
  task->region[0] = region[0];
  task->region[1] = region[1];
  task->region[2] = region[2];
  task->buffer_row_pitch = buffer_row_pitch;
  task->buffer_slice_pitch = buffer_slice_pitch;
  task->host_row_pitch = host_row_pitch;
  task->host_slice_pitch = host_slice_pitch;
  task->ptr = ptr;
  task->evt = evt;

  buffer->retain();
  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  sched_->enqueueTask(asyncReadBufferRect, (void *)task, nullptr);

  if (blocking_read) {
    evt->innerEvent()->wait();
    if (event == nullptr)
      evt->release();
  }
  return CL_SUCCESS;
}

void _cl_command_queue::asyncWriteBufferRect(void *data) {
  WriteBufferRectTask *t = (WriteBufferRectTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  char *dst = (char *)t->buffer->mapBuf(0, t->buffer->getSize(), 
                                        MEM_READ_WRITE);
  size_t *buffer_origin = t->buffer_origin;
  size_t *host_origin   = t->host_origin;
  size_t *region        = t->region;
  size_t buffer_row_pitch   = t->buffer_row_pitch;
  size_t buffer_slice_pitch = t->buffer_slice_pitch;
  size_t host_row_pitch     = t->host_row_pitch;
  size_t host_slice_pitch   = t->host_slice_pitch;
  const void *ptr = t->ptr;

  size_t buffer_offset = buffer_origin[2] * buffer_slice_pitch + 
                         buffer_origin[1] * buffer_row_pitch + 
                         buffer_origin[0];
  size_t host_offset = host_origin[2] * host_slice_pitch +
                       host_origin[1] * host_row_pitch + 
                       host_origin[0];
  const char *src = (char *)ptr;

  // buffer might has been written by device, make sure it's visible to cpu.

  for (size_t depth = 0; depth < region[2]; ++depth) {
    for (size_t height = 0; height < region[1]; ++height) {
      void *dst_line = (void *)(dst + buffer_offset + 
                                depth * buffer_slice_pitch + 
                                height * buffer_row_pitch);
      void *src_line = (void *)(src + host_offset + 
                                depth * host_slice_pitch + 
                                height * host_row_pitch);
      std::memcpy(dst_line, src_line, region[0]);
    }
  }

  // Makes latest buffer content visible to device
  t->buffer->unmapBuf(dst);
  t->buffer->release();

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int 
_cl_command_queue::enqueueWriteBufferRect(cl_mem buffer,
                                          cl_bool blocking_write,
                                          const size_t buffer_origin[3],
                                          const size_t host_origin[3],
                                          const size_t region[3],
                                          size_t buffer_row_pitch,
                                          size_t buffer_slice_pitch,
                                          size_t host_row_pitch,
                                          size_t host_slice_pitch,
                                          const void *ptr,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event *event_wait_list,
                                          cl_event *event) {
  run();

  WriteBufferRectTask *task = mem_pool_->alloc<WriteBufferRectTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->buffer_origin[0] = buffer_origin[0];
  task->buffer_origin[1] = buffer_origin[1];
  task->buffer_origin[2] = buffer_origin[2];
  task->host_origin[0] = host_origin[0];
  task->host_origin[1] = host_origin[1];
  task->host_origin[2] = host_origin[2];
  task->region[0] = region[0];
  task->region[1] = region[1];
  task->region[2] = region[2];
  task->buffer_row_pitch = buffer_row_pitch;
  task->buffer_slice_pitch = buffer_slice_pitch;
  task->host_row_pitch = host_row_pitch;
  task->host_slice_pitch = host_slice_pitch;
  task->ptr = ptr;

  _cl_event *evt = nullptr;

  if ((event != nullptr) || blocking_write) {
    evt = _cl_event::create(this);
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    evt->retain();

    if (event != nullptr)
      *event = evt;
  }

  task->evt = evt;

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  buffer->retain();
  sched_->enqueueTask(asyncWriteBufferRect, (void *)task, nullptr);

  if (blocking_write) {
    evt->innerEvent()->wait();

    if (event == nullptr)
      evt->release();
  }
  return CL_SUCCESS;
}

void _cl_command_queue::asyncFillBuffer(void *data) {
  FillBufferTask *t = (FillBufferTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  char *dst = (char *)t->buffer->mapBuf(0, t->buffer->getSize(), 
                                        MEM_READ_WRITE);
  // buffer might has been written by device, make sure it's visible to cpu.
  for (size_t i = 0; i < t->size / t->pattern_size; ++i) {
    void *dst_line = (void *)(dst + t->offset + i * t->pattern_size);
    std::memcpy(dst_line, t->pattern, t->pattern_size);
  }
  // Makes latest buffer content visible to device
  t->buffer->unmapBuf(dst);
  t->buffer->release();

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue:: enqueueFillBuffer(cl_mem buffer,
                                             const void *pattern,
                                             size_t pattern_size,
                                             size_t offset,
                                             size_t size,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event *event_wait_list,
                                             cl_event *event) {
  run();

  FillBufferTask *task = mem_pool_->alloc<FillBufferTask>();

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->offset = offset;
  task->pattern = pattern;
  task->size = size;
  task->pattern_size = pattern_size;

  _cl_event *evt = nullptr;
  evt = _cl_event::create(this);
  evt->setQueue_time();
  evt->setUserEventStatus(CL_QUEUED);
  evt->retain();
  task->evt = evt;
  
  buffer->retain();

  if (event != nullptr) {
    *event = evt;
    sched_->enqueueTask(asyncFillBuffer, (void *)task, nullptr);
  } else {
    sched_->enqueueTask(asyncFillBuffer, (void *)task, nullptr);
    evt->innerEvent()->wait();
    evt->release();
  }

  return CL_SUCCESS;
}

void _cl_command_queue::asyncCopyBuffer(void *data) {
  CopyBufferTask *t = (CopyBufferTask *)data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  void *dst = t->dst_buffer->mapBuf(t->dst_offset, t->cb, MEM_READ_WRITE);
  void *src = t->src_buffer->mapBuf(t->src_offset, t->cb, MEM_READ);
  std::memcpy(dst, src, t->cb);
  // Makes latest dst_buffer content visible to device
  t->dst_buffer->unmapBuf(dst);
  t->src_buffer->unmapBuf(src);

  t->src_buffer->release();
  t->dst_buffer->release();

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue::enqueueCopyBuffer(cl_mem src_buffer,
                                            cl_mem dst_buffer,
                                            size_t src_offset,
                                            size_t dst_offset,
                                            size_t cb,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event *event_wait_list,
                                            cl_event *event) {
  run();

  CopyBufferTask *task = mem_pool_->alloc<CopyBufferTask>();
  task->mem_pool = mem_pool_;
  task->src_buffer = src_buffer;
  task->dst_buffer = dst_buffer;
  task->src_offset = src_offset;
  task->dst_offset = dst_offset;
  task->cb = cb;
  _cl_event *evt = nullptr;

  src_buffer->retain();
  dst_buffer->retain();

  if (event != nullptr) {
    evt = _cl_event::create(this);
    evt->retain();
    evt->setUserEventStatus(CL_QUEUED);
    evt->setQueue_time();
    *event = evt;
  }

  task->evt = evt;

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  sched_->enqueueTask(asyncCopyBuffer, (void *)task, nullptr);

  return CL_SUCCESS;
}

void _cl_command_queue::asyncCopyBufferRect(void *data) {
  CopyBufferRectTask *t = (CopyBufferRectTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  size_t src_offset      = t->src_offset;
  size_t dst_offset      = t->dst_offset;
  size_t *region         = t->region;
  size_t src_row_pitch   = t->src_row_pitch;
  size_t src_slice_pitch = t->src_slice_pitch;
  size_t dst_row_pitch   = t->dst_row_pitch;
  size_t dst_slice_pitch = t->dst_slice_pitch;
  // Makes latest src_buffer content visible to cpu
  char *dst = (char *)t->dst_buffer->mapBuf(0, t->dst_buffer->getSize(),
                                            MEM_READ_WRITE);
  const char *src = (char *)t->src_buffer->mapBuf(0, t->src_buffer->getSize(),
                                                  MEM_READ);
  for (size_t depth = 0; depth < region[2]; ++depth) {
    for (size_t height = 0; height < region[1]; ++height) {
      void *dst_line = (void *)(dst + dst_offset + depth * dst_slice_pitch + 
                                height * dst_row_pitch);
      void *src_line = (void *)(src + src_offset + depth * src_slice_pitch + 
                                height * src_row_pitch);
      std::memcpy(dst_line, src_line, region[0]);
    }
  }

  // Makes latest dst_buffer content visible to device
  t->dst_buffer->unmapBuf(dst);
  t->src_buffer->unmapBuf((void *)src);

  t->src_buffer->release();
  t->dst_buffer->release();

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue::enqueueCopyBufferRect(cl_mem src_buffer,
                                                cl_mem dst_buffer,
                                                size_t src_offset,
                                                size_t dst_offset,
                                                const size_t src_origin[3],
                                                const size_t dst_origin[3],
                                                const size_t region[3],
                                                size_t src_row_pitch,
                                                size_t src_slice_pitch,
                                                size_t dst_row_pitch,
                                                size_t dst_slice_pitch,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event *event_wait_list,
                                                cl_event *event) {
  run();

  CopyBufferRectTask *task = mem_pool_->alloc<CopyBufferRectTask>();
  task->mem_pool        = mem_pool_;
  task->src_buffer      = src_buffer;
  task->dst_buffer      = dst_buffer;
  task->src_offset      = src_offset;
  task->dst_offset      = dst_offset;
  task->region[0]       = region[0];
  task->region[1]       = region[1];
  task->region[2]       = region[2];
  task->src_row_pitch   = src_row_pitch;
  task->src_slice_pitch = src_slice_pitch;
  task->dst_row_pitch   = dst_row_pitch;
  task->dst_slice_pitch = dst_slice_pitch;

  _cl_event *evt = nullptr;

  if (event != nullptr) {
    evt = _cl_event::create(this);
    evt->retain();
    evt->setUserEventStatus(CL_QUEUED);
    evt->setQueue_time();
    *event = evt;
  }

  task->evt = evt;

  src_buffer->retain();
  dst_buffer->retain();
  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  sched_->enqueueTask(asyncCopyBufferRect, (void *)task, nullptr);

  return CL_SUCCESS;
}

void _cl_command_queue::run() {
  CommandQueueTask *task = mem_pool_->alloc<CommandQueueTask>();
  task->mem_pool = mem_pool_;
  task->cmdq = this;
  task->pid = XoclThread::getProcessID();
  sched_->enqueueTask(runKernels, (void *)task, nullptr);
}

void _cl_command_queue::waitAll() {
  XoclThread::Event *evt = XoclThread::Event::create(false);
  sched_->enqueueTask(nullptr, nullptr, evt);
  evt->wait();
  evt->release();
}

void _cl_command_queue::asyncReadBuffer(void *data) {
  ReadBufferTask *t = (ReadBufferTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  void *src = t->buffer->mapBuf(t->offset, t->cb, MEM_READ);
  std::memcpy(t->ptr, src, t->cb);
  t->buffer->unmapBuf(src);
  t->buffer->release();
  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }
  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue::enqueueReadBuffer(cl_mem buffer,
                                            cl_bool blocking_read,
                                            size_t offset,
                                            size_t cb,
                                            void *ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event *event_wait_list,
                                            cl_event *event) {

  // Make sure all kernels are running
  run();

  _cl_event *evt = nullptr;
  if ((event != nullptr) || blocking_read) {
    evt = _cl_event::create(this);
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);
    evt->retain();

    if (event != nullptr)
      *event = evt;
  }

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  ReadBufferTask *task = mem_pool_->alloc<ReadBufferTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->offset = offset;
  task->cb = cb;
  task->ptr = ptr;
  task->evt = evt;

  buffer->retain();
  sched_->enqueueTask(asyncReadBuffer, (void *)task, nullptr);

  if (blocking_read) {
    evt->innerEvent()->wait();
    if (event == nullptr)
      evt->release();
  }
  return CL_SUCCESS;
}

void _cl_command_queue::asyncWriteBuffer(void *data) {
  WriteBufferTask *t = (WriteBufferTask *) data;
  if (t->evt != nullptr)
    t->evt->setSubmit_time();
  void *dst = t->buffer->mapBuf(t->offset, t->cb, MEM_READ_WRITE);
  std::memcpy(dst, t->ptr, t->cb);
  t->buffer->unmapBuf(dst);
  t->buffer->release();

  if (t->evt != nullptr) {
    t->evt->setEnd_time();
    t->evt->setUserEventStatus(CL_COMPLETE);
    t->evt->release();
  }

  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int _cl_command_queue::enqueueWriteBuffer(cl_mem buffer,
                                             cl_bool blocking_write,
                                             size_t offset,
                                             size_t cb,
                                             const void *ptr,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event *event_wait_list,
                                             cl_event *event) {
  // Make sure all kernels are running
  run();

  _cl_event *evt = nullptr;
  if ((event != nullptr) || blocking_write) {
    evt = _cl_event::create(this);
    evt->retain();
    evt->setQueue_time();
    evt->setUserEventStatus(CL_QUEUED);

    if (event != nullptr)
      *event = evt;
  }

  WriteBufferTask *task = mem_pool_->alloc<WriteBufferTask>();
  task->mem_pool = mem_pool_;
  task->buffer = buffer;
  task->offset = offset;
  task->cb = cb;
  task->ptr = ptr;
  task->evt = evt;

  if (event_wait_list != nullptr)
    doEqueueWaitForEvents(num_events_in_wait_list, event_wait_list);

  buffer->retain();
  sched_->enqueueTask(asyncWriteBuffer, (void *)task, nullptr);

  if (blocking_write) {
    evt->innerEvent()->wait();
    if (event == nullptr)
      evt->release();
  }
  return CL_SUCCESS;
}

void _cl_command_queue::asyncRunMaker(void *data) {
  EventTask *t = (EventTask *) data;
  _cl_event *evt = (_cl_event *) t->evt;
  evt->setSubmit_time();
  evt->setEnd_time();
  evt->setUserEventStatus(CL_COMPLETE);
  evt->release();
  XoclThread::FlexMemPool *p = t->mem_pool;
  p->free(t);
}

cl_int  _cl_command_queue::enqueueMarker(cl_event *event) {
  run();
  cl_event evt = _cl_event::create(this);
  evt->setQueue_time();
  evt->retain();
  evt->setUserEventStatus(CL_QUEUED);

  EventTask *task = mem_pool_->alloc<EventTask>();
  task->mem_pool = mem_pool_;
  task->evt = evt;
  sched_->enqueueTask(asyncRunMaker, (void *)task, evt->innerEvent());
  *event = evt;
  return CL_SUCCESS;
}

void _cl_command_queue::clear() {
#if 0
  // FIXME: Do we need it ?
  for(auto& e : associatedEvents_) {
    cl_int evt_status;
    e->getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS,
               sizeof(cl_int), &evt_status, nullptr);
    if (evt_status == CL_QUEUED)
      e->innerEvent()->wait();
  }
#endif
}

cl_int _cl_command_queue::getInfo(cl_command_queue_info param_name,
                                  size_t param_value_size, void *param_value,
                                  size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_QUEUE_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, (cl_uint)ref_count_,
                             param_value_size, param_value_size_ret);
  case CL_QUEUE_CONTEXT:
    return setParam<cl_context>(param_value, context_,
                                param_value_size, param_value_size_ret);
  case CL_QUEUE_DEVICE:
    return setParam<cl_device_id>(param_value,
                                  getPlatform()->device_,
                                  param_value_size, param_value_size_ret);
  case CL_QUEUE_PROPERTIES:
    return setParam<cl_command_queue_properties>(param_value,
                                                 properties_,
                                                 param_value_size,
                                                 param_value_size_ret);
  default:
    return CL_INVALID_VALUE;
  }
}
