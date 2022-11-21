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

// xmon-main-iss.c -- XMON example using ISS port as debug I/F
 
// The example implements an interrupt handler that's used to get data out
// of device (ISS). This results in an overhead as the ISS already has a properly
// implemented multi-threaded queue so it would be faster if _xmon_in() directly
// maps onot _xmon_iss_in(). However, by having the interrupt handler the code
// is closer to the real system where indeed the interrupt routine will pull
// data out of device.
//
// NOTE: Xtensa doesn't support C++11 threading, we don't want to tie this example,
// to XOS/FreeRTOS/pthreads, so no sync variables are used here. To pull data
// out the simulator (our comm. device) we use a simple flag. Write/read buffer
// is made so it doesn't require any synchronization.

 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xmon.h>

#include <xtensa/secmon.h>


/* Defaults: GDB packet size, int. num., ISS port num */

#if (defined XCHAL_EXTINT0_NUM) && (XCHAL_EXTINT0_LEVEL <= XCHAL_EXCM_LEVEL)
# define DEV_INT       XCHAL_EXTINT0_NUM
#elif (defined XCHAL_EXTINT1_NUM) && (XCHAL_EXTINT1_LEVEL <= XCHAL_EXCM_LEVEL)
# define DEV_INT       XCHAL_EXTINT1_NUM
#elif (defined XCHAL_EXTINT2_NUM) && (XCHAL_EXTINT2_LEVEL <= XCHAL_EXCM_LEVEL)
# define DEV_INT       XCHAL_EXTINT2_NUM
#elif (defined XCHAL_EXTINT3_NUM) && (XCHAL_EXTINT3_LEVEL <= XCHAL_EXCM_LEVEL)
# define DEV_INT       XCHAL_EXTINT3_NUM
#else
# define DEV_INT       -1
#endif

typedef enum  {
    MODE1 = 1,
    MODE2,
} ModeEnum;

static const char * modetxt[] = {
    "",
    "Start w/o exception.",
    "Start w/ exception."   
};

/* When using GDBIO on ISS it actually doesn't use GDBIO. So, a printf will good
   to ISS. So, setting libio_mode below is not needed.
 */

static void DPRINT(const char* fmt, ...)
{
    char tmpbuf[200];
    va_list ap;
    va_start(ap, fmt);
    int n = vsprintf(tmpbuf, fmt, ap);
    va_end(ap);
    tmpbuf[n] = 0;
    printf("%s", tmpbuf);
}

#define PRINTISS(...)   DPRINT(__VA_ARGS__);
#define PRINT(...)      DPRINT(__VA_ARGS__);

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
        PRINTISS("FIFO full %d!\n", bfifo_full(f));
        return;
    }
    f->buffer_[f->head_ % f->len_] = data;
    f->head_++;
}

static bfifo_t xmonBuffer;

int _flag_interrupt = 0;

extern void  _xmon_iss_init(int intNum, int portNum, int debug);
extern void  _xmon_iss_out(char);
extern char  _xmon_iss_in_rdy(void);
extern void  _xmon_iss_flush(void);
extern char  _xmon_iss_in(void);

void execute_loop(void);
void execute_count(void);
int xmon_example_main(int argc, char* const argv[]);

uint32_t start, end;

/****** XMON used functions ********/

int _xmon_flush()
{
    _xmon_iss_flush();
    return 0;
}

/* XMON Logs/Traces handler  */
static void logger(xmon_log_t type, const char *str)
{
    const char* logType = (type == XMON_ERR) ? "ERROR" :
                          (type == XMON_LOG) ? "LOG"   : "???";
    /* XMON logs can't go to GDB as GDBIO (they're sent as notification packets) */
    PRINTISS("**XMON %s**: %s", logType, str);
}

/**************************************************/

/* Interrupt used in this example. 
 * (default or assigned by user) 
 * NOTE: We pull out data from comm device right in the ISR handler because it
 * doesn't matter. What matters is to signal Debug Interrupt from the user code
 */
static void user_comm_device_handler(void* excFrame)
{
    UNUSED(excFrame);
    PRINTISS("In XMON USER interrupt handler\n");     //Can't do GDBIO here if invoked from XMON
    while (_xmon_iss_in_rdy()){
        char c = _xmon_iss_in();
        bfifo_put(&xmonBuffer, c);
    }

    /* Do not trigger exception is XMON is already running. 
     * While it will work, it just takes unnecessary processing time
     */
    if (!_xmon_active()) {
        _flag_interrupt = 1;
    }
    PRINTISS("XMON USER interrupt handler exit\n");  //Can't do GDBIO here if invoked from XMON
}

void _xmon_out(int len, char* c)
{
    int i = 0;
    while (len-- > 0) {
        _xmon_iss_out(c[i++]);
    }
}

