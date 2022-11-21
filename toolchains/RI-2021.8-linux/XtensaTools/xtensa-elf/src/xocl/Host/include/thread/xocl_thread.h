/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_THREAD_H__
#define __XOCL_THREAD_H__

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace XoclThread {

void *aligned_malloc(size_t size, size_t alignment);
unsigned long getProcessID();
unsigned long getThreadID();

// A mutex object wrapper class
class Mutex {
public:
  class Mon {
  private:
    Mutex *mt_;

  public:
    Mon(Mutex *mt) : mt_(mt) {
      if (mt_ == nullptr)
        return;
      mt_->lock();
      mt_->retain();
    }

    void leave() {
      if (mt_ == nullptr)
        return;
      mt_->unlock();
      mt_->release();
      mt_ = nullptr;
    }

    ~Mon() {
      leave();
    }
  };

private:
  int ref_count_;
  struct IMPL_DATA;
  struct IMPL_DATA *impl_data_;

protected:
  Mutex();
  Mutex(Mutex &o) {}
  ~Mutex();

public:
  static Mutex *create() {
    return new Mutex();
  }

  int retain();
  int release();
  void lock();
  void unlock();
};

// A top template class for common thread object
class OBJ {
protected:
  int ref_count_;
  Mutex *mt_;

public:
  OBJ() : ref_count_(1), mt_(Mutex::create()) {
  }

  virtual ~OBJ() {
    mt_->release();
    mt_ = nullptr;
  }

  void release() {
    XoclThread::Mutex::Mon mon(mt_);
    if (ref_count_ < 1)
      return;
    --ref_count_;
    if (ref_count_ == 0)
      delete this;
  }

  int retain() {
    XoclThread::Mutex::Mon mon(mt_);
    if (ref_count_ < 1)
      return 0;
    return ++ref_count_;
  }
};

// A thread wrapper class
class Thread: public OBJ {
  friend class OBJ;

public:
  typedef void (*ThreadFunc)(void *data);
  struct IMPL_DATA;

protected:
  Thread()          {};
  Thread(Thread &o) {};
  Thread(Thread::ThreadFunc func, void *data);
  ~Thread();

  struct IMPL_DATA *impl_data_;

public:
  static Thread *create(Thread::ThreadFunc func, void *data) {
    return new Thread(func, data);
  }

  int run();
  int wait();
};

class Event: public OBJ {
  friend class OBJ;
protected:
  Event(bool autoReset);
  Event(Event &o) {};
  ~Event();

  struct IMPL_DATA;
  struct IMPL_DATA *impl_data_;

public:
  static Event *create(bool autoReset) {
    return new Event(autoReset);
  }

  int reset();
  int set();
  int wait();
};

// A thread safe memory pool for fix size alloc
class FixMemPool: public OBJ {
  friend class OBJ;

private:
  static const uintptr_t HeaderSize = (sizeof(void *));

  struct MemArea {
    MemArea *pPrev;
  };

  union OB {
    union OB *free_list_link;
  };

  size_t block_size_;
  size_t area_block_count_;
  size_t area_size_;
  size_t block_area_size_;

  OB *free_blocks_;
  char *begin_;
  char *end_;
  Mutex *mt_;

  MemArea *chainHeader() const {
    return (MemArea *)(begin_ - HeaderSize);
  }

protected:
  FixMemPool() {};
  FixMemPool(FixMemPool &o) {};

  FixMemPool(size_t alloc_size, size_t area_block_count)
    : OBJ(), block_size_(alloc_size), area_block_count_(area_block_count) {
    block_area_size_ = block_size_ * area_block_count_;
    area_size_ = block_area_size_ + HeaderSize;
    free_blocks_ = nullptr;
    mt_ = Mutex::create();
    begin_ = end_ = (char *)HeaderSize;
  }

  ~FixMemPool() {
    if (begin_ != (char *)HeaderSize)
      this->clear();
    mt_->release();
    mt_ = nullptr;
  }

public:
  static FixMemPool *create(size_t block_alloc_size,
                            size_t area_block_count = 128) {
    return new FixMemPool(block_alloc_size, area_block_count);
  }

  void clear() {
    Mutex::Mon mon(mt_);
    free_blocks_ = nullptr;
    MemArea *pHeader = chainHeader();
    while (pHeader) {
      MemArea *pTemp = pHeader ->pPrev;
      std::free((void *)pHeader);
      pHeader = pTemp;
    }
    begin_ = end_ = (char *)HeaderSize;
  }

  void *alloc() {
    char *block;
    Mutex::Mon mon(mt_);
    if (free_blocks_ != nullptr) {
      block = (char *) free_blocks_;
      free_blocks_ = free_blocks_->free_list_link;
    } else if ((size_t)(end_ - begin_) >= block_size_) {
      end_ -= block_size_;
      block = end_;
    } else {
      MemArea *pNew = (MemArea *)std::malloc(area_size_);
      memset(pNew, 0, area_size_);
      pNew->pPrev = chainHeader();
      begin_ = (char *)(pNew + 1);
      end_ = begin_ + block_area_size_;
      end_ -= block_size_;
      block = end_;
    }
    return block;
  }

  void free(void *block) {
    Mutex::Mon mon(mt_);
    OB *q = (OB *)block;
    q->free_list_link = free_blocks_;
    free_blocks_ = q;
  }
};

class FlexMemPool: public OBJ {
private:
  static const unsigned ALIGN = 8;
  static const unsigned MAX_BYTES = 512;
  static const unsigned NFREELISTS = MAX_BYTES / ALIGN;

  FixMemPool *volatile freelist_[NFREELISTS];

  static size_t FREELIST_INDEX(size_t bytes) {
    return (((bytes) + ALIGN - 1) / ALIGN - 1);
  }

  FlexMemPool(): OBJ() {
    for (size_t i = 0; i < NFREELISTS; ++i)
      freelist_[i] = FixMemPool::create((i + 1) * ALIGN, 32);
  };

  ~FlexMemPool() {
    for (size_t i = 0; i < NFREELISTS; ++i)
      freelist_[i]->release();
  }

public:
  static FlexMemPool *create() {
    return new FlexMemPool();
  }

  void *alloc(size_t n) {
    return aligned_malloc(n, 128);
  }

  void free(void *p, size_t n) {
    std::free(p);
  }

  template<class T>
  T *alloc() {
    return (T *)alloc(sizeof(T));
  }

  template<class T>
  void free(T *p) {
    free(p, sizeof(T));
  }

  void *reAlloc(void *p, size_t ori_size, size_t new_size) {
    void *t;
    if (ori_size > (size_t)MAX_BYTES)
      t = std::realloc(p, new_size);
    else {
      t = this->alloc(new_size);
      size_t mini_size = new_size > ori_size ? ori_size : new_size;
      std::memcpy(t, p, mini_size);
      this->free(p, ori_size);
    }
    return t;
  }
};

// a thread safe data queue for task queue
class QueuePort: public OBJ {
  friend class OBJ;

private:
  struct QItem {
    void *data;
    struct QItem *next;
  };

  QItem *head_;
  QItem *tail_;
  Mutex *mt_;
  Event *evt_;
  FixMemPool *pool_;

protected:
  QueuePort() : OBJ() {
    pool_ = FixMemPool::create(sizeof(QItem));
    head_ = (QItem *)pool_->alloc();
    tail_ = (QItem *)pool_->alloc();
    head_->next = tail_;
    tail_->next = nullptr ;
    mt_  = Mutex::create();
    evt_ = Event::create(true);
    evt_->reset();
  }

  QueuePort(QueuePort &o) {};

  ~QueuePort() {
    for (QItem *p = head_; p != tail_; p = p->next)
      pool_->free(p);

    pool_->free(tail_);
    head_ = nullptr;
    tail_ = nullptr;

    mt_->release();
    mt_ = nullptr;

    evt_->release();
    evt_ = nullptr;

    pool_->release();
    pool_ = nullptr;
  };

public:
  static QueuePort *create() {
    return new QueuePort();
  };

  int postQueue(void *data) {
    QItem *item = (QItem *)pool_->alloc();
    Mutex::Mon mon(mt_);
    tail_->data = data;
    tail_->next = item;
    tail_ = item;
    evt_->set();
    return 0;
  }

  int waitQueue(void **data) {
    for (;;) {
      Mutex::Mon mon(mt_);
      if (head_->next != tail_) {
        QItem *p = head_->next;
        *data = p->data;
        head_->next = p->next;
        pool_->free(p);
        break;
      }
      mon.leave();
      evt_->wait();
    }
    return 0;
  }
};

// The AsyncQueueRunner is defined for cl_command_queue async run function 
struct CommandTask {
  typedef void (*TaskFunc)(void *data);
  TaskFunc func;
  void *data;
  XoclThread::Event *evt;
  CommandTask *next;
};

class AsyncQueueRunner {
private:
  XoclThread::Mutex *mutex_;
  CommandTask *taskList_;
  CommandTask *taskTail_;
  XoclThread::FixMemPool *memPool_;
  XoclThread::QueuePort *queuePort_;
  XoclThread::Event *start_event_;
  XoclThread::Event *stop_event_;
  XoclThread::Thread *thread_;
  int	dispatched_;
  int isRunning_;

private:
  void freeTaskList() {
    for (CommandTask *p = taskList_; p != taskTail_; p = p->next)
      memPool_->free(p);
    memPool_->free(taskTail_);
    taskList_ = nullptr;
    taskTail_ = nullptr;
  }

  void pushTask(CommandTask::TaskFunc func, void *data, 
                XoclThread::Event *evt) {
    CommandTask *tp = (CommandTask *)memPool_->alloc();
    tp->next = nullptr;
    taskTail_->func = func;
    taskTail_->data = data;
    taskTail_->next = tp;
    if (evt != nullptr)
      evt->retain();
    taskTail_->evt = evt;
    taskTail_ = tp;
  }

  void dispatchTask() {
    if (dispatched_)
      return;
    if (taskList_->next == taskTail_)
      return;
    CommandTask *tp = taskList_->next;
    taskList_->next = tp->next;
    dispatched_ = 1;
    queuePort_->postQueue((void *)tp);
  }

  void dispatchNextTask(CommandTask *tp) {
    memPool_->free(tp);
    XoclThread::Mutex::Mon mon(mutex_);
    dispatched_ = 0;
    if (taskList_->next != taskTail_)
      dispatchTask();
  }

public:
  AsyncQueueRunner() {
    mutex_ = XoclThread::Mutex::create();
    memPool_ = XoclThread::FixMemPool::create(sizeof(CommandTask));
    taskList_ = (CommandTask *)memPool_->alloc();
    taskTail_ = (CommandTask *)memPool_->alloc();
    taskList_->next = taskTail_;
    queuePort_ = XoclThread::QueuePort::create();
    start_event_ = XoclThread::Event::create(false);
    stop_event_ = XoclThread::Event::create(false);
    thread_ = XoclThread::Thread::create(runnerThread, (void *)this);
    dispatched_ = 0;
    isRunning_ = 0;
  }

  ~AsyncQueueRunner() {
    mutex_->release();
    mutex_ = nullptr;
    memPool_->release();
    memPool_ = nullptr;
    queuePort_->release();
    queuePort_ = nullptr;
    start_event_->release();
    start_event_ = nullptr;
    stop_event_->release();
    stop_event_ = nullptr;
    thread_->release();
    thread_ = nullptr;
  }

  static void runnerThread(void *data) {
    void *queue_data;
    AsyncQueueRunner *runner = (AsyncQueueRunner *)data;
    runner->start_event_->set();
    XoclThread::QueuePort *port = runner->queuePort_;
    port->retain();
    for (;;) {
      port->waitQueue(&queue_data);
      if (queue_data == nullptr)
        break;
      CommandTask *task = (CommandTask *)queue_data;
      if (task->func)
        task->func(task->data);
      if (task->evt) {
        task->evt->set();
        task->evt->release();
      }
      runner->dispatchNextTask(task);
    }
    runner->freeTaskList();
    port->release();
    runner->stop_event_->set();
  }

  int startRunner() {
    start_event_->reset();
    thread_->run();
    start_event_->wait();
    return 0;
  }

  int stopRunner() {
    stop_event_->reset();
    queuePort_->postQueue(nullptr);
    stop_event_->wait();
    thread_->wait();
    return 0;
  }

  void enqueueTask(CommandTask::TaskFunc func, void *data, 
                   XoclThread::Event *evt) {
    XoclThread::Mutex::Mon mon(mutex_);
    pushTask(func, data, evt);
    dispatchTask();
  }
};

} // XoclThread namespace

#endif // __XOCL_THREAD_H__
