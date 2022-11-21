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
#include <stdio.h>
#include <string.h>
#include <xtensa/xtruntime.h>

#include <xtensa/xmon.h>
#include <xtensa/xtbsp.h>
#include <xtensa/config/core.h>

#include <stdarg.h>

#ifdef XCHAL_EXTINT0_NUM
# define UART_INT      XCHAL_EXTINT0_NUM
#else
# define UART_INT      0
#endif

/* Defaults for  GDB Packet size, int num (0 for xtbsp) */

//#define GDB_PKT_SIZE 0x1000
#define GDB_INT_NUM 0

char xmonBuf[1000]; // This example doesn't use stdio.h

/* Interrupt used in this example. 
 * (default or assigned by user) 
 */
unsigned _intNum;
unsigned _intEdge;

/****** XMON used functions ********/

void _xmon_out(int len, UCHAR* c)
{
  int i = 0;
  while (len--)
    xtbsp_uart_putchar(c[i++]);
}

int _xmon_in(int wait, UCHAR* c)
{
  // accept single chars for now...
  if(xtbsp_uart_get_isready()) {
    c[0] = (unsigned char)xtbsp_uart_getchar();
    if (_intEdge)
      xthal_interrupt_clear(_intNum);  // edge-triggered, clear int.
    return 1;
  }
  return 0;
}

int _xmon_flush( void )
{
  /* Could wait for UART tx to become idle,c but characters
   * are on their way out and for XMON that's all that matters. */
  return 0;
}

void DPRINT(char* fmt, ...)
{
  char tmpbuf[200];
  va_list ap;

  va_start(ap, fmt);
  int n = vsprintf(tmpbuf, fmt, ap);
  va_end(ap);

  tmpbuf[n] = 0;

  xtbsp_display_string(tmpbuf);
  _xmon_consoleString(tmpbuf);
}

/* XMON Logs/Traces handler */
void logger(xmon_log_t type, const char* log)
{
  //if(type != XMON_TRACE)
    xtbsp_display_string(log);
}

void uart_handler(int num)
{
  DPRINT("Got Int. %d\n", num);
  asm("break 1,15" );
}

void execute_loop(void)
{
  volatile int dummy = 0;
  int i = 0; for(i = 0; i< 100000; i++) dummy++;
}

static volatile int count = 0;
void
execute_count(void)
{
  DPRINT("APP: Count %d\n", count);
  count++;
}

int
main(int argc, char* const argv[])
{
  unsigned gdbPktSize = GDB_PKT_SIZE;
  _intNum = UART_INT;
 
  // Make sure we are all initialized before allowing interrupts
  xtos_set_intlevel(XCHAL_EXCM_LEVEL); // no UART interrupts yet, change to appropriate value for your target

  _intEdge = (Xthal_inttype[_intNum] == XTHAL_INTTYPE_EXTERN_EDGE);
  xtos_set_interrupt_handler(_intNum, (xtos_handler) uart_handler, (void*) _intNum, NULL);
  xtos_interrupt_enable(_intNum);

  xtbsp_board_init(); // Will initialize UART too (xtbsp_uart_init_default())
  xtbsp_uart_int_enable(xtbsp_uart_int_rx); // turn on receive interrupt only in 16550 UART

  int err = _xmon_init(xmonBuf, gdbPktSize, logger);
  if(err) {
    DPRINT("XMON couldn't initialize, err:%d\n", err);
    return 0;
  }

  _xmon_log(1, 1, 0, 0);

  DPRINT("XMON version: '%s' started\n", _xmon_version());

  xtos_set_intlevel(0); // allow interrupts now

  /*********** Code to debug ************** */
  while(1)
  {
    execute_loop();
    execute_count();
  }
  /******* End of debugged code *********** */

  _xmon_close();
  DPRINT("XMON close\n");
  return 0;
}
