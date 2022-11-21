#include <xtensa/xos.h>
#include "xocl_thread.h"

// Reserved for per thread xos stack
#define XOS_THREAD_STACK_SIZE (16*1024)

namespace XoclThread {

void *aligned_malloc(size_t size, size_t alignment) {
  return memalign(alignment, size);
}

unsigned long getProcessID() {
  return (unsigned long)xos_thread_id();
}

unsigned long getThreadID() {
  return (unsigned long)xos_thread_id();
}

struct Mutex::IMPL_DATA {
  XosMutex mutex;
};

Mutex::Mutex() : ref_count_(1), impl_data_(new Mutex::IMPL_DATA()) {
  ::xos_mutex_create(&(impl_data_->mutex), 0, 0);
}

Mutex::~Mutex() {
  ::xos_mutex_delete(&(impl_data_->mutex));
  delete impl_data_;
  impl_data_ = nullptr;
}

int Mutex::release() {
  int cnt = 0;
  int will_delete = 0;

  ::xos_mutex_lock(&(impl_data_->mutex));

  if (ref_count_ >= 1) {
    cnt = --ref_count_;

    if (ref_count_ == 0)
      will_delete = 1;
  }

  ::xos_mutex_unlock(&(impl_data_->mutex));

  if (will_delete)
    delete this;

  return cnt;
}

int Mutex::retain() {
  int cnt = 0;

  ::xos_mutex_lock(&(impl_data_->mutex));

  if (ref_count_ >= 1)
    cnt = ++ref_count_;

  ::xos_mutex_unlock(&(impl_data_->mutex));
  return cnt;
}

void Mutex::lock() {
  xos_mutex_lock(&(impl_data_->mutex));
}

void Mutex::unlock() {
  xos_mutex_unlock(&(impl_data_->mutex));
}

struct Thread::IMPL_DATA {
  XosThread th;
  char stack[XOS_THREAD_STACK_SIZE];
  XoclThread::Thread::ThreadFunc func;
  void *data;
};

static int thread_func(void *lpdata, int unused) {
  (void)unused;
  Thread::IMPL_DATA *impl = (Thread::IMPL_DATA *)lpdata;
  impl->func(impl->data);
  return 0;
}

Thread::Thread(XoclThread::Thread::ThreadFunc func, void *data)
  : OBJ(), impl_data_(new Thread::IMPL_DATA) {
  impl_data_->func = func;
  impl_data_->data = data;
}

Thread::~Thread() {
  delete impl_data_;
  impl_data_ = nullptr;
}

int Thread::run() {
  return xos_thread_create(&(impl_data_->th), 0, thread_func, 
                           (void *)impl_data_, "XoclThread", 
                           (void *)&(impl_data_->stack), 
                           XOS_THREAD_STACK_SIZE, 0, 0, 0);
}

int Thread::wait() {
  return xos_thread_join(&(impl_data_->th), nullptr);
}

struct Event::IMPL_DATA {
  XosEvent event;
};

Event::Event(bool autoReset): OBJ(), impl_data_(new Event::IMPL_DATA()) {
  xos_event_create(&(impl_data_->event), 0x1, 
                   autoReset ? XOS_EVENT_AUTO_CLEAR : 0);
}

Event::~Event() {
  xos_event_delete(&(impl_data_->event));
  delete impl_data_;
  impl_data_ = nullptr;
}

int Event::reset() {
  xos_event_clear(&(impl_data_->event), 0x1);
  return 0;
}

int Event::set() {
  xos_event_set(&(impl_data_->event), 0x1);
  return 0;
}

int Event::wait() {
  xos_event_wait_all(&(impl_data_->event), 0x1);
  return 0;
}

} // XoclThread namespace
