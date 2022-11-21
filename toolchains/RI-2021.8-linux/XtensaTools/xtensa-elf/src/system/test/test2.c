/*
 *  *  Simple 'hello world' -- with display of core instance info.
 *   *
 *    *  This is a simple test case where cores don't interact with each other,
 *     *  but which uses memory (globals, heap, and stack) for computation so
 *      *  that it's somewhat sensitive to partition overlap errors in memory maps.
 *       *  The computation is long enough that there is some opportunity for one
 *        *  core to clobber another during computation, if such an error is present.
 *         *  The lack of interaction between cores means we can test running each
 *          *  core instance stand-alone using xt-run (which should work).
 *           */

/*
 *  * Copyright (c) 2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
 *   * These coded instructions, statements, and computer programs are the
 *    * copyrighted works and confidential proprietary information of Tensilica Inc.
 *     * They may not be modified, copied, reproduced, distributed, or disclosed to
 *      * third parties in any manner, medium, or form, in whole or in part, without
 *       * the prior written consent of Tensilica Inc.
 *        */


#include <stdio.h>
#include <xtensa/system/mpsystem.h>
#include <xtensa/mpcore.h>
#include <xtensa/hal.h>
#include <stdint.h>

extern int32_t xthal_L2_ram_base(uint32_t*);

void print_l2_ram_base()
{
uint32_t base = 0xdeadbeef;
#if XCHAL_HAVE_L2
int32_t rv = xthal_L2ram_base(&base);
#endif
printf("l2 ram base = 0x%08x\n", base);
}
