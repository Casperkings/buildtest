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

#include <xtensa/config/specreg.h>
#include "xtensa-libdb-macros.h"
#include <xtensa/xtruntime-frames.h>
//#include <xtensa/tie/xt_regwin.h>
#include "xmon.h"
#include "xmon-internal.h"
#include <stdio.h>
#include <xtensa/xtruntime.h>

#ifndef XCHAL_NUM_IBREAK
# define XCHAL_NUM_IBREAK 0
#endif

#ifndef XCHAL_NUM_DBREAK
# define XCHAL_NUM_DBREAK 0
#endif

/* Version string */
const char *version_string = XMON_STARTUP_STRING;

extern char  _memmap_reset_vector;

/* Imported functions, implemented in asm */
extern void   _flush_i_cache( uint8_t *);

//void (*_xmon_exec_here)( char* buf ) = NULL;

void _xmon_putConsoleString(const char* string);

static ExcFrame*   gExcFrame = NULL;
#if XCHAL_HAVE_WINDOWED
static int         _excWBP = 0;
#endif

uint32_t           _ar_registers[XCHAL_NUM_AREGS];
uint32_t           _sr_registers[256];

/* XMON enabled - used only to clear IBREAK/DBREAK and try to resume */
int gXmonEnabled = 0;

/* Already in XMON - used for re-entrancy handling */
int gInXmon = 0;

int gGdbLogOn   = 1;
int gAppLogOn   = 1;

static int gXmonStep = 0;
static int gXmonStep_pending = 0;

breakpoint_t        _swbreak[XMON_NUM_SWBREAK];
breakpoint_t        _ibreak[XCHAL_NUM_IBREAK];
breakpoint_t        _dbreak[XCHAL_NUM_DBREAK];

static inline void     xmon_wr_areg_index(int, uint32_t);
static inline uint32_t xmon_rd_areg_index(int);


/* WARNING - to properly support reset, init all global vars */
void _xmon_gvars_init(void)
{
  gXmonStep         = 0;
  gXmonStep_pending = 0;

  gXmonEnabled    = 1;
  gInXmon         = 0;

  gGdbLogOn      = 1;
  gAppLogOn      = 1;

  gExcFrame = NULL;

  xmon_memset((uint8_t*)_ar_registers, 0, XCHAL_NUM_AREGS);
  xmon_memset((uint8_t*)_sr_registers, 0, 256);
  xmon_memset((uint8_t*)_swbreak, 0, XMON_NUM_SWBREAK);
  xmon_memset((uint8_t*)_ibreak, 0, XCHAL_NUM_IBREAK);
  xmon_memset((uint8_t*)_dbreak, 0, XCHAL_NUM_DBREAK);
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

  xmon_set_exc_handler(EXCCAUSE_DEBUG, _DebugExceptionHandler, NULL);
  return ret ? -1 : 0;
}

int
_xmon_enable_dbgint (int intNum)
{
  xmon_set_int_handler(intNum, _DebugInterruptHandler);
  return 0;
}

const char*
_xmon_version()
{
  return version_string;
}

static void
xmon_cache_flush_region(void *address, int size)
{
  XLOG("Cache flush region 0x%x, len:%d\n", (uint32_t*)address, size);
  xthal_dcache_region_writeback(address, size);  /* write back D$ */
  xthal_icache_sync();                           /* sync the I$ cache to wait for writes */
  xthal_icache_region_invalidate(address, size); /* Invalidate I$ */
}

/* For CALL0, all AREGs are in excFrame.
   For Windowed ABI, ARs that overlap A0-A7 are in excFrame

   For CALL0, AR REGs are the same as AREGs (and are in excFrame)
   For Windowed ABI, ARs that overlap A0-A7 are in excFrame.

   FIXME: All these caluclations take time, and are done on each reg access
   Consider moving all regs from excFrame into _ar_registers and restoring on resume
 */
