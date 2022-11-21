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

#include <stdlib.h>
#include <xtensa/specreg.h>
#include <xtensa/config/core.h>
#include "xtensa-libdb-macros.h"
#include "xmon.h"
#include "xmon-internal.h"


extern char  _memmap_reset_vector;

/* Version string */
const char *version_string = XMON_STARTUP_STRING;

/* Imported functions, implemented in asm */
extern void   _flush_i_cache( uint8_t *);
extern uint32_t _xmon_get_intenable( void );
extern void   _xmon_set_intenable( uint32_t value );

extern void   xmon_set_spec_reg(uint32_t reg, uint32_t value);
extern void   _xmon_set_user_reg(uint32_t reg, uint32_t value);

extern uint32_t _xmon_get_spec_reg(uint32_t reg);
extern uint32_t _xmon_get_user_reg(uint32_t reg);
extern void   _xmon_exec_here(char* buf);
//void (*_xmon_exec_here)( char* buf ) = NULL;

void putConsoleString(const char* string);

long _ar_registers[XCHAL_NUM_AREGS];
long _sr_registers[256];        /* space for all possible SRs, even though it's probably sparse */

/* XMON enabled - nothing processed until it is  set */
int gXmonEnabled = 0;

/* Already in XMON - used for reentrancy  */
int gInXmon      = 0;

int gGdbLogOn    = 1;
int gAppLogOn    = 1;

static int gXmonStep = 0;
static int gXmonStep_pending = 0;

int _command_failed = 0;
int _try_command    = 0;

breakpoint_t        _swbreak [XMON_NUM_SWBREAK];
breakpoint_t        _hwbreak [XCHAL_NUM_IBREAK];

const char*
_xmon_version()
{
  return version_string;
}


/* WARNING - to properly support reset, init all global vars */
void _xmon_gvars_init(void)
{
  gXmonStep         = 0;
  gXmonStep_pending = 0;
  gXmonEnabled      = 1;
  gInXmon           = 0;
  gGdbLogOn         = 1;
  gAppLogOn         = 1;
  _command_failed    = 0;
  _try_command       = 0;
  
  xmon_memset((uint8_t*)_ar_registers, 0, XCHAL_NUM_AREGS);
  xmon_memset((uint8_t*)_sr_registers, 0, 256);
  xmon_memset((uint8_t*)_swbreak, 0, XMON_NUM_SWBREAK);
  xmon_memset((uint8_t*)_hwbreak, 0, XCHAL_NUM_IBREAK);
}


int
_xmon_init (char* gdbBuf, int gdbBufSize, xmon_logger_t logger)
{
  if(gdbBuf == NULL) {
    XERR("XMON needs GDB buffer\n");
    return -2;
  }

  if(gdbBufSize < 200) {
    XERR("GDB buffer size is < 200\n");
    return -3;
  }

  _xlogger = logger;
  _xmon_gvars_init();
  
  int ret = _xmon_gdbvars_init(gdbBuf, gdbBufSize);
  if (ret) 
    return ret;

  return ret ? -1 : 0;
}

int
_xmon_enable_dbgint (int intNum)
{
  return 0;
}

static void
xmon_cache_flush_region(void *address, int size)
{
  XLOG("Cache flush region 0x%x, len:%d\n", (uint32_t*)address, size);
  xthal_dcache_region_writeback(address, size);  /* write back D$ */
  xthal_icache_sync();                           /* sync the I$ cache to wait for writes */
  xthal_icache_region_invalidate(address, size); /* Invalidate I$ */
}

/* Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 * If MAY_FAULT is non-zero, then we will handle memory faults by returning
 * a 0, else treat a fault like any other fault in the stub.
 */
