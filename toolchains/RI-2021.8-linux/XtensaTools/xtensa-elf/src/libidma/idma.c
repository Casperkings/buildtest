/*
 * Copyright (c) 2018-2020 Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define IDMA_BUILD

#include <stdint.h>
#include <stdarg.h>

#include "idma_internal.h"


#define DO_CB   1
#define NO_CB   0


idma_status_t queue_task(idma_buf_t * task);


WRAPPER_FUNC int32_t
convert_idma_type_to_int(idma_type_t type)
{
  return (int32_t)type;                                                         // parasoft-suppress MISRA2012-RULE-11_3 "conversion from a subset of int values to int"
}

WRAPPER_FUNC uint32_t*
convert_desc_ptr_to_uint_ptr(idma_desc_t* desc)                                 // parasoft-suppress MISRA2012-RULE-8_13_a "no const arg - returned arg is altered"
{
  return (uint32_t*)desc;                                                       // parasoft-suppress MISRA2012-RULE-11_3 "conversion from a limited pointer to a more general one"
}

static inline uint32_t
convert_desc_ptr_to_uint(idma_desc_t* desc)                                     // parasoft-suppress MISRA2012-RULE-8_13_a "no const arg - returned arg is altered"
{
  return (uint32_t)desc;                                                        // parasoft-suppress MISRA2012-RULE-11_4 "use of the conversion is verified"
}


static inline idma_state_t
idma_get_state_ii(int32_t ch)
{
  XLOG(ch, "get_state\n");
  return (idma_state_t) (READ_IDMA_REG(ch, IDMA_REG_STATUS) & IDMA_STATE_MASK);

}

static inline void
idma_enable_ii(int32_t ch)
{
 XLOG(ch, "enable\n");
 WRITE_IDMA_REG(ch, IDMA_REG_CONTROL, 0x1);
}

static inline void
idma_disable_ii(int32_t ch)
{
  DECLARE_PS();
  idma_state_t state = idma_get_state_ii(ch);

  XLOG(ch, "disable\n");

  // Skip if already disabled. Skip if in error state, requires a reset.
  if ((state == IDMA_STATE_IDLE) || (state == IDMA_STATE_ERROR)) {
    return;
  }

  IDMA_DISABLE_INTS();

  // Wait until state is BUSY or DONE or HALT
  do {
    state = idma_get_state_ii(ch);
  }
  while ((state != IDMA_STATE_BUSY) && (state != IDMA_STATE_DONE) && (state != IDMA_STATE_HALT));

  WRITE_IDMA_REG(ch, IDMA_REG_CONTROL, 0x0);

  // Wait until state is IDLE or HALT or ERROR
  do {
    state = idma_get_state_ii(ch);
  }
  while ((state != IDMA_STATE_IDLE) && (state != IDMA_STATE_HALT) && (state != IDMA_STATE_ERROR));

  IDMA_ENABLE_INTS();
}

static inline void
idma_pause_ii(int32_t ch)
{
  idma_disable_ii(ch);
}

static inline void
idma_resume_ii(int32_t ch)
{
  idma_enable_ii(ch);
}

static void
idma_reset_i(int32_t ch)
{
  DECLARE_PS();
  idma_state_t state;

  IDMA_DISABLE_INTS();

  g_idma_cntrl[ch].num_outstanding = 0;

  XLOG(ch, "reset assert\n");
  WRITE_IDMA_REG(ch, IDMA_REG_CONTROL, 0x2);

  do {
    state = idma_get_state_ii(ch);
  }
  while (state != IDMA_STATE_IDLE);

  XLOG(ch, "reset deassert\n");
  WRITE_IDMA_REG(ch, IDMA_REG_CONTROL, 0x0);

  IDMA_ENABLE_INTS();
}

static int32_t
idma_busy_ii(int32_t ch)
{
  idma_state_t  idma_state;
  uint32_t      numDescs ;
  int32_t       busy = 0;

  numDescs   = hw_num_outstanding(ch);
  idma_state = idma_get_state_ii(ch);

  if ((numDescs > 0U) || ( (idma_state != IDMA_STATE_IDLE) && (idma_state != IDMA_STATE_DONE))) {
    busy = 1;
  }

  XLOG(ch, "%s busy (numDescs:%d, status:0x%x)\n", (busy == 1) ? "":"NOT", numDescs, idma_state);
  return busy;
}

static int32_t
idma_busy_i(int32_t ch)
{
  return idma_busy_ii(ch);
}

static void
idma_set_start_address_i(int32_t ch, idma_desc_t *start_desc)
{
  DECLARE_PS();

  IDMA_DISABLE_INTS();

  // Write Fetch Start bit
  uint32_t settings = READ_IDMA_REG(ch, IDMA_REG_SETTINGS);

  settings |= (1U << (uint32_t)IDMA_FETCH_START);
  WRITE_IDMA_REG(ch, IDMA_REG_SETTINGS, settings);

  // Write start address
  WRITE_IDMA_REG(ch, IDMA_REG_DESC_START, convert_desc_ptr_to_uint(start_desc));

  IDMA_ENABLE_INTS();
}

/*
 * Mark desc as done and invoke callback if present
 */
INTERNAL_FUNC void
complete_desc_i(idma_buf_t *task)
{
  XLOG(task->ch, "Task %p: descriptor completed (status:%d)\n", task, task->status);

  if (task->cb_func != NULL) {
    (*task->cb_func)(task->cb_data);
  }

  // If task completed and owner thread is sleeping then wake it
  if (task->status == (int32_t) IDMA_TASK_DONE) {
#ifdef IDMA_USE_XTOS
    idma_thread_unblock(task->thread_id);
#else
    if (task->sleeping > 0) {
      XLOG(task->ch, "thread %p wake\n", task->thread_id);
      task->sleeping = 0;
      idma_thread_unblock(task->thread_id);
    }
#endif
  }
}

