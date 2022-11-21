/*******************************************************************************

Copyright (c) 2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

xtav200-display.c   Part of board-support-package for Avnet LX200 board.

This implements the basic display functions of the XTBSP API. Since this 
board has no basic alphanumeric display, these functions are stubbed out.

*******************************************************************************/

#include <xtensa/xtbsp.h>               /* BSP API */

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

