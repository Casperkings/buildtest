/* Software library for TRAX in Eagleridge */

/*
 * Copyright (c) 2012-2013 Tensilica Inc.
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


/* It assumes that the user level API has C-like calling 
 * functions, that can give the Xtensa-core access to TRAX functionality.
 */

#include <stdio.h>
#include <string.h>
#include <xtensa/trax.h>
#include <xtensa/trax-proto.h>
#include <xtensa/traxfile.h>
#include <xtensa/xdm-regs.h>
#include <xtensa/traxreg.h>
#include <time.h>


/* When specifying a size post the trigger in terms of percentage , 
 * it must be realized that there are some bytes always traced post the trigger
 * point, even with DELAY = 0. These include the SYNC message at the end, etc.
 * When specifying instructions, or bytes, we directly set DELAY.  
 * When specifying % of trace RAM, we set it in terms of bytes traced
 * following trigger
 */

#define POSTSIZE_ADJUST 32      /* max number of bytes of trace output after
                                   post-trigger countdown reaches 0, for CNTU
				   setup to count words; must be multiple of 4
				 */


/* ----------------TRAX REGISTERS ------------------------------------------*/


const trax_regdef_t trax_reglist[] = {
  { "id",      XDM_TRAX_ID,      32, HIDE, RO, 0}, //, 0 },
  { "control", XDM_TRAX_CONTROL, 32, HIDE, RW, 0}, //, trax_control_field_set },
  { "status",  XDM_TRAX_STATUS,  32, HIDE, RO, 0}, //, trax_status_field_set },
  { "data",    XDM_TRAX_DATA,    32, HIDE, RW, 0}, //, 0 },
  { "address", XDM_TRAX_ADDRESS, 32, HIDE, RW, 0}, //, 0 },
  { "trigger", XDM_TRAX_TRIGGER, 32, HIDE, RW, 0}, //, 0 },
  { "match",   XDM_TRAX_MATCH,    5, HIDE, RW, 0}, //, 0 },
  { "delay",   XDM_TRAX_DELAY,   24, SHOW, RW, 0}, //, 0 },
  { "startaddr",XDM_TRAX_STARTADDR,32,HIDE,RW, 0}, //, 0 },
  { "endaddr", XDM_TRAX_ENDADDR, 32, HIDE, RW, 0}, //, 0 },
  { "?",       0, 32, HIDE, RW, 0} //, 0 },
};


const signed int trax_readable_regs[] = {
  XDM_TRAX_ID,
  XDM_TRAX_CONTROL,
  XDM_TRAX_STATUS,
  //XDM_TRAX_DATA,                /* reading this has side-effects, so don't */
  XDM_TRAX_ADDRESS,
  XDM_TRAX_TRIGGER,
  XDM_TRAX_MATCH,
  XDM_TRAX_DELAY,
  XDM_TRAX_STARTADDR,
  XDM_TRAX_ENDADDR,												
  /*  These registers are internal only!:  */								
  XDM_TRAX_P4CHANGE,
  XDM_TRAX_P4REV,
  XDM_TRAX_P4DATE,
  XDM_TRAX_P4TIME,
  XDM_TRAX_PDSTATUS,			/* temporal, but doesn't hurt to log */
  XDM_TRAX_PDDATA,			/* temporal, but doesn't hurt to log */
  XDM_TRAX_MSG_STATUS,
  XDM_TRAX_FSM_STATUS,
  XDM_TRAX_IB_STATUS,
  -1
};

const signed int trax_unamed_header_regs[] = {
	XDM_TRAX_STARTADDR,
	XDM_TRAX_ENDADDR,
	/*  These registers are internal only!:  */
	XDM_TRAX_P4CHANGE,
	XDM_TRAX_P4REV,
	XDM_TRAX_P4DATE,
	XDM_TRAX_P4TIME,
	XDM_TRAX_PDSTATUS,
	XDM_TRAX_PDDATA,
	XDM_TRAX_MSG_STATUS,
	XDM_TRAX_FSM_STATUS,
	XDM_TRAX_IB_STATUS,
	-1
};

/*----------------   SET/GET INTERNAL FUNCTIONS   ----------------*/