/*
 * Mark a task as aborted and invoke callback if present
 */
static void
abort_tasks(idma_buf_t *tasks)
{
  idma_buf_t *task = tasks;

  while (task != NULL) {
    if(task->status > (int32_t)IDMA_TASK_DONE) { //only abort tasks in progress
      XLOG(task->ch, "Task %p: aborted\n", task);
      task->status = (int32_t) IDMA_TASK_ABORTED;

      if (task->cb_func != NULL) {
        (*task->cb_func)(task->cb_data);
      }
    }
    task = task->next_task;
  }
}

static void
set_err_details (int32_t ch, idma_error_details_t* error,  uint32_t status)
{
 /* Set error details for current task */
  error->err_type = (status >> IDMA_ERRCODES_SHIFT);
  error->currDesc = READ_IDMA_REG(ch, IDMA_REG_CURR_DESC );
  error->srcAddr  = READ_IDMA_REG(ch, IDMA_REG_SRC_ADDR );
  error->dstAddr  = READ_IDMA_REG(ch, IDMA_REG_DST_ADDR );

  XLOG(ch, "Error: code:0x%x, src:0x%x, dst:0x%x\n",
       error->err_type, error->srcAddr, error->dstAddr);
}

/*
 * Set task error status, invoke channel error callback if any,
 * and abort any remaining tasks on this channel.
 */
static void
idma_process_error_i(int32_t ch, idma_buf_t* task, uint32_t status)
{
  g_idma_cntrl[ch].initialized = 0;                                             // parasoft-suppress MISRA2012-RULE-18_1_a MISRA2012-DIR-4_1_a "access is within bounds"

  /* Set error details for current task */
  set_err_details (ch, &g_idma_cntrl[ch].error, status);
  task->status = (int32_t)IDMA_TASK_ERROR;

  if(g_idma_cntrl[ch].err_cb_func != NULL) {
    g_idma_cntrl[ch].err_cb_func(&g_idma_cntrl[ch].error);
  }

  XLOG(ch, "Abort other tasks\n");
  abort_tasks((idma_buf_t*)task->next_task);
}

/*
 * Retire any pending tasks and invokes the callback if present.
 */
INTERNAL_FUNC idma_status_t
idma_task_processing_i(int32_t ch)
{
  DECLARE_PS();
  uint32_t    rem_descs;
  uint32_t    idma_status;
  uint32_t    num_completed;
  idma_buf_t* task;
  int32_t     idma_error = 0;

  IDMA_DISABLE_INTS();

  rem_descs   = (uint32_t)READ_IDMA_REG(ch, IDMA_REG_NUM_DESC);
  idma_status = (uint32_t)READ_IDMA_REG(ch, IDMA_REG_STATUS);

  if ((idma_status & (uint32_t)IDMA_STATE_MASK) == (uint32_t)IDMA_STATE_ERROR) {
    idma_error  = 1;
  }

  if((rem_descs == g_idma_cntrl[ch].num_outstanding) && (idma_error == 0)) {    // parasoft-suppress MISRA2012-RULE-18_1_a MISRA2012-DIR-4_1_a "access is within bounds"
    IDMA_ENABLE_INTS();
    return IDMA_OK;
  }

  task = g_idma_cntrl[ch].oldest_task;

  if (g_idma_cntrl[ch].num_outstanding < rem_descs)  {
    XLOG(ch, "Error: Invalid num_outstanding (%d), from HW: %d\n", g_idma_cntrl[ch].num_outstanding, rem_descs);
    abort_tasks(task);
    task->status = (int32_t)IDMA_ERR_TASK_OUTSTAND_NEG;
    if (idma_error == 1) {
      idma_process_error_i(ch, task, idma_status);
      IDMA_ENABLE_INTS();
      return IDMA_ERR_HW_ERROR;
    }
    IDMA_ENABLE_INTS();
    return IDMA_ERR_TASK_OUTSTAND_NEG;
  }

  num_completed = g_idma_cntrl[ch].num_outstanding - rem_descs;
  g_idma_cntrl[ch].num_outstanding = rem_descs;

  XLOG(ch, "Processing %d completed descriptors from task @ %p(%d) (#remaining:%d, #err:%d)\n",
        num_completed, task, task->status,  rem_descs, idma_error);

  while ((num_completed > 0U) && (idma_error == 0)) {
    if(num_completed < (uint32_t)task->status) {
        task->status -= (int32_t)num_completed;
        #pragma no_reorder
        complete_desc_i(task);
        break;
    }
    num_completed -= (uint32_t)task->status;
    task->status = 0;

    #pragma no_reorder
    complete_desc_i(task);
    g_idma_cntrl[ch].oldest_task = task->next_task;

    if((task->next_task == NULL) && (num_completed > 0U)) {
       XLOG(ch, "Error: No next task but more completed descs to process (%d)\n", num_completed);
       if (idma_error == 1) {
         idma_process_error_i(ch, task, idma_status);
         IDMA_ENABLE_INTS();
         return IDMA_ERR_HW_ERROR;
       }
       abort_tasks(task);
       IDMA_ENABLE_INTS();
       return IDMA_ERR_TASK_OUTSTAND_NEG;
    }
    task = task->next_task;
  }

  #pragma no_reorder
  if (idma_error == 1) {
    idma_process_error_i(ch, task, idma_status);
    IDMA_ENABLE_INTS();
    return IDMA_ERR_HW_ERROR;
  }
  IDMA_ENABLE_INTS();
  return IDMA_OK;
}

