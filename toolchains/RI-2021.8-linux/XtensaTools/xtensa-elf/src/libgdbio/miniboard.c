/*
 *  miniboard.c  --  minimal board support
 *
 * Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#pragma weak board_init

// In GDB/XOCD configuration, when libgdbio is used, we don't need
// any support for serial port, because we use JTAG for commucations.
void board_init(void)
{
	// Do nothing
}