/* Functions to set and get parameters */

/* This function is used to return the value of a parameter.
 * called by several trax_get_<parameter-name> functions
 *
 * regmask	: Mask of the bits that need to be observed
 * regno	: TRAX register number
 *
 * returns	: value of the parameter, -1 if unsuccessful
 */
static unsigned get_parm(unsigned regmask, unsigned regno)
{
  unsigned shiftmask = (regmask & (~regmask + 1));  /* lsbit of field */
  unsigned regvalue;

  /*  Read register:  */
  if (trax_read_register_eri (regno, &regvalue)) {
    return -1;
  }

  regvalue &= regmask;
  regvalue /= shiftmask;

  return regvalue;
}

/* Setting ptowhen, ctowhen would involve a call to this function
 * Internally it writes values to appropriate register
 * depending on the parameter passed to it
 * 
 * context	: current TRAX context
 * regmask	: To indicate whether processor-trigger or cross-trigger
 * 		  conditions are to be set
 * value	: 0 if off, 1 if ontrig, 2 if onhalt
 * 
 * returns      : 0 if successful, -1 if unsuccessful
 */
static int set_towhen (trax_context *context, unsigned regmask, int value)
{
  int trax_control_ctowt_common = TRAX_CONTROL_CTOWT;
  int trax_control_ptows_common = TRAX_CONTROL_PTOWS;

  /* RE.0 onwards, the bits change position */
  if (context->trax_version >= TRAX_VER_3_0) {
    trax_control_ctowt_common = TRAX_CONTROL_CTOWT_ER;
    trax_control_ptows_common = TRAX_CONTROL_PTOWS_ER;
  }

  /* If regmask is 0, write the value passed to the processor
   * trigger field, else write it to the cross trigger field
   */
  if (regmask) {
    return trax_write_register_field_eri (XDM_TRAX_CONTROL, 
    (trax_control_ctowt_common | TRAX_CONTROL_CTOWS), value);
  } else {
    return trax_write_register_field_eri (XDM_TRAX_CONTROL,
    (trax_control_ptows_common | TRAX_CONTROL_PTOWT), value);
  }
}

/* Showing ptowhen, ctowhen would involve a call to this function
 * Internally it reads appropriate registers depending on the
 * parameter passed to it
 *
 * context	: current TRAX context
 * regmask	: indicates whether processor-trigger or cross-trigger 
 * 		  information is to be shown
 *
 * returns      : 0 if off, 1 if ontrig, 2 if onhalt , -1 if unsuccessful
 */
static int get_towhen (trax_context *context, unsigned regmask)
{
  unsigned control, value;

  int trax_control_ctowt_common = TRAX_CONTROL_CTOWT;
  int trax_control_ptows_common = TRAX_CONTROL_PTOWS;

  if (context->trax_version >= TRAX_VER_3_0) {
    trax_control_ctowt_common = TRAX_CONTROL_CTOWT_ER;
    trax_control_ptows_common = TRAX_CONTROL_PTOWS_ER;
  }
  
  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;
  
  /* Trigger happends before halt, so test that first: */
  if ((control & 
     (regmask ? trax_control_ctowt_common : TRAX_CONTROL_PTOWT)) != 0)
    value = 1 /*"ontrig"*/;
  else if ((control & 
     (regmask ? TRAX_CONTROL_CTOWS : trax_control_ptows_common)) != 0)
    value = 2 /*"onhalt"*/;
  else
    value = 0 /*"off"*/;

  return value;
}

/*----------- INITIALIZATION - HEADER/CONTEXT/TRACEFILE -----------*/

/*
 * Generating the header for the tracefile
 * 
 * size		: size of the trace
 * hflags	: flags passed from the context
 * header_ptr	: pointer to the generated header
 *
 * return	: 0 if successful, -1 if unsuccessful
 *
 */