static inline uint32_t *
xmon_areg_ptr_index(const uint32_t index)
{
  XLOG("Get areg pointer (%d)\n", index);

  if (index > 16)
    return NULL;

#ifdef __XTENSA_CALL0_ABI__
  return &((uint32_t*)&gExcFrame->a0)[index];
#else
  return (index > 7) ? & ((uint32_t*)&gExcFrame->a8)[index - 8] :
                       & AR_REG(index + _excWBP);
#endif
}

static inline uint32_t*
xmon_ar_reg_ptr_index(const int index)
{
#ifdef __XTENSA_CALL0_ABI__
  return xmon_areg_ptr_index(index);
#else
  if ((index >= _excWBP) && (index < (_excWBP + 8))) {
    return xmon_areg_ptr_index(index%8);
  }
  return &AR_REG(index % XCHAL_NUM_AREGS);
#endif
}

static inline void
xmon_wr_areg_index(int index, uint32_t value)
{
  uint32_t* regp = xmon_areg_ptr_index(index);
  regp[0] = value;
}

static inline uint32_t
xmon_rd_areg_index(int index)
{
  uint32_t* regp = xmon_areg_ptr_index(index);
  return regp[0];
}

void
xmon_wr_areg(int reg, uint32_t value)
{
  xmon_wr_areg_index(XTENSA_DBREGN_A_INDEX(reg), value);
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
  uint8_t   ch      = 0;
  uint32_t  value   = 0;
  uint32_t  *pValue = NULL;
  int       offset  = 0;
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
xmon_write_mem(const register uint8_t* buf, register uint8_t* mem, int count, const int flush, const int nohex)
{
  uint8_t   ch       = 0;
  uint8_t*  start    = mem;
  uint32_t  len      = count;
  uint32_t* pValue   = NULL;
  uint32_t  value    = 0;
  uint32_t  newValue = 0;
  int       offset   = 0;
  int       mask[4];

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


/* NOTE: Support for BREAK1 only */
int
xmon_add_sw_breakpoint (uint32_t addr, const int size)
{
  int i;

  XLOG("Add SW BP @ 0x%x\n", addr);

  static uint8_t be_break[3]  = "40";
  static uint8_t le_break[3]  = "04";

  if (size != 1) {
    gCatchErr = remote_err_bp_wrong_size;
    return -1;
  }

  uint8_t* bp =  xmon_is_bigendian() ? be_break: le_break;

  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if (_swbreak[i].valid == 0) {
      _swbreak[i].valid = 1;
      _swbreak[i].addr = addr;
      //_swbreak[i].size = size;
      xmon_read_mem((uint8_t *)addr, _swbreak[i].mem, 1/*1-byte BP*/, 1);
      xmon_write_mem(bp, (uint8_t *)addr, 1/*1-byte BP*/, 1, 0);
      return 0;
    }
  }
  gCatchErr = remote_err_bp_full;
  XERR("SW BP table full!\n");
  return -1;
}

int
xmon_remove_sw_breakpoint (const uint32_t addr, const int size)
{
  int i;
  XLOG("Remove SW BP @ 0x%x\n", addr);
  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if (_swbreak[i].addr == addr && _swbreak[i].valid /*&& size == _swbreak[i].size*/) {
      xmon_write_mem((uint8_t *)_swbreak[i].mem, (uint8_t *)addr, size, 1, 1);
      return 0;
    }
  }
  XERR("No BP @ 0x%x!\n", addr);
  gCatchErr = remote_err_bp_not_found;
  return -1;
}

int
xmon_add_hw_breakpoint (const uint32_t addr)
{
#if XCHAL_NUM_IBREAK > 0
  int i;
  XLOG("Add IBREAK BP @ 0x%x\n", addr);
  for (i = 0; i < XCHAL_NUM_IBREAK; i++) {
    if (_ibreak[i].valid == 0) {
      _ibreak[i].valid = 1;
      _ibreak[i].addr = addr;
      xmon_set_spec_reg(IBREAKA_0 + i, addr);
      xmon_set_spec_reg(IBREAKC_0 + i, (1 << 31) ); //FB, Monitor set
      return 0;
    }
  }
  XERR("IBREAK table full!\n");
  gCatchErr = remote_err_bp_full;
#endif
  return -1;
}

