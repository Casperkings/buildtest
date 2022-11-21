/*
 * Copyright (c) 2015 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#if defined (LIBIDMA_USE_CHANNEL0)
# ifndef IDMA_USE_MULTICHANNEL
# define IDMA_USE_MULTICHANNEL
# endif
# define IDMA_CHANNEL   0
#endif
#if defined (LIBIDMA_USE_CHANNEL1)
# ifndef IDMA_USE_MULTICHANNEL
# define IDMA_USE_MULTICHANNEL
# endif
# define IDMA_CHANNEL   1
#endif

#if defined (LIBIDMA_USE_CHANNEL0) || defined (LIBIDMA_USE_CHANNEL1) || defined (IDMA_USE_MULTICHANNEL)
# ifndef IDMA_CHANNEL
# define IDMA_CHANNEL           0
# endif
# define IDMA_CH_ARG            IDMA_CHANNEL,
# define IDMA_CH_ARG_SINGLE     IDMA_CHANNEL
#else
# define IDMA_CH_ARG
# define IDMA_CH_ARG_SINGLE
#endif

#include <xtensa/idma.h>
#include <xtensa/hal.h>
#ifdef _XOS
#include <xtensa/xos.h>
#else
#include <xtensa/xtruntime.h>
#endif
#include <xtensa/xtutil.h>

#ifdef IDMA_DEBUG
#define DPRINT(fmt...)	do { printf(fmt);} while (0);
#else
#define DPRINT(fmt...)	do {} while (0);
#endif

void
bufRandomize(char *ptr, int size)
{
  int i;
  for(i = 0 ; i < size; i++)
    ptr[i] = (rand() + 1) & 0xFF;
#if 0
  DPRINT("Image %p :", ptr);
  for(i = 0; i < size; i++)
    DPRINT("%x-", ptr[i]);
  DPRINT("\n");
#endif
 // xthal_dcache_region_writeback_inv(ptr, size);
}


void idmaLogHander (const char* str)
{
  DPRINT("libidma: %s", str);
}

void printBuffer(char* src, int size)
{
  int i;
  for(i = 0; i < size; i++)
    DPRINT("%x-", src[i]);
  DPRINT("\n");
}

int
compareBuffers(int expected, char* src, char* dst, int size)
{
  int i;
  for(i = 0 ; i < size; i++) {
    if (dst[i] != src[i]) {
      if (expected) {
        printf("MISMATCH EXPECTED @ byte %d (dst:%x, src:%x)!!\n", i, dst[i], src[i]);
      }
      else {
        printf("COPY FAILED @ byte %d (dst:%x, src:%x)!!\n", i, dst[i], src[i]);
	return  -1;
      }
    }
  }
  printf("Compare OK (src:%p, dst:%p, len:%d)\n", src, dst, size);
  return 0;
}


/* This is a simple function that assumes src/dst pitch is row size, so no gaps. 
   The function does acknowledge the row mask 
 */
int
compareRows(int expected, char* src, char* dst, int size, int row_size, char* row_mask)
{
   int i;
#if 0
   printf("SRC: ");
   for (i = 0; i < size; i++) {
     if (!(i%row_size))
       printf("\n");
     printf("%x-", src[i]);
   }
   printf("\n");

   printf("DST: ");
   for (i = 0; i < size; i++) {
     if (!(i%row_size))
       printf("\n");
     printf("%x-", dst[i]);
   }
   printf("\n");
#endif

  printf("Comparing rows (%d, %d)...\n", row_size, size/row_size);

  for (i = 0; i < size; i+=row_size, src+=row_size) {
    int row = i/row_size;
    int maskbit = (row_mask[row/8] >> (row%8)) & 1;
    printf("Row %d: ", row);
    if (maskbit == 0) {
       printf("MASKED...\n");
       continue;
    }
    int ret = compareBuffers(expected, src, dst, row_size);
    if (ret) return -1;
    dst += row_size;
  }
  return 0;
}

// Check function for predicated transfers. Compare rows while taking into
// account predicates, transfer size, number of rows, src and dst pitches.
int
compareRowsPred(int    expected,
                char * src,
                char * dst,
                int    size,        // currently unused
                int    row_size,    // bytes copied in each row
                char * row_mask,    // pointer to row mask bytes
                int    num_rows,    // number of rows to check
                int    src_pitch,
                int    dst_pitch)
{
    int i;

    for (i = 0; i < num_rows; i++) {
        int maskbit = (row_mask[i/8] >> (i%8)) & 1;

        printf("Row %d: ", i);
        if (maskbit == 0) {
            printf("MASKED...\n");
            src += src_pitch;
            continue;
        }

        int ret = compareBuffers(expected, src, dst, row_size);
        if (ret)
            return -1;
        dst += dst_pitch;
        src += src_pitch;
    }

    return 0;
}

#if defined (IDMA_USE_MULTICHANNEL) && (XCHAL_IDMA_NUM_CHANNELS > 1)

void
idmaErrCB_ch0(const idma_error_details_t* data)
{
  DPRINT("ERROR CALLBACK: iDMA in Error\n");
  idma_error_details_t* error = idma_error_details(0);
  printf("CH0 COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
}

void
idmaErrCB_ch1(const idma_error_details_t* data)
{
  DPRINT("ERROR CALLBACK: iDMA in Error\n");
  idma_error_details_t* error = idma_error_details(1);
  printf("CH1 COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
}

#endif

void
idmaErrCB(const idma_error_details_t* data)
{
  DPRINT("ERROR CALLBACK: iDMA in Error\n");
  idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
  printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
}


//************** XOS STUFF *****************//
#ifdef _XOS
#define NUM_THREADS 1

#define STACK_SIZE      (0x400 + XCHAL_TOTAL_SA_SIZE)
#define TICK_CYCLES     (xos_get_clock_freq()/100)
#define TIMER_INTERVAL   0x1423

XosThread test_tcb;
char   test_stack[STACK_SIZE];

int test(void* arg, int unused);

XosThread test_threads[NUM_THREADS];
char   test_stacks[NUM_THREADS][STACK_SIZE];

int
test_xos()
{
    int32_t ret;

    xos_set_clock_freq(XOS_CLOCK_FREQ);
    ret = xos_thread_create(&test_tcb,
                            0,
                            test,
                            0,
                            "test",
                            test_stack,
                            STACK_SIZE,
                            7,
                            0,
                            0);
    xos_start(0);
    return -1; // should never get here
}
#endif