int
xmon_read_mem(const uint8_t* mem, char* buf, uint32_t count, const int nohex)
{
  uint8_t ch      = 0;
  uint32_t  value   = 0;
  uint32_t  *pValue = NULL;
  int     offset  = 0;
  uint32_t  mask[4];

  XLOG("Read Memory @ 0x%x, len:%d\n", mem, count);

  mask[0] = 0x000000ff;
  mask[1] = 0x0000ff00;
  mask[2] = 0x00ff0000;
  mask[3] = 0xff000000;

  while (count-- > 0)
  {
    pValue = (uint32_t *) (  ((uint32_t)mem) & ~0x3 );
    offset = ((uint32_t)mem) & 0x3;

    if ( xmon_is_bigendian() )
      offset = 3 - offset;

    value = *pValue;
    value = value & mask[offset];
    value = value >> (offset * 8);
    ch = (uint8_t)value;

    mem++;
    if(!nohex) {
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch & 0xf];
    }
    else
      *buf++ = ch;
  }

  *buf = 0;
  return 0;
}


/* Decode a hex string and write it into memory. */
int
xmon_write_mem(const register uint8_t* buf, register uint8_t *mem, int count, const int flush, const int nohex)
{
  uint8_t ch         = 0;
  uint8_t *start     = mem;
  uint32_t  len      = count;
  uint32_t  *pValue  = NULL;
  uint32_t  value    = 0;
  uint32_t  newValue = 0;
  int            offset   = 0;
  int            mask[4];

  XLOG("Write Memory @ 0x%x, len:%u\n", mem, count);

  mask[0] = 0xffffff00;
  mask[1] = 0xffff00ff;
  mask[2] = 0xff00ffff;
  mask[3] = 0x00ffffff;

  while (count-- > 0)
  {
    if(!nohex) {
      ch =  xmon_hex(*buf++) << 4;
      ch |= xmon_hex(*buf++);
    }
    else
      ch = *buf++;

    // Ugh, this needs to work for both endians....
    pValue = (uint32_t *) (  ((uint32_t)mem) & ~0x3 );
    offset = ((uint32_t)mem) & 0x3;

    if ( xmon_is_bigendian() )
      offset = 3 - offset;

    // This is terribly inefficient....
    // But on Xtensa we don't know where the IRAM is, and if we are
    // writing to IRAM we have to use s32i and l32i instructions.

    value = *pValue;
    value = value & mask[offset];
    newValue = (uint32_t)ch;
    newValue = newValue << (offset * 8);
    newValue = newValue | value;

    *pValue = newValue;

    mem += 1;
  }

  if( flush )
    xmon_cache_flush_region(start, len);

  return 0;
}

int
xmon_add_sw_breakpoint (uint32_t addr, int size)
{
  int i;

  XLOG("Add SW BP @ 0x%x\n", addr);

  static uint8_t be_break[6]  =  "000400";
  static uint8_t le_break[6]  = "004000";
  static uint8_t be_breakn[4] = "d20f";
  static uint8_t le_breakn[4] = "2df0";

  uint8_t* bp =  xmon_is_bigendian() ?
               (size == 2) ? be_breakn : be_break:
               (size == 2) ? le_breakn : le_break;

  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if (_swbreak[i].valid == 0) {
      _swbreak[i].valid = 1;
      _swbreak[i].addr = addr;
      _swbreak[i].size = size;
      xmon_read_mem((uint8_t *)addr, _swbreak[i].mem, size, 1);
      xmon_write_mem(bp, (uint8_t *)addr, size, 1, 0);
      return 0;
    }
  }

  XERR("SW BP table full!\n");
  return -1;
}

int
xmon_remove_sw_breakpoint (uint32_t addr, int size)
{
  int i;
  XLOG("Remove SW BP @ 0x%x\n", addr);

  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if (_swbreak[i].addr == addr && _swbreak[i].valid) {
      size = _swbreak[i].size;
      xmon_write_mem((uint8_t *)_swbreak[i].mem, (uint8_t *)addr, size, 1, 1);
      return 0;
    }
  }

  XERR("No BP @ 0x%x!\n", addr);
  return -1;
}