int
xmon_remove_hw_breakpoint (const uint32_t addr)
{
#if XCHAL_NUM_IBREAK > 0
  //FIXME - need to check if monitor bit is set, not to remove OCD BP
  int i;
  XLOG("Remove IBREAK BP @ 0x%x\n", addr);

  for (i = 0; i < XCHAL_NUM_IBREAK; i++) {
    if (_ibreak[i].addr == addr && _ibreak[i].valid) {
      _ibreak[i].valid = 0;
      xmon_set_spec_reg(IBREAKC_0 + i, 0);
      return 0;
    }
  }
  XERR("No IBREAK BP @ 0x%x!\n", addr);
  gCatchErr = remote_err_bp_not_found;
#endif
  return -1;
}

int
xmon_add_hw_watchpoint (const uint32_t addr, const int type)
{
#if XCHAL_NUM_DBREAK > 0
  int i;
  XLOG("Add DBREAK BP @ 0x%x\n", addr);

  for (i = 0; i < XCHAL_NUM_DBREAK; i++) {
    if (_dbreak[i].valid == 0) {
      _dbreak[i].valid = 1;
      _dbreak[i].addr = addr;
      xmon_set_spec_reg(DBREAKA_0 + i, addr);
      uint32_t storeOrLoad = (type == 2) ? (1 << 31)/*SBE*/ : (1 << 30)/*LBE*/;
      xmon_set_spec_reg(DBREAKC_0 + i, storeOrLoad | (1 << 27)/*Monitor set*/);
      return 0;
    }
  }
  XERR("IBREAK table full!\n");
  gCatchErr = remote_err_bp_full;
#endif
  return -1;
}

int
xmon_remove_hw_watchpoint (const uint32_t addr, const int type)
{
  //FIXME - need to check if monitor bit is set, not to remove OCD BP
#if XCHAL_NUM_DBREAK > 0
  int i;
  XLOG("Remove DBREAK BP @ 0x%x\n", addr);

  for (i = 0; i < XCHAL_NUM_DBREAK; i++) {
    if (_dbreak[i].addr == addr && _dbreak[i].valid) {
      _dbreak[i].valid = 0;
      xmon_set_spec_reg(DBREAKC + i, 0);
      return 0;
    }
  }
  XERR("No DBREAK BP @ 0x%x!\n", addr);
  gCatchErr = remote_err_bp_not_found;
#endif
  return -1;
}

static inline void 
xmon_clear_all_dbreaks(void)
{
#if XCHAL_NUM_DBREAK > 0
  //FIXME - cannot clear if set by OCD - clear only if Monitor bit is set !
  int i;
  for (i = 0; i < XCHAL_NUM_DBREAK; i++)
    xmon_set_spec_reg(DBREAKC_0 + i, 0);
#endif
}

static inline void 
xmon_clear_all_ibreaks(void)
{
#if XCHAL_NUM_IBREAK > 0
  //FIXME - cannot clear if set by OCD - clear only if Monitor bit is set !
  int i;
  for (i = 0; i < XCHAL_NUM_IBREAK; i++)
    xmon_set_spec_reg(IBREAKC_0 + i, 0);
#endif
}

void
_xmon_single_step(const int type)
{
  gXmonStep_pending = 1;
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
    //SR_REG(PC) = addr;
    //FIXME - do write srreg
  }
   gXmonStep_pending = 0;
}

