/*
 * Copyright (c) 2003-2007 Tensilica Inc.  ALL RIGHTS RESERVED.
 * 
 * These coded instructions, statements, and computer programs are
 * the copyrighted works and confidential proprietary information of
 * Tensilica Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but no adapted or modified version
 * may be disclosed or distributed to third parties in any manner,
 * medium, or form, in whole or in part, without the prior  written
 * consent of Tensilica Inc.
 */

/*
// This file provides driver routines for the XTMP System Simulation API.
// Instead of using SystemC (see mp_driver.cxx), we use QuickThreads (QT).
*/

/*
// Customers may replace the following #ifdef...#endif
// with just a #include "iss/mp.h"
*/
#if defined(_BUILD_INTERNAL_)
/*
// When we build locally we need to use the mp.h
// in the local file.
*/
#include "mp.h"
#else
/*
// When the customer builds this file,
// refer to the mp.h installed in swtools.
*/
#include "iss/mp.h"
#endif
#include "qt.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

/* 
// MAX_DELTA_TIME must have form 2^n-1.
// When an event is scheduled beyond MAX_DELTA_TIME
// we switch to a hybrid dense/sparse representation of events
*/
#define MAX_DELTA_TIME 1023
#define WHEEL_MASK ((MAX_DELTA_TIME))

/*
// The default stack size works with Xtensa cores.  If you have your
// own thread and that thread allocates large stack frames, you may
// need to pass a bigger number to threadBaseNew.
*/
#define DEFAULT_STACK_SIZE 0x20000

/*
// Macros to help with aligning stack.  The value of QT_STKALIGN
// depends on the machine and, currently, QuickThreads takes care
// of the details.
*/
#define ROUND_DOWN(x) (((size_t)(x))&~(QT_STKALIGN-1))
#define ROUNDUP(x) ROUND_DOWN((x)+(QT_STKALIGN-1))

/*
// For fast execution with processors in master mode, 
// compile this with the thread heap.
*/
#if !defined(USING_THREAD_HEAP)
#define USING_THREAD_HEAP 1
#endif

/*
// XTMP uses two kinds of threads: CPU threads and user threads.
// If you use a pre-emptive thread package other than QuickThreads,
// you will need to re-implement the function cpuThreadCycle to match
// your thread package.
*/
typedef struct _Thread Thread;
struct _Thread {
  qt_t *sp;			/* the stack pointer, and QT thread handle */
  Thread *next;			/* the next thread in this time slot */
  Thread *link;			/* links all threads together */
  const char *name;		/* the name of this thread */
  char *stackSpace;		/* allocated stack region */
  char *alignedStackSpace;	/* base of stack region after accounting for
				   stack alignment. */
  unsigned stackSize;
  union {
    struct {
      XTMP_time (*stepCPU)(XTMP_threadInfo*);
      void (*syncCPU)(XTMP_threadInfo *, XTMP_timeDelta);
    } pair;
    void (*f)(XTMP_threadInfo*);
  } callBack;
  bool cpuThread;

  XTMP_threadInfo *threadInfo;

#if USING_THREAD_HEAP
  Thread *heap_link;		/* the thread at the next time slot when
				   using a thread heap */
  XTMP_time     heap_link_time;	/* Next scheduled time if this is in the heap
				   link when using a thread heap */
#endif
};

/*
// A heap structure for use when the events are sparsely distributed
*/

#if USING_THREAD_HEAP
struct ThreadHeap {
  Thread *top;
};

static struct ThreadHeap *s_thread_heap = NULL;

#endif /* USING_THREAD_HEAP */

/*
// The time wheel
*/
static XTMP_time currentTime;
static unsigned wheelIndex;
static Thread *wheel[MAX_DELTA_TIME+1];
static Thread *currentThreadList;
static Thread *cpuThreadList;
static unsigned numberOfThreads;
static Thread *currentThread;
static signed runCycles;


/*
// A linked list of all the threads
*/
static Thread *allThreads = NULL;

/*
// A data structure to represent the main thread
*/
static Thread mainThread;

/* Forward definitions */
static void push(Thread *, unsigned );
static void driverInit(void);
static void cpuThreadCycle(Thread *);
static void threadStub(Thread *);
static Thread *threadBaseNew( const char *, unsigned);
static void *threadNew( const char *, void (*)(XTMP_threadInfo *),
			XTMP_threadInfo *);
