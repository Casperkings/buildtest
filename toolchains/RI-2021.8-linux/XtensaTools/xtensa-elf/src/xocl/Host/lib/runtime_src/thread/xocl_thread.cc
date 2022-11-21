/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "xocl_thread.h"
#include <malloc.h>

namespace XoclThread {

void *aligned_malloc(size_t size, size_t alignment) {
  return ::memalign(alignment, size);
}

unsigned long getProcessID() {
  return (unsigned long)getpid();
}

unsigned long getThreadID() {
  return (unsigned long)pthread_self();
}

struct Mutex::IMPL_DATA {
  pthread_mutexattr_t mutex_attr;
  pthread_mutex_t mutex;
};

Mutex::Mutex(): ref_count_(1), impl_data_(new Mutex::IMPL_DATA()) {
  pthread_mutexattr_init(&(impl_data_->mutex_attr));
  pthread_mutexattr_settype(&(impl_data_->mutex_attr),
                            PTHREAD_MUTEX_RECURSIVE_NP);
  pthread_mutex_init(&(impl_data_->mutex), &(impl_data_->mutex_attr));
}

Mutex::~Mutex() {
  pthread_mutex_destroy(&(impl_data_->mutex));
  pthread_mutexattr_destroy(&(impl_data_->mutex_attr));
  delete impl_data_;
  impl_data_ = nullptr;
}

int Mutex::release() {
  int cnt = 0;
  int will_delete = 0;

  pthread_mutex_lock(&(impl_data_->mutex));

  if (ref_count_ >= 1) {
    cnt = --ref_count_;

    if (ref_count_ == 0)
      will_delete = 1;
  }

  pthread_mutex_unlock(&(impl_data_->mutex));

  if (will_delete)
    delete this;

  return cnt;
}

int Mutex::retain() {
  int cnt = 0;
  pthread_mutex_lock(&(impl_data_->mutex));

  if (ref_count_ >= 1)
    cnt = ++ref_count_;

  pthread_mutex_unlock(&(impl_data_->mutex));
  return cnt;
}

void Mutex::lock() {
  pthread_mutex_lock(&(impl_data_->mutex));
}

void Mutex::unlock() {
  pthread_mutex_unlock(&(impl_data_->mutex));
}

struct Thread::IMPL_DATA {
  pthread_t th;
  XoclThread::Thread::ThreadFunc func;
  void *data;
};

static void *thread_func(void *lpdata) {
  Thread::IMPL_DATA *impl = (Thread::IMPL_DATA *)lpdata;
  impl->func(impl->data);
  return nullptr;
}

Thread::Thread(XoclThread::Thread::ThreadFunc func, void *data)
  : OBJ(), impl_data_(new Thread::IMPL_DATA) {
  impl_data_->func = func;
  impl_data_->data = data;
  impl_data_->th = 0;
}

Thread::~Thread() {
  if (impl_data_->th != 0)
    impl_data_->th = 0;

  delete impl_data_;
  impl_data_ = nullptr;
}

int Thread::run() {
  if (impl_data_->th != 0)
    return -1;

  return pthread_create(&(impl_data_->th), nullptr, thread_func, 
                        (void *)impl_data_);
}

int Thread::wait() {
  return pthread_join(impl_data_->th, nullptr);
}

struct Event::IMPL_DATA {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  int val;
  bool autoReset;
};

Event::Event(bool autoReset):
  OBJ(), impl_data_(new Event::IMPL_DATA()) {
  pthread_mutex_init(&(impl_data_->mutex), nullptr);
  pthread_cond_init(&(impl_data_->cond), nullptr);
  impl_data_->val = 0;
  impl_data_->autoReset = autoReset;
}

Event::~Event() {
  pthread_cond_destroy(&(impl_data_->cond));
  pthread_mutex_destroy(&(impl_data_->mutex));
  delete impl_data_;
  impl_data_ = nullptr;
}

int Event::reset() {
  impl_data_->val = 0;
  return 0;
}

int Event::set() {
  pthread_mutex_lock(&(impl_data_->mutex));
  impl_data_->val = 1;
  pthread_cond_signal(&(impl_data_->cond));
  pthread_mutex_unlock(&(impl_data_->mutex));
  return 0;
}

int Event::wait() {
  pthread_mutex_lock(&(impl_data_->mutex));

  while (impl_data_->val == 0)
    pthread_cond_wait(&(impl_data_->cond), &(impl_data_->mutex));

  if (impl_data_->autoReset)
    impl_data_->val = 0;

  pthread_mutex_unlock(&(impl_data_->mutex));
  return 0;
}

} // XoclThread namespace