static void xmon_set_restore_exception_handlers(const int entry)
{
  static xmon_exc_handler_t saved_handler[8];
  xmon_set_exc_handler(EXCCAUSE_INSTRUCTION,  entry ? _DebugExceptionHandler : saved_handler[0] , &saved_handler[0]);
  xmon_set_exc_handler(EXCCAUSE_INSTRUCTION,  entry ? _DebugExceptionHandler : saved_handler[1] , &saved_handler[1]);
  xmon_set_exc_handler(EXCCAUSE_EXTERNAL,     entry ? _DebugExceptionHandler : saved_handler[2] , &saved_handler[2]);
  xmon_set_exc_handler(EXCCAUSE_SYSCALL,      entry ? _DebugExceptionHandler : saved_handler[3] , &saved_handler[3]);
  xmon_set_exc_handler(EXCCAUSE_MEMORY,       entry ? _DebugExceptionHandler : saved_handler[4] , &saved_handler[4]);
  xmon_set_exc_handler(EXCCAUSE_HARDWARE,     entry ? _DebugExceptionHandler : saved_handler[5] , &saved_handler[5]);
  xmon_set_exc_handler(EXCCAUSE_CP_DISABLED,  entry ? _DebugExceptionHandler : saved_handler[6] , &saved_handler[6]);
}

#if 0
static void _unplant_sw_breakpoints(void)
{
  int i;

  // Unplant all BPs so they don't get hit by XMON, if in a shared function
  for (i = 0; i < XMON_NUM_SWBREAK; i++) {
    if(_swbreak[i].valid) {
      xmon_write_mem(_swbreak[i].mem, (uint8_t *)_swbreak[i].addr, 1/*BREAK1 only*/, 1, 1);
      _swbreak[i].valid = 0;
    }
  }
}
#endif

//FIXME - check XTOS_DISABLE_INT_ON_EXC use !!
//FIXME - check XTOS_EXC_AFTER_INTR use !!


/* This is the main C handler for both interrupts and exceptions, executed
   even if XMON is reentered. For faster processing, we could split interrupts/
   exceptions in assembler handlers (but note that interrupt handler still
   needs to save ARs, like exception handler, thus they share the code).
   Also, since on XMON-reentry on interrupts we just call the user handler
   and return, we could do that in assembler so it would be faster.
   All done in C as it's more convenient
 */