uint8_t _xmon_in()
{
    while (bfifo_empty(&xmonBuffer)) {
        /* Process any further input if available; interrupt could be masked */
        if (_xmon_iss_in_rdy()) {
            user_comm_device_handler(NULL);
        }
    }
    return bfifo_get(&xmonBuffer);
}

/**************************************************/

void execute_loop(void)
{
    static volatile int dummy = 0;
    for(int i = 0; i< 100000; i++) {
        dummy++;
    }
}

void execute_count(void)
{
    static volatile int count = 0;
    //DPRINT("APP: Count %d\n", count);
    count++;
}

//Use: arg1=1 arg2=2 ..., no space on either side of '='
volatile  int _a=0, _b, _c=2, _d=3,_e=1;

// External debugger will clear this variable through XMON
volatile  int test_run_xmon = 1;

int
xmon_example_main(int argc, char* const argv[])
{
    int err;

    // all input arguments
    int portNum        = 20000;
    int gdbPktSize     = GDB_PKT_SIZE;
    int logApp         = 1;
    int logGDB         = 1;
    int logISS         = 1;
    int deviceIntrNum  = DEV_INT;
    int dbgIntNum      = DEV_INT+1;
    static int mode    = MODE1;

    bfifo_init(&xmonBuffer);

    while (argc-- > 1) {
        char *type = strtok(argv[argc], "=");
        if (!type) {
            break;
        }
        int val = strtol(strtok(NULL, ""), NULL, 0);
        {
            if (strcmp(type, "port") == 0)     portNum       = val;
            if (strcmp(type, "devint") == 0)   deviceIntrNum = val;
            if (strcmp(type, "dbgint") == 0)   dbgIntNum   = val;
            if (strcmp(type, "pktsize") == 0)  gdbPktSize  = val;
            if (strcmp(type, "logapp") == 0)   logApp      = val;
            if (strcmp(type, "logiss") == 0)   logISS      = val;
            if (strcmp(type, "loggdb") == 0)   logGDB      = val;
            if (strcmp(type, "mode") == 0)    mode         = val;
        }
        PRINTISS ("ARG: '%s=%d'\n", type, val);
    }
  
    PRINTISS ("\n\n ** Test details: %s **\n\n", modetxt[mode]);

    if (deviceIntrNum == -1) {
        PRINTISS("No suitable device interrupt found.  Please specify one with \"devint=\".\n");
        return -1;
    }

    /* Buffer used by XMON */
    char* xmonBuf = (char*)malloc(gdbPktSize);

    /* Secmon initialization */
    if ((err = xtsm_init())) {
        PRINTISS("SECMON couldn't initialize, err:%d\n", err);
        return err;
    }

    /* Secmon nonsecure interrupt registration */
    PRINTISS ("\n Register handler for intr %d\n", deviceIntrNum);
    if ((err = xtsm_set_interrupt_handler(deviceIntrNum, 
            (xtsm_handler_t *)user_comm_device_handler, 
            (void*)deviceIntrNum))) {
        PRINTISS("SECMON couldn't register nonsecure interrupt, err:%d\n", err);
        return err;
    }
    xtos_interrupt_enable(deviceIntrNum);

    /* Init ISS character device to attach to GDB */
    _xmon_iss_init(deviceIntrNum, portNum, logISS);

    _xmon_enable_dbgint(dbgIntNum);
    _xmon_logger (logApp, logGDB);

    if ((err = _xmon_init(xmonBuf, gdbPktSize, logger))) {
        PRINTISS("XMON couldn't initialize, err:%d\n", err);
        return err;
    }

    PRINTISS("XMON version: '%s' started, port:%d, deviceIntrNum:%d\n", _xmon_version(), portNum, deviceIntrNum);

    /* XMON can be entered before GDb attached */
    if (mode == MODE2) {
        _a += 1;
        _b += _a;
        _c -= 2;
        _d -= _c;
        _e += _a+_b+_c+_d+1;
        _xmon_enter();
        PRINT("Break Done!\n"); //FIXME - could go to ISS
    }

    /*********** Code to debug ************** */

    while(test_run_xmon)
    {
        execute_loop();
        execute_count();
        static int count = 0;
        printf("Count:%d\n", count++);

        if (_flag_interrupt) {
            _flag_interrupt = 0;
            PRINTISS("Raise DebugInt.\n");    //Can't do GDBIO here if invoked from XMON
            _xmon_enter();
            PRINTISS("DebugInt Done\n");      //Can't do GDBIO here if invoked from XMON
        }
    }

    /******* End of debugged code *********** */

    _xmon_close();
    PRINTISS("XMON close\n");
    free(xmonBuf);
    return 0;
}