static int trax_generate_header (int size, int hflags, 
				 trax_file_header *header_ptr)
{
  char *username;
  trax_file_header header = *header_ptr;
  int i;
  /* Generate header */
  memset (&header, 0, sizeof (header));
  strncpy (header.magic, TRAX_FHEAD_MAGIC, 8);
  header.version = TRAX_FHEAD_VERSION;
  header.trace_ofs = sizeof (header);
  header.trace_size = size;
  header.filesize = sizeof (header) + size;
  header.dumptime = time (0);
  header.flags = hflags;

  username="(unknown)";
  
  if (username != 0)
    strncpy (header.username, username, 15);
  strcpy (header.toolver, "toolver");

  /* ID */
  if (trax_read_register_eri(trax_readable_regs[0], &header.id))
    return -1;
 
  /* Control */
  if (trax_read_register_eri(trax_readable_regs[1], &header.control))
    return -1;

  /* Status */
  if (trax_read_register_eri(trax_readable_regs[2], &header.status))
    return -1;
  
  /* Address */
  if (trax_read_register_eri(trax_readable_regs[3], &header.address))
    return -1;
  
  /* Trigger */
  if (trax_read_register_eri(trax_readable_regs[4], &header.trigger))
    return -1;
  
  /* Match */
  if (trax_read_register_eri(trax_readable_regs[5], &header.match))
    return -1;
  
  /* Delay */
  if (trax_read_register_eri(trax_readable_regs[6], &header.delay))
    return -1;
  
  /* Retrieve some set of registers (including startaddr and endaddr, if
   * defined)*/
  for (i = 0; i < NUM_HEADER_UNAMED_REGS; i++) {
    if (trax_read_register_eri(trax_unamed_header_regs[i], &header.trax_regs[i])) {
      return -1;
    }
  }
  
  *header_ptr = header;

  return 0;
}


/* Initializing the trace context */
int trax_context_init_eri (trax_context *context)
{
  /* Assuming you have already connected to TRAX */
  unsigned id = 0, status, ram_size;
  /* You read the ID twice , first might be a dummy read */
  trax_read_register_eri (XDM_TRAX_ID, &id);

  if (trax_read_register_eri (XDM_TRAX_ID, &id) != 0 || id == 0 || id ==
     0xFFFFFFFF) {
    /* Trax ID read incorrectly */
    return -1;
  }

  context->trax_version = TRAX_ID_VER (id);

  if (trax_read_register_eri (XDM_TRAX_STATUS, &status))
    return -1;

  ram_size = TRAX_MEM_SIZE (status);

  if ((ram_size < 128) || (ram_size > 1024*1024*1024))
    return -2;

  context->trax_tram_size = ram_size;
  context->hflags = TRAX_FHEADF_OCD_ENABLED;

  context->get_trace_started = 0;
  context->address_read_last = 0;
  context->bytes_read = 0;

  /* Lets assume initially that the total memory traced is
   * the trace RAM size */
  context->total_memlen = context->trax_tram_size;  
  return 0;
}


/* This will initialize the get_trace function. It is
 * intended to be called only once, when the user
 * calls the get_trace function the first time. 
 * This function would read the address registers, etc. and figure out
 * if the wraparound had taken place. 
 * This function would help in determining the total memory traced and 
 * the address from which the trax_get_trace function would start reading
 * memory
 *
 * context	: pointer to the structure containing the trace context
 *
 * return	: 0 if successful, -1 if read of any register unsuccessful
 * 		  -2 if trace is already in progress while accessing memory
 */ 
