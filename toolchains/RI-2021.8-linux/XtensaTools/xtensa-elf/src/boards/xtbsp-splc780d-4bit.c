/*******************************************************************************
Copyright (c) 2009-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

xtbsp-splc780d-4bit.c   Board-support-package display routines for these boards:
	Avnet LX110 (XT-AV110)
	Xilinx ML605 (XT-ML605)
	Xilinx KC705 (XT-KC705)

Implements the BSP's display functions for the MYTech MOC-16216B-B LCD display
with a SUNPLUS SPLC780D controller operating in 4 bit mode.

The API supports only a simple display concept: a single line of visible chars.
This driver treats the display as a single line of visible ASCII characters.
Strings begin at the left and run to the right. Nothing is written beyond the
visible window, the display is never shifted, and no cursor is visible.

More complex display functions can be accessed by driving it directly using
the board-specific definitions in <xtensa/board.h>

*******************************************************************************/

#include <xtensa/xtbsp.h>               /* BSP API */
#include <xtensa/lcd-splc780d-4bitmode-board.h>  /* LCD cont, display and driver info. */

#ifdef SPLC780D_4BIT_REGBASE

static splc780d_4bit_t *lcd = (splc780d_4bit_t *)(SPLC780D_4BIT_REGBASE);


int xtbsp_display_exists(void)
{
    return 1;
}

void xtbsp_display_init(void)
{
    splc780d_4bit_init_default(lcd);
}

void xtbsp_display_char(unsigned pos, const char c)
{
    splc780d_4bit_write_char(lcd, pos, c);
}

void xtbsp_display_string(const char *s)
{
    splc780d_4bit_write_string(lcd, s);
}

void xtbsp_display_blank(void)
{
    splc780d_4bit_blank(lcd);
}

#else

int xtbsp_display_exists(void)
{
    return 0;
}

void xtbsp_display_init(void)
{
}

void xtbsp_display_char(unsigned pos, const char c)
{
}

void xtbsp_display_string(const char *s)
{
}

void xtbsp_display_blank(void)
{
}

#endif

