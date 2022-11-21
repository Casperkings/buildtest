/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __CL_INTERNAL_OBJECTS_H__
#define __CL_INTERNAL_OBJECTS_H__

#include <string>
#include <stack>
#include <vector>
#define CL_TARGET_OPENCL_VERSION 120
#include "CL/cl.h"
#include "kernel.h"
#include "thread/xocl_thread.h"
#include <queue>
#include <map>
#include <algorithm>
#include <cassert>

struct xrp_buffer;
typedef xrp_buffer *buf_obj;

const cl_platform_id getPlatform();

enum xocl_mem_access_flag { 
  MEM_READ, 
  MEM_WRITE, 
  MEM_READ_WRITE 
};

struct _cl_device_id {
public:
  _cl_device_id() : pref_wg_size_mul_(0), max_wg_size_(0), pref_min_wg_num_(0),
                    ref_count_(1), mt_(XoclThread::Mutex::create()), 
                    dev_q_mt_(XoclThread::Mutex::create()) {}

  virtual ~_cl_device_id() {
    mt_->release();
    mt_ = NULL;
    dev_q_mt_->release();
    dev_q_mt_ = NULL;
  };

  cl_int getInfo(cl_device_info param_name, size_t param_value_size,
                 void *param_value, size_t *param_value_size_ret);

  virtual void release() {
    XoclThread::Mutex::Mon mon(mt_);
    if (ref_count_ < 1)
      return;
    --ref_count_;
    if (ref_count_ == 0)
      delete this;
  }

  virtual int retain() {
    XoclThread::Mutex::Mon mon(mt_);
    if (ref_count_ < 1)
      return 0;
    return ++ref_count_;
  }

  // For cl_mem sychronization operations.
  virtual void *mapBuf(buf_obj buf, unsigned offset, unsigned size,
                       xocl_mem_access_flag map_flag) = 0;

  virtual void unmapBuf(buf_obj buf, void *virt_ptr) = 0;

  // For cl_mem allocation/free operations
  virtual cl_int allocMem(cl_mem mem, void *host_ptr) = 0;
  virtual void freeMem(cl_mem mem) = 0;
  virtual void freeDeviceResources() = 0;
  virtual bool isValidOffset(cl_mem mem, size_t offset) = 0;

  // Other memory operations
  // AllocDevMem returns device pointer.
  virtual buf_obj allocDevMem(cl_context ctx, size_t size) = 0;
  virtual void freeDevMem(buf_obj dev_buf) = 0;
  virtual void copyToDevMem(buf_obj dev_buf, const void *src, size_t size) = 0;
  virtual void *allocDevLocalMem(cl_context ctx, size_t size) = 0;
  virtual void freeDevLocalMem(void *dev_ptr) = 0;
  virtual size_t getLocalMemSize() const = 0;
  virtual size_t getPrintfBufferSize() = 0;
  virtual void createSyncBuffer() = 0;
  virtual void createPrintBuffer() = 0;
  virtual size_t getDeviceAddressBits() const = 0;

  virtual int createSubDevices(const cl_device_partition_property *properties,
                               cl_uint num_devices,
                               cl_device_id *out_devices,
                               cl_uint *num_devices_ret) {
    *num_devices_ret = 0;
    return 0;
  }

  virtual cl_device_partition_property *getSubDeviceType() const {
    return nullptr;
  }

  virtual unsigned getProcCount() const {
    return 1;
  }

  virtual int launchKernelsOnDevice(size_t use_num_cores,
                                    void *shared_buf_group,
                                    bool use_printf,
                                    unsigned long long id) { 
    return CL_SUCCESS; 
  };

  size_t getPreferedWGSizeMul() const {
    return pref_wg_size_mul_;
  }

  size_t getMaxWGSize() const {
    return max_wg_size_;
  }

  size_t getPreferredMinWGNum() const {
    return pref_min_wg_num_;
  }

  void dev_lock() {
    dev_q_mt_->lock(); 
  }

  void dev_unlock() { 
    dev_q_mt_->unlock(); 
  }

protected:
  size_t pref_wg_size_mul_;
  size_t max_wg_size_;
  size_t pref_min_wg_num_;
  int ref_count_;
  XoclThread::Mutex *mt_;
  XoclThread::Mutex *dev_q_mt_;
};

