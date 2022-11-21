/* Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/*
This example demonstrates use of the minimal board-support-package (BSP) on 
the display and UART of a supported board, when linked with the board's LSP.
A terminal emulator connected to the serial port should be set to 38400 8N1.
*/

#include <stdio.h>
#include <xtensa/xtbsp.h>

int main(int argc, char *argv[])
{
    char freqbuf[9];        /* room for 8 chars + NUL terminator */
    int count;
    float mhz = (float)xtbsp_clock_freq_hz() / 1000000;

    xtbsp_display_blank();

    printf("\n\nHello, World!\n");
    printf("I'm running on a %s board at %-1.3f MHz!\n", 
            xtbsp_board_name(), mhz);

    if (xtbsp_display_exists())
        printf("I'll print chars and flash the display until you stop me.\n");
    else
        printf("I'll print chars until you stop me.\n");
    sprintf(freqbuf, "%-2.1f MHz", mhz);
    for (count = 0; 1; count++) {
        if (count % 25 == 0) {
            printf("\n%9u ", count<<1);
            fflush(stdout);
        }
        xtbsp_display_string(xtbsp_board_name());
        printf("+"); fflush(stdout);
        xtbsp_delay_ns(1000000000);
        xtbsp_display_string(freqbuf);
        printf("-"); fflush(stdout);
        xtbsp_delay_ns(1000000000);
    }

    return 0;
}