static int get_trace_init(trax_context *context)
{
  
  unsigned id, status, addrreg, memaddr, ram_size, address_mask, 
  wrap_mask, startaddr, endaddr;
  int memlen = 0;

  /* Check if its safe to access memory */
  if (trax_read_register_eri (XDM_TRAX_STATUS, &status))
    return -1;
  if ((status & TRAX_STATUS_TRACT) != 0) {
    /* Trace is in progress so cant access memory */
    return -2;
  }

  /* Get the address register */
  if (trax_read_register_eri (XDM_TRAX_ADDRESS, &addrreg))
    return -1;
  
  ram_size = context->trax_tram_size;

  startaddr = 0;
  endaddr = (ram_size >> 2) - 1;

  /* Figure out if it is mem_shared or not */
  if (trax_read_register_eri (XDM_TRAX_ID, &id))
    return -1;
  
  if (TRAX_ID_VER(id) >= TRAX_VER_3_0)
  {
    if (trax_read_register_eri (XDM_TRAX_STARTADDR, &startaddr))
      return -1;
    if (trax_read_register_eri (XDM_TRAX_ENDADDR, &endaddr))
      return -1;
  }

#if DEBUG
  fprintf(stderr, "Start address: %x, End Address: %x\n", startaddr, endaddr);
#endif

  /* Determine the memory address from which to read and allocate memory in
   * a buffer using malloc */
  address_mask = (ram_size >> 2) - 1;	/* address indexes words */
  wrap_mask = ~address_mask;
  
  /* Figure the wraparound */
  if (addrreg & wrap_mask) {
#if DEBUG
    fprintf(stderr, "Wraparound occured\n");
#endif
    
    memaddr = (addrreg & address_mask);
    memlen = (endaddr - startaddr + 1) * 4;

    if (memlen <= 0)
      memlen = memlen + ram_size;

  }else {
    memaddr = startaddr;
    memlen = ((addrreg & address_mask)  - startaddr) * 4;
    if (memlen <= 0)
      memlen = memlen + ram_size;
  }

#if DEBUG
  fprintf (stderr, "Memaddr: 0x%x, Length: %d\n", memaddr, memlen);
#endif

  /* Maintain state of the address that was last read and
   * the total trace memory to be read */
  context->address_read_last = memaddr;
  context->total_memlen = memlen;

  return 0;
}



/* ------------------------ BASIC TRAX ACTIONS ---------------------------*/

/* Reset */
int trax_reset (trax_context *context)
{
  /*  TRAX control library is expected to call reset for every new trace
   *  session and if there is a need to reset the entire tracing mechanism
   *  for the current session. Therefore, it re-initializes some parameters
   *  corresponding to the context
   */
  context->get_trace_started = 0;
  context->address_read_last = 0;
  context->bytes_read = 0;
  context->total_memlen = context->trax_tram_size;  
  
  if (trax_stop_halt(context, TRAX_STOP_QUIET) < 0)
    return -1;

  /*  Initialize TRAX registers per reset:  */
  /*  (note: bit 7 of CONTROL reg is TMEN on VER_2_0 and later, ignored prior 
   *  to that)  */
  if (trax_write_register_eri (XDM_TRAX_CONTROL, 0x00001080))
    return -1;
  if (trax_write_register_eri (XDM_TRAX_ADDRESS, 0))
    return -1;
  if (trax_write_register_eri (XDM_TRAX_TRIGGER, 0))
    return -1;
  if (trax_write_register_eri (XDM_TRAX_MATCH, 0))
    return -1;
  if (trax_write_register_eri (XDM_TRAX_DELAY, 0))
    return -1;

  if (context->trax_version >= TRAX_VER_3_0) {
    if (trax_write_register_eri (XDM_TRAX_STARTADDR, 0))
      return -1;
    if (trax_write_register_eri (XDM_TRAX_ENDADDR, 
        (context->trax_tram_size >> 2) - 1))
      return -1;
  }
  /*  But don't touch trace RAM.  Real reset doesn't either.  */
  return 0;
}




