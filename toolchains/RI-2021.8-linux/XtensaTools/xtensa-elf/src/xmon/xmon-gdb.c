/*
 * Copyright (c) 1999-2013 Tensilica Inc.
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

/******************************************************************************
 *
 * NOTE: The list of supported commands is in README
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <stdlib.h>
#include "xmon.h"
#include "xmon-internal.h"
#include <setjmp.h>
#include <stdio.h>

#define XMON_REMOTE_ERR_STRING

#ifdef XMON_REMOTE_ERR_STRING

static const char * remote_err_msg[] = {
    "OK",
    "unknown register\n",
    "invalid register specification\n",
    "register read failure\n",
    "register write failure\n",
    "invalid address specification\n",
    "memory read failure\n",
    "memory write failure\n",
    "buffer overflow\n",
    "unknown_syscall",
    "breakpoint wrong size\n",
    "unknown error\n",
};
#endif

/* forward definitions */
static inline void    putDebugChar(const char);   /* write a single character      */
static inline uint8_t getDebugChar();                /* read and return a single char */
static inline int     flushOutBuffer();

static GdbPacket gGdbPkt;
static jmp_buf gEnvBuffer;

static char*    _remcomIn   = NULL;
static char*    _remcomOut  = NULL;
static uint32_t _remcomSize = 0;

xmon_logger_t _xlogger = NULL;
char gCatchErr = 0;

int
_xmon_gdbvars_init(char* gdbBuf, int gdbBufSize)
{
  if(gdbBuf == NULL) {
    XERR("XMON needs GDB buffer\n");
    return -2;
  }

  if(gdbBufSize < 200) {
    XERR("GDB buffer size is < 200\n");
    return -3;
  }
  _remcomSize   = gdbBufSize;
  _remcomIn     = gdbBuf;
  _remcomOut    = gdbBuf;
  _remcomOut[0] = 0;
  gGdbPkt.state = GDB_PKT_NOT_STARTED;
  return 0;
}

void
_xmon_logger(char app_log_en, char gdb_log_en)
{
  gAppLogOn   = app_log_en;
  gGdbLogOn   = gdb_log_en;
}

void
_xmon_consoleString(const char* str)
{
  _xmon_putConsoleString(str);
}


void
_xmon_close(void)
{
  gXmonEnabled = 0;
}

void _xmon_enter()
{
  asm("break 1,15" ); 
}


int
_xmon_active ()
{
  return gInXmon;
}

void _xmon_longjmp()
{
  longjmp(gEnvBuffer, 1);
}

/*************************************************************************/

/* Send the packet to GDB */
static void
putpacket(char *buffer)
{
  uint8_t checksum;
  uint8_t ack;
  uint8_t ch;
  int count;

  int retryCnt = 0;
  int dummyCnt = 0;

  /*  $<packet info>#<checksum>. */
  do {
    putDebugChar('$');

    checksum = 0;
    count = 0;

    while ((ch = buffer[count]) != 0)
    {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }

    putDebugChar('#');
    putDebugChar(hexchars[checksum >> 4]);
    putDebugChar(hexchars[checksum & 0xf]);

    flushOutBuffer();
    ch = getDebugChar();
    ack = ch & 0x7f;

    if(ack == '+') {
      //XLOG("GOT ACK!!\n");
      break;
    }

    if(ack == 3) {
      XLOG("Interrupt instead of ack\n");
      break;
    }

    if(ack ==  '-') {
      XERR("Send failed, checksum:%x\n", checksum);
      if(retryCnt++ == 5) {
         XERR("Won't retry, abandon sending packet...\n");
         break;
      }
      continue;
    }

    // here we just got some char that makes no sense.
    if(dummyCnt++ == 5) {
      XERR("Too much noise, abandon sending packet...\n");
      break;
    }
  }
  while (1);
}