#if XCHAL_HAVE_INTERRUPTS && IDMA_USE_INTR

/*
 * IDMA interrupt handler. Retires any pending task and invokes the
 * callback (if present) associated with the completed requests
 */
static void
idma_task_done_intr_handler(void * arg)
{
  int32_t ch = cvt_voidp_to_int32(arg);

  XLOG(ch, "task_done intr\n");
  // Return value not useful here.
  (void) idma_task_processing_i(ch);
}

static void
idma_task_err_intr_handler(void * arg)
{
  int32_t ch = cvt_voidp_to_int32(arg);

  XLOG(ch, "task_err intr\n");
  // Return value not useful here.
  (void) idma_task_processing_i(ch);
}

#endif

static int32_t
buffer_error_processing(int32_t ch, int32_t do_cb)
{
  uint32_t     idma_status;
  idma_buf_t * buf;

  buf = g_idma_buf_ptr[ch];        // parasoft-suppress MISRA2012-RULE-18_1_a MISRA2012-DIR-4_1_a "access is within bounds"

  // Check if the hardware is in error state
  idma_status = (uint32_t) READ_IDMA_REG(ch, IDMA_REG_STATUS);
  if ((idma_status & (uint32_t)IDMA_STATE_MASK) != (uint32_t)IDMA_STATE_ERROR) {
    #pragma frequency_hint FREQUENT
    return 0;
  }

  // If buffer already in error, skip processing
  if (buf->status == (int32_t)IDMA_ERR_BUFFER_IN_ERROR) {
    return buf->status;
  }

  buf->status = (int32_t)IDMA_ERR_BUFFER_IN_ERROR;
  set_err_details(ch, &g_idma_cntrl[ch].error, idma_status);    // parasoft-suppress MISRA2012-RULE-18_1_a MISRA2012-DIR-4_1_a "access is within bounds"
  g_idma_cntrl[ch].initialized = 0;
  XLOG(ch, "Error: HW in error (%x)\n", g_idma_cntrl[ch].error.err_type);

  if (do_cb == DO_CB) {
    if (g_idma_cntrl[ch].err_cb_func != NULL) {
      (*g_idma_cntrl[ch].err_cb_func)(&g_idma_cntrl[ch].error);
    }
  }

  return buf->status;
}

static int32_t
buffer_status_processing(int32_t ch)
{
  idma_buf_t * buf = g_idma_buf_ptr[ch];        // parasoft-suppress MISRA2012-RULE-18_1_a MISRA2012-DIR-4_1_a "access is within bounds"
  buf->status = (int32_t) READ_IDMA_REG(ch, IDMA_REG_NUM_DESC);
  return buf->status;
}

#if XCHAL_HAVE_INTERRUPTS && IDMA_USE_INTR

static void
idma_loop_done_intr_handler(void * arg)
{
  int32_t      ch  = cvt_voidp_to_int32(arg);
  idma_buf_t * buf = g_idma_buf_ptr[ch];

  XLOG(ch, "buf_done intr\n");

  if (buf != NULL) {
    // Update buffer status and schedule next buffer if any.
    // Return value not useful here.
    (void) idma_buffer_status_i(ch);

    // Buffer completion callback
    if (buf->cb_func != NULL) {
      (*buf->cb_func)(buf->cb_data);
    }

#ifdef IDMA_USE_XTOS
    idma_thread_unblock(buf->thread_id);
#else
    if (buf->sleeping > 0) {
      XLOG(ch, "thread %p wake\n", buf->thread_id);
      buf->sleeping = 0;
      idma_thread_unblock(buf->thread_id);
    }
#endif
  }
}

static void
idma_loop_err_intr_handler(void * arg)
{
  int32_t      ch  = cvt_voidp_to_int32(arg);
  idma_buf_t * buf = g_idma_buf_ptr[ch];

  XLOG(ch, "buf_err intr\n");

  // Return value not useful here.
  (void) buffer_error_processing(ch, DO_CB);

  if (buf != NULL) {
#ifdef IDMA_USE_XTOS
    idma_thread_unblock(buf->thread_id);
#else
    if (buf->sleeping > 0) {
      XLOG(ch, "thread %p wake\n", buf->thread_id);
      buf->sleeping = 0;
      idma_thread_unblock(buf->thread_id);
    }
#endif
  }
}

#endif

static void
add_jump(idma_buf_t * new_buf, idma_buf_t * old_buf)
{
  uint32_t * last_desc_ptr = convert_desc_ptr_to_uint_ptr(old_buf->last_desc);
  uint32_t   last_desc_val = convert_desc_ptr_to_uint(&new_buf->desc);

  *last_desc_ptr = last_desc_val;

  XLOG(new_buf->ch, "Add jump %p (%p) -> %p (%p)\n", old_buf, old_buf->last_desc, new_buf, new_buf->desc);
}