int
xmon_add_hw_breakpoint (uint32_t addr)
{
  int i;

  XLOG("Add HW BP @ 0x%x\n", addr);
  
  for (i = 0; i < XCHAL_NUM_IBREAK; i++) {
    if (_hwbreak[i].valid == 0) {
      _hwbreak[i].valid = 1;
      _hwbreak[i].addr = addr;
      //SR_REG((i == 0 ? IBREAKA_0 : IBREAKA_1)) = addr;
      //SR_REG(IBREAKENABLE) = SR_REG(IBREAKENABLE) | (1 << i);
      xmon_set_spec_reg( (i == 0 ? IBREAKA_0 : IBREAKA_1), addr );
      uint32_t iben = _xmon_get_spec_reg(IBREAKENABLE);
      xmon_set_spec_reg( IBREAKENABLE, iben | (1 << i));
      return 0;
    }
  }
  XERR("HW BP table full!\n");
  return -1;
}

int
xmon_remove_hw_breakpoint (const uint32_t addr)
{
  int i;
  XLOG("Remove HW BP @ 0x%x\n", addr);

  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if (_hwbreak[i].addr == addr && _hwbreak[i].valid) {
      _hwbreak[i].valid = 0;
      //SR_REG(IBREAKENABLE) = SR_REG(IBREAKENABLE) & ~(1 << i);
      uint32_t iben = _xmon_get_spec_reg(IBREAKENABLE);
      xmon_set_spec_reg( IBREAKENABLE, iben & ~(1 << i));
      return 0;
    }
  }
  XERR("No HW BP @ 0x%x!\n", addr);
  return -1;
}

int
xmon_add_hw_watchpoint (uint32_t addr, int type)
{
  return -1;
}

int
xmon_remove_hw_watchpoint (uint32_t addr, int type)
{
  return -1;
}


void 
_xmon_single_step(int mode)
{
  /* Set the icount level to one more than the interrupt level.
     This will allow single-stepping through handlers.
     The intlevel is always < debug level else XMON is not entered.
  */
  int intlevel = SR_REG(DBG_EPS) & PS_INTLEVEL_MASK;
  SR_REG(ICOUNT) = -2;
  SR_REG(ICOUNTLEVEL) = intlevel+1;   /* could be XCHAL_DEBUGLEVEL */
}

#if 0
static void _unplant_sw_breakpoints(void)
{
  int i;

  // Unplant all BPs so they don't get hit by XMON, if in a shared function
  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if(_swbreak[i].valid) {
      xmon_write_mem(_swbreak[i].mem, (uint8_t *)_swbreak[i].addr, _swbreak[i].size, 1, 1);
      _swbreak[i].valid = 0;
    }
  }
}
#endif