static void
remote_send_stop_cause ( int sigval)
{
  XLOG("Send Stop Cause\n");
  xmon_sprintf(_remcomOut, "T0%1x", (sigval & 0xff));
  putpacket(_remcomOut);
}

/*
Return an error reply "Enn" to a remote protocol request packet,
where "nn" is a hex-ascii representation of XMON error code n.
Append the error message as GDB can show it although it's not per the spec.
*/
static void remote_reply_error(const remote_err_t n)
{
  char* buf = _remcomOut;
#ifdef XMON_REMOTE_ERR_STRING
  xmon_sprintf(buf, "E%2x : XMON : %s", (n & 0xff), remote_err_msg[n]);
#else
  xmon_sprintf(buf, "E%2x", (n & 0xff));
#endif
  putpacket(buf);
}

/*
Return an empty response to an unsupported packet
*/
static void remote_reply_unsupported()
{
  XLOG("Unsupported command '%s'!\n", _remcomIn);
  _remcomOut[0] = 0;
  putpacket(_remcomOut);
}

/*
Return an "OK" reply to a remote protocol request packet.
*/
static void remote_reply_ok()
{
  xmon_sprintf(_remcomOut, "OK");
  putpacket(_remcomOut);
}

static void
gdbpkt_clear_state(GdbPacket* gdbpkt)
{
  gdbpkt->state = GDB_PKT_NOT_STARTED;
  gdbpkt->xmitcsum = 0;
  gdbpkt->checksum = 0;
  gdbpkt->count = 0;
}

/* Scan for the sequence $<data>#<checksum>  */
static int
getpacket(char* buffer)
{
  int fail;

  while (1) {
    /* if GDB attached just wait for a char, otherwise return NULL
     * so XMON can exit until full/correct packet is received
     */
    uint8_t ch = getDebugChar();

    switch (gGdbPkt.state)
    {
      case GDB_PKT_NOT_STARTED:
          if(ch == '$') {
            gGdbPkt.state = GDB_PKT_PENDING;
	    buffer[gGdbPkt.count] = 0;
	  }
          break;

      case GDB_PKT_PENDING:
          // if buffer ovfl go back to init state.
          // Can send response to GDB even if not attached (FIXME?).
          if (gGdbPkt.count >= _remcomSize)  {
              putDebugChar('+');            /* ack w/o requesting retransmit */
              buffer[0] = '\0';             /* reply nothing was received. */
              remote_reply_error((remote_err_t) remote_err_buf_overflow);
              XERR("ack w/o req. transmit\n");
              gdbpkt_clear_state(&gGdbPkt); /* Go back to init state */
              break;
          }
          // At this point we have received a valid char and have space for it.

          // End of a packet
          if(ch == '#') {
            gGdbPkt.state = GDB_PKT_CHKSUM;
            buffer[gGdbPkt.count] = 0;
            break;
          }

          // packet in progress, update state, keep pending.
          gGdbPkt.checksum = gGdbPkt.checksum + ch;
          buffer[gGdbPkt.count] = ch;
          gGdbPkt.count++;
          break;

      case GDB_PKT_CHKSUM:
          gGdbPkt.xmitcsum  = (xmon_hex(ch & 0xff) << 4);
          gGdbPkt.state = GDB_PKT_RECEIVED;
          break;

      case GDB_PKT_RECEIVED:
          gGdbPkt.xmitcsum |= (xmon_hex(ch) & 0xff);
          fail =  (gGdbPkt.checksum != gGdbPkt.xmitcsum);
          if (fail) {
              putDebugChar('-');  /* failed checksum */
              XERR("Failed chksum\n");
              flushOutBuffer();
              gdbpkt_clear_state(&gGdbPkt);
              continue;
          }
          else  {
             putDebugChar('+'); /* successful transfer */
             flushOutBuffer();
             gdbpkt_clear_state(&gGdbPkt);
             goto done;
          }
          break;
      default:
          break;

    } // end FSM switch
  }

done:
  XLOG("Packet: '%s'\n", buffer);
  return 1;

  return 0;
}