idma_status_t
queue_task(idma_buf_t *task)
{
#ifdef IDMA_DEBUG
  uint32_t numdesc;
#endif
  int32_t  busy;
  int32_t  ch;
  DECLARE_PS();

  ch = task->ch;
  if (g_idma_cntrl[ch].initialized == 0U) {
    return IDMA_ERR_NOT_INIT;
  }
  if (g_idma_cntrl[ch].error.err_type != IDMA_NO_ERR) {
    return IDMA_ERR_HW_ERROR;
  }

  IDMA_DISABLE_INTS();
  task->status = task->num_descs;
  busy = idma_busy_i(ch);
  if ( ( (busy > 0) || (g_idma_cntrl[ch].num_outstanding > 0U))  && (g_idma_cntrl[ch].newest_task != NULL)) {
    idma_buf_t* old_task = g_idma_cntrl[ch].newest_task;
    add_jump(task, old_task);
    old_task->next_task = task;
  }
  else {
    XLOG(ch, "Start task %p desc %p num %d\n", task, &task->desc, task->status);
    idma_disable_ii(ch);
    idma_set_start_address_i(ch, &task->desc);
    g_idma_cntrl[ch].oldest_task = task;
  }

  /* Track number of queued descriptors */
  g_idma_cntrl[ch].num_outstanding += (uint32_t)task->num_descs;
  g_idma_cntrl[ch].newest_task = task;

#ifdef IDMA_DEBUG
  numdesc = READ_IDMA_REG(ch, IDMA_REG_NUM_DESC);
  XLOG(ch, "Queue task %p w/ %d descs, oldest pending task @ %p (#OUTSTANDING lib:%d, hw:%d): \n",
        task, task->status, g_idma_cntrl[ch].oldest_task, g_idma_cntrl[ch].num_outstanding, numdesc);
#endif
#if XCHAL_HAVE_XEA2 || XCHAL_HAVE_XEA3
  XT_MEMW();
#endif
  idma_enable_i(ch);
  /* Queue the descriptor(s) */
  WRITE_IDMA_REG(ch, IDMA_REG_DESC_INC, (uint32_t)task->status);
  IDMA_ENABLE_INTS();

  return IDMA_OK;
}


/********** API ***************/

void
idma_log_handler(idma_log_h xlog)
{
#ifdef IDMA_DEBUG
 g_idma_cntrl[IDMA_CHANNEL_0].xlogh = xlog;
#else
 (void) xlog; /* Suppress unused-parameter warning */
#endif
}

/* Initialize the idma channel and its associated control structure.
 */
idma_status_t
idma_init_i(int32_t              ch,
            uint32_t             flags,
            idma_max_block_t     block_sz,
            uint32_t             pif_req,
            idma_ticks_cyc_t     ticks_per_cyc,
            uint32_t             timeout_ticks,
            idma_err_callback_fn err_cb_func)
{
  DECLARE_PS();
  uint32_t haltable  = 0U;
  uint32_t maxpifreq = pif_req;
  idma_max_block_t bsz = block_sz;

  if (ch >= XCHAL_IDMA_NUM_CHANNELS) {
    XLOG(ch, "ERROR: Invalid channel number\n");
    return IDMA_ERR_BAD_INIT;
  }

  // pif_req = 0 will use the max allowed by the config. So zero is allowed.
  if (maxpifreq > (uint32_t)XCHAL_IDMA_MAX_OUTSTANDING_REQ) {
    XLOG(ch, "ERROR: Invalid max outstanding PIF requests\n");
    return IDMA_ERR_BAD_INIT;
  }

  switch (bsz) {
    case MAX_BLOCK_2:
    case MAX_BLOCK_4:
    case MAX_BLOCK_8:
      break;
    case MAX_BLOCK_16:
      // If the reorder buffer is present, the max block size allowed is 8.
#if XCHAL_IDMA_HAVE_REORDERBUF
      bsz = MAX_BLOCK_8;
#endif
      break;
    default:
      // For unsupported values select the max supported.
#if XCHAL_IDMA_HAVE_REORDERBUF
      bsz = MAX_BLOCK_8;
#else
      bsz = MAX_BLOCK_16;
#endif
      break;
  }

  IDMA_DISABLE_INTS();

  idma_stop_i(ch);
  idma_reset_i(ch);

  if ((flags & (uint32_t)IDMA_OCD_HALT_ON) == (uint32_t)IDMA_OCD_HALT_ON) {
    haltable = 1U;
  }

  if (maxpifreq == (uint32_t)XCHAL_IDMA_MAX_OUTSTANDING_REQ) {
    maxpifreq = 0U;
  }

  g_idma_cntrl[ch].settings  = ((uint32_t)bsz & IDMA_MAX_BLOCK_SIZE_MASK) << IDMA_MAX_BLOCK_SIZE_SHIFT;
  g_idma_cntrl[ch].settings |= (maxpifreq & IDMA_OUTSTANDING_REG_MASK)    << IDMA_OUTSTANDING_REG_SHIFT;
  g_idma_cntrl[ch].settings |= (haltable & IDMA_HALT_ON_OCD_MASK)         << IDMA_HALT_ON_OCD_SHIFT;

  // Set default bundle size to 4. Older versions of hardware do not support these bits so writing
  // them will have no effect.
  g_idma_cntrl[ch].settings |= ((uint32_t)4U << IDMA_BUNDLE_SHIFT);

  g_idma_cntrl[ch].timeout   = ((uint32_t)ticks_per_cyc & IDMA_TIMEOUT_CLOCK_MASK) << IDMA_TIMEOUT_CLOCK_SHIFT;
  g_idma_cntrl[ch].timeout  |= (timeout_ticks & IDMA_TIMEOUT_THRESHOLD_MASK) << IDMA_TIMEOUT_THRESHOLD_SHIFT;

  WRITE_IDMA_REG(ch, IDMA_REG_SETTINGS, g_idma_cntrl[ch].settings);
#if !defined (IDMA_USERMODE)
  WRITE_IDMA_REG(ch, IDMA_REG_TIMEOUT, g_idma_cntrl[ch].timeout);
#endif

  g_idma_cntrl[ch].reg_base        = IDMAREG_BASE((uint32_t)ch);
  g_idma_cntrl[ch].num_outstanding = 0;
  g_idma_cntrl[ch].oldest_task     = NULL;
  g_idma_cntrl[ch].newest_task     = NULL;

  g_idma_cntrl[ch].err_cb_func     = err_cb_func;
  g_idma_cntrl[ch].error.err_type  = 0;
  g_idma_cntrl[ch].error.currDesc  = 0;
  g_idma_cntrl[ch].error.srcAddr   = 0;
  g_idma_cntrl[ch].error.dstAddr   = 0;

  g_idma_cntrl[ch].initialized = 1;

  g_idma_buf_ptr[ch] = NULL;

  IDMA_ENABLE_INTS();
  return IDMA_OK;
}