void
xmon_C_handler()
{
  int sigval = XMON_SIGTRAP;

  uint8_t pc[8];    // pad storage--xmon_read_mem() adds a null character
  xmon_read_mem((uint8_t *)SR_REG(PC), (char*)pc, 4, 1);

  uint32_t sarg = BREAK_S(pc);
  uint32_t targ = BREAK_T(pc);
  uint32_t is_break  = IS_BREAK(pc);
  uint32_t is_breakn = IS_BREAKN(pc);

   /*** WARNING - no shared funcs and logs until removing all the planted BPs ***/
   /*** FIXME - xmon_read_mem() is above - find a workaround - e.g. read the  ***/
   /*** list of BPs first and see if a BP at @PC is there, then read args from it. ***/
  //_unplant_sw_breakpoints();

  if(gInXmon)
    XERR("XMON re-entered\n");

  /* Handle re-entrancy due to a failing command */
  _command_failed = gInXmon && _try_command;

  gXmonStep    = 0; // assume step won't be needed
  _try_command  = 0;
  gInXmon      = 1;
  
  int syscall = 0;

  // 1. Read Debug Cause to see the cause of break
  // 2. If XMON is not enabled by _xmon_init, we continue...
  //    For some unexpected cases, XMON can actually clear this unexpected event
  //    (e.g. clear IBREAKENABLE on IBREAK, jump over break 1,x....)
  // 2. Decide to stop for GDB connection or not, based on the break condition
  //    (For sw breaks, this requires reading operands and interpreting those)
  // 3. Choose proper sigval based on the break condition
  //    (For break 1,x read the EXCCAUSE to set the proper sigval)
  // 4. Misc: Set PC=EPC1 and PS.EXCM=0 when break 1,1 for proper backtrace)
  //          Clear ICOUNTLEVEL on ICOUNT exception
  // 5. When resuming, need to know if the step over breakpoint is needed
  //    (planted ones, break 0,x, are stepped over by GDB and ICOUNT)

  XLOG("Handle Exception\n");

  if (SR_REG(DEBUGCAUSE) & DC_ICOUNT)
  {
    XLOG("ICOUNT occurred\n");
    sigval = XMON_SIGTRAP;
    SR_REG(ICOUNTLEVEL) = 0;
    if(!gXmonEnabled) {
      //FIXME - unexpected situation: disable ICOUNT INT
      SR_REG(ICOUNT)  = 0;
      XLOG("Unexpected IBREAK\n");
    }
  }
  else if (SR_REG(DEBUGCAUSE) & DC_DEBUGINT)
  {
   XLOG("DBGINT occurred\\n");
    sigval = XMON_SIGINT;
    if(!gXmonEnabled) {
      XLOG("Unexpected Debug Int. ignored\n");
      //FIXME - unexpected situation: ignore entry to XMON. How is some
      //        internal break going to make this core debuggable anyway?
    }
  }
  else if (SR_REG(DEBUGCAUSE) & DC_IBREAK)
  {
    XLOG("IBREAK occurred\n");
    sigval = XMON_SIGINT;
    if(!gXmonEnabled) {
      #if XCHAL_NUM_IBREAK > 0
      XLOG("Unexpected IBREAK\n");
      //FIXME - unexpected situation: try helping by simple clearing IBREAKENABLE
      SR_REG(IBREAKENABLE) = 0;
      #endif
    }
  }
  else if (SR_REG(DEBUGCAUSE) & DC_DBREAK)
  {
    XLOG("DBREAK occurred\n");
    if(!gXmonEnabled) {
    //FIXME - unexpected situation: try helping by simple clearing HW DBREAK
      #if XCHAL_NUM_DBREAK >= 1
      XLOG("Unexpected DBREAK\n");
      SR_REG(DBREAKC_0) = 0;
      #endif
      #if XCHAL_NUM_DBREAK >= 2
      SR_REG(DBREAKC_1) = 0;
      #endif
    }
  }
  else if (SR_REG(DEBUGCAUSE) & DC_BREAK)
  {
    if(!is_break)
	  XERR("BREAK expected @pc:0x%lx, debugcause:%x\n", SR_REG(PC), SR_REG(DEBUGCAUSE));

    sigval = XMON_SIGINT;

    XLOG("BREAK %d,%d occurred\n", sarg, targ);

    if( sarg == 1 && (targ <= 1)) {
      sigval = (SR_REG(EXCCAUSE) == EXCCAUSE_ILLEGAL) ?
                                    XMON_SIGILL  :
               (SR_REG(EXCCAUSE) == EXCCAUSE_SYSCALL)?
                                    XMON_SIGTRAP :
               (SR_REG(EXCCAUSE) == EXCCAUSE_INSTR_ERROR) ?
                                    XMON_SIGSEGV :
               (SR_REG(EXCCAUSE) == EXCCAUSE_LOAD_STORE_ERROR) ?
                                    XMON_SIGSEGV :
               (SR_REG(EXCCAUSE) == EXCCAUSE_LEVEL1_INTERRUPT) ?
                                    XMON_SIGINT  :
                                    XMON_SIGTRAP ;
    }

    // breaks not handled when XMON not initialized. Just step over the break.
    // break 0,x shouldn't happen when GDB is detached. Just print warning.
    if (!gXmonEnabled)
      XLOG("break while XMON not active. Ignoring...\n");

    if(sarg == 1 && targ == 14) {
      XLOG("SYSCALL...\n");
      syscall = 1;
    }

    if (sarg == 1 && targ == 15)
      XLOG("Wait for GDB...\n");

    // only planted BPs (s=0) are stepped over by GDB and ICOUNT
    // Otherwise, we step over a break and continue with the next instr.
   gXmonStep = (sarg != 0) ? 3 : 0;

  }
  else if (SR_REG(DEBUGCAUSE) & DC_BREAKN)
  {
    if(!is_breakn) {
      XERR("BREAK.N expected per DEBUGCAUSE @PC=0x%x\n", SR_REG(PC));
    }

    XLOG("BREAK.N %u occurred\n", BREAKNO(pc));
    
    //only planted BPs are stepped over by GDB and ICOUNT. BREAK.N is planted
    gXmonStep = 0;
  }

  //start receiving GDB commands until resume. Only if xmon initialized
  if(gXmonEnabled) {
    XLOG("XMON under GDB control\n");
    _xmon_gdbserver( sigval, syscall);
  }

  //When resuming, step over the break if needed.
  SR_REG(DBG_EPC) = SR_REG(DBG_EPC) + gXmonStep;

  /* return to user program */
  gInXmon = 0;
  XLOG("XMON returns %s STEP to PC:0x%x\n\n" , gXmonStep ? "w/":"w/o", SR_REG(DBG_EPC));
}

