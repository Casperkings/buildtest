/* xmon.h - XMON definitions
 *
 * $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/xmon/xmon.h#1 $
 *
 * Copyright (c) 2001-2013 Tensilica Inc.
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

#ifndef __H_XMON
#define __H_XMON

#ifndef XCHAL_HAVE_XEA3
# define XCHAL_HAVE_XEA3 0
#endif

/* Default GDB packet size */
#define GDB_PKT_SIZE 4096

/* XMON signals */
#define XMON_SIGINT    2   /*target was interrupted */
#define XMON_SIGILL    4   /*illegal instruction */
#define XMON_SIGTRAP   5   /*general exception */
#define XMON_SIGSEGV   11  /*page faults */

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

#include <stdint.h>

/* Error codes used by XMON for remote protocol replies */
typedef enum _remote_err {
    remote_err_ok,
    remote_err_reg_unknown,
    remote_err_reg_invalid,
    remote_err_reg_read,
    remote_err_reg_write,
    remote_err_addr_invalid,      // invalid address specified in various commands
    remote_err_mem_read,
    remote_err_mem_write,
    remote_err_buf_overflow,
    remote_err_unknown_syscall,
    remote_err_bp_wrong_size,     // invalid BP size in z/Z packet
    remote_err_bp_full,           // All available SW or HW BPs used.
    remote_err_bp_not_found,      // Couldn't find specified breakpoint
    remote_err_unknown,
} remote_err_t;

/* Type of log message from XMON to the application */
typedef enum {
   XMON_LOG,
   XMON_ERR,
} xmon_log_t;

typedef void (*xmon_user_int_handler_t)(void* const);
typedef void (*xmon_logger_t)(xmon_log_t, const char*);
typedef void (*xmon_int_handler_t)( void* arg);
typedef void (*xmon_exc_handler_t)( void* arg);

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * THE FOLLOWING ROUTINES ARE USED BY THE USER
 */

/** 
 * Initialize XMON so GDB can attach.
 * gdbBuf     - pointer to a buffer XMON uses to comm. with GDB
 * gdbPktSize - Size of the allocated buffer for GDB communication.
 * xlog       - log handler for XMON produced errors/logs/traces
 
 */
int
_xmon_init (char* gdbBuf, int gdbBufSize, xmon_logger_t logger);

/** 
 * Detach from XMON. Can execute at any time
 */
void  _xmon_close(void);

/** 
 * Check if XMON is active
 */
int
_xmon_active ();


/** 
 * Print message to GDB 
 */
void  _xmon_consoleString(const char* str);

/** 
 * XMON version
 */
const char* _xmon_version();

/** 
 * Enable disable various logging and tracing chains
 * app_log_en   - enable/disable logging to the app log handler (default is enabled).
 * gdb_log_en   - enable/disable log notifications to the GDB (default is enabled)..
 */
void  _xmon_logger(char app_log_en, char gdb_log_en);

/* 
 * Force entry into XMON (e.g. simulate debug interrupt)
 */
void _xmon_enter();

/*
 *  FOLLOWING ROUTINES NEED TO BE PROVIDED BY USER
 */

/* 
 * Receive remote input char from GDB, buffering input or blocking when unavailable
 * Returns: next character received 
 */
uint8_t _xmon_in(void);

/* 
 * Output an array of chars to GDB 
 * len - number of chars in the array
 */
void _xmon_out(int len, char*);

/* 
 * Flush output characters 
 * XMON invokes this one when a full response is ready
 */
int _xmon_flush(void);  // flush output characters


int _xmon_enable_dbgint(int);

/* OS PROVIDED */
void  xmon_set_exc_handler (int num, xmon_exc_handler_t handler, xmon_exc_handler_t* old);
void  xmon_set_int_handler (int num, xmon_int_handler_t handler);

#ifdef  __cplusplus
}
#endif

#endif //!ASSEMBLER

#endif
