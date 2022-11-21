// This example demonstrates the use of the bank memory manager API

#include <stdlib.h>
#include <stdio.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/xos.h>
//#include <xtensa/xmem_bank.h>
#include "../install/xmem_bank.h"

#define MAX_TRIES         (3)
#define THREAD_STACK_SIZE (16*1024)

XosThread thread1;
XosThread thread2;

XosBarrier barrier;

// Per thread stack
char thread1_stack[THREAD_STACK_SIZE];
char thread2_stack[THREAD_STACK_SIZE];

void *bank0_start;
void *bank1_start;
int bank0_free_space;
int bank1_free_space;

int thread_func(void *arg, int unused)
{
  xmem_bank_status_t status;
  void *p0, *p1;
  int ok = 1;

  int tid = *(int *)arg;

  printf("In thread%d\n", tid);

  xmem_bank_t banks[2];
  banks[0]._start = (void *)((uintptr_t)bank0_start + tid*bank0_free_space/2);
  banks[0]._size = bank0_free_space/2;

  banks[1]._start = (void *)((uintptr_t)bank1_start + tid*bank1_free_space/2);
  banks[1]._size = bank1_free_space/2;

  xmem_thread_init_banks(XMEM_BANK_HEAP_ALLOC, banks, 2);

  void *start0, *start1;
  size_t fs0, fs1;
  fs0 = xmem_bank_get_free_space(0, 1, &start0, NULL, NULL);
  fs1 = xmem_bank_get_free_space(1, 1, &start1, NULL, NULL);

  printf("bank-0 start %p free space %d\n", start0, fs0);
  printf("bank-1 start %p free space %d\n", start1, fs1);

  // Each thread tries to allocate space from its banks
  // possibly get suspended.
  p0 = xmem_bank_alloc(0, 1024, 1, &status);
  if (p0 == NULL) {
    printf("thread%d: could not allocate\n", tid);
    ok = 0;
  }
  printf("thread%d: Allocating from bank-0, %p\n", tid, p0);
  printf("thread%d: Alloc: %d, Free: %d, Unused %d bank-0\n", tid, 
         xmem_bank_get_allocated_bytes(0), xmem_bank_get_free_bytes(0),
         xmem_bank_get_unused_bytes(0));
         

  p1 = xmem_bank_alloc(1, 1024, 1, &status);
  if (p1 == NULL) {
    printf("thread%d: could not allocate\n", tid);
    ok = 0;
  }
  printf("thread%d: Allocating from bank-1, %p\n", tid, p1);
  printf("thread%d: Alloc: %d, Free: %d, Unused %d bank-1\n", tid, 
         xmem_bank_get_allocated_bytes(1), xmem_bank_get_free_bytes(1),
         xmem_bank_get_unused_bytes(1));

  // Free allocated memory; wakeup any sleeping threads
  xmem_bank_free(0, p0);
  xmem_bank_free(1, p1);

  printf("thread%d: Free space bank-0, %d\n", tid, xmem_bank_get_free_bytes(0));
  printf("thread%d: Free space bank-1, %d\n", tid, xmem_bank_get_free_bytes(1));

  if (ok)
    printf("thread%d pass\n", tid);
  else
    printf("thread%d fail\n", tid);

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
  bank0_free_space = xmem_bank_get_free_space(0, 1, // align
                                              &bank0_start, NULL, NULL);
  printf("Free space in bank-0 %d at %p\n", bank0_free_space, bank0_start);

  // Find free space in bank-1
  bank1_free_space = xmem_bank_get_free_space(1, 1, // align
                                              &bank1_start, NULL, NULL);
  printf("Free space in bank-1 %d at %p\n", bank1_free_space, bank1_start);

  #define TICK_CYCLES (xos_get_clock_freq()/10)
  xos_start_system_timer(-1, TICK_CYCLES);

  // Create worker thread
  int thread1_id = 0;
  xos_thread_create(&thread1, 0, thread_func, &thread1_id, "thread1",
                    thread1_stack, THREAD_STACK_SIZE, 1, 0, 0);
  int thread2_id = 1;
  xos_thread_create(&thread2, 0, thread_func, &thread2_id, "thread2",
                    thread2_stack, THREAD_STACK_SIZE, 1, 0, 0);

  return 0;
}