static char*  parse_token(char **arg)
{
  char c, *s = *arg, *t;

  while (*s && (*s == ' ' || *s == '\t'))
    s++;              /* skip whitespace */
  if (*s == '#')      /* skip comments to end-of-line */
    s += xmon_strlen(s);
  *arg = s;
  if (!*s)
    return 0;         /* no more tokens */
  for (t = s; (c = *t) != 0 && c != ' ' && c != '\t' && c != '#'; t++)
    ;                 /* skip until whitespace or end or comment */
  if (c) {
    *t++ = 0;         /* terminate token */
    if (c == '#')
      t += xmon_strlen(t); /* skip comment to end-of-line */
  }
  *arg = t;
  return s;
}

/*  Execute GDB "monitor" command.  */
static void
process_monitor_command_query(const char *orig_cmd)
{
  char *keyword;
  char cmd_buffer[64];
  char *cmd;

  XLOG("monitor command\n");

  xmon_memset((uint8_t*)cmd_buffer, 0, sizeof(cmd_buffer));
  xmon_sprintf(cmd_buffer, "%s", orig_cmd);
  cmd = &cmd_buffer[0];

  keyword = parse_token(&cmd);

#if 0
  if ((keyword == 0) || (xmon_strncmp(keyword, "help", 4) == 0))
  {
    _XMON_XGDB("monitor Usage\n");
    _XMON_XGDB("=====================================================================\n");
    _XMON_XGDB("monitor trace  [on|off]        // Trace internal function calls      \n");
    _XMON_XGDB("monitor log    [on|off]        // Enable logging to GDB              \n");
    _XMON_XGDB("\n");
    goto done;
  }
#endif

  /* monitor log|trace [on|off] */
  int *enabled = 0;
  int log = 0;
  
  if (xmon_strncmp(keyword, "applog", 6) == 0) {
    enabled = &gAppLogOn;
    log = 1;
  }
  
  else if (xmon_strncmp(keyword, "log", 3) == 0) {
    enabled = &gGdbLogOn;
    log = 1;
  }

  if (enabled)
  {
    char *onoff = parse_token(&cmd);

    if (onoff == 0)
      goto fail_onoff;

    if (xmon_strncmp(onoff, "on", 2) == 0)
      *enabled = 1;
    else if (xmon_strncmp(onoff, "off", 3) == 0)
      *enabled = 0;
    else
      goto fail;

    _XMON_XGDB( (*enabled && log) ? "Log Enabled\n"   :
                                    "Log Disabled\n");

    remote_reply_ok();
    return;
  }

  /*  If not a recognized command, just fail:  */

fail_onoff :
   _XMON_XGDB("Need on/off parameter\n");
fail:
  _XMON_XGDB("Unrecognised command:  '");
  _XMON_XGDB(orig_cmd);
  remote_reply_error((remote_err_t)remote_err_reg_unknown);
  return;
}


static int
xmon_tie_read(char* out, const char* pkt)
{
  XLOG("TIE Read\n");
  /*
   * Read TIE register:
   *
   * qxtreg/regno/:/len/:/nops/:/op0len/:/op0[0]/
   * Ex: Actual Packet from xt-gdb:
   *     packet = "qxtreg00000060:0008:03:04:84:b2"
   *                     ^        ^    ^  ^  ^  ^
   *       hex regno ----+        |    |  |  |  +--- ?
   *       hex   len -------------+    |  |  +------ op0[0]
   *                                   |  +--------- op0len
   *                                   +------------ nops
   */

  const char *ptr = pkt + 6;
  uint32_t regnum = 0, reglen = 0;

  if(!xmon_hexToInt(&ptr, &regnum) || *ptr++ != ':' || !xmon_hexToInt(&ptr, &reglen) || *ptr++ != ':')
    return -1;

  gCatchErr = remote_err_reg_read;
  char valbuf[MAX_TIE_LEN];
  if(_xmon_execute_tie_instr(valbuf, ptr))
    return -1;

  int i, j;
  for (i = reglen - 1, j = 2 * reglen; i >= 0; i--) {
    out[--j] = hexchars[(valbuf[i] & 0xf)];
    out[--j] = hexchars[(valbuf[i] >> 4)];
  }
  out[reglen*2] = 0;
  XLOG("TIE Read success\n");
  return 0;
}

