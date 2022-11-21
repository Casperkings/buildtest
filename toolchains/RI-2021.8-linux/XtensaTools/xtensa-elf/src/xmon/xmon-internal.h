/* xmon-internal.h - XMON internal definitions
 *
 * $Id: //depot/dev/Eaglenest/Xtensa/OS/xmon/xmon_internal.h#1 $
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

#ifndef __H_XMON_INTERNAL
#define __H_XMON_INTERNAL

#include "xmon.h"

/*  String that gets displayed at startup (by BSP-specific _xmon_init()):  */
#define XMON_STARTUP_STRING     "XMON v." XTHAL_RELEASE_NAME

/* E00 is reserved for internal errors! */
#define GDB_REPLY_ERROR_OTHER			"E01"
#define GDB_REPLY_ERROR_MALFORMED		"E02"
#define GDB_REPLY_ERROR_MEMORY			"E03"
#define GDB_REPLY_ERROR_DISCONNECTED		"E04"
#define GDB_REPLY_ERROR_ARGUMENT		"E05"
#define GDB_REPLY_ERROR_REGISTER		"E06"
#define GDB_REPLY_ERROR_INTERNAL		"E07"

/* For XEA2 */
#define DC_ICOUNT	0x01
#define DC_IBREAK	0x02
#define DC_DBREAK	0x04
#define DC_BREAK	0x08
#define DC_BREAKN	0x10
#define DC_DEBUGINT	0x20

/* stack pointer within
 a1 contains the stack pointer from amongst a0-a15, hence the register number is set to 1 */
#define SP_REGNUM       1

#define AREGS_MASK      (XCHAL_NUM_AREGS-1)
#define WB_MASK         (AREGS_MASK >> 2)
#define WS_MASK         (~((~0)<<(XCHAL_NUM_AREGS/4)))

#if XCHAL_HAVE_LE

#define IS_BREAKN(p) ((p)[0]==0x2d && ((p)[1]&0xf0)==0xf0)
#define IS_BREAK(p)  ((p)[2]==0x00 && ((p)[0]&0x0f)==0x00 && ((p)[1]&0xf0)==0x40)
#define IS_BREAK1(p) ((p)[0]==0x2d && ((p)[1]&0xf0)==0xf0)
#define BREAKNO(p)   (IS_BREAKN(p) ? ((p)[1]&0x0f) : -1 )
#define BREAK_S(p)   ((p)[1]&0x0f)
#define BREAK_T(p)   (((p)[0]&0xf0)>>4)

#else

#define IS_BREAKN(p) ((p)[0]==0xd2 && ((p)[1]&0x0f)==0x0f)
#define IS_BREAK(p)  ((p)[2]==0x00 && ((p)[0]&0xf0)==0x00 && ((p)[1]&0xf)==0x04)
#define IS_BREAK1(p) ((p)[0]==0x2d && ((p)[1]&0xf0)==0xf0)
#define BREAKNO(p)   (IS_BREAKN(p) ? (((p)[1]&0xf0)>>4) : -1 )
#define BREAK_S(p)   (((p)[1]&0xf0)>>4)
#define BREAK_T(p)   ((p)[0]&0x0f)

#endif

#define SR_REG(n)       (_sr_registers[n])
#define AR_REG(n)       (_ar_registers[n])

/* Registers of program under debug */
#define PC      (EPC+XCHAL_DEBUGLEVEL)
#define DBG_EPC (EPC+XCHAL_DEBUGLEVEL)
#define DBG_EPS (EPS+XCHAL_DEBUGLEVEL)

//#define SR_IBREAKA(i)    SR_IBREAKA##i

/* Macros to extract fields of PS */
#define GET_PSINTLVL(ps)  ((ps)&0xf)
#define GET_PSUSRMODE(ps) (((ps)>>5)&0x1)
#define GET_PSOWB(ps)     (((ps)>>8)&0xf)
#define GET_PSCALLINC(ps) (((ps)>>16)&0x3)
#define GET_PSWOE(ps)     (((ps)>>18)&0x1)

#define NOT_GDB_PKT         0
#define GDB_PKT_PROCESS     1
#define GDB_PKT             2

#define XMON_SYSCALL_OPEN		-2
#define XMON_SYSCALL_CLOSE		-3
#define XMON_SYSCALL_READ		-4
#define XMON_SYSCALL_WRITE		-5
#define XMON_SYSCALL_LSEEK		-6
#define XMON_SYSCALL_RENAME		-7
#define XMON_SYSCALL_UNLINK		-8
#define XMON_SYSCALL_STAT		-9
#define XMON_SYSCALL_FSTAT		-10
#define XMON_SYSCALL_GETTIMEOFDAY	-11
#define XMON_SYSCALL_ISATTY		-12
#define XMON_SYSCALL_SYSTEM		-13

#define XMON_NUM_SWBREAK 20

