// This example demonstrates the use of the bank memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/xos.h>
#include <xtensa/xmem_bank.h>

#define MAX_TRIES         (3)
#define THREAD_STACK_SIZE (16*1024)

XosThread thread1;
XosThread thread2;

XosBarrier barrier;

// Per thread stack
char thread1_stack[THREAD_STACK_SIZE];
char thread2_stack[THREAD_STACK_SIZE];

int free_space;

// Dummy compute
static void compute(int delay_count)
{
  int i;
  for (i = 0; i < delay_count; i++)
    asm volatile ("_nop");
}

int thread_func(void *arg, int unused)
{
  xmem_bank_status_t status;
  void *p;

  int tid = *(int *)arg;

  printf("In thread%d\n", tid);

  int i;
  for (i = 0; i < MAX_TRIES; ++i) {

    // Each thread tries to allocate space from local memory. Could 
    // possibly get suspended.
    printf("thread%d: Trying to alloc %d bytes from bank-0\n", tid, free_space);
    p = xmem_bank_alloc_wait(0, free_space, 1, 10000000, &status);

    if (p == NULL) {
      printf("thread%d: could not allocate %d bytes\n", tid, free_space);
      goto fail;
    }

    // Perform some compute. Note, during compute the thread could
    // get pre-empted.
    printf("thread%d: compute\n", tid);
    compute(100000);

    printf("thread%d: Freeing allocated memory, signalling other thread\n", 
           tid);
    // Free allocated memory; wakeup any sleeping threads
    xmem_bank_free_signal(0, p);

    // Yield execution to other thread
    xos_thread_yield();
  }

fail:
  printf("thread%d done\n", tid);

  if (i != MAX_TRIES) {
    printf("thread%d fail\n", tid);
  } else {
    printf("thread%d pass\n", tid);
  }

  // Wait for all threads
  xos_barrier_wait(&barrier);

  exit(0);
}

int main()
{
  xmem_bank_status_t status;

  xos_start_main_ex("main", 0, 8192);

  // Initialize the banks and manage them using heap allocation policy
  // Reserve 16-Kb for stack space. Note, only if stack is present in
  // local memories would this get reserved (eg: using sim-stacklocal LSP)
  status = xmem_init_local_mem(XMEM_BANK_HEAP_ALLOC, 16*1024);
  if (status != XMEM_BANK_OK && status != XMEM_BANK_ERR_STACK_RESERVE_FAIL) {
    printf("xmem_init_local_mem failed, status = %d\n", status);
    return -1;
  }

  // Create barrier for all threads to sync at end
  xos_barrier_create(&barrier, 2);

  // Find free space in bank-0
  free_space = xmem_bank_get_free_space(0, 1, // align
                                        NULL, NULL, NULL);
  printf("Free space in bank-0 %d\n", free_space);

  #define TICK_CYCLES (xos_get_clock_freq()/10)
  xos_start_system_timer(-1, TICK_CYCLES);

  // Create 2 worker threads
  int thread1_id = 1;
  xos_thread_create(&thread1, 0, thread_func, &thread1_id, "thread1",
                    thread1_stack, THREAD_STACK_SIZE, 1, 0, 0);
  int thread2_id = 2;
  xos_thread_create(&thread2, 0, thread_func, &thread2_id, "thread2",
                    thread2_stack, THREAD_STACK_SIZE, 1, 0, 0);

  return 0;
}
