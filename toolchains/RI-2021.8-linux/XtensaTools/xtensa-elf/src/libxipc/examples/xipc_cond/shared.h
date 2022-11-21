/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <xtensa/xipc/xipc_primitives.h>
#include <xtensa/xipc/xipc.h>

#define NUM_ITERS 10
#define NUM_ITEMS 2

/* Structure to define a shared queue of integers
 * with multiple producers/consumers */
typedef struct {
  xipc_mutex_t lock;      // lock for the conditions
  xipc_cond_t not_full;   // condition to wait for when buffer is full
  xipc_cond_t not_empty;  // condition to wait for when buffer is empty
  int count;              // number of elements in buffer
  int put_ptr;            // write index when adding next element
  int take_ptr;           // read index when reading next element
  int data[NUM_ITEMS];    // the actual buffer of integers
} buffer_t;

extern buffer_t buffer;
/* All consumers add up the received values in the global sum */
extern xipc_atomic_int_t sum;
/* Barrier for synchronizing all cores */
extern xipc_barrier_t shared_barrier;
/* uncached region in shared memory */
extern unsigned _shared_sysram_uncached_data_start;
