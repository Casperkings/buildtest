/*******************************************************************************

Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

lcd-splc780d.c  Driver for Sunplus SPLC780D LCD display controller.

Simple driver for a SPLC780D LCD controller (board-independent implementation).

Used by board-support-packages with one or more LCDs driven by a SPLC780D.
Each instance of the controller uses a separate instance of the driver, 
initialized with the base address of the controller instance.

This is a very simple driver that implements only the minimal functionality
required by the BSP API (xtbsp.h) and handles the required timing loops by
the BSP's xtbsp_delay_ns() function (usually spins on the cycle counter).
More complex capabilities may be added later beyond what the BSP requires.

*******************************************************************************/

#include <xtensa/lcd-splc780d-board.h>  /* LCD controller and display info */
#include <xtensa/xtbsp.h>               /* BSP API */


/* Interface function comments are in the header <xtensa/lcd-splc780d.h>. */

void splc780d_init(splc780d_t *lcd, unsigned func, unsigned entry, unsigned on)
{
    xtbsp_delay_ns(20000000);   /* 20 ms */
    lcd->inst = SPLC780D_INST_FUNC_ | func;
    xtbsp_delay_ns(120000);     /* 120 us */
    lcd->inst = SPLC780D_INST_ENTRY_ | entry;
    xtbsp_delay_ns(40000);      /* 40 us */
    lcd->inst = SPLC780D_INST_CLEAR_;
    xtbsp_delay_ns(2000000);    /* 2 ms */
    lcd->inst = SPLC780D_INST_ON_ | on;
    xtbsp_delay_ns(40000);      /* 40 us */
}

void splc780d_write_char(splc780d_t *lcd, unsigned pos, const char c)
{
    if (pos >= DISPLAY_VISIBLE_LEN)
        return;

    lcd->inst = SPLC780D_INST_DRAM_ | pos;
    xtbsp_delay_ns(40000);
    lcd->data = c;
    xtbsp_delay_ns(40000);
}

void splc780d_write_string(splc780d_t *lcd, const char *s)
{
    unsigned pos;

    /* set pos to left and overwrite (quicker than "clear" or "home") */
    lcd->inst = SPLC780D_INST_DRAM_;
    xtbsp_delay_ns(40000);

    /* display chars up to end of string or visible window */
    for (pos = 0; *s != '\0' && pos < DISPLAY_VISIBLE_LEN; ++pos) {
        lcd->data = *s++;
        xtbsp_delay_ns(40000);
    }

    /* pad with spaces to end of visible window */
    for (; pos < DISPLAY_VISIBLE_LEN; ++pos) {
        lcd->data = ' ';
        xtbsp_delay_ns(40000);
    }

}

void splc780d_blank(splc780d_t *lcd)
{
    #if 0
    lcd->inst = SPLC780D_INST_CLEAR_;
    xtbsp_delay_ns(2000000);
    #else
    splc780d_write_string(lcd, ""); /* believe it or not, this is quicker! */
    #endif
}