/* Qxtreg/regno/:/len/:/nops/:/op0len/:/op0[0]/=/val/ */
static inline int
xmon_tie_write(const char* pkt)
{
  uint32_t regnum, reglen;
  const char *ptr = pkt + 6;

  XLOG("TIE write\n");

  gCatchErr = remote_err_reg_invalid;
  if(!xmon_hexToInt(&ptr, &regnum) || *ptr++ != ':' || !xmon_hexToInt(&ptr, &reglen) || *ptr++ != ':')
    return -1;

  uint32_t i;
  char valbuf[MAX_TIE_LEN];
  char *valptr = _xmon_strchr(ptr, '=');
  valptr++;

  for (i = 0; i < reglen; i++)
    valbuf[i] = (xmon_hex(valptr[i*2+0]) << 4)  | (xmon_hex(valptr[i*2+1]));

  gCatchErr = remote_err_reg_read;
  if(_xmon_execute_tie_instr(valbuf, ptr))
    return -1;

  return 0;
}

/*  GDB protocol queries starting with 'q'  */
static void
gdbpkt_q_general( char* out, const char* pkt )
{
  gCatchErr = remote_err_reg_invalid;
  switch (pkt[1]) 
  {
    case 'R':    /* qRcmd */
    {
      char *pktp;
      char* cs;

      if (xmon_strncmp(pkt + 2, "cmd" , 3)) 
        goto unsupported;

      const char* cmd = pkt; //see code below - write and read won't overlap
      for(pktp = (char*)pkt + 6, cs = (char*)cmd; *pktp; )
      {
        int v1 = xmon_hex(*pktp++);
        int v2 = xmon_hex(*pktp++);
        if ((v1 == -1) || (v2 == -1)) {
          remote_reply_error((remote_err_t)remote_err_ok);
          return;
        }
        *cs++ = (v1 << 4)|v2;
      }
      *cs = 0;
      /* Process the command(still needs to be written, is a stub now)*/
      process_monitor_command_query(cmd);
      return;
    }

    case 'S':
      if (xmon_strncmp(pkt+2, "upported", 8))  /* qSupported */
        goto unsupported;
      xmon_sprintf(out, "PacketSize=0x%x", _remcomSize - 24);
      break;

    case 'C':
      if (xmon_strncmp(pkt+2,"C",1))  /* qsThreadInfo */
        goto unsupported;
      xmon_sprintf(out, "l");
      break;

    case 's':
      if (xmon_strncmp(pkt+2,"ThreadInfo",10))  /* qsThreadInfo */
        goto unsupported;
      xmon_sprintf(out, "l");
      break;

    case 'f':
      if (xmon_strncmp(pkt+2,"ThreadInfo",10))  /* qfThreadInfo */
        goto unsupported;
      xmon_sprintf(out, "m0,l");
      break;

    default:
      goto unsupported;
  }
  putpacket(out);
  return;

unsupported:
  remote_reply_unsupported();
}

static void inline 
gdbpkt_qxt (char* out, const char* pkt)
{
  switch( pkt[3] ) 
  {
    case 'n':  /* qxtn - Query target command */
      xmon_sprintf(out, "%s", version_string);
      putpacket(out);
      return;

    case 'r': /* qxtreg command */
       if (xmon_strncmp(pkt + 4, "eg", 2))
         goto unsupported;

       if (xmon_tie_read(out, pkt))
         goto err;

       putpacket(out);
       return;

     default:
       goto unsupported;
  }

err:
  remote_reply_error((remote_err_t)gCatchErr);
  return;
unsupported:
  remote_reply_unsupported();
  return;
}