#define MAX_TIE_LEN 512 //bytes - 4096-bit wide register file

#define EXCCAUSE_FULLTYPE(a)   ((a >> EXCCAUSE_FULLTYPE_SHIFT) & EXCCAUSE_FULLTYPE_MASK)

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)

/* Version string */
extern const char *version_string;

/* forward definitions */
extern uint32_t _old_a0;
extern uint32_t  _old_a1;

extern int     gXmonEnabled;
extern int     gInXmon;
extern int     gGdbLogOn;
extern int     gAppLogOn;
extern int     gXmonSyscall;
extern int     _command_failed;
extern int     _try_command;

extern volatile void* gCatchErr_address;

extern xmon_user_int_handler_t xmon_user_intr_handler;
extern xmon_logger_t _xlogger;

/* forward utils definitions */
void xmon_memset(uint8_t *ptr, uint8_t value, int num);
int  xmon_is_bigendian();
int  xmon_sprintf(char * buf, const char * fmt, ...);
int  xmon_charToHex (unsigned char c);
int  xmon_hex(const uint8_t ch);
int  xmon_strlen( const char *cp );
int  xmon_strncmp(const char* s1, const char* s2, int n);
void _xmon_log(int type, const char* fmt, ...);
int  xmon_hexToInt(const char **ptr, unsigned int *intValue);
char *_xmon_strchr(const char *s, int c);
void _XMON_XGDB(const char* fmt, ...);

#ifdef XMON_DEBUG
#define XLOG(...) do {                                 \
	           _xmon_log(XMON_LOG, __VA_ARGS__);   \
                  } while (0);
#define XERR(...) do {                                 \
	           _xmon_log(XMON_ERR, __VA_ARGS__);   \
                  } while (0);
                 
#else		  
#define XLOG(...) do {} while (0);
#define XERR(...) do {} while (0);
#endif

/* forward xmon definitions */
int      xmon_add_sw_breakpoint    (uint32_t addr, int size);
int      xmon_remove_sw_breakpoint (const uint32_t addr, const int size);
int      xmon_add_hw_breakpoint    (uint32_t addr);
int      xmon_remove_hw_breakpoint (uint32_t addr);
int      xmon_add_hw_watchpoint    (uint32_t addr, int type);
int      xmon_remove_hw_watchpoint (uint32_t addr, int type);

void     _xmon_single_step(int type);
void     _xmon_continue(const uint32_t* addr);
void     _xmon_kill();
int      _xmon_process_syscall(char* buffer);
uint32_t _xmon_set_reg_value(uint32_t libdb_target_number, const uint32_t value);
void     _xmon_sw_reset(void);
int      xmon_read_mem(const uint8_t* mem, char* buf, uint32_t count, const int nohex);
int      xmon_write_mem(const register uint8_t* buf, register uint8_t* mem, int count, const int flush, const int nohex);
int      _xmon_execute_tie_instr(char* buf, const char* strexec);
uint8_t* _xmon_get_reg_ptr(const uint32_t libdb_target_number);
void     xmon_wr_areg(int reg, uint32_t value);
inline uint32_t  _xmon_rd_areg(int reg);
int      _xmon_gdbvars_init(char* gdbBuf, int gdbBufSize);


int _xmon_get_reg(uint32_t* regptr, const uint32_t libdb_target_number);
void _xmon_convert_var_to_hex(uint32_t value, char* buf);

void  xmon_interrupt_handler(void* arg);
void  xmon_exception_handler(void* arg);

extern void _DebugExceptionHandler(void*);
extern void _DebugInterruptHandler(void*);

void _xmon_gdbserver ( int sigval, int syscall );
void _xmon_putConsoleString(const char *s);

void _xmon_longjmp(void);

typedef enum pkt_fsm_type {
    GDB_PKT_NOT_STARTED = 0,
    GDB_PKT_PENDING,
    GDB_PKT_CHKSUM,
    GDB_PKT_RECEIVED
} pkt_fsm_t;

typedef struct gdbpkt_state_struct {
  uint8_t checksum;
  uint8_t xmitcsum;
  pkt_fsm_t state;
  uint32_t count;
} GdbPacket;

typedef struct {
  int     valid;
  uint32_t addr;
  uint32_t size;
  char mem[8];
} breakpoint_t;

static const char hexchars[] = "0123456789abcdef";

extern char gCatchErr;

#ifdef  __cplusplus
extern "C" {
#endif
uint32_t _xmon_get_spec_reg(uint32_t reg);
uint32_t _xmon_get_user_reg(uint32_t reg);
void   _xmon_exec_here(char* buf);
uint32_t   _xmon_reentry_setup();
void   xmon_set_spec_reg(uint32_t reg, uint32_t value);
void   _xmon_set_user_reg(uint32_t reg, uint32_t value);
#ifdef  __cplusplus
}
#endif

#endif //!ASSEMBLER

#endif
