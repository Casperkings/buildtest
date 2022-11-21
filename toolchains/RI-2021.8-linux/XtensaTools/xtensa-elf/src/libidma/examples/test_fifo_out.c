/*
 * Copyright (c) 2014 by Cadence Design Systems. ALL RIGHTS RESERVED.
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
 
/* EXAMPLE:
  Transfer image from local to global memory using add/schedule calls
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idma_tests.h"

#undef IDMA_POLL

#define IMAGE_BUFFER_DEPTH	  8  /* Circular buffer num of entries */
#define IDMA_XFER_SIZE          800  /* Size of each transfer */

/* total num of idma transfers, size to fit in dataram */
#if defined (IDMA_USE_DRAM1)
#define NUM_TRANSFERS		((XCHAL_DATARAM1_SIZE - 4096) / IDMA_XFER_SIZE)
#else
#define NUM_TRANSFERS		((XCHAL_DATARAM0_SIZE - 4096) / IDMA_XFER_SIZE)
#endif

/* limit to 64000 bytes */
#if (NUM_TRANSFERS > 80)
#undef NUM_TRANSFERS
#define NUM_TRANSFERS           80
#endif

#define IDMA_IMAGE_SIZE		(IDMA_XFER_SIZE*NUM_TRANSFERS)

/* Copied image  */
char _image_dst[IDMA_IMAGE_SIZE];
/* Processed image - for final compare*/
ALIGNDCACHE char _image_src[IDMA_IMAGE_SIZE] IDMA_DRAM;

char* img_src_ptr = _image_src;
char* img_dst_ptr = _image_dst;

int _outstanding = 0; //current number of scheduled descs

IDMA_BUFFER_DEFINE(buffer, IMAGE_BUFFER_DEPTH, IDMA_1D_DESC);

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for buffer @ %p done\n", data);
}

int create_chunks(idma_buffer_t* buffer, int num)
{
  int scheduled = 0;
  while ((scheduled < (IMAGE_BUFFER_DEPTH - _outstanding)) && (scheduled < num)) {
    bufRandomize(img_src_ptr, IDMA_XFER_SIZE);
#ifdef IDMA_POLL
    int notify_w_int = 0;
#else
    int notify_w_int = (rand() & 0x1) ? DESC_NOTIFY_W_INT : 0;
#endif
     printf("Adding descriptor...\n");
    int err = idma_add_desc(buffer, img_dst_ptr, img_src_ptr, IDMA_XFER_SIZE, notify_w_int );
    if (err) {
      DPRINT("Error: Desc couldn't get added, err :%d\n", err);
      return scheduled;
    }
    scheduled++;
    img_src_ptr += IDMA_XFER_SIZE;
    img_dst_ptr += IDMA_XFER_SIZE;
  }
  return scheduled;
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  printf("Images in local/main memory @ %p/%p\n", _image_src, _image_dst);
  printf("Create descriptor circular buffer with %d entries\n", IMAGE_BUFFER_DEPTH);

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, idmaErrCB);
  int register notDone = 0;

#ifdef IDMA_POLL
  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, NULL, NULL);
#else
  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, buffer, idma_cb_func);
#endif

  int _num_transfers = 0;
  int _total_transfers = IDMA_IMAGE_SIZE / IDMA_XFER_SIZE;
  int num_to_schedule;
  int lastIndex = 0;
  
  while(_num_transfers < _total_transfers) 
  {
    /* Keep adding random number of descriptors - idmalib will handle overflow */
    num_to_schedule = 1 + (rand() & 0x3F);
    DPRINT("Trying to add %d descriptors...\n", num_to_schedule);
    
    /* If end of test (last chunk), schedule just enough to avoid compare problems */
    if((num_to_schedule + _num_transfers) >= _total_transfers) {
      num_to_schedule = _total_transfers - _num_transfers;
      DPRINT("Last set of transfers, limit schedule to %d descriptors\n", num_to_schedule);
    }

    if(num_to_schedule) {
      num_to_schedule = create_chunks(buffer, num_to_schedule);
      DPRINT("Added %d descriptors!\n", num_to_schedule);
      _num_transfers += num_to_schedule;
      lastIndex = idma_schedule_desc(IDMA_CH_ARG  num_to_schedule);
      _outstanding = num_to_schedule;
    }
  }

  while (idma_desc_done(IDMA_CH_ARG  lastIndex) == 0){notDone++;};

  if(idma_buffer_status(IDMA_CH_ARG_SINGLE) < 0) {
    idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
    printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
    exit(0);
  };
  printf("Last descriptor DONE! %d\n", notDone);

  xthal_dcache_region_invalidate(_image_dst, IDMA_IMAGE_SIZE);
  compareBuffers(0,_image_src, _image_dst, IDMA_IMAGE_SIZE );

  exit(0);
  return -1;
}

int
main (int argc, char**argv)
{
   int ret = 0;
   printf("\n\n\nTest '%s'\n\n\n", argv[0]);

#if defined _XOS
   ret = test_xos();
#else
   ret = test(0, 0);
#endif
  // test() exits so this is never reached
  return ret;
}
