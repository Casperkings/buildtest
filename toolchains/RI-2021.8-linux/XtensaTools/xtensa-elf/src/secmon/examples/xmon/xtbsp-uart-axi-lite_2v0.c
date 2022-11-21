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

// xtbsp-uart-axi-lite_2v0.c -- Part of board-support-package for boards using a UART.
// Implements the XTBSP API UART functions for the Xilinx AXI UART Lite v2.0 IP.


#include <xtensa/xtbsp.h>               /* BSP API */
#include <xtensa/uart-16550-board.h>    /* UART and driver info */

#include "xtbsp-uart-axi-lite_2v0.h"


/* C interface to UART registers. */
typedef volatile struct {
    uint32_t rx_fifo;
    uint32_t tx_fifo;
    uint32_t stat_reg;
    uint32_t ctrl_reg;
} uart_lite_dev_t;

#define _RXB_LITE(u)        ((u)->rx_fifo)
#define _TXB_LITE(u)        ((u)->tx_fifo)

/* STAT_REG bits are read-only */
#define RX_FIFO_VALID       0x01
#define RX_FIFO_FULL        0x02
#define TX_FIFO_EMPTY       0x04
#define TX_FIFO_FULL        0x08
#define INTR_ENABLED        0x10
#define OVERRUN_ERR         0x20
#define FRAME_ERR           0x40
#define PARITY_ERR          0x80
#define RCVR_READY_LITE(u)  ((u)->stat_reg & RX_FIFO_VALID)
#define XMIT_READY_LITE(u)  ((u)->stat_reg & TX_FIFO_EMPTY)

/* CTRL_REG bits are write-only */
#define RESET_TX_FIFO       0x01
#define RESET_RX_FIFO       0x02
#define ENABLE_INT          0x10


#if (defined UART_LITE_REGBASE)
static uart_lite_dev_t * uart = (uart_lite_dev_t *)UART_LITE_REGBASE;
#else
static uart_lite_dev_t * uart = 0;
#endif

#if (defined UART_LITE_INTNUM)
static unsigned intnum = UART_LITE_INTNUM;
#else
static unsigned intnum = (unsigned)-1;
#endif


int xtbsp_uart_exists(void)
{
    return 1;
}

int xtbsp_uart_init(unsigned baud, unsigned ndata, 
                    unsigned parity, unsigned nstop)
{
    // Device powers up at 38400,8n1
    UNUSED(baud);
    UNUSED(ndata);
    UNUSED(parity);
    UNUSED(nstop);
    return 0;
}

char xtbsp_uart_getchar(void)
{
    while (!RCVR_READY_LITE(uart)) {
    }
    return _RXB_LITE(uart);
}

void xtbsp_uart_putchar(const char c)
{
    while (!XMIT_READY_LITE(uart)) {
    }
    _TXB_LITE(uart) = c;
}

int xtbsp_uart_get_isready(void)
{
    return RCVR_READY_LITE(uart);
}

int xtbsp_uart_put_isready(void)
{
    return XMIT_READY_LITE(uart);
}

xtbsp_uart_int xtbsp_uart_int_enable_status(void)
{
    return 0;
}

void xtbsp_uart_int_enable(const xtbsp_uart_int mask)
{
    UNUSED(mask);
    uart->ctrl_reg = ENABLE_INT;
}

void xtbsp_uart_int_disable(const xtbsp_uart_int mask)
{
    UNUSED(mask);
    uart->ctrl_reg = 0;
}

int xtbsp_uart_int_number(const xtbsp_uart_int mask)
{
    UNUSED(mask);
    return intnum;
}