static void inline 
gdbpkt_p (char* out, const char* pkt)
{ 
  const char *ptr= &pkt[1];	
  uint32_t addr;

  gCatchErr = remote_err_addr_invalid;
  if(!xmon_hexToInt(&ptr, &addr)) 
    goto err;

  gCatchErr = remote_err_reg_invalid;
  uint8_t *regp = _xmon_get_reg_ptr( addr);
  if (!regp)
    goto err;
	    
  gCatchErr = remote_err_reg_read;
  if(xmon_read_mem( regp, out, 4, 0))
     goto err;

  putpacket(out);
  return;
err:
  remote_reply_error((remote_err_t)gCatchErr);
}

static void inline 
gdbpkt_P (const char* pkt)
{ 
  const char *ptr= &pkt[1];	
  uint32_t value, addr;

  gCatchErr = remote_err_addr_invalid;
  if(!xmon_hexToInt(&ptr, &addr) || *ptr++ != '=' )
    goto err;

  gCatchErr = remote_err_mem_write;
  if (xmon_write_mem( (uint8_t *)ptr, (uint8_t *)&value, 4, 0, 0 ))
    goto err;

  gCatchErr = remote_err_mem_write;
  if (_xmon_set_reg_value(addr, value))
    goto err;
    
  remote_reply_ok();
  return;
err:
  remote_reply_error((remote_err_t)gCatchErr);
}


static void inline 
gdbpkt_m (char* out, const char* pkt)
{
  const char *ptr= &pkt[1];
  uint32_t addr, length;
  
  gCatchErr = remote_err_addr_invalid;
  if (!xmon_hexToInt(&ptr, &addr) || *ptr++ != ',' || !xmon_hexToInt(&ptr, &length))
    goto err;

  gCatchErr = remote_err_mem_read;
  if (xmon_read_mem((uint8_t *)addr, out, length, 0))
    goto err;

  putpacket(out);
  return;
err:
  XERR("m packet error\n");
  remote_reply_error((remote_err_t)gCatchErr);
}

static void inline 
gdbpkt_M (const char* pkt)
{ 
  const char *ptr= &pkt[1];
  uint32_t addr, length;

  gCatchErr = remote_err_addr_invalid;
  if (!xmon_hexToInt(&ptr, &addr) || *ptr++ != ',' || !xmon_hexToInt(&ptr, &length) ||  *ptr++ != ':')
    goto err;

  gCatchErr = remote_err_mem_write;
  if (xmon_write_mem((uint8_t *)ptr, (uint8_t *)addr, length, 1, 0))
    goto err;

  remote_reply_ok();
  return;
err:
  remote_reply_error((remote_err_t)gCatchErr);
}

static void inline 
gdbpkt_c (const char* pkt)
{ 
 const char *ptr = &pkt[1];
 uint32_t addr;
 _xmon_continue (xmon_hexToInt(&ptr, &addr) ? &addr : NULL); //FIXME - any bad packet error ??
}

static void inline 
gdbpkt_F (const char* pkt)
{ 
  int retcode = -1, reterrno = 0, intr = 0;
  const char *ptr;

  /* Determine parameters */
  retcode = strtoul((char *)&pkt[1], NULL, 16);
  if ((ptr = _xmon_strchr((char *)pkt, ',')) != NULL) {
    reterrno = strtoul(ptr + 1, NULL, 16);
    if ((ptr = _xmon_strchr(ptr + 1, ',')) != NULL)
      intr = ((*(ptr+1)) | 0x20) == 'c' ? 1 : 0;
  }

  /* FIXME: If syscall was interrupted before completion (errno==4),
     let it replay once we continue (will it ?).
  */
  if (intr) {
    remote_send_stop_cause(XMON_SIGTRAP);
    return;
  }

  /* FIXME:  if it was step, interrupt as well ? */

  /* Set return values */
  xmon_wr_areg(2, retcode);
  xmon_wr_areg(3, reterrno);
}