void
idma_enable_i(int32_t ch)
{
  idma_enable_ii(ch);
}

void
idma_resume_i(int32_t ch)
{
  idma_resume_ii(ch);
}

void
idma_stop_i(int32_t ch)
{
  idma_disable_ii(ch);
}

idma_state_t
idma_get_state_i(int32_t ch)
{
  return idma_get_state_ii(ch);
}

void idma_pause_i(int32_t ch)
{
  idma_pause_ii(ch);
}

idma_status_t
idma_sleep_i(int32_t ch)
{
  DECLARE_PS();
  uint32_t idma_status;

  if (ch >= XCHAL_IDMA_NUM_CHANNELS) {
    return IDMA_ERR_BAD_CHAN;
  }

  IDMA_DISABLE_INTS();

  idma_status = (uint32_t) READ_IDMA_REG(ch, IDMA_REG_STATUS);
  if ((idma_status & (uint32_t) IDMA_STATE_MASK) == (uint32_t) IDMA_STATE_ERROR) {
    g_idma_cntrl[ch].initialized = 0;
    IDMA_ENABLE_INTS();
    XLOG(ch, "HW in error, cannot sleep\n");
    return IDMA_ERR_HW_ERROR;
  }

#ifdef IDMA_USE_XTOS

  if(hw_num_outstanding(ch) == 0U) {
    IDMA_ENABLE_INTS();
    XLOG(ch,"No outstanding descriptors, cannot sleep\n");
    return IDMA_CANT_SLEEP;
  }

  idma_thread_block(NULL);
  IDMA_ENABLE_INTS();
  return IDMA_OK;

#else

  // We can sleep only if (a) thread buffer is active with pending transfers
  // or (b) thread buffer is pending.
  idma_buf_t * buf = idma_chan_buf_get(ch);

  if (buf != NULL) {
    if (((buf == g_idma_buf_ptr[ch]) && (hw_num_outstanding(ch) > 0U)) ||
        (buf->pending > 0)) {
      XLOG(ch, "thread %p sleep\n", buf->thread_id);
      buf->sleeping = 1;
      idma_thread_block(buf->thread_id);
      IDMA_ENABLE_INTS();
      return IDMA_OK;
    }
  }

  IDMA_ENABLE_INTS();
  XLOG(ch, "No valid buf or no pending transfers, cannot sleep\n");
  return IDMA_CANT_SLEEP;

#endif
}

/******************** TASK MODE **********************/

static inline idma_status_t
idma_init_task_ii (int32_t ch,
                   idma_buffer_t *taskh,
                   idma_type_t type,
                   int32_t num_descs,
                   idma_callback_fn cb_func,
                   void *cb_data)
{
  idma_buf_t*   task;
  idma_desc_t*  desc;
  int32_t       typei;
  int32_t       ret = 0;

  if ((ch >= XCHAL_IDMA_NUM_CHANNELS) || (taskh == NULL)) {
    return IDMA_ERR_BAD_INIT;
  }

  task             = convert_buffer_to_buf(taskh);
  typei            = convert_idma_type_to_int(type);
  task->ch         = ch;
  task->type       = typei;
  task->num_descs  = num_descs;
  task->cb_data    = cb_data;
  task->cb_func    = cb_func;

#if XCHAL_HAVE_INTERRUPTS && IDMA_USE_INTR
  ret = idma_register_interrupts(ch, idma_task_done_intr_handler, idma_task_err_intr_handler);
#endif

  desc = &task->desc;
  task->next_add_desc = desc;
  task->last_desc = &desc[num_descs*typei];

  XLOG(ch, "Initialize task @ %p w/ %d desc (DESC @ %p, LAST DESC @ %p)\n",
         task, task->num_descs, &task->desc, task->next_add_desc);
  return (ret == 0) ? IDMA_OK : IDMA_ERR_BAD_INIT;	/* parasoft-suppress MISRA2012-RULE-14_3_zc-2 "Not a constant expression in all configs" */
}

idma_status_t
idma_init_task_i (int32_t ch,
                  idma_buffer_t *taskh,
		  idma_type_t type,
		  int32_t ndescs,
		  idma_callback_fn cb_func,
		  void *cb_data)
{
  return idma_init_task_ii(ch, taskh, type, ndescs, cb_func, cb_data);
}

idma_status_t
idma_schedule_task( idma_buffer_t *taskh)
{
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);
  return queue_task(task);
}

idma_status_t
idma_copy_task_i( int32_t ch,
                idma_buffer_t *taskh,
                void *dst,
                void *src,
                size_t size,
                uint32_t flags,
                void *cb_data,
                idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);
  ret = idma_init_task_ii(ch, taskh, IDMA_1D_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_desc (taskh, dst, src, size, flags);
    return queue_task(task);
  }
  return ret;
}

idma_status_t
idma_copy_2d_task_i( int32_t ch,
                    idma_buffer_t *taskh,
                    void *dst,
                    void *src,
                    size_t row_sz,
                    uint32_t flags,
                    uint32_t nrows,
                    uint32_t src_pitch,
                    uint32_t dst_pitch,
                    void *cb_data,
                    idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_2D_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_2d_desc (taskh, dst, src, row_sz, flags, nrows, src_pitch, dst_pitch);
    return queue_task(task);
  }
  return ret;
}