int trax_get_trace (trax_context *context, void *buf, 
                    int bytes_to_be_read)
{
  unsigned total_memlen, bytes_read, address = 0;
  int bytes_actually_read = 0;
  int ret_val = 0;
  trax_file_header header;
  
  if (!context->get_trace_started) {
    /* This shall be called only during the first pass */
    context->get_trace_started = 1;

    if ((ret_val = get_trace_init(context)) < 0)
      return ret_val;

    if (trax_generate_header (context->total_memlen, 
                              context->hflags, &header)) {
#if DEBUG
      fprintf(stderr, "Error in generating header\n");
#endif
      return -1;
    }

    /* The first pass needs to read the header completely
     * hence, the bytes to be read must atleast be 
     * equal to or greater than the size of the header
     */
    if (bytes_to_be_read < sizeof(header))
      return -3;

    memcpy((char *)buf, &header, sizeof(header));
    
    /* Increment buffer by the size of the header */
    buf = (int *)((char *)buf + sizeof(header));
    
    /* Since we have read the header, reduce the bytes to be read */
    bytes_to_be_read -= sizeof(header);

    if (bytes_to_be_read == 0) {
      return sizeof (header); 
    }
  }

  bytes_read = context->bytes_read;
  total_memlen = context->total_memlen;

  /* If bytes_to_be_read is greater than the bytes left in the trace,
   * we read the bytes left in the trace only */
  
  if (bytes_to_be_read + context->bytes_read > context->total_memlen){

#if DEBUG
    fprintf (stderr, "User has passed more bytes than trace left\
             \n.Bytes to be read: %d, bytes read: %d, total memlen: %d\n", 
             bytes_to_be_read, context->bytes_read, 
	     context->total_memlen);
#endif

    bytes_to_be_read = context->total_memlen - context->bytes_read;
  }

  if (bytes_to_be_read > 0) {
    
    /* Read memory API function takes in the address from which to read,
     * the number of bytes to read and puts the result into a buffer */

    if (trax_read_memory_eri 
        (context->address_read_last, bytes_to_be_read, (int *) buf, &address)) {
#if DEBUG
      printf("Error reading %d bytes of trace memory at offset 0x%x", 
              bytes_to_be_read, context->address_read_last);
#endif
      return -1;
    }
  }

  bytes_actually_read = bytes_to_be_read;
  
  /* Once the bytes have been read, we advance the address read last */
  context->address_read_last = address;

#if DEBUG 
  fprintf (stderr, "After completing one round: \
           Last address read: %x, bytes_to_be_read: %d\n", 
           context->address_read_last, bytes_to_be_read); 
#endif

  context->bytes_read = bytes_read + bytes_to_be_read;
  context->total_memlen = total_memlen;


#if DEBUG
  fprintf (stderr, "Bytes read till now: %d, and total memlen: %d\n", 
           context->bytes_read, total_memlen);
#endif

  return bytes_actually_read;
}


/* Start */
int trax_start (trax_context *context)
{
  unsigned status, control;

  /*  Make sure that the trace is not already active:  */
  if (trax_read_register_eri (XDM_TRAX_STATUS, &status) < 0)
    return -1;
  
  if ((status & TRAX_STATUS_TRACT) != 0) {
    /* Trace is already active */
    return 1;
  }
  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control) < 0)
    return -1;

  if ((control & TRAX_CONTROL_TREN) != 0) {
    /*  Clear the Enable bit to ensure 0-1 transition:  */
    if (trax_write_register_eri 
         (XDM_TRAX_CONTROL, control & ~TRAX_CONTROL_TREN) < 0)
      return -1;
  }
  /*  Finally, enable trace:  */
  if (trax_write_register_eri 
      (XDM_TRAX_CONTROL, control | TRAX_CONTROL_TREN) < 0)
    return -1;
  
  return 0;
}

/* Stop */
int trax_stop_halt (trax_context *context, int flags)
{
 
  unsigned status, control, i;
  int halt = ((flags & TRAX_STOP_HALT) != 0);

  /* First check if trace if active. If not, nothing to halt */
  if (trax_read_register_eri (XDM_TRAX_STATUS, &status))
    return -1;

  
  if ((status & TRAX_STATUS_TRACT) ==0) {
    /* Trace is already halted */
    return 1;
  }
  /* Trace is indeed active */

  if (halt)
    /* Clear delay to cancel any post-trigger: */
    if (trax_write_register_eri (XDM_TRAX_DELAY, 0))
      return -1;

  /* Request manual stop: */
  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;

  if ((control & TRAX_CONTROL_TRSTP) == 0) {

    control = control | TRAX_CONTROL_TRSTP;
    if (trax_write_register_eri (XDM_TRAX_CONTROL, control))
      return -1;
  } else {
    /* Manual stop request (clear TREN): */
    if (trax_write_register_eri (XDM_TRAX_CONTROL, control & 
                                 ~TRAX_CONTROL_TRSTP))
      return -1;
    if (trax_write_register_eri (XDM_TRAX_CONTROL, 
                                 control | TRAX_CONTROL_TRSTP))
      return -1;
  }

  if (!halt) {
    return 0;
  }
  
  /* Verify that trace has now indeed halted (try a couple of times, to give
   * it time to settle): */
  for (i = 0; i < 3; i++) {
    if (trax_read_register_eri (XDM_TRAX_STATUS, &status))
      return -1;
    if ((status & TRAX_STATUS_TRACT) == 0) {
      /* Halt Done */
      return 0;
    }
  }
  /* Oops, trace hasn't stopped */

  return -1;
}