static void inline 
gdbpkt_Q (char* out, const char* pkt)
{
  gCatchErr = remote_err_mem_write;

  /* Not Xtensa specific commands(non-"Qxt") */
  if (pkt[1] != 'x' || pkt[2] != 't')
    return;

  /*  Qxt****  */
  switch(pkt[3] ) {

    case 'e':
      if (pkt[4] != 'x' && pkt[5] != 'e' && pkt[6] != '=') {      /* Qxtexe=... */
        //ExecuteSomeInstruction( &pkt[7] );
        out[0] = '\0';
      }
      return;  /* ignore anything that's not Qxtexe=... */

    case 'r': /* Qxtreg command */
      if (xmon_strncmp(pkt+4, "eg", 2))
        return;

      if(xmon_tie_write(pkt)) {
        remote_reply_error((remote_err_t)gCatchErr);
	return;
      }
      remote_reply_ok();
      return;

     default:
       remote_reply_unsupported();
       return;
  }
}

static void inline 
gdbpkt_reset ()
{
  _xmon_sw_reset();
  remote_reply_ok();
  remote_send_stop_cause(XMON_SIGINT);
  _XMON_XGDB("XMON prepared reset. Resume to start from reset\n");
}

static void inline 
gdbpkt_q (char* out, const char* pkt)
{
  if (pkt[1] != 'x' || pkt[2] != 't')
    gdbpkt_q_general(out, pkt);
  else
    gdbpkt_qxt(out, pkt);
}

static void inline 
gdbpkt_breakpoint (const char* pkt)
{
  uint32_t addr, size;
  uint32_t type = xmon_hex(pkt[1]);
  int ret;

  gCatchErr = remote_err_mem_read;
  if (pkt[2] != ',' || pkt[3] == 0)
    goto err;

  if (type > 4 /* 1 */) 
    goto err;

  const char* ptr = &pkt[3];

  gCatchErr = remote_err_addr_invalid;
  if (!xmon_hexToInt(&ptr, &addr) || *ptr++ != ','  || !xmon_hexToInt(&ptr, &size))
    goto err;

  /* Find a spot in the list and add the breakpoint/watchpoint to it. */
  ret = (pkt[1] == '0') ? (pkt[0] == 'Z') ? xmon_add_sw_breakpoint(addr, size) :
                                            xmon_remove_sw_breakpoint(addr, size) :
        (pkt[1] == '1') ? (pkt[0] == 'Z') ? xmon_add_hw_breakpoint(addr) :
                                            xmon_remove_hw_breakpoint(addr) :
        (pkt[1] == '2') ? (pkt[0] == 'Z') ? xmon_add_hw_watchpoint(addr, type) :
                                            xmon_remove_hw_watchpoint(addr, type) : -1; 
  if(ret)
    goto err;

  remote_reply_ok();
  return;
err:
  remote_reply_error((remote_err_t)gCatchErr);
}

