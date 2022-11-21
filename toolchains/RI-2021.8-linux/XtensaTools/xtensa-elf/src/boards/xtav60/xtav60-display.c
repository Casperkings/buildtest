/*******************************************************************************

Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

xtav60-display.c   Part of board-support-package for Avnet LX60 board.

Implements the BSP's display functions for the MYTech MOC-16216B-B LCD display
with a SUNPLUS SPLC780D controller.

The API supports only a simple display concept: a single line of visible chars.
This driver treats the display as a single line of visible ASCII characters.
Strings begin at the left and run to the right. Nothing is written beyond the
visible window, the display is never shifted, and no cursor is visible.

More complex display functions can be accessed by driving it directly using
the board-specific definitions in <xtensa/xtav60.h>

*******************************************************************************/

#include <xtensa/xtbsp.h>               /* BSP API */
#include <xtensa/lcd-splc780d-board.h>  /* LCD cont, display and driver info. */

#ifdef SPLC780D_REGBASE

static splc780d_t *lcd = (splc780d_t *)(SPLC780D_REGBASE);


int xtbsp_display_exists(void)
{
    return 1;
}

void xtbsp_display_init(void)
{
    splc780d_init_default(lcd);
}

void xtbsp_display_char(unsigned pos, const char c)
{
    splc780d_write_char(lcd, pos, c);
}

void xtbsp_display_string(const char *s)
{
    splc780d_write_string(lcd, s);
}

void xtbsp_display_blank(void)
{
    splc780d_blank(lcd);
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