/* FIXME - get_reg/set_reg have similar code. Also, these two are similar */

#if XCHAL_HAVE_WINDOWED
# define _xmon_rd_areg(reg)		AR_REG((XTENSA_DBREGN_A_INDEX(reg) + SR_REG(WINDOWBASE) * 4) % XCHAL_NUM_AREGS);
#else
# define _xmon_rd_areg(reg)		AR_REG(XTENSA_DBREGN_A_INDEX(reg) % XCHAL_NUM_AREGS);
#endif

void 
xmon_wr_areg(int reg, uint32_t value)
{
#if XCHAL_HAVE_WINDOWED
  int  wb = SR_REG(WINDOWBASE);
#else
  int  wb = 0;
#endif
  AR_REG((XTENSA_DBREGN_A_INDEX(reg) + wb * 4) % XCHAL_NUM_AREGS) = value;
}


/* libdb generates fetch/store sequences of up to 12 instructions,
 * or 36 bytes.  Add 3 bytes for return, and round to multiple of 4.
 */
#define XMON_EXEC_INSTR_SIZE    40

extern uint8_t g_retw_instr[3];
extern uint8_t _xmon_exec_instr[XMON_EXEC_INSTR_SIZE];

int
_xmon_execute_tie_instr(char* buf, const char* strexec)
{
  uint8_t instructions[XMON_EXEC_INSTR_SIZE];
  uint8_t *opcode;
  int i, j, len, total, flush;

  XLOG("Exec. TIE ins.\n");

  do {
    flush = total = 0;
    opcode = instructions;
    xmon_memset(instructions, 0, XMON_EXEC_INSTR_SIZE);
    if (opcode == NULL) {
      XERR("TIE opcode NIL\n");
      return -1;
    }
    do {
      len = (xmon_charToHex(strexec[0]) << 4) | xmon_charToHex(strexec[1]);
      if (len >= 32) {
        XERR("TIE instr too wide");
        return -1;
      }
      if (len + total > XMON_EXEC_INSTR_SIZE - 3 - 1) {
        /* Instruction sequence is too long to run in one batch.
         * NOTE: 3 bytes for return instruction, 1 for xmon_read_mem() padding.
         */
        flush = 1;
        break;
      }
  
      /* Encode opcode. */
      for (j = i = 0; i < len; i++, j+=3) {
        opcode[i] = (xmon_charToHex(strexec[3+j]) << 4) | xmon_charToHex(strexec[4+j]);
  
        /* Malformed packet? get out of the loop */
        if (i < len - 1 && strexec[5+j] != ':') {
          XERR("TIE Malformed");
          return -1;
        }
      }
  
      strexec += j+2;
      opcode += len;
      total += len;
    }
    while (*strexec++ == ':');

    xmon_read_mem(g_retw_instr,  (char*)instructions+total, 3, 1);
    xmon_write_mem(instructions, _xmon_exec_instr, total+3, 1, 1);
    _xmon_exec_here(buf);
  } while (flush);

  XLOG("TIE exec done\n");
  return 0;
}