/* --------------- SET AND SHOW PARAMS---------------------------------------*/


int trax_get_ram_boundaries (trax_context *context, unsigned *startaddr,
				unsigned *endaddr)
{
  *startaddr =  get_parm (-1, XDM_TRAX_STARTADDR);
  *endaddr = get_parm (-1, XDM_TRAX_ENDADDR);

  return 0;
}

int trax_set_ram_boundaries (trax_context *context, unsigned startaddr,
				unsigned endaddr)
{
  unsigned startaddr_read, endaddr_read;
  /* Check for error conditions if the value passed is incorrect */
  if ((startaddr > (context->trax_tram_size >> 2) - 1) || 
      (endaddr > (context->trax_tram_size >> 2) - 1))
    return -2;

  int trace_size = (int)endaddr - (int)startaddr;

  if (trace_size < 0)
    trace_size += ((context->trax_tram_size >> 2) - 1);
  /* In terms of bytes */
  trace_size = (trace_size + 1) << 2;
  
  if (trace_size < TRAX_MIN_TRACEMEM)
    return -2;
  
  if (trax_write_register_field_eri (XDM_TRAX_STARTADDR, -1, startaddr))
    return -1;

  if (trax_write_register_field_eri (XDM_TRAX_ENDADDR, -1, endaddr))
    return -1;

  /* If memory shared option is not configured, the start and the end address
   * of the trace RAM are set to default, so there is a need to read them back
   * and check */

  startaddr_read =  get_parm (-1, XDM_TRAX_STARTADDR);
  endaddr_read = get_parm (-1, XDM_TRAX_ENDADDR);

  if ((startaddr_read != startaddr) || (endaddr_read != endaddr))
    return -3;

  return 0;
}


/* Get ctistop */
int trax_get_ctistop (trax_context *context)
{
  unsigned value;
  value = get_parm (TRAX_CONTROL_CTIEN, XDM_TRAX_CONTROL);
  if (value > 0)
    return 1;
  else
    return 0;
}


/* Set ctistop */
int trax_set_ctistop (trax_context *context, unsigned value)
{
  return trax_write_register_field_eri (XDM_TRAX_CONTROL, TRAX_CONTROL_CTIEN, value);
}

/* Get cto */
int trax_get_cto (trax_context *context)
{
  unsigned value;
  value = get_parm (TRAX_STATUS_CTO, XDM_TRAX_STATUS);
  if (value > 0)
    return 1;
  else
    return 0;
}

/* Get ctowhen */
int trax_get_ctowhen (trax_context *context)
{
  return get_towhen (context, 1);
}


/* Set ctowhen */
int trax_set_ctowhen (trax_context *context, int option)
{
  if (option < 0 || option > 2)
    option = 0;
  return set_towhen (context, 1, option);
}


/* Get pcstop0 */
int trax_get_pcstop (trax_context *context, int *index,
			   unsigned long *startaddress,
			   unsigned long *endaddress, int *flags)
{
  unsigned control, pc, pcmatch;
  int reverse;
  *flags = 0;
  *startaddress = 0;
  *endaddress = 0;
  *index = 0;

  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;

  if ((control & TRAX_CONTROL_PCMEN) == 0) {
    /* pcstop0 is off */
    return 0;
  }
  
  if (trax_read_register_eri (XDM_TRAX_TRIGGER, &pc))
    return -1;

  if (trax_read_register_eri (XDM_TRAX_MATCH, &pcmatch))
    return -1;

  reverse = ((pcmatch & 0x80000000) != 0);
  *flags = reverse;
  *startaddress = pc;

  *endaddress = pc + (1 << (pcmatch & 0x1F)) - 1;
  return 0;
}