int
xmon_C_handler(void* const excFrameP, const int interrupt)
{
  /* If XMON is reentered due to debug interrupt simply resume */
  if (interrupt && gInXmon)
    return gInXmon;

  /* If XMON is reentered due to a GDB caused exception we jump to a special code
     which will just execute longjmp to a previously set jmp_buf
   *  Also - what info is sent to GDB - state before original exception or the new one ?
   */
  if (gInXmon) {
     XLOG("\n\n\nReentered! (EXCCAUSE:%x, EXCVADDR:%x\n", ((ExcFrame*)excFrameP)->exccause, ((ExcFrame*)excFrameP)->excvaddr);
     ((ExcFrame*)excFrameP)->pc = (uint32_t)_xmon_longjmp;
     return gInXmon;
  }

  /***************************************************/
  /*  CODE BELOW IS REACHED ONLY ON 1st XMON ENTRY   */
  /***************************************************/

  gInXmon   = 1;
  gXmonStep = 0; // assume step won't be needed

  int syscall = 0;

  /* Default sigval to GDB */
  int sigval = XMON_SIGTRAP;

  /* Record XMON 1st entry exception frame pointer. */
  // FIXME - not needed if reentered XMON will exit
  gExcFrame = (ExcFrame*)excFrameP;

  /* Install XMON as the handler for all exceptions */
  // FIXME - better overwrite the exc_table pointer, but OS needs to allow that
  xmon_set_restore_exception_handlers( 1/*entry*/ );

  // Calculate WB.P of the interrupted function so we can easily access registers

#if XCHAL_HAVE_WINDOWED
  _excWBP = (((SR_REG(WB) & 0xF) - 1) * 8) % XCHAL_NUM_AREGS;
#endif

#if 0
  int i;
  printf ("\nAREGS:\n");  for (i=0;i<16;i++)              printf ("  A[%d]: %x\n", i, xmon_rd_areg_index(i));
  printf ("\nARREGS:\n"); for (i=0;i<XCHAL_NUM_AREGS;i++) printf ("  AR[%d]: %x\n", i, xmon_rd_ar_reg_index(i));
  //printf ("\nAR_REGS:\n"); for (i=0;i<XCHAL_NUM_AREGS;i++) printf ("  AR_REG[%d]: %x\n", i, AR_REG(i));

  //printf ("\nEXCFRAME:\n");
  //printf ("  a8 : %x\n",  gExcFrame->a8 );
  //printf ("  a9 : %x\n",  gExcFrame->a9 );
  //printf ("  a10: %x\n",  gExcFrame->a10);
  //printf ("  a11: %x\n",  gExcFrame->a11);
  //printf ("  a12: %x\n",  gExcFrame->a12);
  //printf ("  a13: %x\n",  gExcFrame->a13);
  //printf ("  a14: %x\n",  gExcFrame->a14);
  //printf ("  a15: %x\n",  gExcFrame->a15);

#endif

  /* First thing to do is to read instruction @ exception/interrupt PC */
  uint8_t pcinstr[8];
  xmon_read_mem((uint8_t*)gExcFrame->pc, (char*)pcinstr, 4, 1);

  uint32_t sarg = BREAK_S(pcinstr);
  uint32_t targ = BREAK_T(pcinstr);

   /*** WARNING - no shared funcs and logs until removing all the planted BPs ***/
   /*** FIXME - xmon_read_mem() is above - find a workaround - e.g. read the  ***/
   /*** list of BPs first and see if a BP at @PC is there, then read args from it. ***/
  //_unplant_sw_breakpoints();

  /* Process the exception/interrupt:
   * - Get the Exception cause.
   * - If XMON is not enabled by _xmon_init, we continue...
   *   For some unexpected cases, XMON can actually clear this unexpected event
   *   (e.g. clear IBREAKENABLE on IBREAK, jump over break 1,x....)
   * - Decide to stop for GDB connection or not, based on the break condition
   *   (For sw breaks, this requires reading operands and interpreting those)
   * - Choose proper sigval based on the break condition
   * - (For break 1,x read the EXCCAUSE to set the proper sigval)
   * - Misc: Set PC=EPC1 and PS.EXCM=0 when break 1,1 for proper backtrace) FIXME !!!
   * - When resuming, need to know if the step over breakpoint is needed
   *   (planted ones, break 0,x, are stepped over by GDB and ICOUNT)
   */

  if (interrupt) {
     XLOG("Handle Debug Interrupt\n");
     sigval = XMON_SIGINT;
     goto got_interrupt;
  }

  switch (EXCCAUSE_FULLTYPE(gExcFrame->exccause))
  {
    case (EXC_TYPE_SINGLE_STEP) :
       XLOG("SingleStep occurred\n");
       sigval = XMON_SIGTRAP;
       // FIXME - clear PS. SS ??
       // FIXME - what about 0x004 - "Generic" SS ?
       break;

    case (EXC_TYPE_IBREAK) :
       XLOG("IBREAK occurred\n");
       sigval = XMON_SIGINT;
       if(!gXmonEnabled) {
          XERR("Unexpected IBREAK\n");
          xmon_clear_all_ibreaks();
       }
      break;

    case (EXC_TYPE_DBREAK) :
       XLOG("DBREAK occurred\n");
       sigval = XMON_SIGINT;
       if(!gXmonEnabled) {
          XLOG("Unexpected DBREAK\n");
          xmon_clear_all_dbreaks();
       }
       break;

    case (EXC_TYPE_BREAKN) :
       if(!IS_BREAKN(pcinstr)) {
         XERR("BREAK.N expected per EXCCAUSE (%x) @ PC=0x%x\n", gExcFrame->exccause, gExcFrame->pc);
       }
       XLOG("BREAK.N %u occurred\n", BREAKNO(pcinstr));
       //BREAK.N is a planted ?? FIXME ?
       gXmonStep = 0;
       break;

    case (EXC_TYPE_BREAK1) :
       if(!IS_BREAK1(pcinstr)) {
         XERR("BREAK1 expected per EXCCAUSE (%x) @ PC=0x%x\n", gExcFrame->exccause, gExcFrame->pc);
       }
       sigval = XMON_SIGINT;
       // BREAK1 is planted (FIXME - what if it's for OCD but OCD not enabled - we can't remove BP if OCD has instr. bytes)
       gXmonStep = 1;
       break;

    case (EXC_TYPE_BREAK) :
       if(!IS_BREAK(pcinstr)) {
         XERR("BREAK expected per EXCCAUSE (%x) @ PC=0x%x\n", gExcFrame->exccause, gExcFrame->pc);
       }

       sigval = XMON_SIGINT;
       XLOG("BREAK %d,%d occurred\n", sarg, targ);

       if(sarg == 1 && targ == 14) {
         syscall = 1;
       }

       // only planted BPs (s=0) are stepped over by GDB and ICOUNT
       // Otherwise, we step over a break and continue with the next instr.
       gXmonStep = (sarg != 0) ? 3 : 0;
       break;

    default:
        XERR("Unhandled exception EXCCAUSE (%x) @ PC=0x%x\n", gExcFrame->exccause, gExcFrame->pc);
	sigval = XMON_SIGSEGV;
	break;
  }

got_interrupt:
  _xmon_gdbserver( sigval, syscall );

  /* Restore original exception handlers */
  xmon_set_restore_exception_handlers(0/*exit*/);

  /* When resuming, step over the break if needed */
  gExcFrame->pc += gXmonStep;

  /* return to user program */
  XLOG("XMON returns %s STEP to PC:0x%x\n\n" , gXmonStep ? "w/":"w/o", gExcFrame->pc);
  gInXmon = 0; //FIXME - clear in assmebler
  return gInXmon;
}

