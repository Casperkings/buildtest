/*
 *  Simple 'hello world' -- with display of core instance info.
 *
 *  This is a simple test case where cores don't interact with each other,
 *  but which uses memory (globals, heap, and stack) for computation so
 *  that it's somewhat sensitive to partition overlap errors in memory maps.
 *  The computation is long enough that there is some opportunity for one
 *  core to clobber another during computation, if such an error is present.
 *  The lack of interaction between cores means we can test running each
 *  core instance stand-alone using xt-run (which should work).
 */

/*
 * Copyright (c) 2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <stdio.h>
#include <xtensa/system/mpsystem.h>
#include <xtensa/mpcore.h>
#include <xtensa/hal.h>

extern void print_l2_ram_base();

int main(int argc, char **argv)
{
    /*  Announce oneself:  */
    printf("Hello world, I am core #%d\n", XSHAL_CORE_INDEX);
    fflush(stdout);
    printf("Core %d of %d:  my name is %s of system %s, my config is %s.\n",
	XSHAL_CORE_INDEX, XMP_NUM_CORES, XCORE_CORE_NAME, XMP_SYS_NAME, XCORE_CONFIG_NAME);
    fflush(stdout);
#if XCHAL_HAVE_L2
    printf("L2RAMSize   = 0x%08x\n", Xthal_L2ram_size);
    printf("L2CacheSize = 0x%08x\n", Xthal_L2cache_size);
    fflush(stdout);
#endif
    print_l2_ram_base();
    /* ... alloc array from heap, gen pseudo-random sequence seeded by core number
    	   (using global data for pseudo-random generation?),
    	   sort it recursively (with extra recursions), and CRC it
	   (along with more pseudo-random numbers, so that we use global data late
	   as well, not just early), and display result with core# ... */

    /*  Done.  Let exit flush out this printf, to make sure exit works correctly.  */
    printf("Core %d done.\n", XSHAL_CORE_INDEX);
    return 0;
}

