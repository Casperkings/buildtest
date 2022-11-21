/*******************************************************************************

Copyright (c) 2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

uart-16550-sysclk.c Extension of Driver for National Semiconductor 16550 UART

This an optional extension of the generic 16550 UART driver in uart-16550.c.

Measures the system (processor) clock frequency by comparing against a known
UART clock frequency. Boards that do not otherwise know the system clock and
that use a 16550 UART with a fixed clock frequency can use this (otherwise it
is not built or linked into the final executable).

Sets the UART to a specific baud rate. Should be called before the UART driver
is initialized. Conversely the UART needs to be reinitialized after this.

*******************************************************************************/

#include <xtensa/uart-16550-board.h>    /* UART info */


/*
 *  Use the UART clock as a reference to measure system clock frequency.
 *  To use this, the UART's crystal or clock input must be a known constant.
 *  The UART is put temporarily in loopback mode so we can measure character
 *  transmission delay without outputting junk on the console.
 *  It is necessary to (re)initialize the UART after this function is used.
 */
unsigned uart16550_measure_sys_clk( uart_dev_t *u )
{
    unsigned tstart, tend, freq;

    #define  DIV100BPS  UART_DIVISOR(UART_16550_XTAL_FREQ, 100)

    /* Initialize UART to 100 baud, 8N1 (10 bits per char), loopback. */
    _LCR(u) = DLAB_ENABLE;
    _DLL(u) = DIV100BPS;
    _DLM(u) = DIV100BPS>>8;
    _LCR(u) = WORD_LENGTH(8);
    _IER(u) = 0;
    _MCR(u) = LOOP_BACK;
    _FCR(u) = _FIFO_ENABLE | RCVR_FIFO_RESET | XMIT_FIFO_RESET;

    /*
     *  Send two characters at 10 bits/char.  The first lets us sync up with
     *  the UART clock, and we then measure transmission time of the second
     *  character (time from receipt of the 1st to that of the 2nd character).
     */
    _TXB(u) = '@';
    _TXB(u) = '@';
    while (!RCVR_READY(u));
    tstart = xthal_get_ccount();
    _RXB(u);                        /* read to clear Rx ready status */
    while (!RCVR_READY(u));
    tend = xthal_get_ccount();
    _RXB(u);                        /* read to clear Rx ready status */
    freq = (tend - tstart) * 10;    /* cycles per second (Hz) */

    /* Round frequency per UART clock frequency precision, if given. */
    #ifdef UART_16550_XTAL_PRECISION
    freq += UART_16550_XTAL_PRECISION / 2;
    freq = freq / UART_16550_XTAL_PRECISION;
    freq = freq * UART_16550_XTAL_PRECISION;
    #endif

    return freq;                    /* cycles per second (Hz) */
}
 