static void *cpuThreadNew( const char *,
			   XTMP_time (*)(XTMP_threadInfo*),
			   void (*)(XTMP_threadInfo*,XTMP_timeDelta),
			   XTMP_threadInfo *);
static void *switchThread ( qt_t *sp, void *old, void *new );
static void *abortThread ( qt_t *sp, void *old, void *new );
static void driverStart( int ncycles );
static Thread *advanceTime(void);
static void driverWait(unsigned);
static void driverStop(void);
static XTMP_time clockTime(void);
static u32 phase;

extern bool scheduled_actions_empty(void);
extern void advance_scheduled_actions_time(u32 t);

#if defined(DRIVER_TRACE)
static Thread *get_current_time_thread(void);
#endif
static Thread *remove_current_time_thread(void);

#if USING_THREAD_HEAP
static void thread_heap_convert_from_wheel(void);
static void thread_heap_insert_at_time(Thread *t, XTMP_time event_time);
#endif

/* "safe" malloc */
static void *
tryMalloc(const char *caller, size_t size)
{
  void *p = malloc(size);
  if (!p) {
    perror(caller);
    exit(1);
  }
  return p;
}


/* Events */

typedef struct evt_list {
  struct evt_list* next;
  Thread* thread;
} *evt_list_t;

typedef struct event {
  evt_list_t threads;
  bool fired;
} *event_t;

static evt_list_t event_list_free_list;

XTMP_event
eventNew(void)
{
  event_t evt = (event_t) tryMalloc("event_t", sizeof(struct event));
  evt->threads = NULL;
  evt->fired = false;
  return (XTMP_event)evt;
}

static evt_list_t
eventListENew(Thread *thread)
{
  evt_list_t n;
#if !DEBUG_FREE_LIST
  if (event_list_free_list) {
    n = event_list_free_list;
    event_list_free_list = n->next;
    n->thread = thread;
    n->next = NULL;
    return n;
  }
#endif
  n = (evt_list_t) tryMalloc("evt_list_t", sizeof(*n));
  n->next = NULL;
  n->thread = thread;
  return n;
}

static void
eventListEFree(evt_list_t old)
{
#if DEBUG_FREE_LIST
  free(old);
#else
  old->next = event_list_free_list;
  old->thread = NULL;
  event_list_free_list = old;
#endif
}



void
eventFree(XTMP_event event)
{
  event_t evt;
  evt_list_t old, threads;

  evt = (event_t)event;
  threads = evt->threads;
  evt->threads = NULL;
  while (threads) {
    old = threads;
    threads = threads->next;
    eventListEFree(old);
  }
  free(evt);
}

void
waitOnEvent_internal(XTMP_event event, Thread* thread)
{
  event_t evt;
  evt_list_t el;

  evt = (event_t)event;
  el = eventListENew(thread);
  el->next = evt->threads;
  evt->threads = el;
}

void
waitOnEvent(XTMP_event event)
{
  Thread* next;

  waitOnEvent_internal(event, currentThread);
  if( (next = advanceTime()) != currentThread )
    QT_BLOCK(switchThread, currentThread, next, next->sp);
}

void
fireEvent(XTMP_event event)
{
  event_t evt;
  evt_list_t old, threads;

  evt = (event_t)event;
  evt->fired = true;
  threads = evt->threads;
  while (threads) {
    old = threads;
    push(threads->thread, 0);
    threads = threads->next;
    eventListEFree(old);
  }
  evt->threads = NULL;
}

bool
hasEventFired(XTMP_event event)
{
  return ((event_t)event)->fired;
}

#if USING_THREAD_HEAP
void thread_heap_insert_at_time(Thread *t, XTMP_time the_time)
{
  /* This is currently a linked linear list */
  Thread **t_p = &s_thread_heap->top;
  t->heap_link_time = the_time;

  for (; (*t_p) && ((*t_p)->heap_link_time < the_time);
       t_p = &(*t_p)->heap_link);
  
  if ((*t_p) == NULL) {
    // add it to the end
    (*t_p) = t;
  } else if ((*t_p)->heap_link_time == the_time) {
    // add it to the front of the events at this time.
    t->heap_link = (*t_p)->heap_link;
    t->next = (*t_p);
    (*t_p)->heap_link = NULL;
    (*t_p) = t;
  } else {
    /* (*t_p)->heap_link_time > the_time) */
    t->heap_link = *t_p;
    (*t_p) = t;
  }
  return;
}

