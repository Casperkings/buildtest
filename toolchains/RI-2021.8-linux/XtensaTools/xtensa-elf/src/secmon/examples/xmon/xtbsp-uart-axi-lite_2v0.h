// Copyright (c) 2006-2021 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// xtbsp-uart-axi-lite_2v0.h -- Part of board-support-package for boards using a UART.
// Definitions of XTBSP API UART functions for the Xilinx AXI UART Lite v2.0 IP.


#include <xtensa/xtbsp.h>               /* BSP API */
#include <xtensa/uart-16550-board.h>    /* UART and driver info */

/* UART Lite register base address */
#define UART_LITE_REGBASE       0x20000000

/* UART Lite interrupt line */
#define UART_LITE_INTNUM        16


/* Prototypes of driver functions */
extern int  xtbsp_uart_exists(void);
extern int  xtbsp_uart_init(unsigned baud, unsigned ndata, 
                            unsigned parity, unsigned nstop);
extern char xtbsp_uart_getchar(void);
extern void xtbsp_uart_putchar(const char c);
extern int  xtbsp_uart_get_isready(void);
extern int  xtbsp_uart_put_isready(void);
extern xtbsp_uart_int xtbsp_uart_int_enable_status(void);
extern void xtbsp_uart_int_enable(const xtbsp_uart_int mask);
extern void xtbsp_uart_int_disable(const xtbsp_uart_int mask);
extern int  xtbsp_uart_int_number(const xtbsp_uart_int mask);