#if (IDMA_USE_64B_DESC > 0)
	
idma_status_t
idma_copy_task64_i( int32_t ch,
                idma_buffer_t *taskh,
                void *dst,
                void *src,
                size_t size,
                uint32_t flags,
                void *cb_data,
                idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);
  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_desc64(taskh, dst, src, size, flags);
    return queue_task(task);
  }
  return ret;
}

idma_status_t
idma_copy_2d_task64_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   size_t row_sz,
                   uint32_t flags,
                   uint32_t nrows,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
                   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_2d_desc64(taskh, dst, src, row_sz, flags, nrows, src_pitch, dst_pitch);
    return queue_task(task);
  }
  return ret;
}

idma_status_t
idma_copy_2d_pred_task64_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   size_t row_sz,
                   uint32_t flags,
                   void* pred_mask,
                   uint32_t nrows,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
                   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_2d_pred_desc64(taskh, dst, src, row_sz, flags, pred_mask, nrows, src_pitch, dst_pitch);
    return queue_task(task);
  }
  return ret;
}


idma_status_t
idma_copy_3d_task64_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   uint32_t flags,
                   size_t   row_sz,
                   uint32_t nrows,
		   uint32_t ntiles,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
		   uint32_t src_tile_pitch,
		   uint32_t dst_tile_pitch,
		   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // return value not useful here.
    (void) idma_add_3d_desc64(taskh, dst, src, flags, row_sz, nrows, ntiles, src_pitch, dst_pitch, src_tile_pitch, dst_tile_pitch);
    return queue_task(task);
  }
  return ret;
}

#if (IDMA_USE_WIDE_API > 0)

idma_status_t
idma_copy_task64_wide_i( int32_t ch,
                         idma_buffer_t *taskh,
                         void *dst,
                         void *src,
                         size_t size,
                         uint32_t flags,
                         void *cb_data,
                         idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_desc64_wide(taskh, dst, src, size, flags);
    return queue_task(task);
  }
  return ret;
}

idma_status_t
idma_copy_2d_task64_wide_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   size_t row_sz,
                   uint32_t flags,
                   uint32_t nrows,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
                   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_2d_desc64_wide(taskh, dst, src, row_sz, flags, nrows, src_pitch, dst_pitch);
    return queue_task(task);
  }
  return ret;
}


idma_status_t
idma_copy_2d_pred_task64_wide_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   size_t row_sz,
                   uint32_t flags,
	           void* pred_mask,
                   uint32_t nrows,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
                   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_2d_pred_desc64_wide(taskh, dst, src, row_sz, flags, pred_mask, nrows, src_pitch, dst_pitch);
    return queue_task(task);
  }
  return ret;
}

idma_status_t
idma_copy_3d_task64_wide_i( int32_t ch,
                   idma_buffer_t *taskh,
                   void *dst,
                   void *src,
                   uint32_t flags,
                   size_t   row_sz,
                   uint32_t nrows,
		   uint32_t ntiles,
                   uint32_t src_pitch,
                   uint32_t dst_pitch,
		   uint32_t src_tile_pitch,
		   uint32_t dst_tile_pitch,
		   void *cb_data,
                   idma_callback_fn cb_func)
{
  idma_status_t ret;
  idma_buf_t *task;
  task = convert_buffer_to_buf(taskh);

  ret = idma_init_task_ii(ch, taskh, IDMA_64B_DESC, 1, cb_func, cb_data);
  if (ret == IDMA_OK) {
    // Return value not useful here.
    (void) idma_add_3d_desc64_wide(taskh, dst, src, flags, row_sz, nrows, ntiles, src_pitch, dst_pitch, src_tile_pitch, dst_tile_pitch);
    return queue_task(task);
  }
  return ret;
}

#endif   // IDMA_USE_WIDE_API
#endif   // IDMA_USE_64B_DESC

/* Process tasks that are done. Any callbacks associated
 * with completed tasks gets invoked.
 * Returns the idma error code if there are any, else returns IDMA_OK.
 */
idma_status_t
idma_process_tasks_i(int32_t ch)
{
  return idma_task_processing_i(ch);
}

idma_error_details_t*
idma_error_details_i(int32_t ch)
{
  return (&g_idma_cntrl[ch].error);
}

idma_hw_error_t
idma_buffer_check_errors_i(int32_t ch)
{
  int32_t error;
  idma_hw_error_t ret;
  DECLARE_PS();

  IDMA_DISABLE_INTS();

  error = buffer_error_processing(ch, NO_CB);
  if(error != 0) {
   ret = g_idma_cntrl[ch].error.err_type;
  }
  else {
    ret = IDMA_NO_ERR;
  }
  IDMA_ENABLE_INTS();
  return ret;
}

idma_status_t
idma_abort_tasks_i(int32_t ch)
{
  idma_buf_t* task;
  task =  (idma_buf_t*)g_idma_cntrl[ch].oldest_task;

  idma_stop_i(ch);
  idma_reset_i(ch);

  // Restore channel settings after reset.
  WRITE_IDMA_REG(ch, IDMA_REG_SETTINGS, g_idma_cntrl[ch].settings);
  WRITE_IDMA_REG(ch, IDMA_REG_TIMEOUT,  g_idma_cntrl[ch].timeout);

  abort_tasks(task);
  return IDMA_OK;
}

/******************** FIXED BUFFER MODE **********************/

// Initialize the descriptor buffer and associate it with the
// channel (XTOS mode) or the current thread (OS mode).