/* thread heap functions */
void thread_heap_convert_from_wheel(void) {
  int i;
  int idx = wheelIndex;

  if (s_thread_heap == NULL) {
    s_thread_heap = (struct ThreadHeap *)
      tryMalloc("ThreadHeap", sizeof(struct ThreadHeap));
    s_thread_heap->top = NULL;
  }
  
  for (i = 0; i <= MAX_DELTA_TIME;
       ++i, idx = (idx + 1) & WHEEL_MASK) {
    Thread *the_thread = wheel[idx];
    if (the_thread) {
      wheel[idx] = NULL;
      thread_heap_insert_at_time(the_thread, currentTime+i);
    }
  }
}
#endif

/*
// Push a thread into the time slot deltaT cycles in the
// future.
*/
static void
push(Thread *t, unsigned deltaT )
{
  unsigned wi;

#if defined(DRIVER_TRACE)
  fprintf( stderr, "QT push( 0x%08x \"%s\", %d ) @ " PCT_LLD "\n",
	   (unsigned)t, t->name, deltaT, currentTime );
#endif

  /*
  // If you know that you won't ever exceed this limit, you can
  // safely "#if 0" this code out.
  */

#if 1
#if USING_THREAD_HEAP
  if (deltaT > MAX_DELTA_TIME ) {
    if (s_thread_heap == NULL) {
      thread_heap_convert_from_wheel();
    }
  }
  if (s_thread_heap) {
    XTMP_time sched_t = currentTime + deltaT;
    thread_heap_insert_at_time(t, sched_t);
    return;
  }
#else
  /*
  // If you know that you won't ever exceed this limit, you can
  // safely "#if 0" this code out.
  */
  if( deltaT > MAX_DELTA_TIME ) {
    fprintf( stderr, "qt_driver.c: time delay (%d) larger than %d\n"
             "If you want to do this you have to increase MAX_DELTA_TIME\n"
             "or set USING_THREAD_HEAP qt_driver.c\n", deltaT, MAX_DELTA_TIME );
    exit(1);
  }
#endif
#endif

  wi = (wheelIndex+deltaT)&WHEEL_MASK;
  t->next = wheel[wi];
  wheel[wi] = t;
}

#if defined(DRIVER_TRACE)
Thread *get_current_time_thread(void) {
#if USING_THREAD_HEAP
  if (s_thread_heap) {
    if (s_thread_heap->top->heap_time == currentTime) {
      return s_thread_heap->top;
    }
    return NULL;
  }
#endif
  return wheel[wheelIndex];
}
#endif

Thread *remove_current_time_thread(void) {
  Thread *ct;
#if USING_THREAD_HEAP
  if (s_thread_heap) {
    Thread *top_thread = s_thread_heap->top;
    if (top_thread && top_thread->heap_link_time == currentTime) {
      s_thread_heap->top = top_thread->heap_link;
      top_thread->heap_link = NULL;
      return top_thread;
    }
    assert(!top_thread || top_thread->heap_link_time >= currentTime);
    return NULL;
  }
#endif
  ct = wheel[wheelIndex];
  wheel[wheelIndex] = NULL;
  return ct;
}

static void increment_current_time(u32 t) {
  currentTime += t;
#if USING_THREAD_HEAP
  if (s_thread_heap) {
    return;
  }
#endif
  wheelIndex = (wheelIndex + t) & WHEEL_MASK;
}


#if defined(DRIVER_TRACE)
void
printSlot(void)
{
  Thread *t = get_current_time_thread();
  fprintf(stderr, PCT_LLD ":", currentTime);
  if( t )
    {
      while(t)
	{
	  fprintf(stderr, " 0x%08x \"%s\";", (unsigned)t, t->name);
	  t = t->next;
	}
    }
  fputc('\n',stderr);
}		      
#endif