struct _cl_platform_id {
  friend const cl_platform_id getPlatform();

private:
  _cl_platform_id();
  ~_cl_platform_id() {} // device should be deleted by clReleasedevice()

public:
  // currently we only support one device.
  cl_device_id device_;
  mxpa::Kernel *createXtKernel(mxpa::KernelRegistry *kern_reg, cl_kernel owner);

  void registerDevice(cl_device_id newDev);

  cl_int getDeviceIDs(cl_device_type device_type, cl_device_id *devices,
                      cl_uint *num_devices);

  cl_int getInfo(cl_platform_info param_name, size_t param_value_size,
                 void *param_value, size_t *param_value_size_ret);

  void initDeviceIOBuf() {
    if (device_ && !is_init_device_print_buf_) {
      // We init device in constructor function. so we can not know which
      // kernels will call printf functions. so put it here.
      device_->createPrintBuffer();
      is_init_device_print_buf_ = true;
    }
  }

  void addDevice(cl_device_id dev) {
    alldevices_.push_back(dev);
  }

  std::vector<cl_device_id>& getDeviceList() {
    return alldevices_;
  }

private:
  std::vector<cl_device_id> alldevices_;
  bool is_init_device_print_buf_;
};

struct _cl_mem : public XoclThread::OBJ {
  friend class XoclThread::OBJ;
  friend struct _cl_device_id;
  friend struct _cl_context;

public:
  typedef void (*notify_func)(cl_mem memobj, void *user_data);

  _cl_mem(cl_context c, cl_mem_flags f, size_t size,
          void *host_ptr, cl_mem parent, size_t offset);
  virtual ~_cl_mem();

  cl_int getInfo(cl_mem_info param_name, size_t param_value_size,
                 void *param_value, size_t *param_value_size_ret);

  cl_int setMemObjectDestructorCallback(notify_func pfn_notify,
                                        void *user_data);

  void *getHostPtr() const {
    return host_ptr_;
  }

  void *&getPhyPtr() {
    return phy_ptr_;
  }

  size_t getSize() const {
    return size_;
  }

  size_t getOffset() const {
    return offset_;
  }

  cl_context getContext() const {
    return context_;
  }

  cl_mem_flags getFlags() const {
    return flags_;
  }

  bool isSubBuffer() const {
    return parent_mem_ != nullptr;
  }

  buf_obj getBuf() const {
    return buf_;
  }

  void setBuf(buf_obj buf) {
    buf_ = buf;
  }

  void *mapBuf(unsigned offset, unsigned size, xocl_mem_access_flag map_flag);
  void unmapBuf(void *virt_ptr);

private:
  cl_context context_;
  cl_mem_flags flags_;
  size_t size_;
  size_t offset_;
  cl_mem parent_mem_;
  std::stack<void *> user_data_stack_;
  std::stack<notify_func> pfn_notify_stack_;

  // Note! Assume we have only *one* device
  buf_obj buf_;
  // When flags_ has CL_MEM_USE_HOST_PTR, host_ptr_ = host_ptr.
  void *host_ptr_;
  void *phy_ptr_;
};

struct _cl_context : public XoclThread::OBJ {
  friend class XoclThread::OBJ;

public:
  _cl_context(cl_platform_id plat, const cl_context_properties *properties,
              const cl_device_id *device_id_list, size_t listSize);

  ~_cl_context();

  cl_int getInfo(cl_context_info info, size_t argsize, void *arg,
                 size_t *outsize);

  cl_event createUserEvent(cl_int *errcode_ret);

  void setCmdq(cl_command_queue queue) {
    cmdq_ = queue;
  }

  cl_command_queue getCmdq() const {
    return cmdq_;
  }

  const cl_device_id *getDeviceList() const {
    return device_list_;
  }

  cl_int createBuffer(cl_mem *out, cl_mem_flags flags,
                      size_t size, void *host_ptr);

  cl_int createSubBuffer(cl_mem *out, cl_mem parent, cl_mem_flags flags,
                         size_t size, size_t offset);

  void destroyBuffer(cl_mem mem);
  void destroyAllBuffers();
  bool isValidMemoryObject(cl_mem mem) const;

  buf_obj allocDevMem(size_t size);
  void freeDevMem(buf_obj dev_buf);
  void copyToDevMem(buf_obj dev_buf, const void *src, size_t size);
  void *allocDevLocalMem(size_t size);
  void freeDevLocalMem(void *dev_ptr);