int
_xmon_process_syscall(char* buffer)
{
  int syscallnr;
  unsigned long arg0, arg1, arg2, arg3, arg4, arg5, arg6;

  syscallnr = _xmon_rd_areg(2);
  arg0      = _xmon_rd_areg(6);
  arg1      = _xmon_rd_areg(3);
  arg2      = _xmon_rd_areg(4);
  arg3      = _xmon_rd_areg(5);
  arg4      = _xmon_rd_areg(8);
  arg5      = _xmon_rd_areg(9);
  arg6      = _xmon_rd_areg(10);

  switch (syscallnr) {
    case XMON_SYSCALL_OPEN:
      xmon_sprintf(buffer, "Fopen,%lx/%lx,%lx,%lx\0", arg0, arg3, arg1, arg2);
      break;
    case XMON_SYSCALL_CLOSE:
      xmon_sprintf(buffer, "Fclose,%lx", arg0);
      break;
    case XMON_SYSCALL_READ:
      xmon_sprintf(buffer, "Fread,%lx,%lx,%lx\0", arg0, arg1, arg2);
      break;
    case XMON_SYSCALL_WRITE:
      xmon_sprintf(buffer, "Fwrite,%lx,%lx,%lx\0", arg0, arg1, arg2);
      break;
    case XMON_SYSCALL_LSEEK:
      xmon_sprintf(buffer, "Flseek,%lx,%lx,%lx\0", arg0, arg1, arg2);
      break;
    case XMON_SYSCALL_RENAME:
      xmon_sprintf(buffer, "Frename,%lx/%lx,%lx/%lx\0", arg0, arg3, arg1, arg2);
      break;
    case XMON_SYSCALL_UNLINK:
      xmon_sprintf(buffer, "Funlink,%lx/%lx\0", arg0, arg1);
      break;
    case XMON_SYSCALL_STAT:
      xmon_sprintf(buffer, "Fstat,%lx/%lx,%lx\0", arg0, arg2, arg1);
      break;
    case XMON_SYSCALL_FSTAT:
      xmon_sprintf(buffer, "Ffstat,%lx,%lx\0", arg0, arg1);
      break;
    case XMON_SYSCALL_GETTIMEOFDAY:
      xmon_sprintf(buffer, "Fgettimeofday,%lx,%lx\0", arg0, arg1);
      break;
    case XMON_SYSCALL_ISATTY:
      xmon_sprintf(buffer, "Fisatty,%lx\0", arg0);
      break;
    case XMON_SYSCALL_SYSTEM:
      xmon_sprintf(buffer, "Fsystem,%lx/%lx\0", arg0, arg1);
      break;
    default:
      XERR("Invalid System Call\n");
      goto error;
  }

  return 0;
  /* GDB is handling a system call now */

error:
   XERR("Syscall Failed!\n");
   return -1;
}

/*
 * This function does all command processing for interfacing to gdb.
 */
uint32_t live_register_val;
uint32_t exec_space[4];

