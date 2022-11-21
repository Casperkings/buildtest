
// msg_test.c - XOS message test

// Copyright (c) 2015 Cadence Design Systems, Inc.
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


#include <string.h>
#include <stdlib.h>

#include "test.h"


#if !USE_MAIN_THREAD
XosThread msg_test_tcb;
uint8_t   msg_test_stk[STACK_SIZE];
#endif

XosThread send1;
XosThread send2;
XosThread send3;
XosThread recv1;
XosThread recv2;
XosThread recv3;

uint8_t stack1[STACK_SIZE];
uint8_t stack2[STACK_SIZE];
uint8_t stack3[STACK_SIZE];
uint8_t stack4[STACK_SIZE];
uint8_t stack5[STACK_SIZE];
uint8_t stack6[STACK_SIZE];

typedef struct test_msg {
    uint32_t index;
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
} test_msg;

// Test queue, 40 messages
XOS_MSGQ_ALLOC(testq, 40, sizeof(test_msg));


#define MSG_COUNT	200

test_msg src_list[MSG_COUNT];
test_msg dst_list[MSG_COUNT];
volatile uint32_t tx_count;
volatile uint32_t rx_count;

XOS_MSGQ_ALLOC(q1, 34, 16);
XOS_MSGQ_ALLOC(q2, 19, 32);
XOS_MSGQ_ALLOC(q3, 77, 28);

int32_t
check_alloc(void)
{
    uint32_t * p1;
    uint32_t * p2;
    int32_t    ret = 0;

    p1 = q1_buf.q.msg;
    p2 = q1_buf.b;
    if (p2 != (p1+1)) {
        ret = -1;
    }

    p1 = q2_buf.q.msg;
    p2 = q2_buf.b;
    if (p2 != (p1+1)) {
        ret = -1;
    }

    p1 = q3_buf.q.msg;
    p2 = q3_buf.b;
    if (p2 != (p1+1)) {
        ret = -1;
    }

    if (ret != 0) {
        printf("Problem with XOS_MSGQ_ALLOC macro\n");
    }
    return ret;
}


int32_t
get_func(void * arg, int32_t unused)
{
    int32_t    ret;
    test_msg   recv;
    test_msg * pdst;
    uint32_t   ps;
    uint32_t   index;

    while (1) {
        ps = xos_disable_interrupts();
        index = rx_count;
        if (index < MSG_COUNT)
            rx_count++;
        xos_restore_interrupts(ps);
        if (index >= MSG_COUNT)
            break;

        ret = xos_msgq_get(testq, (uint32_t *)&recv);
        XOS_ASSERT(ret == XOS_OK);

        pdst = dst_list + recv.index;
        memcpy(pdst, &recv, sizeof(test_msg));

        xos_thread_sleep((rand() % 10) * TICK_CYCLES);
    }

    return 0;
}


int32_t
put_func(void * arg, int32_t unused)
{
    int32_t    ret;
    uint32_t   ps;
    uint32_t   index;
    test_msg * psrc;

    while (1) {
        ps = xos_disable_interrupts();
        index = tx_count;
        if (index < MSG_COUNT)
            tx_count++;
        xos_restore_interrupts(ps);
        if (index >= MSG_COUNT)
            break;

        psrc = src_list + index;

        ret = xos_msgq_put(testq, (uint32_t *)psrc);
        XOS_ASSERT(ret == XOS_OK);

        xos_thread_sleep((rand() % 10) * TICK_CYCLES);
    }

    return 0;
}


int32_t
msg_test(void * arg, int32_t unused)
{
    int32_t  ret;
    int32_t  i;

    ret = check_alloc();
    if (ret != 0) {
#if !USE_MAIN_THREAD
        exit(ret);
#else
        return ret;
#endif
    }

    for (i = 0; i < MSG_COUNT; i++) {
        src_list[i].index = i;
        src_list[i].data1 = i << 8;
        src_list[i].data2 = i << 16;
        src_list[i].data3 = 0xfeedfeed;
    }

    xos_msgq_create(testq, 40, sizeof(test_msg), 0);

    ret = xos_thread_create(&send1, 0, put_func, 0, "send1",
                            stack1, STACK_SIZE, 5, 0, 0);
//#if 0
    ret = xos_thread_create(&send2, 0, put_func, 0, "send2",
                            stack2, STACK_SIZE, 5, 0, 0);
    ret = xos_thread_create(&send3, 0, put_func, 0, "send3",
                            stack3, STACK_SIZE, 5, 0, 0);
//#endif
    ret = xos_thread_create(&recv1, 0, get_func, 0, "recv1",
                            stack4, STACK_SIZE, 5, 0, 0);
//#if 0
    ret = xos_thread_create(&recv2, 0, get_func, 0, "recv2",
                            stack5, STACK_SIZE, 5, 0, 0);
    ret = xos_thread_create(&recv3, 0, get_func, 0, "recv3",
                            stack6, STACK_SIZE, 5, 0, 0);
//#endif
    ret = xos_thread_join(&send1, &ret);
    ret = xos_thread_join(&send2, &ret);
    ret = xos_thread_join(&send3, &ret);
    ret = xos_thread_join(&recv1, &ret);
    ret = xos_thread_join(&recv2, &ret);
    ret = xos_thread_join(&recv3, &ret);

    ret = 0;
    for (i = 0; i < MSG_COUNT; i++) {
        if (dst_list[i].index != src_list[i].index) {
            printf("FAIL: data mismatch %d\n", i);
            ret = -1;
        }
    }

    if (ret == 0)
        printf("PASS: data match\n");

#if !USE_MAIN_THREAD
    exit(ret);
#endif
    return ret;
}


int32_t
main()
{
    int32_t ret;

#if defined BOARD
    xos_set_clock_freq(xtbsp_clock_freq_hz());
#else
    xos_set_clock_freq(XOS_CLOCK_FREQ);
#endif

#if USE_MAIN_THREAD
    xos_start_main_ex("main", 7, STACK_SIZE);
    xos_start_system_timer(-1, TICK_CYCLES);

    ret = msg_test(0, 0);
    return ret;
#else
    ret = xos_thread_create(&msg_test_tcb,
                            0,
                            msg_test,
                            0,
                            "msg_test",
                            msg_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);

    xos_start_system_timer(-1, TICK_CYCLES);
    xos_start(0);
    return -1; // should never get here
#endif
}