 private:
  cl_platform_id plat_;
  cl_device_id *device_list_;
  cl_uint device_count_;
  cl_context_properties *context_properties_;
  cl_command_queue cmdq_;
  size_t context_properties_len_;
  std::vector<cl_mem> buffers_;
};

struct _cl_kernel : public XoclThread::OBJ {
  friend class XoclThread::OBJ;

public:
  _cl_kernel(const char *const name, cl_program program);
  ~_cl_kernel();

  cl_int getInfo(cl_kernel_info param_name, size_t param_value_size,
                 void *param_value, size_t *param_value_size_ret);

  cl_int getArgInfo(cl_uint arg_indx, cl_kernel_arg_info param_name,
                    size_t param_value_size, void *param_value,
                    size_t *param_value_size_ret);

  cl_context getContext() const {
    return context;
  }

  void setBindedDevice(cl_device_id dev) {
    dev_ = dev;
  }

  cl_device_id getBindedDevice() {
    return dev_;
  }

  mxpa::Kernel *mxpa_kernel;
  cl_context context;
  cl_program program;

private:
  cl_device_id dev_;
  char *pKernel_name_;
};

struct _cl_event : public XoclThread::OBJ {
  friend class XoclThread::OBJ;

private:
  _cl_event(cl_command_queue cmd_queue);
  _cl_event(const _cl_event &o);

public:
  int isCreated() {
    return is_create_;
  }

  void setWaitUsrEvtNum(int t) {
    wait_usrevt_num_ = t;
  }

  int getWaitUsrEvtNum() {
    return wait_usrevt_num_;
  }

  void pushWaitUsrEvt(_cl_event *evt) {
    wait_usrevt_vec_.push_back(evt);
  }

  _cl_event *popWaitUsrEvt(int i) {
    return wait_usrevt_vec_.at(i);
  }

protected:
  ~_cl_event();

private:
  cl_command_queue cmd_q_;
  cl_int event_status_;
  cl_command_type command_type_;
  cl_ulong queue_time_;
  cl_ulong start_time_;
  cl_ulong submit_time_;
  cl_ulong end_time_;
  int is_set_;
  int user_stop_;
  int is_create_;

  typedef void (CL_CALLBACK *NOTIFY_FUNC)(cl_event event,
                                          int32_t event_command_exec_status, 
                                          void *user_data);

  std::queue<void *> *user_data_queue_;
  std::queue<NOTIFY_FUNC> *pfn_notify_queue_;
  std::vector< _cl_event *> wait_usrevt_vec_;
  int wait_usrevt_num_;
  XoclThread::Event *evt_;
  XoclThread::Mutex *mt_;

  inline unsigned long long getNTime() {
#ifdef __XCC__
    unsigned t;
  __asm__ __volatile__ ( "rsr.ccount %0" : "=a" (t) :: "memory" );
  return t;
#else
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return ((unsigned long long)tv.tv_nsec) + 
            (unsigned long long)(tv.tv_sec *  1000000000);
#endif
  }

public:
  int getExeStatus() {
    return user_stop_;
  }

  void setExeStatus(int st) {
    user_stop_ = st;
  }

  void setQueue_time() {
    queue_time_ = getNTime();
  }

  cl_ulong getQueue_time() {
    return queue_time_;
  }

  cl_command_queue getCommandQueue() {
    return cmd_q_;
  }

  cl_int getExeStatus() const {
    return event_status_;
  }

  XoclThread::Event *innerEvent() {
    return evt_;
  }

  cl_int setUserEventStatus(cl_int execution_status);

  void setCommandType(cl_command_type cmd_type) {
    command_type_ = cmd_type;
  }

  void setSubmit_time() {
    submit_time_ = getNTime();
  }

  // Submit time and start time are same
  cl_ulong getSubmit_time() {
    return submit_time_;
  }

  void setStart_time() {
    start_time_ = getNTime();
  }

  cl_ulong getStart_time() {
    return submit_time_;
  }

  void setEnd_time() {
    end_time_ = getNTime();
  }

  cl_ulong getEnd_time() {
    return end_time_;
  }

  static _cl_event *create(cl_command_queue cmd_queue);

  cl_int getProfilingInfo(cl_profiling_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret);

  cl_int getInfo(cl_event_info param_name,
                 size_t param_value_size,
                 void *param_value,
                 size_t *param_value_size_ret);

