/*
 * Copyright (c) 2016 by Cadence Design Systems. ALL RIGHTS RESERVED.
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
 
/* TEST:
   save and restore with PSO being simulated by just clearing the idma regs 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <xtensa/hal.h>
#include <xtensa/xtruntime.h>

#include "idma_tests.h"

#define IDMA_XFER_SIZE 5000 

ALIGNDCACHE char _mem[IDMA_XFER_SIZE];
ALIGNDCACHE char _mem2[IDMA_XFER_SIZE];
ALIGNDCACHE char IDMA_DRAM _lbuf[IDMA_XFER_SIZE] ;

IDMA_BUFFER_DEFINE(buffer, 20, IDMA_1D_DESC);

void idma_shutoff_faked(int ch)
{
  WRITE_IDMA_REG(ch, IDMA_REG_SETTINGS, 0);
  WRITE_IDMA_REG(ch, IDMA_REG_TIMEOUT, 0);
  WRITE_IDMA_REG(ch, IDMA_REG_DESC_START, 0);
  WRITE_IDMA_REG(ch, IDMA_REG_CONTROL, 0);
  WRITE_IDMA_REG(ch, IDMA_REG_USERPRIV, 0);
}

void idma_do_fake_pso(unsigned wait)
{
#define RESTORE_RET_VAL	0x55
  XtosCoreState saveArea;
  uint32_t flags = !wait ? XTOS_IDMA_NO_WAIT : 0;
  int ret = xtos_core_save(XTOS_COREF_PSO | XTOS_KEEPON_MEM | flags,  &saveArea, NULL);
  if(ret == RESTORE_RET_VAL)
  {
    printf ("IDMA SAVE STATUS:%d\n", (uint32_t)saveArea.idmaregs[5]);
    return;
  }
  idma_shutoff_faked(IDMA_CHANNEL_0);
  xtos_core_restore(RESTORE_RET_VAL, &saveArea);
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
  srand(time(NULL));
  //int ch = IDMA_CHANNEL_0;
  int ret = 0;

  idma_log_handler(idmaLogHander);
  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 2, TICK_CYCLES_2, 0, idmaErrCB); 
  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, 20, NULL, NULL); 

  bufRandomize(_mem, IDMA_XFER_SIZE);
  bufRandomize(_lbuf, IDMA_XFER_SIZE);
  xthal_dcache_region_writeback(_mem, IDMA_XFER_SIZE);

  idma_copy_desc(IDMA_CH_ARG  _lbuf, _mem, IDMA_XFER_SIZE, 0 );
  idma_do_fake_pso(1);

  compareBuffers(0, _mem, _lbuf, IDMA_XFER_SIZE);

  idma_init(IDMA_CH_ARG  0, MAX_BLOCK_2, 1, TICK_CYCLES_16, 0, idmaErrCB); 
  idma_init_loop(IDMA_CH_ARG  buffer, IDMA_1D_DESC, 20, NULL, NULL);

  bufRandomize(_mem2, IDMA_XFER_SIZE);
  bufRandomize(_lbuf, IDMA_XFER_SIZE);
  xthal_dcache_region_writeback(_mem2, IDMA_XFER_SIZE);

  idma_copy_desc(IDMA_CH_ARG _mem2, _lbuf, IDMA_XFER_SIZE, 0 );
  idma_do_fake_pso(0);
  idma_stop();

  xthal_dcache_region_invalidate(_mem2, IDMA_XFER_SIZE);
  compareBuffers(1, _lbuf, _mem2, IDMA_XFER_SIZE);

  return ret;
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
