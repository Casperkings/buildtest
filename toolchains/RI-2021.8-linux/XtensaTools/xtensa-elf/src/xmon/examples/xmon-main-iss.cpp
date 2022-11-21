/*
 * Copyright (c) 1999-2016 Cadence Design Systems.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
/* The example implements an interrupt handler that's used to get data out
 * of device (ISS). This results in an overhead as the ISS already has a properly
 * implemented multi-threaded queue so it would be faster if _xmon_in() directly
 * maps onot _xmon_iss_in(). However, by having the interrupt handler the code
 * is closer to the real system where indeed the interrupt routine will pull
 *  data out of device.
 * NOTE: Xtensa doesn't support C++11 threading, we don't want to tie this example,
 * to XOS/FreeRTOS/pthreads, so no sync variables are used here. To pull data
 * out the simulator (our comm. device) we use a simple flag. Write/read buffer
 * is made so it doesn't require any synchronization.
   
*/
 
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xmon.h>
#include <sstream>
#include <iostream>

//extern int libio_mode;

using namespace std;

/* Defaults: GDB packet size, int. num., ISS port num */

#ifdef XCHAL_EXTINT0_NUM
# define UART_INT      XCHAL_EXTINT0_NUM
#else
# define UART_INT      0
#endif

typedef enum  {
  MODE1 = 1,
  MODE2,
} ModeEnum;

static const char * modetxt[] = {
  "",
  "Start w/o exception."  ,
  "Start w/ exception."   
};

/* When using GDBIO on ISS it actually doesn't use GDBIO. So, a printf will good
   to ISS. So, setting libio_mode below is not needed.
 */

void DPRINT(int mode, const char* fmt, ...)
{
  char tmpbuf[200];
  va_list ap;
  va_start(ap, fmt);
  int n = vsprintf(tmpbuf, fmt, ap);
  va_end(ap);
  tmpbuf[n] = 0;
  //int old_mode = libio_mode;
  //libio_mode = mode;
  printf("%s", tmpbuf);
  //libio_mode = old_mode;
}

#define PRINTISS(...)	DPRINT(1, __VA_ARGS__);
#define PRINT(...)	DPRINT(1/*libio_mode*/, __VA_ARGS__);

/*
   Note that the FIFO implemented here is thread safe, and interrupt safe w/o
   any synchronization involved; this is because  tail/head never wrap around.
   However, the program will brake once tail/head wrap around (32-bit values)
 */
template <typename T>
class Buffer 
{
  static const int MAX_SIZE = 100000;
  public:
    Buffer(int size)  { len_ = MAX_SIZE; head_ = tail_ = 0;  }
    ~Buffer(){}

    bool full()  { return ((head_ - tail_) == len_); }
    bool empty() { return (head_ == tail_); }

    T get() {
      if (empty())
        return 0;

      T data = buffer_[tail_ % len_];
      tail_++;
      return data;
    }

    void put(T data) {
      if (full()) {
          PRINTISS("FIFO full %d!\n", full());
      }
      buffer_[head_ % len_] = data;
      head_++;
    }

  private:
    volatile uint32_t head_;  /* first byte of data */
    volatile uint32_t tail_;  /* last byte of data */
    T buffer_[MAX_SIZE];
    uint32_t len_;
};

static Buffer<char> xmonBuffer(1000);

int _flag_interrupt = 0;

extern "C" {
extern void  _xmon_iss_init(int intNum, int portNum, int debug);
extern void  _xmon_iss_out(char);
extern char  _xmon_iss_in_rdy();
extern void  _xmon_iss_flush();
extern char  _xmon_iss_in();
}
uint32_t start, end;

/****** XMON used functions ********/

int _xmon_flush()
{
  _xmon_iss_flush();
  return 0;
}

/* XMON Logs/Traces handler  */
void logger(xmon_log_t type, const char *str)
{
  const char* logType = (type == XMON_ERR) ? "ERROR" :
                        (type == XMON_LOG) ? "LOG"   : "???";
  /* XMON logs can't go to GDB as GDBIO (they're sent as notification packets) */
  PRINTISS("**XMON %s**: %s", (char*)logType, (char*)str);
}

/**************************************************/

/* Interrupt used in this example. 
 * (default or assigned by user) 
 * NOTE: We pull out data from comm device right in the ISR handler because it
 * doesn't matter. What matters is to signal Debug Interrupt from the user code
 */
static void user_comm_device_handler(void* excFrame)
{
  PRINTISS("In XMON USER interrupt handler\n");     //Can't do GDBIO here if invoked from XMON
  while (_xmon_iss_in_rdy()){
     char c = _xmon_iss_in();
     xmonBuffer.put(c);
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
  while (len-- > 0)
    _xmon_iss_out(c[i++]);
}

uint8_t _xmon_in()
{
  while (xmonBuffer.empty()) {
      /* Process any further input if available; interrupt could be masked */
      if (_xmon_iss_in_rdy()) {
          user_comm_device_handler(NULL);
      }
  }
  return xmonBuffer.get();
}

/**************************************************/

void execute_loop(void)
{
  static volatile int dummy = 0;
  for(int i = 0; i< 100000; i++) dummy++;
}

void execute_count(void)
{
  static volatile int count = 0;
  //DPRINT("APP: Count %d\n", count);
  count++;
}

//Use: arg1=1 arg2=2 ..., no space on either side of '='
volatile  int _a=0, _b, _c=2, _d=3,_e=1;

int
main(int argc, char* const argv[])
{
  int err;

  //libio_mode = 1;
  // all input arguments
  int portNum        = 20000;
  int gdbPktSize     = GDB_PKT_SIZE;
  int logApp         = 1;
  int logGDB         = 1;
  int logISS         = 1;
  int deviceIntrNum  = UART_INT;
  int dbgIntNum      = UART_INT+1;
  static int mode    = MODE1;

  string type;
  //string val;  //all values are int
  int val;

  while (argc-- > 1) {
    stringstream ss(argv[argc]);
    if (ss.eof()) break;
    getline(ss, type, '=');
    ss >> val;
    //getline(ss, val, ' '); //all values are int
    {
       if (type == "port")     portNum       = val;
       if (type == "devint")   deviceIntrNum = val;
       if (type == "dbgint")   dbgIntNum   = val;
       if (type == "pktsize")  gdbPktSize  = val;
       if (type == "logapp")   logApp      = val;
       if (type == "logiss")   logISS      = val;
       if (type == "loggdb")   logGDB      = val;
       //if (type == "iomode")  libio_mode  = val;
       if (type == "mode")    mode         = val;
    }
    //PRINTISS ("ARG: '%s=%d'\n", type.c_str(), val);
  }
  
  PRINTISS ("\n\n ** Test details: %s **\n\n", modetxt[mode]);
 
  /* Buffer used by XMON */
  char* xmonBuf = (char*)malloc(gdbPktSize);

  PRINTISS ("\n Register handler for intr %d\n", deviceIntrNum);
  xtos_set_interrupt_handler(deviceIntrNum, user_comm_device_handler, (void*)deviceIntrNum, NULL);
  xtos_interrupt_enable(deviceIntrNum);

  /* Init ISS character device to attach to GDB */
  _xmon_iss_init(deviceIntrNum, portNum, logISS);

  _xmon_enable_dbgint(dbgIntNum);
  _xmon_logger (logApp, logGDB);

  if ((err = _xmon_init(xmonBuf, gdbPktSize, logger))) {
    PRINTISS("XMON couldn't initialize, err:%d\n", err);
    return 0;
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

  while(1)
  {
    execute_loop();
    execute_count();
    static int count = 0;
    //libio_mode = 2;
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