uint8_t *
_xmon_get_reg_ptr(const uint32_t libdb_target_number)
{
  uint8_t *regp  = NULL;
  int   offset = 0;

  XLOG( "Get reg 0x%x\n", libdb_target_number);

  if (XTENSA_DBREGN_IS_AR(libdb_target_number))
  {
    XLOG("Get AR reg\n");
    offset = XTENSA_DBREGN_AR_INDEX(libdb_target_number);
    regp = (uint8_t *)&AR_REG(offset);
  }
  else if (XTENSA_DBREGN_IS_A(libdb_target_number))
  {
    XLOG("Get A reg\n");
#if XCHAL_HAVE_WINDOWED
    int  wb = SR_REG(WINDOWBASE);
#else
    int  wb = 0;
#endif
    offset = XTENSA_DBREGN_A_INDEX(libdb_target_number);
    regp = (uint8_t *)&AR_REG((offset + wb * 4) % XCHAL_NUM_AREGS);
    }
  else if (XTENSA_DBREGN_IS_SREG(libdb_target_number))
  {
    offset = XTENSA_DBREGN_SREG_INDEX(libdb_target_number);
    if (/* PC[], EPS[], EXCSAVE[] libdb numbers are in contiguous 16-blocks.
           Access the live regs for levels > debuglevel. */
        (offset >= EPC && offset < EXCSAVE+0x10
        && (offset & 0xf) > XCHAL_DEBUGLEVEL) ||
        offset == CONFIGID0 ||
        offset == CONFIGID1
       )
    {
      XLOG("Read live SREG %d\n", offset);
      /* Access this register live. */
      live_register_val = _xmon_get_spec_reg( offset);
      regp = (uint8_t *)&live_register_val;
    }
    else if (offset == INTENABLE)
    {
      XLOG("Read INTENABLE\n");
      /* Access this register through a special handler. */
      live_register_val = _xmon_get_intenable();
      regp = (uint8_t *)&live_register_val;
    }
    else
    {
       XLOG("Read saved SREG %d\n", offset);
      /* Access the saved user copy of this register. */
      if (offset == PS)     /* translate PS to user PS (saved EPS[debug]) */
          offset = DBG_EPS;
      regp = (uint8_t *)&SR_REG(offset);
    }
  }
  else if (XTENSA_DBREGN_IS_UREG(libdb_target_number))
  {
    XLOG("Read UREG %d\n", libdb_target_number);
    offset = XTENSA_DBREGN_UREG_INDEX(libdb_target_number);
    live_register_val = _xmon_get_user_reg( offset);
    regp = (uint8_t *)&live_register_val;
  }
  else if (XTENSA_DBREGN_IS_PC(libdb_target_number))
  {
      XLOG("Read PC\n");
      offset = PC;      /* EPC[DEBUGLEVEL] */
      regp = (uint8_t *)&SR_REG(offset);
  }
  else
  {
    regp = NULL;
  }
  return regp;
}


uint32_t
_xmon_set_reg_value(uint32_t libdb_target_number, const uint32_t value)
{
  uint8_t *regp;

  XLOG("Write reg 0x%x\n", libdb_target_number);

  if (XTENSA_DBREGN_IS_UREG(libdb_target_number))
  {
    XLOG("Set saved UREG %d\n", libdb_target_number);
    _xmon_set_user_reg( XTENSA_DBREGN_UREG_INDEX(libdb_target_number), value);
    return 0;
  }
  else if (XTENSA_DBREGN_IS_SREG(libdb_target_number))
  {
    int offset = XTENSA_DBREGN_SREG_INDEX(libdb_target_number);
    if (/* PC[], EPS[], EXCSAVE[] libdb numbers are in contiguous 16-blocks.
           Access the live regs for levels > debuglevel. */
        (offset >= EPC && offset < EXCSAVE+0x10
        && (offset & 0xf) > XCHAL_DEBUGLEVEL)
       )
    {
      /* Access this register live. */
      XLOG("Set live SREG %d\n", offset);
      xmon_set_spec_reg( offset, value );
      return 0;
    }
    else if (offset == INTENABLE)
    {
      /* Access this register through a special handler. */
      XLOG("Set INTENABLE OK\n");
      _xmon_set_intenable( value );
    }
    else
    {
      XLOG("Set saved SREG %d\n", offset);
      /* Access the saved user copy of this register. */
      if (offset == PS)     /* translate PS to user PS (saved EPS[debug]) */
           libdb_target_number = XTENSA_DBREGN_SREG(DBG_EPS);
      /* (fall through) */
    }
  }

  /* By the time we get here we've handled all the live regs */
  regp = _xmon_get_reg_ptr(libdb_target_number);
  if (regp != NULL) {
    long *tmp_ptr = (long *)regp;
    *tmp_ptr = value;
    return 0;
  }

  XERR("WR Reg failed");
  return -1;
}

