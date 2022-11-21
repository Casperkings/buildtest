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
  IO brings data into the main memory. DMA transfers it to local memory for CPU
  processing. Main memory and local memory implement double buffering for faster
  processing. DMA descriptors are also organized into a double buffer with JUMP 
  from 2nd to 1st. DMA descriptors have fixed source and destination fields to
  point to each of the data buffers in local/main memory. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idma_tests.h"

#define IMAGE_BUFFER_DEPTH	2
#define IMAGE_CHUNK_SIZE	200
#define IDMA_XFER_SIZE		IMAGE_CHUNK_SIZE
#define IDMA_IMAGE_SIZE		IDMA_XFER_SIZE*100

/* Original image - for final compare */
char _image_src[IDMA_IMAGE_SIZE];
/* Processed image - for final compare*/
char _image_dst[IDMA_IMAGE_SIZE];

void
idma_cb_func (void* data)
{
  DPRINT("CALLBACK: Copy request for buffer @ %p is done\n", data);
}


/* Consume chunk prepared by DMA (either by CPU or IO) */
void _consume_chunk(char* src, int size)
{
  static int tmp_count = 0;
  xthal_dcache_region_invalidate(src, size);

#if 0
  int i;
  DPRINT("Consume from %p (IMAGE:%p):", src, _image_dst + tmp_count);
  for(i = 0; i < size; i++)
    DPRINT("%x-", src[i]);
  DPRINT("\n");
#endif
  /* Also, write test image so it can be compared with the one created by DMA */
  memcpy(_image_dst + tmp_count, src, size);
  tmp_count += size;
}

void _create_chunk(char* src, int size, int flush)
{
  static int tmp_count = 0;

  /* Create random data to serve as new image chunk */
  bufRandomize(src, size);
#if 0
  int i;
  DPRINT("Create @ %p (IMAGE:%p):", src, _image_src + tmp_count);
  for(i = 0; i < size; i++)
    DPRINT("%x-", src[i]);
  DPRINT("\n");
#endif
  /* Also, write test image so it can be compared with the one created by DMA */
  memcpy(_image_src + tmp_count, src, size);

  xthal_dcache_region_writeback_inv(src , size);

  tmp_count += size;
}

/* Double buffer in the Local Memory, DMA fills */
ALIGNDCACHE char IDMA_DRAM _buf0[IMAGE_CHUNK_SIZE] ;
ALIGNDCACHE char IDMA_DRAM _buf1[IMAGE_CHUNK_SIZE] ;

/* Double buffer in the main memory, IO fills */
ALIGNDCACHE char _mem0[IMAGE_CHUNK_SIZE];
ALIGNDCACHE char _mem1[IMAGE_CHUNK_SIZE];

IDMA_BUFFER_DEFINE(buffer, IMAGE_BUFFER_DEPTH, IDMA_1D_DESC);

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  int i;

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);

  DPRINT("Double buffer allocated at %p/%p: \n", _buf0, _buf1);

  /* Create 2 descriptors to support double buffer.
     Add JUMP from the 2nd to the 1st one.
     Each desc. destination points to the one double buffer entry.
   */
   DPRINT("Create descriptor double buffer.\n");

   idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, IMAGE_BUFFER_DEPTH, buffer, idma_cb_func);

   uint32_t desc_opts = DESC_NOTIFY_W_INT | DESC_IDMA_PRIOR_H ;
   idma_add_desc(buffer, _buf0, _mem0, IDMA_XFER_SIZE,  desc_opts );
   idma_add_desc(buffer, _buf1, _mem1, IDMA_XFER_SIZE,  desc_opts );

   /* A process (external DMA or IO) brings data chunks to the main memory.
      DMA fetches data from the main memory and transfers them to Local memory.
      Double buffering is used for speed-up.
      Start from IO to _mem0 (and DMA to corresponding _buf0)
    */
   i = 0;
   _create_chunk(_mem0, IMAGE_CHUNK_SIZE, 1);

   for (i = 1; i <= IDMA_IMAGE_SIZE / IMAGE_CHUNK_SIZE; i++)
   {
     char* cur_dst  = (i & 1) ? _buf0 : _buf1;
     char* next_src = (i & 1) ? _mem1 : _mem0;
    
     // DMA can now transfer the chunk from main to local memory
#if IDMA_DEBUG
      char* cur_src  = (i & 1) ? _mem0 : _mem1;
     DPRINT("  Schedule DMA %p -> %p\n", cur_src, cur_dst);
#endif
     idma_schedule_desc_fast(IDMA_CH_ARG  1);
    
     // IO keeps filling the next buffer
     if(i < (IDMA_IMAGE_SIZE / IMAGE_CHUNK_SIZE))
       _create_chunk(next_src, IMAGE_CHUNK_SIZE, 1);
    
     // Can proceed only when DMA is done
     // NOTE: Can't check if outstanding descriptor only if there 
     // interrupt setup on desc completion and err/completion ints
     // are not masked out.
     DPRINT("  Waiting for DMA to free a descriptor....\n");
     while (idma_buffer_status(IDMA_CH_ARG_SINGLE) > 0 )
       idma_sleep(IDMA_CH_ARG_SINGLE  );
    
    //4. CPU can now consume the buffer from local memory
    DPRINT(" CPU consumes buffer %p\n", cur_dst);
    _consume_chunk(cur_dst, IMAGE_CHUNK_SIZE);
   }

   if(idma_buffer_status(IDMA_CH_ARG_SINGLE) < 0) {
     idma_error_details_t* error = idma_error_details(IDMA_CH_ARG_SINGLE);
     printf("COPY FAILED, Error 0x%x at desc:%p, PIF src/dst=%x/%x\n", error->err_type, (void*)error->currDesc, error->srcAddr, error->dstAddr);
     exit(0);
   }
   compareBuffers(0,_image_src, _image_dst, IDMA_IMAGE_SIZE);

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