int
_xmon_process_syscall(char* buffer)
{
  int syscallnr;
  unsigned long arg0, arg1, arg2, arg3, arg4, arg5, arg6;

  syscallnr = xmon_rd_areg_index(2);
  arg0      = xmon_rd_areg_index(6);
  arg1      = xmon_rd_areg_index(3);
  arg2      = xmon_rd_areg_index(4);
  arg3      = xmon_rd_areg_index(5);
  arg4      = xmon_rd_areg_index(8);
  arg5      = xmon_rd_areg_index(9);
  arg6      = xmon_rd_areg_index(10);

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

  gXmonStep_pending = 0;
  return 0;
  /* GDB is handling a system call now */

error:
   XERR("Syscall Failed!\n");
   return -1;
}

/*
 * This function does all command processing for interfacing to gdb.
 */
uint32_t register_val;
uint32_t exec_space[4];

/*
 * xmon_get_reg_ptr does all command processing for interfacing to gdb.
 */
/* NOTE: For each SR reg we would need to know if it's offset in _sr_registers
   (so we don't allocate this struct to be 256 words but just what's needed),
   and whether it's saved in XMON, ExcFrame, or should be accessed live.
   This map needs storage, needs access functions, so we skip it - will
   have a simpler but hard-coded version, in the code that accesses SRs.
   Consider adding such map as it could include the register location address
   so it could be take right out of map
 */
void* find_saved_reg_location(int offset)
{
  return (offset == EPC)        ? &gExcFrame->pc :
	 (offset == PS)         ? &gExcFrame->ps :
         (offset == EXCCAUSE)   ? &gExcFrame->exccause :
	 (offset == EXCVADDR)   ? &gExcFrame->excvaddr :
#if XCHAL_HAVE_LOOPS
	 (offset == LCOUNT)     ? &gExcFrame->lcount :
	 (offset == LEND)       ? &gExcFrame->lend :
	 (offset == LBEG)       ? &gExcFrame->lbeg :
#endif
#if XCHAL_HAVE_WINDOWED
	 (offset == WB)         ? &_sr_registers[WB]:
#endif
	 (offset == 95      )   ? &_sr_registers[95]:
#if XCHAL_HAVE_CCOUNT
	 (offset == CCOUNT)     ? &_sr_registers[CCOUNT] :
#endif
	                          NULL;
}