void
cpuThreadCycle(Thread *this)
{
  XTMP_timeDelta  delta;
  XTMP_time time;
  while(!this->threadInfo->exitFlag) 
    {

      time = this->callBack.pair.stepCPU(this->threadInfo);

      /*
	// Adjust the time
	// ---------------
	//
	// The following two ways of computing delta
	// are equivalent:
	//
	// delta = time - XTMP_clockTime() ;
	// delta = time - clockTime();
      */
      delta = time - XTMP_clockTime() ;

#if 1
      /*
	// Safety check for synchronization between ISS and
	// system time.
	// You can "#if 0" code if you don't think you need
	// this.
      */
#if USING_THREAD_HEAP
#else
      if(delta > MAX_DELTA_TIME ) {
	fprintf( stderr, "qt_driver.c: ISS's cycle count has been stepped way"
		 "ahead of "
		 "system time (delta=" PCT_LLD ").\n"
		 "\tEither adjust MAX_DELTA_TIME in qt_driver.c;\n"
		 "\tcheck for bogus delay values returned in your devices;\n"
		 "\tand check for bogus or uninitialized writeBusyTime in "
		 "your devices;\n",
		 delta );
	exit(1);
      }
      if( delta < -MAX_DELTA_TIME ) {
	fprintf( stderr, "qt_driver.c: ISS's cycle count has fallen way"
		 " behind of system time (delta=" PCT_LLD ").\n"
		 "\tThis is an internal error; report to Tensilica\n",
		 delta );
	abort();
      }
#endif
#endif
      
      /* Now synchronize processor with system time */
      this->callBack.pair.syncCPU(this->threadInfo, delta);
    }
  driverStop();
}


void
threadStub(Thread *this)
{
  this->callBack.f(this->threadInfo);
  driverStop();
}

static u32
numThreads(void)
{
  return numberOfThreads;
}


/*
// On NT, ISS takes the form of a DLL.  To avoid circular dependency with
// the Visual C++ linker, the XTMP (inside the ISS DLL) cannot reference
// the funtions in this file directly.  Instead, XTMP must use a function
// table.
*/
static XTMP_driverTable driverTable = {
  XTMP_VERSION, NULL, NULL, driverInit,
  threadNew, cpuThreadNew, driverStart, driverStop, driverWait, eventNew,
  eventFree, waitOnEvent, fireEvent, hasEventFired, clockTime, numThreads,
};


static XTMP_event userThreadEvent;

static void
driverInit(void)
{
  userThreadEvent = XTMP_eventNew();
}


/*
// Common initialization for both kinds of threads.
// Allocate stack, and push into first time slot.
*/
static Thread *
threadBaseNew( const char *name, unsigned stackSize )
{
  Thread *t = (Thread *) tryMalloc("Thread", sizeof(Thread));
  size_t pageSize;
  void *guardPage;

  assert( t!= 0 );

  /* Create stack region */
  t->stackSpace = (char *) tryMalloc("Thread::stackSpace", stackSize);
  t->alignedStackSpace = (char *)ROUNDUP(t->stackSpace);
  t->stackSize = ROUND_DOWN(stackSize);

  /*
   * On i86 Linux, the stack grows downwards from higher addresses.
   * Create a guard page to catch stack overflows. 
   */
  pageSize = getpagesize();
  guardPage = (void *)(((size_t)(t->stackSpace) + pageSize) & (-pageSize));
  if (mprotect(guardPage, pageSize, PROT_NONE) < 0)
    fprintf(stderr, "qt_driver.c: error creating guard page for thread stack\n");

  t->sp = QT_SP(t->alignedStackSpace, t->stackSize);
  t->next = NULL;
  t->link = allThreads;
#if USING_THREAD_HEAP
  t->heap_link = NULL;
  t->heap_link_time = 0;
#endif
  allThreads = t;
  t->name = strdup(name);
  numberOfThreads++;
  return t;
}

/*
// Create a device thread
*/
static void *
threadNew( const char *name,
	   void (*f)(XTMP_threadInfo*),
	   XTMP_threadInfo *ti)
{
  Thread *t = threadBaseNew(name, DEFAULT_STACK_SIZE);
  t->sp = QT_ARGS(t->sp, t, NULL, NULL, threadStub);
  t->callBack.f = f;
  t->threadInfo = ti;
  t->cpuThread = false;
  waitOnEvent_internal(userThreadEvent, t);
  return t;
}