void
_xmon_sw_reset(void)
{
  //Do SW reset, set DEPC to something. When XMON is entered again (on main),
  //we will check DEPC to see if it was re-entered as a part of reset and jump to
  //reset code (or just send OK to GDB and continue XMON normally - wait for new GDB commands)
  //However, this approach doesn't allow for debugging from reset - likely IBREAK is
  //to be used to stop the core early (will the state be sufficent if reset vectore is not executed ?)
  //set break on main ? (cannot put at the reset vector ?)
  SR_REG(PS)           = 0x1;
  SR_REG(INTCLEAR)     = 0xffffffff;
  SR_REG(ICOUNT)       = 0;
  SR_REG(ICOUNTLEVEL)  = 0;
  SR_REG(INTENABLE)    = 0;

#if XCHAL_HAVE_CCOUNT
  SR_REG(CCOUNT)       = 0;
#endif
#if XCHAL_HAVE_WINDOWED
  SR_REG(WINDOWBASE)   = 0;
  SR_REG(WINDOWSTART)  = 1;
#endif
#if XCHAL_HAVE_CP
  SR_REG(CPENABLE)     = 0xffffffff;
#endif
#if XCHAL_HAVE_S32C1I
  SR_REG(ATOMCTL) = 0x28;
#endif

#if XCHAL_HAVE_CACHEATTR
  SR_REG(CACHEATTR)    = 0x22222222;
#endif
#if XCHAL_HAVE_PREFETCH
  SR_REG(PREFCTL) = 0;
#endif
#if XCHAL_DCACHE_IS_COHERENT || XCHAL_LOOP_BUFFER_SIZE
  SR_REG(MEMCTL) = 0;
#endif
#if XCHAL_HAVE_MEM_ECC_PARITY
  SR_REG(MECR) = 0;
#endif

#if XCHAL_NUM_DBREAK >= 1
  SR_REG(DBREAKC_0)     = 0;
#endif
#if XCHAL_NUM_DBREAK >= 2
  SR_REG(DBREAKC_1)     = 0;
#endif
#if XCHAL_NUM_IBREAK > 0
  SR_REG(IBREAKENABLE) = 0;
#endif
#if XCHAL_HAVE_LOOPS
  SR_REG(LCOUNT)       = 0;
#endif
#if XCHAL_HAVE_ABSOLUTE_LITERALS
  SR_REG(LITBASE) = 0;
#endif
#if XCHAL_ITLB_ARF_WAYS > 0 || XCHAL_DTLB_ARF_WAYS > 0
  SR_REG(PTEVADDR) = 0;
#endif
#if XCHAL_HAVE_VECBASE
  SR_REG(VECBASE) = XCHAL_VECBASE_RESET_VADDR;
#endif

  //SR_REG(LCOUNTLEVEL)  = 0;
  //SR_REG(FCR)          = 0;
  //SR_REG(FSR)          = 0;
  //SR_REG(ITLBCFG)      = 0;
  //SR_REG(DTLBCFG)      = 0;

  SR_REG(PC) = (uint32_t)&_memmap_reset_vector;
  gXmonStep = 0;
}

void 
_xmon_kill()
{
  gXmonStep_pending = 0;
}

void 
_xmon_continue(const uint32_t* addr)
{
  if (addr) {
    SR_REG(PC) = *addr;
    //FIXME - do write srreg
  }
  gXmonStep_pending = 0;
}