//FIXME - make xmon_in/out failures to result in return 0 (gdb not enabled)
void
_xmon_gdbserver ( int sigval, int syscall )
{
  // Before trying a command, set long jump. If exception happens while in XMON, 
  // it'll return to a special PC which will execute longjmp, back here.
  int jmpret = setjmp(gEnvBuffer);
  if (jmpret != 0) {
    // Send reply to the failing, previous command
    remote_reply_error((remote_err_t)gCatchErr);
  }

  /* Tell gdb that we have stopped. If syscall, send it's response instead  */
  if (syscall) {
    if (!_xmon_process_syscall(_remcomIn))
        putpacket(_remcomIn);
    else
        remote_send_stop_cause(XMON_SIGTRAP);    // FIXME - some error to be sent here (like got syscall but couldn't process it ?
  }
  else {
    remote_send_stop_cause(sigval);
  }

  XLOG("In GDBSERVER loop\n");
  while (1) {

    if (!getpacket(_remcomIn)) 
       return;         // requested exit while waiting for packet

    switch (_remcomIn[0])  {

      case '?':   /* RM FIXME: Should upgrade to T packet. */
        remote_send_stop_cause(sigval);
        break;

      case 'R':   /* RXX -- remote restart. */
                  gdbpkt_reset();
                  break;
      case 'p':
                  gdbpkt_p (_remcomOut, _remcomIn);
                  break;
      case 'P':
                  gdbpkt_P (_remcomIn);
                  break;
      case 'm':   /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
                  gdbpkt_m (_remcomOut, _remcomIn);
                  break;
      case 'M':   /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
                  gdbpkt_M(_remcomIn);
                  break;
      case 'v':
                 remote_reply_ok();
	         break;
      case 'q':
                 gdbpkt_q(_remcomOut, _remcomIn);
                 break;
      case 'Q':
                gdbpkt_Q(_remcomOut, _remcomIn);
                break;

      case 'F': /* F/retcode/,/errno/,/Ctrl-C/;/.../ -- F - reply */
                gdbpkt_F(_remcomIn);
	        return;
      case 'c':  /* cAA..AA    Continue at address AA..AA(optional) */
                 /* try to read optional parameter, pc unchanged if no parm */
                 gdbpkt_c(_remcomIn);
                 return;
      case 's':
                 _xmon_single_step(0);
                 return;

      case 'D':  /* detach. */
                 remote_reply_ok();
	         return;
      case 'k':     /* kill the program */
                /* FIXME - Send 'exit-code' if gdb thinks we are running. Possible ?
                   Only GDB can detach (?) as opposed to XOCD where this can happen in
                   other cases. For XMON, sudden detach would be leaving XMON as well.
                 */
                //send_reply(target, "W00");
                XLOG("Kill the program\n");
                _xmon_kill();
                return;

      /* SW breakpoint support:  "Zt,addr,len" t=0 */
      case 'Z':
      case 'z':
       gdbpkt_breakpoint(_remcomIn);
       break;

      default:
        remote_reply_unsupported();
        break;

    }  /* switch first char */
  }
}

/* Send the packet to GDB */
static void
putNotifpacket(const char *buffer)
{
  uint8_t checksum = 0;
  int count = 0;
  uint8_t ch;

  /*  $<packet info>#<checksum>. */
  putDebugChar('%');

  while ((ch = buffer[count]) != 0)
  {
    putDebugChar(ch);
    checksum += ch;
    count += 1;
  }

  putDebugChar('#');
  putDebugChar(hexchars[checksum >> 4]);
  putDebugChar(hexchars[checksum & 0xf]);
  flushOutBuffer();
}

/*
 *  Output to GDB console.
 *  Can only be called if GDB thinks the target is running.
 */
void  _xmon_putConsoleString(const char *s)
{
  char c;
  while ((c = *s++) != 0) {
    char buf[300], *bufptr = buf;
    *bufptr++ = 'O';     /* 'O:' indicates console output packet */
    *bufptr++ = ':';
    /*  Hex encode characters to output, one buffer's worth at a time:  */
    while (1) {
      *bufptr++ = hexchars[(uint8_t)c >> 4];
      *bufptr++ = hexchars[c & 0xF];
      if (bufptr >= buf + sizeof(buf) - 1)
        break;
      if ((c = *s) == 0)
        break;
      s++;
    }
    *bufptr = 0;     /* terminate string */
    putNotifpacket(buf);
  }
}

/******************************************************************

Xtensa hardware-dependent utilities

*******************************************************************/
static inline void putDebugChar(const char c)
{
  _xmon_out(1, (char*)&c);
}

static inline uint8_t getDebugChar()
{
  return _xmon_in();
}

static inline int flushOutBuffer()
{
  return _xmon_flush();
}

