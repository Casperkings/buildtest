/*******************************************************************************

Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

uart-16550.c    Driver for National Semiconductor 16550 UART

Simple polling driver for a 16550 UART (board-independent implementation).

This is used by board-support-packages with one or more 16550 compatible UARTs.
Each instance of the UART uses a separate instance of the driver, initialized
with the base address of the UART instance.

Note that a 16552 DUART (Dual UART) is simply two instances of a 16550 UART.

*******************************************************************************/

#include <xtensa/uart-16550-board.h>    /* UART info */


/*
Set up 16550 UART with the specifid baud rate and comm. parameters:
  parity = 0 (none), 1 (odd), 2 (even), 3 (force 1), 4 (force 0).
  nstop  = 1 or 2 (stop bits) (2 implies 1.5 stop bits if ndata == 5).
  ndata  = 5, 6, 7 or 8 (data bits).
*/
void uart16550_init( uart_dev_t *u, unsigned baud, 
                     unsigned ndata, unsigned parity, unsigned nstop )
{
    unsigned short divisor = UART_DIVISOR(UART_16550_XTAL_FREQ, baud);

    _LCR(u) = DLAB_ENABLE;
    _DLL(u) = divisor;
    _DLM(u) = divisor>>8;
    _LCR(u) = WORD_LENGTH(ndata) |
              (nstop > 1 ? STOP_BIT_ENABLE : 0) |
              (parity ? PARITY_ENABLE : 0) |
              (parity & 1 ? 0 : EVEN_PARITY )  |
              (parity > 2 ? FORCE_PARITY : 0) ;
//  _IER(u) = RCVR_STATUS_INTENABLE;	/* enable break interrupts */
    _IER(u) = 0;
    _MCR(u) = _DTR | _RTS;
    _FCR(u) = _FIFO_ENABLE | RCVR_FIFO_RESET | XMIT_FIFO_RESET;
}

/*  Output char to UART, waiting for transmitter to be ready if necessary:  */
void uart16550_out( uart_dev_t *u, char c )
{
    while(!XMIT_READY(u))
	;
    _TXB(u) = c;
}

/*  Return next input char from UART, waiting for one if none available:  */
char uart16550_in( uart_dev_t *u )
{
    while(!RCVR_READY(u))
	;
    return _RXB(u);
}

