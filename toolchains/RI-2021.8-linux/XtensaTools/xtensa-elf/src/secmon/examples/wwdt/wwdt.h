// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// wwdt.h -- Windowed watchdog timer functions


#ifndef WWDT_H
#define WWDT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>

#if XCHAL_HAVE_WWDT == 0
#error "WWDT configuration required for this application"
#endif


/* Status register flags */
#define WWDT_STATUS_INFO_DERR_INJ_DIS           0x0200UL
#define WWDT_STATUS_INFO_TVF_ERR                0x0100UL
#define WWDT_STATUS_INFO_ERR_INJ                0x0080UL
#define WWDT_STATUS_INFO_OOR_EXP                0x0040UL
#define WWDT_STATUS_INFO_OOR_KICK               0x0020UL
#define WWDT_STATUS_INFO_ILL_KICK               0x0010UL
#define WWDT_STATUS_INFO_CNT_EXP                0x0008UL
#define WWDT_STATUS_DISABLE                     0x0004UL
#define WWDT_STATUS_DEBUG_MODE                  0x0002UL
#define WWDT_STATUS_WWDT_STATUS_FAULT_ASSERTED  0x0001UL

/* Custom status values */
#define WWDT_STATUS_NORMAL_EXPIRY               (WWDT_STATUS_WWDT_STATUS_FAULT_ASSERTED|WWDT_STATUS_INFO_CNT_EXP)
#define WWDT_STATUS_INJECTED_ERROR              (WWDT_STATUS_WWDT_STATUS_FAULT_ASSERTED|WWDT_STATUS_INFO_ERR_INJ)
#define WWDT_STATUS_TIMEOUT                     0xffffffffUL

/* Register access */
#define WWDT_READ_STATUS()                      (XT_RER(XTHAL_WWDT_STATUS))
#define WWDT_READ_COUNTDOWN()                   (XT_RER(XTHAL_WWDT_WD_COUNTDOWN))
#define WWDT_READ_RESET_COUNTDOWN()             (XT_RER(XTHAL_WWDT_RESET_COUNTDOWN_COUNT))

/* Test error codes */
#define WWDT_TEST_OK                            0
#define WWDT_TEST_TIMEOUT                       -1
#define WWDT_TEST_OUTOFWINDOW                   -2
#define WWDT_TEST_INITFAIL                      -3
#define WWDT_TEST_INJECTFAIL                    -4

/* Timeout values */
#define WWDT_INITIAL                            0xfffUL
#define WWDT_BOUND                              0x7ffUL
#define WWDT_RESET                              0xfffUL
#define WWDT_FAIL_TIME                          0x7fffUL


extern
int32_t test_xthal_wwdt_initialize(uint32_t initial, uint32_t bound, uint32_t reset);

extern
int32_t test_xthal_wwdt_kick(uint32_t initial, uint32_t bound, uint32_t reset);

extern
int32_t test_xthal_wwdt_inject_error(uint32_t initial, uint32_t bound, uint32_t reset);

extern
int32_t test_xthal_wwdt_clear_error(uint32_t initial, uint32_t bound, uint32_t reset);


#endif /* WWDT_H */