/* Set pcstop0 */
int trax_set_pcstop (trax_context *context, int index, 
                     unsigned long startaddress, unsigned long endaddress, 
		     int flags)
{
  unsigned spanbit, spanindex;
  unsigned pc, pcmatch;
  int size;

  size = endaddress - startaddress + 1;
  
  if (size < 0)
    return -2;

  spanbit = (size & (1 + ~size));

  /* size must always be a power of 2 */
  if (spanbit != size)
    return -2;

  pc = startaddress;

  if (trax_write_register_eri (XDM_TRAX_TRIGGER, (int)pc))
    return -1;

  /* Figure out the first bit position set (starts from 1 at LSB) */
  spanindex = ffs(size) - 1;
 
  /* The XDM_TRAX_MATCH register just has a 5 bit field as of now
   * to support the range of addresses where the PC could lie */

  if (spanindex > 0x1F) {
    return -1;
  }
  
  pcmatch = ((spanindex & 0x1F) | 
             ((flags == TRAX_PCSTOP_REVERSE) ? 0x80000000 : 0));

  if (trax_write_register_eri (XDM_TRAX_MATCH, pcmatch))
    return -1;
  
  if (trax_write_register_field_eri (XDM_TRAX_CONTROL, TRAX_CONTROL_PCMEN, 1))
    return -1;

  return 0;
}

int trax_get_postsize (trax_context *context, int *count_unit, int *unit)
{
  unsigned  delay;
  unsigned control;
  
  *count_unit = 0;
  *unit = 0;

  if (trax_read_register_eri (XDM_TRAX_DELAY, &delay))
    return -1;
  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;

  /* If count_unit was set to less than POSTSIZE_ADJUST bytes in 
   * trax_set_postsize, the delay written in the XDM_TRAX_DELAY register would
   * be zero
   */

  if ((control & TRAX_CONTROL_CNTU) != 0) {
    *count_unit = (int)delay;
    *unit = 1;
  }else {
    if (delay > 0)
      delay += (POSTSIZE_ADJUST/4);
    *count_unit = (int)(delay * 4);
    *unit = 0;
  }

  return 0;
}


/* Set postsize */
int trax_set_postsize (trax_context *context, int count_unit, int unit)
{
  unsigned long trax_tram_size = context->trax_tram_size;
  unsigned max, id;
  unsigned startaddr = 0;
  unsigned endaddr = (trax_tram_size >> 2) - 1;
  int trace_size = 0;

  if (trax_read_register_eri (XDM_TRAX_ID, &id))
    return -1;

  if (TRAX_ID_VER (id) >= TRAX_VER_3_0) {
    if (trax_read_register_eri (XDM_TRAX_STARTADDR, &startaddr))
      return -1;
    if (trax_read_register_eri (XDM_TRAX_ENDADDR, &endaddr))
      return -1;
  }
  
  /* Figure out the total trace memory size in words */
  trace_size = (int)endaddr - (int)startaddr;
  if (trace_size < 0)
    trace_size += ((trax_tram_size >> 2) - 1);
  /* In terms of bytes */
  trace_size = (trace_size + 1) << 2;
  
  /* evnt_to_be_counted indicates which events are to be counted
   * 0 means traceRAM words consumed
   * 1 means target processor instructions executed and exceptions/interrupts
   * taken */
  
  int evnt_to_be_counted = 0;

  /* unit: 0 = bytes, 1= instruction, 2 = %age */

  /* If instructions, then set evnt_to_be_counted */
  if (unit == TRAX_POSTSIZE_INSTR)
    evnt_to_be_counted = 1;
    
  if (unit == TRAX_POSTSIZE_PERCENT) {
    /* Percentage */
    if (trace_size  < TRAX_MIN_TRACEMEM) {
      /* startaddr and endaddr must have a difference of 64 bytes atleast */
      return -1;
    }
    max = 0x04000000 / trace_size;
    if (count_unit > max * 100) {
      return -2;
    }
    count_unit = ((count_unit * (trace_size/16)) /25);	/* convert to number 
    							   of words */
  } else if (evnt_to_be_counted == 0) {
    /* Bytes */
    if (count_unit >= 0x04000000) {
      /* Bytes are too long, max is 64MB */
      return -2;
    }
    count_unit /= 4;
  } else if (count_unit >=0x01000000) {
    return -2;
  }

  /* Adjust for internal buffers */
  
  /* If count_unit was set to less than POSTSIZE_ADJUST bytes
   * the delay written in the XDM_TRAX_DELAY register would be zero
   */
  
  if (evnt_to_be_counted == 0)
    count_unit = (count_unit < (POSTSIZE_ADJUST/4)) ? 0 : count_unit - (POSTSIZE_ADJUST/4);
  
  /* Write the delay */
  if (trax_write_register_eri (XDM_TRAX_DELAY, count_unit))
    return -1;
  
  return trax_write_register_field_eri (XDM_TRAX_CONTROL, TRAX_CONTROL_CNTU, 
                                        evnt_to_be_counted);
}

