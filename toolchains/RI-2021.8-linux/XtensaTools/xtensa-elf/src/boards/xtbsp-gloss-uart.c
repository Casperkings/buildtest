/*******************************************************************************

Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

xtbsp-gloss-uart.c  Part of board-support-package for XTBSP board using UART.

Glue to interface BSP with libgloss. Implements functions libgloss expects
in terms of the XTBSP API, using the UART for character I/O. This is common
to most boards. Boards that use a different device as the default character
I/O device must use a board-specific version <board>/<board>-gloss.c .

*******************************************************************************/

#include <xtensa/xtbsp.h>           /* Function descriptions are in here */


/* FIXME:
Could provide an assembly implementation of this that jumps to the xtbsp_*()
functions, avoiding an extra call layer that could trigger a window overflow.
Beware possible need to truncate sign-extension in result of inbyte().
*/

void board_init(void)
{
    xtbsp_board_init();
}

int inbyte(void)
{
    return (unsigned char) xtbsp_uart_getchar();
}

void outbyte(int c)
{
    xtbsp_uart_putchar(c);
}