uint8_t*
_xmon_get_reg_ptr(const uint32_t libdb_target_number)
{
  XLOG("Get reg pointer (libdb num: 0x%x)\n", libdb_target_number);
  uint8_t *regp  = NULL;
  int   offset = 0;

  // FIXME - ARs that offset a0-15 in CALL0 or a0-a7 need to be redirected to excFrame
  // What we could do is to dump those regs in _ar_registers, however, writing
  // them means dumping back into excFrame, on return. But that might still be faster
  // searching for reg location every-time it's to be written.
  if (XTENSA_DBREGN_IS_AR(libdb_target_number)) {
    offset = XTENSA_DBREGN_AR_INDEX(libdb_target_number);
    XLOG("Read AR reg (%d)\n", offset);
    return (uint8_t *)xmon_ar_reg_ptr_index(offset);
  }

  else if (XTENSA_DBREGN_IS_A(libdb_target_number))  {
    offset = XTENSA_DBREGN_A_INDEX(libdb_target_number);
    XLOG("Read A reg (%d)\n", offset);
    return (uint8_t *) xmon_areg_ptr_index(offset);
  }

  /* Access SR regs, live or saved (in ExcFrame, or XMON saved) */
  else if (XTENSA_DBREGN_IS_SREG(libdb_target_number)) {
    offset = XTENSA_DBREGN_SREG_INDEX(libdb_target_number);

    void* location = find_saved_reg_location(offset);
    if (location) {
      XLOG("Read saved reg %d @ %p\n", offset, location);
      return (uint8_t*) location;
    }

    XLOG("Read live reg %d\n", offset);
    register_val = _xmon_get_spec_reg(offset);
    return (uint8_t *)&register_val;
  }

  /* Access URs */
  else if (XTENSA_DBREGN_IS_UREG(libdb_target_number))
  {
    XLOG("Read UREG (%d)\n", libdb_target_number);
    offset = XTENSA_DBREGN_UREG_INDEX(libdb_target_number);
    register_val = _xmon_get_user_reg( offset);
    regp = (uint8_t *)&register_val;
  }

  else if (XTENSA_DBREGN_IS_PC(libdb_target_number))
  {
    XLOG("Read PC\n", gExcFrame);
    regp = (uint8_t *)&gExcFrame->pc;
  }
  else
  {
    regp = NULL;
  }
  return regp;
}

int
_xmon_get_reg(uint32_t* regptr, const uint32_t libdb_target_number)
{
  XLOG("Get reg (libdb num: 0x%x)\n", libdb_target_number);
  int  index = 0;

  if (XTENSA_DBREGN_IS_AR(libdb_target_number)) {
    index = XTENSA_DBREGN_AR_INDEX(libdb_target_number);
    XLOG("Read AR reg (%d)\n", index);
    uint32_t* regp = xmon_ar_reg_ptr_index(index);
    *regptr = regp[0];
    return 0;
  }

  else if (XTENSA_DBREGN_IS_A(libdb_target_number))  {
    index = XTENSA_DBREGN_A_INDEX(libdb_target_number);
    XLOG("Read A reg (%d)\n", index);
     uint32_t* regp  = xmon_areg_ptr_index(index);
     *regptr = regp[0];
     return 0;
  }

  /* Access SR regs, live or saved (in ExcFrame, or XMON saved) */
  else if (XTENSA_DBREGN_IS_SREG(libdb_target_number)) {
    index = XTENSA_DBREGN_SREG_INDEX(libdb_target_number);

    void* location = find_saved_reg_location(index);
    if (location) {
      XLOG("Read saved reg %d @ %p\n", index, location);
      *regptr = *(uint32_t*)location;
      return 0;
    }

    XLOG("Read live reg %d\n", index);
    *regptr = _xmon_get_spec_reg(index);
    return 0;
  }

  /* Access URs */
  else if (XTENSA_DBREGN_IS_UREG(libdb_target_number))
  {
    XLOG("Read UREG (%d)\n", libdb_target_number);
    index = XTENSA_DBREGN_UREG_INDEX(libdb_target_number);
    *regptr = _xmon_get_user_reg(index);
    return 0;
  }

  else if (XTENSA_DBREGN_IS_PC(libdb_target_number))
  {
    XLOG("Read PC\n", gExcFrame);
    *regptr = gExcFrame->pc;
    return 0;
  }

  return -1;
}