/*
// Create a CPU thread
*/
static void *
cpuThreadNew( const char *name,
	      XTMP_time (*stepCPU)(XTMP_threadInfo*),
	      void (*syncCPU)(XTMP_threadInfo *,XTMP_timeDelta),
	      XTMP_threadInfo *ti)
{
  Thread *t = threadBaseNew(name,  DEFAULT_STACK_SIZE);
  t->sp = QT_ARGS(t->sp, t, NULL, NULL, cpuThreadCycle);
  t->callBack.pair.stepCPU = stepCPU;
  t->callBack.pair.syncCPU = syncCPU;
  t->threadInfo = ti;
  t->cpuThread = true;
  push( t, 0 );
  return t;
}

/*
// switchThread, and abortThread:
// Context switch helper functions for QuickThreads.
// Refer to the QuickThread documentation:
// ftp://ftp.cs.washington.edu/tr/1993/05/UW-CSE-93-05-06.PS.Z
*/
static void *switchThread ( qt_t *sp, void *old, void *new )
{
  /* Save old stack pointer */
  if( old )
    ((Thread *)old)->sp = sp;

  /* set current thread to new one.  sp already loaded by QuickThreads.
   */
  currentThread = (Thread *)new;
  return NULL;
}

static void *abortThread ( qt_t *sp, void *old, void *new )
{
  currentThread = (Thread *)new;
  return NULL;
}

/*
// Start running for ncycles.
*/
static void
driverStart( int ncycles )
{
  Thread *next;
  runCycles = ncycles;
  if( ncycles && numberOfThreads) {
    next = advanceTime();
    if( next!=&mainThread ) {
      QT_BLOCK( switchThread, &mainThread, next, next->sp );
    }
  }
}


static Thread*
getNextThread()
{
  Thread* thread;

  if ( currentThreadList == NULL ) {
    currentThreadList = remove_current_time_thread();
  }
  thread = currentThreadList;
  if ( thread ) {
    currentThreadList = thread->next;
    thread->next = NULL;
  }
  return thread;
}

#if USING_THREAD_HEAP
static u32
find_next_event_cycle()
{
  if (currentThreadList != NULL)
    return 0;
#if USING_THREAD_HEAP
  if (s_thread_heap) {
    if (s_thread_heap->top == NULL)
      return 0;
    return (s_thread_heap->top->heap_link_time - currentTime);
  }
#endif
  {
    int i, idx;
    idx = wheelIndex;
    for (i = 0; i <= MAX_DELTA_TIME; ++i) {
      if (wheel[idx] != NULL)
	return i;
      idx = (idx + 1) & WHEEL_MASK;
    }
  }
  /* In this case, there appear to be no threads scheduled.  returning
     zero will cause execution to proceed normally and check that there
     are no more threads.  */
  return 0;
}
#endif


