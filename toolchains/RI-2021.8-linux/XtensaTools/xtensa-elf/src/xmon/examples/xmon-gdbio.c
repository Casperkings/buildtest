/*
 * Copyright (c) 1999-2013 Tensilica Inc.
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xmon.h>
#include <xtensa/config/core.h>
#include <xtensa/xtbsp.h>

#define INT_NUM      0
#define GDB_PKT_SZ   0x1000

#ifdef XCHAL_EXTINT0_NUM
# define UART_INT	XCHAL_EXTINT0_NUM
# define UART_MASK	XCHAL_EXTINT0_MASK
#else
# define UART_INT	0
# define UART_MASK	0
#endif

char xmonBuf[1000];  //This example doesn't use stdio.h

void _xmon_out(char c)
{
    xtbsp_uart_putchar(c);
}

int _xmon_in( void )
{
    return (unsigned char) xtbsp_uart_getchar();
}

int _xmon_flush( void )
{
    /*  Nothing.  We could wait for UART tx to become idle,c
     *  but characters are on their way out and for XMON
     *  that's all that matters.
     */
  return 0;
}

char*  xmon_filename = "C:/temp/gdbio_test_file.txt";
FILE* xmonfile = NULL;

static char xmon_running = 0;

void uart_handler(int num)
{
  asm("break 1,15" );
  xmon_running = 1;  //signal that GDB is attached
}

void create_host_file (void)
{
  int count = 0;
  xmonfile = fopen (xmon_filename, "w+");
  while (count++ < 1000){
    fprintf(xmonfile, "This message will print thousand times, count:%d\n", count);
  }
  fclose(xmonfile);
}

int
main(int argc, char* const argv[])
{
  int err;
  unsigned intNum = 0;

  xtos_set_interrupt_handler(INT_NUM, uart_handler, NULL, NULL);
  xtos_interrupt_enable(intNum);

  xtbsp_board_init();
  xtbsp_uart_int_enable(UART_MASK);
  err = _xmon_init(xmonBuf, GDB_PKT_SZ, NULL);
  if(err) {
    fprintf(stderr, "XMON couldn't initialize!\n");
    return 0;
  }

  while (!xmon_running){}; //signal that GDB is attached
  create_host_file();

  return 0;
}
