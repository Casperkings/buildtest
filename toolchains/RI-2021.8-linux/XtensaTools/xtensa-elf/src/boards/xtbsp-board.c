/*******************************************************************************
Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

xtbsp-board.c      Part of board-support-package for most Tensilica-supported
		   emulation boards.

Implementation of BSP initialization, characteristics and convenience functions.

*******************************************************************************/

#include <xtensa/xtbsp.h>           /* Function descriptions are in here */
#include <xtensa/board.h>           /* board-specific information (assumes -I...) */

/* Clock frequency and period are computed once on initialization. */
static unsigned _clock_freq_hz;
static unsigned _clock_period_ps;


/* API Functions */

void xtbsp_board_init(void)
{
    #ifdef XTBOARD_CLKFRQ_REG
    /*
    Compute and cache clock frequency and period from register on board.
    We only want to do this once (especially the divide) at initialization.
    */
    _clock_freq_hz = XTBOARD_CLKFRQ_REG;
    // To fit the ps period computation into 32-bit integer arithmetic, use:
    // _clock_period_ps = (1000000000 / _clock_freq_hz) * 1000;
    // To retain good precision at freq approx 50 MHz and avoid multiply,
    // scale 10^9 numerator by 1000/(2^8) and denominator by 2^8 (shift).
    _clock_period_ps = (3906250000U / (_clock_freq_hz>>8));
    #else
    /* Defaults in case board clock freq register is not accessible. */
    _clock_freq_hz = 50000000;
    _clock_period_ps = 20000;
    #endif

    /* Initialize UART */
    xtbsp_uart_init_default();

    /* Init display last because it requires a long busy-wait. */
    xtbsp_display_init();
}

const char * xtbsp_board_name(void)
{
    return XTBOARD_NAME;
}

void xtbsp_board_reset(void)
{
    #ifdef XTBOARD_SWRST_REG 
    XTBOARD_SWRST_REG = XTBOARD_SWRST_RESETVALUE;
    #endif
}

unsigned xtbsp_clock_freq_hz(void)
{
    return _clock_freq_hz;
}

unsigned xtbsp_clock_period_ps(void)
{
    return _clock_period_ps;
}

void xtbsp_delay_cycles(unsigned cycles)
{
    #if XCHAL_HAVE_CCOUNT
    unsigned expiry = xthal_get_ccount() + cycles;
    while( (long)(expiry - xthal_get_ccount()) > 0 );
    #else
    /*
    Approximate the cycle count by a loop iteration count. 
    Clearly this is highly dependent on config and optimization.
    */
    volatile unsigned i;
    for (i = cycles>>3; i > 0; --i);
    #endif
}

void xtbsp_delay_ns(unsigned ns)
{
    #if 0
    /*
    Scale for clock freq as low as ~1MHz, giving granularity ~1us.
    Compile-time integer divide yields 0 cycles if freq < 977 kHz.
    Shift right (integer divide by pwr of 2) yields 0 cycles if ns < 1024.
    Supports full 32-bit range of ns specifications at freq up to 1 GHz.
    */
    xtbsp_delay_cycles(
        (ns>>10) * (_clock_freq_hz / (1000000000U/(1<<10)))
        );
    #endif

    /*
    Scale for clock freq as low as ~10MHz, giving granularity 128ns.
    Compile-time integer divide yields 0 cycles if freq < 7.8 MHz.
    Shift right (integer divide by pwr of 2) yields 0 cycles if ns < 128.
    Supports full 32-bit range of ns specifications (up to 4s) at freq up
    to 1 GHz (for higher frequencies, different scaling might be needed).
    */
    xtbsp_delay_cycles(
        (ns>>7) * (_clock_freq_hz / (1000000000U>>7))
        );
}