/*
// Find the next thread to run, advancing time if necessary.  The main
// simulation loop consists of the following steps:
//
// 1. XTMP_startOfCycleProcessing
// 
// 2. Step the cores, until all of their clock times indicate they have 
//    advanced to the next cycle.  Just prior to advancing, each core 
//    processes its stall clock (state update) and free clock (pop 
//    requests and export state) actions. 
// 
// 3. Execute actions scheduled on the global scheduler.  This ensures 
//    all post requests originated by the cores are processed.
// 
// 4. Fire userThreadEvent.
// 
// 	If no user threads were waiting on this event, go to step 5. 
// 	Else, run those threads, until there are no further delta 
// 	cycles possible.  (Threads may wake up as a consequence of 
// 	XTMP_wait(0) or user events being fired.)  Go to 4. 
// 
// 5. Advance the time.  This includes, in order:
//    * Call any functions that handle TIE in-ports (import wires and 
//      input queues)
//    * Run the global scheduler actions to process scheduled responses.
//    * Advance global time and step the global scheduler.
// 
// 6. If all threads have exited, terminate simulation.  Else, go to 1. 
// 
*/
static Thread*
advanceTime(void)
{
  Thread* thread;
  Thread* next;
  bool eventFired;

  while ( runCycles ) {
    
#if USING_THREAD_HEAP
    if (s_thread_heap
	&& phase == 0
	&& (runCycles&1) != runCycles /* e.g. not 0 or 1 */
	&& (((event_t)userThreadEvent)->threads) == NULL
	&& scheduled_actions_empty()
	) { 
      
      /* In global master mode, ignore all of the phases of a cycle
	 find the next cycle with a scheduled thread and skip. */
      u32 t = find_next_event_cycle();
      if (t > 0) {
	if (runCycles >= 0) {
	  if (t > runCycles)
	    t = runCycles;
	}

	/* Update all of the global times */
	increment_current_time(t);

	if (runCycles >= 0)
	  runCycles -= t;
	advance_scheduled_actions_time(t);
	if (runCycles == 0) break;
      } else {
	if (s_thread_heap && s_thread_heap->top == NULL) {
	  fprintf( stderr, "qt_driver.c: Event driven simulation deadlock "
		   "detected at time " PCT_LLD ".\n",
		   currentTime);
	  XTMP_dumpCoreStatus();
	  exit(1);
	}
      }
    }
#endif

    switch (phase) {
    case 0:
      while ( (thread = getNextThread()) != NULL ) {
	if (thread->cpuThread) {
	  thread->next = cpuThreadList;
	  cpuThreadList = thread;
	}
	else
	  return thread;
      }

      assert(!currentThreadList);
      currentThreadList = cpuThreadList;
      cpuThreadList = NULL;
      XTMP_startOfCycleProcessing();
      phase++;

    case 1:
      /* Process the cores. */
      if ( (thread = getNextThread()) != NULL )
	return thread;
      /*
       * Process actions scheduled by the cores.  These may include post 
       * requests that were deferred to the scheduler to model contention. 
       */
      phase++;
      
    case 2:
      /* Run user threads waiting on userThreadEvent. */
      for ( eventFired = false; ; eventFired = true ) {
	while ( (thread = getNextThread()) != NULL ) {
	  eventFired = false;
	  if (thread->cpuThread) {
	    thread->next = cpuThreadList;
	    cpuThreadList = thread;
	  }
	  else
	    return thread;
	}
	if (eventFired)
	  break;
	else
	  XTMP_fireEvent(userThreadEvent);
      }
      phase = 0;

      /* Now advance the global clock. */
      if (  numberOfThreads == 0 )
	/* end of simulation */
	break;
      XTMP_endOfCycleProcessing();

      /* This may have fired an event that a core or user thread was
	 waiting for. */
      thread = remove_current_time_thread();
      while (thread) {
	next = thread->next;
	thread->next = NULL;
	if (thread->cpuThread)
	  push(thread, 1);
	else 
	  waitOnEvent_internal(userThreadEvent, thread);
	thread = next;
      }

      increment_current_time(1);

      /* Schedule runnable user threads onto the next post-core event. */
      thread = remove_current_time_thread();

      while (thread) {
	next = thread->next;
	thread->next = NULL;
	if (thread->cpuThread)
	  push(thread, 0);
	else 
	  waitOnEvent_internal(userThreadEvent, thread);
	thread = next;
      }

      if ( runCycles >= 0 )
	runCycles--;
    }
  }

  return &mainThread;
}

/*
// Suspend execution for ncycles.
*/
static void
driverWait(unsigned ncycles)
{
  Thread *next;
  push(currentThread, ncycles );
  next = advanceTime();
  if( next != currentThread ) {
    QT_BLOCK(switchThread, currentThread, next, next->sp);
  }
}

/*
// Destroy the current thread.
*/
static void
driverStop(void)
{
  Thread *next;
  if( numberOfThreads == 0 || currentThread == &mainThread
      || currentThread == NULL ) {
    fprintf( stderr, "qt_driver.c: can't call XTMP_stop "
	     " from non thread context (e.g. XTMP_main )\n");
    exit(1);
  }
  numberOfThreads--;
  if( numberOfThreads==0 ) {
    QT_BLOCK( switchThread, 0, &mainThread, mainThread.sp);
    abort();
  } else {
    Thread *old = currentThread;
    currentThread = 0;
    next = advanceTime();
    assert(next);
    QT_ABORT( abortThread, old, next, next->sp);
  }
}

static XTMP_time
clockTime(void)
{
  return currentTime;
}


/*
// If you don't want to use XTMP_main, you can simply put the code
// in XTMP_main directly into this main program.
*/
int
main( int argc, char **argv )
{
  int status = 0;
  XTMP_initialize(&driverTable);
  status = XTMP_main(argc, argv);
  XTMP_cleanup();
  return status;
}

