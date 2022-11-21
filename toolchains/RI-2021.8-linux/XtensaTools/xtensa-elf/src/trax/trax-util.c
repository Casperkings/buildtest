/* List of utility functions*/

/*
 * Copyright (c) 2012-2013 Tensilica Inc.
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

#include <stdio.h>
#include <xtensa/trax.h>
#include <xtensa/trax-proto.h>
#include <xtensa/trax-util.h>
#include <xtensa/xdm-regs.h>
#include <fcntl.h>
#include <unistd.h>

/* Poll */
int trax_poll (trax_context *context)
{
  static int regs_needed[] = {
    XDM_TRAX_ID, XDM_TRAX_STATUS, XDM_TRAX_CONTROL, XDM_TRAX_ADDRESS,
    XDM_TRAX_DELAY, XDM_TRAX_TRIGGER, XDM_TRAX_MATCH, -1 };
		    
  unsigned regs[8] = {0};
  int *p, i;
  
  for (p = regs_needed, i = 0; *p >= 0; p++, i++)
    if (trax_read_register_eri (*p, &regs[i]))
      return -1;
#if DEBUG
  printf("Register values: ID:0x%x, status:0x%x, control:0x%x, address:0x%x,\
          delay:0x%x, trigger:0x%x, match:0x%x\n", regs[0], regs[1], regs[2],
	                                 regs[3], regs[4], regs[5], regs[6]);
#endif
  
  return 0;
}

/* Save */
/* This is a higher level save routine that a user can use */
int trax_save (trax_context *context, char *filename)
{
  int tfile;
  /* Allocate a buffer of 256 bytes on a stack */
  char buf[256];
  int ret_val = 0;

  if (filename == NULL) {
    return -1;
  }

  tfile = open (filename, O_CREAT | O_RDWR | O_APPEND, 0777);

  if (tfile < 0)
    return -1;

  /* While there are bytes to be read inside the trace RAM (i.e.
   * we havent yet read all the traced data), read contents into the buffer */

  while((ret_val = trax_get_trace (context, buf, 256)) > 0) {
    if (write (tfile, buf, ret_val) != ret_val){
      close(tfile);
      return -1;
    }
  }
  close(tfile);
  
  return 0;
}