#define XCHAL_DEBUGLEVEL 0
#define PS_INTLEVEL_MASK  0

uint32_t
_xmon_set_reg_value(uint32_t libdb_target_number, const uint32_t value)
{
  uint32_t offset;
  XLOG("Write reg 0x%x\n", libdb_target_number);

  if (XTENSA_DBREGN_IS_AR(libdb_target_number)) {
    offset = XTENSA_DBREGN_AR_INDEX(libdb_target_number);
    uint32_t* regptr = xmon_ar_reg_ptr_index(offset);
    XLOG("Write AR reg (%d)\n", offset);
    *regptr = value;
    return 0;
  }

  else if (XTENSA_DBREGN_IS_A(libdb_target_number))  {
    offset = XTENSA_DBREGN_A_INDEX(libdb_target_number);
    XLOG("Write A reg (%d)\n", offset);
    uint32_t* regptr = xmon_areg_ptr_index(offset);
    *regptr = value;
    return 0;
  }

  else if (XTENSA_DBREGN_IS_UREG(libdb_target_number)) {
      XLOG("Write saved UREG %d\n", libdb_target_number);
      _xmon_set_user_reg( XTENSA_DBREGN_UREG_INDEX(libdb_target_number), value);
      return 0;
  }
  else if (XTENSA_DBREGN_IS_SREG(libdb_target_number)) {
      int offset = XTENSA_DBREGN_SREG_INDEX(libdb_target_number);

      void* location = find_saved_reg_location(offset);
      if (location) {
         XLOG("Set saved SREG %d\n", offset);
         *(uint32_t*)location = value;
	 return 0;
      }
      else {
          XLOG("Set live SREG %d\n", offset);
          xmon_set_spec_reg( offset, value );
          return 0;
      }
  }
  XERR("Write Reg failed");
  return -1;
}

extern uint8_t g_retw_instr[];
extern uint8_t _xmon_exec_instr[];

int
_xmon_execute_tie_instr(char* buf, const char* strexec)
{
  uint8_t instructions[33]; // +3 for ret
  uint8_t *opcode = instructions;
  int i, j, len, total_len = 0;

  XLOG( "Exec. TIE ins.\n");

  do {
    xmon_memset(instructions, 0, 33);
    len = (xmon_charToHex(strexec[0]) << 4) | xmon_charToHex(strexec[1]);
    if (len >= 32) {
      XERR("TIE instr too wide");
      return -1;
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

    if (opcode == NULL) {
      XERR("TIE opcode NIL\n");
      return -1;
    }
    xmon_write_mem(instructions, _xmon_exec_instr + total_len, len, 1, 1);
    total_len += len;
  }
  while (*strexec++ == ':');

  xmon_read_mem(g_retw_instr,  (char*)_xmon_exec_instr+total_len, 3, 1);
  _xmon_exec_here(buf);
  return 0;
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

#if XCHAL_HAVE_CCOUNT
  SR_REG(CCOUNT)       = 0;
#endif
#if XCHAL_HAVE_WINDOWED
  SR_REG(WB)   = 0;
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
  xmon_clear_all_ibreaks();
  xmon_clear_all_dbreaks();
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