idma_status_t
idma_init_loop_i (int32_t          ch,
                  idma_buffer_t *  bufh,
                  idma_type_t      type,
                  int32_t          ndescs,
                  void *           cb_data,
                  idma_callback_fn cb_func)  // parasoft-suppress MISRA2012-RULE-8_13_a-4 "cannot make this const, pass through"
{
  idma_buf_t *  buf;
  idma_desc_t * desc;
  int32_t       typei;
  uint32_t *    last_desc_ptr;
  uint32_t      last_desc_val;
  int32_t       ret = 0;

  if ((ch >= XCHAL_IDMA_NUM_CHANNELS) || (bufh == NULL)) {
    return IDMA_ERR_BAD_INIT;
  }

  buf   = convert_buffer_to_buf(bufh);
  typei = convert_idma_type_to_int(type);
  desc  = &buf->desc;

  buf->last_desc = &desc[ndescs * typei];
  last_desc_ptr  = convert_desc_ptr_to_uint_ptr(buf->last_desc);
  last_desc_val  = convert_desc_ptr_to_uint(desc);
  *last_desc_ptr = last_desc_val;

  // For XTOS mode, disable the channel and bind the buffer to it.
  // For OS mode, we cannot disable the channel because it might be
  // active. Set the thread's active buffer pointer.
#ifdef IDMA_USE_XTOS
  idma_disable_ii(ch);
  idma_set_start_address_i(ch, desc);
#endif
  idma_chan_buf_set(ch, buf);

  XLOG(ch, "Set Fixed-Buffer mode w/ %d %s descriptors (JUMP %p -> %p)\n",
        ndescs, (type == IDMA_1D_DESC) ? "1D" : "2D", buf->last_desc, desc);

  buf->ch            = ch;
  buf->cur_desc_i    = 0;
  buf->num_descs     = ndescs;
  buf->next_desc     = desc;
  buf->next_add_desc = desc;
  buf->type          = typei;
  buf->status        = (int32_t)IDMA_TASK_EMPTY;
  buf->pending       = 0;
  buf->sleeping      = 0;
  buf->thread_id     = idma_thread_id();
  buf->pending_desc_cnt = 0;

#if XCHAL_HAVE_INTERRUPTS && IDMA_USE_INTR
  buf->cb_data = cb_data;
  buf->cb_func = cb_func;
  ret = idma_register_interrupts(ch, idma_loop_done_intr_handler, idma_loop_err_intr_handler);
#else
  (void) cb_func;
  (void) cb_data;
#endif
  return (ret == 0) ? IDMA_OK : IDMA_ERR_BAD_INIT;
}

#ifndef IDMA_USE_XTOS
static void
update_next_desc_buf(idma_buf_t * buf, uint32_t count)
{
  idma_desc_t * next = &buf->next_desc[count * (uint32_t)buf->type];

  if (next >= buf->last_desc) {
    next = &buf->desc + (next - buf->last_desc);
  }
  buf->next_desc = next;
}
#endif

#ifndef IDMA_USE_XTOS
// Bind the thread's buffer to the specified channel and schedule the
// required number of descriptors from the buffer. This function is
// called with interrupts disabled. Never directly called from user
// code so parameter checking is not needed.
//
// 1) If the channel is idle then bind buffer to channel and start
//    transfers.
// 2) If the buffer is already the current active buffer, then just
//    schedule more transfers.
// 3) If the buffer is already in the pending list, then increment
//    the pending descriptor count for the buffer.
// 4) If the channel is busy and this buffer is not in the channel's
//    pending list, then add it to the pending list and set the
//    pending descriptor count for the buffer.

int32_t
schedule_thread_buffer(int32_t ch, uint32_t count)
{
  DECLARE_PS();

  idma_buf_t * buf = idma_chan_buf_get(ch);
  idma_buf_t * active_buf;

  if (buf == NULL) {
    return IDMA_ERR_NO_BUF;
  }

  IDMA_DISABLE_INTS();

  active_buf = g_idma_buf_ptr[ch];

  if (active_buf == NULL) {
    // case (1)
    idma_disable_ii(ch);
    idma_set_start_address_i(ch, buf->next_desc);
    g_idma_buf_ptr[ch] = buf;
    g_idma_cntrl[ch].oldest_task = buf;
    g_idma_cntrl[ch].newest_task = buf;
    buf->next_task = NULL;
    active_buf = buf; // Needed by next check
    XLOG(ch, "Bind buf %p to ch %d\n", buf, ch);
  }

  buf->cur_desc_i += (int32_t)count;
  buf->cur_desc_i &= INT32_C(0x7fffffff);

  if (buf == active_buf) {
    // case (2)
    update_next_desc_buf(buf, count);
    hw_schedule(ch, count);
    XLOG(ch, "Schedule %d desc from buf %p index %d\n", count, buf, buf->cur_desc_i);
  }
  else if (buf->pending) {
    // case (3)
    buf->pending_desc_cnt += count;
    XLOG(ch, "Add %d desc to pending buf %p (count %d)\n", count, buf, buf->pending_desc_cnt);
  }
  else {
    // case (4)
    // newest_task must not be NULL and newest_task->next_task must be NULL
    buf->pending = 1;
    buf->pending_desc_cnt = count;
    buf->next_task = NULL;
    g_idma_cntrl[ch].newest_task->next_task = buf;
    g_idma_cntrl[ch].newest_task = buf;
    XLOG(ch, "Make buf %p pending (count %d)\n", buf, count);
    // Force buffer update.
    idma_buffer_status_i(ch);
  }

  IDMA_ENABLE_INTS();
  return buf->cur_desc_i;
}
#endif

// Check channel status, process current buffer if complete and
// schedule next buffer if pending.