  cl_int setEventCallback(cl_int command_exec_callback_type,
                          void (CL_CALLBACK *pfn_event_notify)(
                                cl_event event,
                                cl_int event_command_exec_status,
                                void *user_data),
                                void *user_data);
};

struct _cl_command_queue: public XoclThread::OBJ {
  friend class XoclThread::OBJ;

public:
  struct NDRangeTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_command_queue cmdq;
    mxpa::Kernel *kernel;
    const cl_event *usrevt_vec;
    cl_event evt;
    int usrevt_size;
    unsigned long long pid;
  };

  struct ReadBufferRectTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    size_t buffer_origin[3];
    size_t host_origin[3];
    size_t region[3];
    size_t buffer_row_pitch;
    size_t buffer_slice_pitch;
    size_t host_row_pitch;
    size_t host_slice_pitch;
    void *ptr;
    _cl_event *evt;
  };

  struct WriteBufferRectTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    size_t buffer_origin[3];
    size_t host_origin[3];
    size_t region[3];
    size_t buffer_row_pitch;
    size_t buffer_slice_pitch;
    size_t host_row_pitch;
    size_t host_slice_pitch;
    const void *ptr;
    _cl_event *evt;
  };

  struct FillBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    size_t offset;
    size_t size;
    size_t pattern_size;
    const void *pattern;
    cl_event evt;
  };

  struct CopyBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem src_buffer;
    cl_mem dst_buffer;
    size_t src_offset;
    size_t dst_offset;
    size_t cb;
    _cl_event *evt;
  };

  struct CopyBufferRectTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem src_buffer;
    cl_mem dst_buffer;
    size_t src_offset;
    size_t dst_offset;
    size_t region[3];
    size_t src_row_pitch;
    size_t src_slice_pitch;
    size_t dst_row_pitch;
    size_t dst_slice_pitch;
    _cl_event *evt;
  };

  struct ReadBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    size_t offset;
    size_t cb;
    void *ptr;
    cl_event evt;
  };

  struct WriteBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    size_t offset;
    size_t cb;
    const void *ptr;
    cl_event evt;
  };

  struct MapBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    void *mapped_ptr;
    cl_event evt;
  };

  struct UnMapBufferTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_mem buffer;
    void *mapped_ptr;
    cl_event evt;
  };

  struct EventTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_event evt;
  };

  struct NdrangedKernel {
    mxpa::Kernel *krnl;
    cl_event evt;
  };

  struct CommandQueueTask {
    XoclThread::FlexMemPool *mem_pool;
    cl_command_queue cmdq;
    unsigned long long pid;
  };

  static void asyncWaitEvent(void *data);
  static void ndRangeKernel(void *data);
  static void runKernels(void *command_queue);
  static void asyncReadBufferRect(void *data);
  static void asyncWriteBufferRect(void *data);
  static void asyncFillBuffer(void *data);
  static void asyncCopyBuffer(void *data);
  static void asyncCopyBufferRect(void *data);
  static void asyncReadBuffer(void *data);
  static void asyncWriteBuffer(void *data);
  static void asyncRunMaker(void *data);
  static void asyncMapBuffer(void *data);
  static void asyncUnMapBuffer(void *data);

  _cl_command_queue(cl_context c, cl_device_id dev,
                    cl_command_queue_properties properties);
  ~_cl_command_queue();

  void doEqueueWaitForEvents(cl_uint num_events, const cl_event *event_list);
  void *enqueueMapBuffer(cl_mem buffer,
                         cl_bool blocking_map,
                         cl_map_flags map_flags,
                         size_t offset,
                         size_t cb,
                         cl_uint num_events_in_wait_list,
                         const cl_event *event_wait_list,
                         cl_event *event,
                         cl_int *errcode_ret);

  cl_int enqueueUnmapMemObject(cl_mem buffer,
                               void *mapped_ptr,
                               cl_uint num_events,
                               const cl_event *wait_events,
                               cl_event *outevent);

  cl_int enqueueWaitForEvents(cl_uint num_events, const cl_event *event_list);

  cl_int enqueueNDRangeKernel(mxpa::Kernel *kernel,
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list,
                              cl_event *event);

  cl_int enqueueReadBufferRect(cl_mem buffer,
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
                               cl_event *event);

  cl_int enqueueWriteBufferRect(cl_mem buffer,
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
                                cl_event *event);

  cl_int  enqueueFillBuffer(cl_mem buffer,
                            const void *pattern,
                            size_t pattern_size,
                            size_t offset,
                            size_t size,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event);

  cl_int enqueueCopyBuffer(cl_mem src_buffer,
                           cl_mem dst_buffer,
                           size_t src_offset,
                           size_t dst_offset,
                           size_t cb,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event);

  cl_int enqueueCopyBufferRect(cl_mem src_buffer,
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
                               cl_event *event);

  cl_int enqueueReadBuffer(cl_mem buffer,
                           cl_bool blocking_read,
                           size_t offset,
                           size_t cb,
                           void *ptr,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event);

  cl_int enqueueWriteBuffer(cl_mem buffer,
                            cl_bool blocking_write,
                            size_t offset,
                            size_t cb,
                            const void *ptr,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event);

  cl_int enqueueMarker(cl_event *event);

  cl_int getInfo(cl_command_queue_info param_name,
                 size_t param_value_size,
                 void *param_value,
                 size_t *param_value_size_ret);

  void run();
  void waitAll();
  void clear();

  // Returns kernels in current task, in evaluation order.
  virtual const std::vector<mxpa::Kernel *> &kernels() const {
    assert(false && "Unreachable.");
    static std::vector<mxpa::Kernel *> tmp;
    return tmp;
  }

  // Returns ND range for specified kernel.
  virtual size_t taskBatchCount(mxpa::Kernel *krnl) const {
    return krnl->getBatchCount();
  }

  cl_context getContext() const {
    return context_;
  }

  cl_device_id getDevice() const {
    return dev_;
  }

  cl_command_queue_properties getProperties() const {
    return properties_;
  }

  void insertAssociatedEvent(cl_event evt) {
    associatedEvents_.push_back(evt);
  }

  void eraseAssociatedEvent(cl_event evt) {
    auto it = std::find(associatedEvents_.begin(), 
                        associatedEvents_.end(), evt);
    if (it != associatedEvents_.end())
      associatedEvents_.erase(it);
  }

  void insertNdrangedKernels(mxpa::Kernel *krnl, cl_event evt) {
    ndrangedKernels_.insert(std::pair<mxpa::Kernel *, cl_event>(krnl, evt));
  }

  void setKernelEventAsSubmit() {
    for (auto &k : ndrangedKernels_)
      if (k.second != NULL)
        k.second->setSubmit_time();
  }

  void releaseNdrangedKernels() {
    for (auto &k : ndrangedKernels_) {
      if (k.second != NULL) {
        if (cl_status_ != CL_SUCCESS)
          k.second->setUserEventStatus(cl_status_);
        else
          k.second->setUserEventStatus(CL_COMPLETE);
        k.second->setEnd_time();
        k.second->release();
      }
      for (auto it : k.first->getMemArgs())
        it.second->release();
      clReleaseKernel(k.first->getCLKrnlObj());
      delete k.first;
    }
    ndrangedKernels_.clear();
  }

  void clearNdrangedKernels() {
    ndrangedKernels_.clear();
  }

  std::map<mxpa::Kernel *, cl_event>& getNdrangedKernels() {
    return ndrangedKernels_;
  }

  buf_group shared_buf_group_;