/* Get ptistop */
int trax_get_ptistop (trax_context *context)
{
  unsigned value;
  value = get_parm (TRAX_CONTROL_PTIEN, XDM_TRAX_CONTROL);
  if (value > 0)
    return 1;
  else
    return 0;
}

/* Set ptistop */
int trax_set_ptistop (trax_context *context, unsigned value)
{
  return trax_write_register_field_eri (XDM_TRAX_CONTROL, TRAX_CONTROL_PTIEN, value); 
}

/* Get pto */
int trax_get_pto (trax_context *context)
{
  unsigned value;
  value = get_parm (TRAX_STATUS_PTO, XDM_TRAX_STATUS);
  if (value > 0)
    return 1;
  else
    return 0;
}

/* Get ptowhen */
int trax_get_ptowhen (trax_context *context)
{
  return get_towhen (context, 0);
}


/* Set ptowhen */
int trax_set_ptowhen (trax_context *context, int option)
{
  if (option < 0 || option > 2)
    option = 0;
  return set_towhen (context, 0, option);
}


/* Get syncper */
int trax_get_syncper (trax_context *context)
{
  unsigned control, smper;

  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;
  smper = (control & TRAX_CONTROL_SMPER) >> TRAX_CONTROL_SMPER_SHIFT;
  
  if (smper)
    return (1 << (9-smper));
  else
    return 0;
}


/* Option: 0 = off, -1: auto, 1 = on, value: frequency of sync */
/* Set syncper */
int trax_set_syncper (trax_context *context, int option)
{
  /* TRAX doc indicates how this table was computed */
  static const char smper4memsz[32] = 
    { 6,6,6,6, 6,6,6,5, 5,4,4,3, 3,2,2,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 };
  unsigned status, memsz, smper, control;
  int aten_avail = 0;
  int tmem_avail = 0;

  if (trax_read_register_eri (XDM_TRAX_CONTROL, &control))
    return -1;

  /* If ATEN enabled, throw a warning if syncper is set
   * to off. Also, if no trace RAM, and ATEN enabled, set syncper
   * to be 1
   */
  aten_avail = control & TRAX_CONTROL_ATEN;
  tmem_avail = control & TRAX_CONTROL_TMEN;
 
  switch (option) {
   
  case 0:
    if (aten_avail > 0){
      /*Warning: Turning syncper off while ATB output is enabled*/
      smper = 1;
      break;
    }
    smper = 0;	/* off */
    break;
  case -1:	/* auto */
    /* TODO: Check if OCD mode is disabled or not and then fall through if not
     * disabled? Do we need to account for the erratum? */
    /* FALLTHROUGH */
  case 1:
    /* Set optimal sync period per trace RAM size */
    if ((aten_avail > 0) && (!tmem_avail)) {
      smper = 1;
      break;
    }
    if (trax_read_register_eri (XDM_TRAX_STATUS, &status))
      return -1;
    memsz = (status & TRAX_STATUS_MEMSZ) >> TRAX_STATUS_MEMSZ_SHIFT;
    
    smper = smper4memsz[memsz];
    
    break;
  case 8:     smper = 6;      break;
  case 16:    smper = 5;      break;
  case 32:    smper = 4;      break;
  case 64:    smper = 3;      break;
  case 128:   smper = 2;      break;
  case 256:   smper = 1;      break;
  default:
    /* Value must be a power of 2 */
    return -2;
  }
  
  return trax_write_register_field_eri (XDM_TRAX_CONTROL, TRAX_CONTROL_SMPER, 
                                        smper);
}

