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
  Transfer image from local to global memory using single add&schedule call
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idma_tests.h"
#include <xtensa/hal.h>

#define NUM_TRANSFERS           400 /* total num of idma transfers */
#define IMAGE_BUFFER_DEPTH	16  /* Circular buffer num of entries */
#define IDMA_XFER_SIZE          100 /* Size of each transfer */
#define IDMA_IMAGE_SIZE		IDMA_XFER_SIZE*NUM_TRANSFERS

/* Copied image  */
char _image_dst[IDMA_IMAGE_SIZE];
/* Processed image - for final compare*/
ALIGNDCACHE char _image_src[IDMA_IMAGE_SIZE] IDMA_DRAM;

char* img_src_ptr = _image_src;
char* img_dst_ptr = _image_dst;

IDMA_BUFFER_DEFINE(buffer, IMAGE_BUFFER_DEPTH, IDMA_1D_DESC);

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  printf("Images in local/main memory @ %p/%p\n", _image_src, _image_dst);
  printf("Create descriptor circular buffer with %d entries\n", IMAGE_BUFFER_DEPTH);

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_8, 100000, NULL);

  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, NULL, NULL);

  int _num_transfers = 0;

  while(_num_transfers < IDMA_IMAGE_SIZE / IDMA_XFER_SIZE)
  {
    int ret;
    bufRandomize(img_src_ptr, IDMA_XFER_SIZE);
    if(_num_transfers == 10)
       ret = idma_copy_desc(IDMA_CH_ARG  img_dst_ptr, img_dst_ptr, IDMA_XFER_SIZE, 0 ); // generate error
    else
       ret = idma_copy_desc(IDMA_CH_ARG  img_dst_ptr, img_src_ptr, IDMA_XFER_SIZE, 0 );
    DPRINT("Added descriptor @ index %d\n", ret);
    _num_transfers++;
    img_src_ptr += IDMA_XFER_SIZE ;
    img_dst_ptr += IDMA_XFER_SIZE;

    idma_hw_error_t err = idma_buffer_check_errors(IDMA_CH_ARG_SINGLE);
    if(err != IDMA_NO_ERR) {
        idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
        printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
        exit(0);
    }
  }
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