private:
  cl_context context_;
  cl_device_id dev_;
  cl_command_queue_properties properties_;
  XoclThread::FlexMemPool *mem_pool_;
  int cl_status_;
  XoclThread::AsyncQueueRunner *sched_;
  bool use_printf_;
  std::map<mxpa::Kernel *, cl_event> ndrangedKernels_;
  std::vector<cl_event> associatedEvents_;
  unsigned id_;
};

struct _cl_program : public XoclThread::OBJ {
  friend class XoclThread::OBJ;

public:
  _cl_program(cl_context ctx, const std::string &file_hash) : 
      context(ctx),
      orig_file_hash(file_hash),
      binary_type(CL_PROGRAM_BINARY_TYPE_NONE),
      build_status(CL_BUILD_NONE)
  {
    this->file_hash = file_hash;
    context->retain();
  }

  ~_cl_program() {
    context->release();
  }

  cl_int getInfo(cl_program_info param_name, size_t param_value_size,
                 void *param_value, size_t *param_value_size_ret);
  cl_context context;
  std::string orig_file_hash;
  std::string orig_file_src;
  std::string file_hash;
  std::string file_src;
  std::string build_options;
  int         binary_type;
  int         build_status;
};

#endif // __CL_INTERNAL_OBJECTS_H__