int32_t
idma_buffer_status_i(int32_t ch)
{
  int32_t ret;
  DECLARE_PS();

#ifdef IDMA_USE_XTOS

  IDMA_DISABLE_INTS();
  ret = buffer_error_processing(ch, DO_CB);
  if (ret == 0) {
    ret = buffer_status_processing(ch);
  }
  IDMA_ENABLE_INTS();
  return ret;

#else

  // Process the active buffer and move to the next buffer if any.
  // Make sure to return the status of the thread's buffer which
  // may not be the current active buffer. The calling thread's
  // buffer may be in one of 3 states:
  // - currently active (in process)
  // - completed
  // - pending (queued)

  int32_t      tmp;
  idma_buf_t * buf        = idma_chan_buf_get(ch);
  idma_buf_t * active_buf;

  IDMA_DISABLE_INTS();

  active_buf = g_idma_buf_ptr[ch];

  // If there is no active buffer, there's no processing to be done.
  if (active_buf == NULL) {
    if (buf == NULL) {
      ret = IDMA_ERR_NO_BUF;
    }
    else {
      ret = (buf->pending > 0) ? buf->pending_desc_cnt : buf->status;
    }

    IDMA_ENABLE_INTS();
    return ret;
  }

  // Update the status of the active buffer (this may or may not be
  // the buffer we are interested in).
  ret = buffer_error_processing(ch, DO_CB);
  if (ret == 0) {
    ret = buffer_status_processing(ch);
  }

  if (ret > 0) {
    // Not finished yet. If this is our buffer 'ret' has the number of
    // remaining descriptors. If the active buffer is not our buffer
    // then return our buffer's status (if complete) or pending desc
    // count (if incomplete).
    if (buf == NULL) {
      ret = IDMA_ERR_NO_BUF;
    }
    else {
      if (buf != active_buf) {
        ret = (buf->pending > 0) ? buf->pending_desc_cnt : buf->status;
      }
    }

    IDMA_ENABLE_INTS();
    return ret;
  }

  // If we got here, either the buffer has completed or it had an error.
  XLOG(ch, "Buf %p done (%s)\n", active_buf, (ret == 0) ? "ok" : "error");

  if (ret < 0) {
    // Buffer has error. Error info was saved by buffer_error_processing()
    // above (saved in g_idma_cntrl[ch].error). Resetting the channel will
    // not clear the error info.
    idma_stop_i(ch);
    idma_reset_i(ch);

    // Restore channel settings after reset.
    WRITE_IDMA_REG(ch, IDMA_REG_SETTINGS, g_idma_cntrl[ch].settings);
    WRITE_IDMA_REG(ch, IDMA_REG_TIMEOUT,  g_idma_cntrl[ch].timeout);

    // If there is no pending buffer after this one, clear channel state.
    // If there is a pending buffer then the update happens below.
    if (active_buf->next_task == NULL) {
      g_idma_cntrl[ch].oldest_task = NULL;
      g_idma_cntrl[ch].newest_task = NULL;
      g_idma_buf_ptr[ch] = NULL;
    }
  }

  // If there is a thread sleeping on this buffer, we have to wake it.
  // But don't do that here - we're in the middle of a critical section
  // and don't want to cause a context switch while the idma channel
  // state is being changed.

  // Schedule the next buffer if any. If the buffer completed OK and
  // there is no next buffer, unbind the buffer from the channel.
  if (active_buf->next_task != NULL) {
    idma_buf_t * next_buf = active_buf->next_task;

    g_idma_cntrl[ch].oldest_task = next_buf;
    g_idma_buf_ptr[ch] = next_buf;
    idma_disable_ii(ch);
    idma_set_start_address_i(ch, next_buf->next_desc);
    XLOG(ch, "Bind buf %p to ch %d\n", next_buf, ch);
    IDMA_ASSERT(next_buf->pending_desc_cnt > 0);
    update_next_desc_buf(next_buf, next_buf->pending_desc_cnt);
    hw_schedule(ch, next_buf->pending_desc_cnt);
    XLOG(ch, "Schedule %d desc from buf %p index %d\n",
             next_buf->pending_desc_cnt, next_buf, next_buf->cur_desc_i);
    tmp = next_buf->pending_desc_cnt;
    next_buf->pending = 0;
    next_buf->pending_desc_cnt = 0;
  }
  else {
    g_idma_buf_ptr[ch] = NULL;
    tmp = ret;
  }

  // If our buffer is now the active buffer, return the pending count.
  // Else return status (if complete) or pending count (if incomplete).
  if (buf == NULL) {
    ret = IDMA_ERR_NO_BUF;
  }
  else if (g_idma_buf_ptr[ch] == buf) {
    ret = tmp;
  }
  else {
    ret = (buf->pending > 0) ? buf->pending_desc_cnt : buf->status;
  }

  // Now wake the completed buffer's thread if needed.
  if (active_buf->sleeping > 0) {
    XLOG(ch, "thread %p wake\n", active_buf->thread_id);
    active_buf->sleeping = 0;
    idma_thread_unblock(active_buf->thread_id);
  }

  IDMA_ENABLE_INTS();
  return ret;

#endif
}

// If the application is compiled without defining IDMA_APP_USE_XTOS
// but then linked with libidma-xtos.a there is a problem because of
// schedule_thread_buffer() not being defined. To allow this mode to
// work, we'll define the function for XTOS mode here. It is a copy
// of schedule_desc(). Normally this would never get used because
// IDMA_APP_USE_XTOS must be defined if linking with libidma-xtos.a.
#ifdef IDMA_USE_XTOS
int32_t schedule_thread_buffer(int32_t ch, uint32_t count);

int32_t
schedule_thread_buffer(int32_t ch, uint32_t count)
{
  update_next_desc(ch, count);
  return schedule_desc_fast(ch, count);
}
#endif

