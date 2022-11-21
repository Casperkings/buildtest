// Copyright (c) 1999-2021 Cadence Design Systems, Inc.
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

// xmon-main-serial.c -- XMON example using serial port as debug I/F


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xtensa/xtruntime.h>

#include <xtensa/secmon.h>

#include <xtensa/xmon.h>
#include <xtensa/config/core.h>

#include "xtbsp-uart-axi-lite_2v0.h"

#include <stdarg.h>


/*
   Note that the FIFO implemented here is thread safe, and interrupt safe w/o
   any synchronization involved; this is because  tail/head never wrap around.
   However, the program will brake once tail/head wrap around (32-bit values)
 */
#define MAX_SIZE        100000
typedef struct {
    volatile uint32_t head_;  /* first byte of data */
    volatile uint32_t tail_;  /* last byte of data */
    uint32_t len_;
    uint8_t buffer_[MAX_SIZE];
} bfifo_t;

static void 
bfifo_init(bfifo_t *f)
{
    if (f) {
        f->head_ = f->tail_ = 0;
        f->len_ = MAX_SIZE;
    }
}
        
static int
bfifo_full(bfifo_t *f)
{
    return f ? ((f->head_ - f->tail_) == f->len_) : 0;
}

static int
bfifo_empty(bfifo_t *f)
{
    return f ? (f->head_ == f->tail_) : 1;
}

static uint8_t
bfifo_get(bfifo_t *f)
{
    if (!f || bfifo_empty(f)) {
        return 0;
    }
    uint8_t data = f->buffer_[f->tail_ % f->len_];
    f->tail_++;
    return data;
}

static void
bfifo_put(bfifo_t *f, uint8_t data)
{
    if (!f || bfifo_full(f)) {
        return;
    }
    f->buffer_[f->head_ % f->len_] = data;
    f->head_++;
}

static bfifo_t inputBuffer;

int _flag_interrupt = 0;

/* Interrupt used in this example. 
 * (default or assigned by user) 
 */
unsigned _intNum;
unsigned _intEdge;

void execute_loop(void);
void execute_count(void);
int xmon_example_main(int argc, char* const argv[]);

/****** XMON used functions ********/

/* Interrupt used in this example. 
 * (default or assigned by user) 
 * NOTE: We pull out data from comm device right in the ISR handler because it
 * doesn't matter. What matters is to signal Debug Interrupt from the user code
 */
static void user_comm_device_handler(void* excFrame)
{
    UNUSED(excFrame);
    if (_intEdge) {
        xthal_interrupt_clear(_intNum);  // edge-triggered, clear int.
    }
    if (!xtbsp_uart_get_isready()) {
        // not an RX interrupt
        return;
    }
    do {
        char c = (unsigned char)xtbsp_uart_getchar();
        bfifo_put(&inputBuffer, c);
    } while (xtbsp_uart_get_isready());

    /* Do not trigger exception is XMON is already running. 
     * While it will work, it just takes unnecessary processing time
     */
    if (!_xmon_active()) {
        _flag_interrupt = 1;
    }
}

void _xmon_out(int len, char* c)
{
    int i = 0;
    while (len--) {
        xtbsp_uart_putchar(c[i++]);
    }
}

uint8_t _xmon_in()
{
    while (bfifo_empty(&inputBuffer)) {
        /* Process any further input if available; interrupt could be masked */
        if (xtbsp_uart_get_isready()) {
            user_comm_device_handler(NULL);
        }
    }
    return bfifo_get(&inputBuffer);
}

int _xmon_flush( void )
{
    /* Could wait for UART tx to become idle,c but characters
     * are on their way out and for XMON that's all that matters.
     */
    return 0;
}

static void DPRINT(char* fmt, ...)
{
    char tmpbuf[200];
    va_list ap;

    va_start(ap, fmt);
    int n = vsprintf(tmpbuf, fmt, ap);
    va_end(ap);

    tmpbuf[n] = 0;
    _xmon_consoleString(tmpbuf);
}

/* XMON Logs/Traces handler */
static void logger(xmon_log_t type, const char* log)
{
    UNUSED(type);
    DPRINT("%s", log);
}

void execute_loop(void)
{
    volatile int dummy = 0;
    for(int i = 0; i< 100000; i++) {
        dummy++;
    }
}

static volatile int count = 0;

void
execute_count(void)
{
    DPRINT("APP: Count %d\r\n", count);
    count++;
}

// External debugger will clear this variable through XMON
volatile int test_run_xmon = 1;

int
xmon_example_main(int argc, char* const argv[])
{
    int err;

    unsigned gdbPktSize = GDB_PKT_SIZE;
    _intNum = UART_LITE_INTNUM;

    /* Buffer used by XMON */
    char* xmonBuf = (char*)malloc(gdbPktSize);

    UNUSED(argc);
    UNUSED(argv);
 
    DPRINT("SECMON/XMON example\n");
    bfifo_init(&inputBuffer);

    /* Secmon initialization */
    if ((err = xtsm_init())) {
        DPRINT("SECMON couldn't initialize, err:%d\n", err);
        return err;
    }

    // Make sure we are all initialized before allowing interrupts
    xtos_set_intlevel(XCHAL_EXCM_LEVEL); // no UART interrupts yet, change to appropriate value for your target

    /* Secmon nonsecure interrupt registration */
    DPRINT ("\n Register handler for intr %d\n", _intNum);
    _intEdge = (Xthal_inttype[_intNum] == XTHAL_INTTYPE_EXTERN_EDGE);
    if ((err = xtsm_set_interrupt_handler(_intNum, 
                                          (xtsm_handler_t *)user_comm_device_handler, 
                                          (void*)_intNum))) {
        DPRINT("SECMON couldn't register nonsecure interrupt, err:%d\n", err);
        return err;
    }
    xtos_interrupt_enable(_intNum);
    DPRINT ("\n Intr %d (%s) enabled\n", _intNum, _intEdge ? "edge" : "level");

    //xtbsp_board_init(); // Will initialize UART too (xtbsp_uart_init_default())
    //xtbsp_uart_int_enable(xtbsp_uart_int_rx); // turn on receive interrupt only in 16550 UART
    xtbsp_uart_int_enable(0);

    err = _xmon_init(xmonBuf, gdbPktSize, logger);
    if(err) {
        DPRINT("XMON couldn't initialize, err:%d\n", err);
        return 0;
    }

    _xmon_logger(1, 1);

    DPRINT("XMON version: '%s' started\n", _xmon_version());

    xtos_set_intlevel(0); // allow interrupts now

    /*********** Code to debug ************** */
    while(test_run_xmon)
    {
        execute_loop();
        execute_count();

        if (_flag_interrupt) {
            _flag_interrupt = 0;
            DPRINT("Raise DebugInt.\n");    //Can't do GDBIO here if invoked from XMON
            _xmon_enter();
            DPRINT("DebugInt Done\n");      //Can't do GDBIO here if invoked from XMON
        }
    }
    /******* End of debugged code *********** */

    _xmon_close();
    DPRINT("XMON close\n");
    free(xmonBuf);
    return 0;
}
