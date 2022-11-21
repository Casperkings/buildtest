#include <stdio.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xmem_bank.h>
#include <xtensa/xos.h>
#include "xrp_api.h"
#include "xrp_dsp_hw.h"
#include "xrp_dsp_user.h"
#include "xdyn_lib.h"
#include "xdyn_lib_callback_if.h"

// Underlying RTOS preemption enable callback
static uint32_t rtos_enable_preemption()  { return xos_preemption_enable(); }

// Underlying RTOS preemption disable callback
static uint32_t rtos_disable_preemption() { return xos_preemption_disable(); }

static uint32_t rtos_get_thread_priority() { 
  return xos_thread_get_priority(XOS_THREAD_SELF); 
}

static uint32_t rtos_thread_id() { 
  return (uint32_t)xos_thread_id() & 0xffff; 
}

// Define the set of callback functions for the dynamic library
static xdyn_lib_callback_if xdyn_lib_cb_if = {
  .utils_cb_if     =  {xt_iss_profile_enable, xt_iss_profile_disable,
                       printf, snprintf, vsnprintf, abort, malloc,
                       realloc, free},
  .dma_cb_if       =  {idma_init_loop, idma_add_desc, idma_add_2d_desc,
                       idma_copy_2d_desc, idma_schedule_desc, idma_sleep,
                       idma_buffer_status, idma_desc_done,
                       idma_buffer_check_errors},
  .xrp_cb_if       =  {xrp_get_buffer_from_group, xrp_release_buffer,
                       xrp_map_buffer, xrp_unmap_buffer},
  .xmem_bank_cb_if =  {xmem_bank_alloc, xmem_bank_free,
                       xmem_bank_get_free_space, xmem_bank_checkpoint_save,
                       xmem_bank_checkpoint_restore, xmem_bank_get_alloc_policy,
                       xmem_bank_get_num_banks},
   .rtos_cb_if     =  {rtos_enable_preemption, rtos_disable_preemption,
                       rtos_get_thread_priority, rtos_thread_id}
};

static void hang(void) __attribute__((noreturn));
static void hang(void)
{
  for (;;)
    xrp_hw_panic();
}

void abort(void)
{
  printf("abort() is called; halting\n");
  hang();
}

static void xos_exception(XosExcFrame *frame)
{
  printf("%s: EXCCAUSE = %ld, EXCVADDR = 0x%08lx, PS = 0x%08lx, "
         "EPC1 = 0x%08lx\n", __func__,
         (unsigned long)frame->exccause, (unsigned long)frame->excvaddr,
         (unsigned long)frame->ps, (unsigned long)frame->pc);
  hang();
}

static void register_exception_handlers(void)
{
#if XCHAL_HAVE_XEA3
  static const int cause[] = {
    EXCCAUSE_INSTRUCTION,
    EXCCAUSE_ADDRESS,
    EXCCAUSE_HARDWARE,
    EXCCAUSE_MEMORY,
  };
#else
  static const int cause[] = {
    EXCCAUSE_ILLEGAL,
    EXCCAUSE_INSTR_ERROR,
    EXCCAUSE_LOAD_STORE_ERROR,
    EXCCAUSE_DIVIDE_BY_ZERO,
    EXCCAUSE_PRIVILEGED,
    EXCCAUSE_UNALIGNED,
    EXCCAUSE_INSTR_DATA_ERROR,
    EXCCAUSE_LOAD_STORE_DATA_ERROR,
    EXCCAUSE_INSTR_ADDR_ERROR,
    EXCCAUSE_LOAD_STORE_ADDR_ERROR,
    EXCCAUSE_ITLB_MISS,
    EXCCAUSE_ITLB_MULTIHIT,
    EXCCAUSE_INSTR_RING,
    EXCCAUSE_INSTR_PROHIBITED,
    EXCCAUSE_DTLB_MISS,
    EXCCAUSE_DTLB_MULTIHIT,
    EXCCAUSE_LOAD_STORE_RING,
    EXCCAUSE_LOAD_PROHIBITED,
    EXCCAUSE_STORE_PROHIBITED,
  };
#endif // XCHAL_HAVE_XEA3
  unsigned i;

  for (i = 0; i < sizeof(cause) / sizeof(cause[0]); ++i) {
    xos_register_exception_handler(cause[i], xos_exception);
  }
}

int main(void)
{
  enum xrp_status status;
  xdyn_lib_status_t xdyn_lib_status;
  xmem_bank_status_t xb_status;
  struct xrp_device *device;
  static uint32_t main_priority[] = {0, 1};

  register_exception_handlers();

  xrp_hw_init();

  // Initialize local memory manager. Reserve 16Kb for stack
  xb_status = xmem_init_local_mem(XMEM_BANK_HEAP_ALLOC, 16*1024);
  if (xb_status != XMEM_BANK_OK &&
      xb_status != XMEM_BANK_ERR_STACK_RESERVE_FAIL) {
    printf("PROC%d: main: Failed to initialize local memory\n", XT_RSR_PRID());
    return -1;
  }

#if XCHAL_HAVE_L2
  // Initialize L2 memory manager
  xb_status = xmem_init_l2_mem();
  if (xb_status != XMEM_BANK_OK) {
    printf("PROC%d: main: Failed to initialize L2 memory\n", XT_RSR_PRID());
    return -1;
  }
#endif

  // Init xrp device
  device = xrp_open_device(0, &status);
  if (status != XRP_STATUS_SUCCESS) {
    printf("PROC%d: xrp_open_device failed\n", XT_RSR_PRID());
    return -1;
  }

  // Initialize dynamic library manager
  xdyn_lib_status = xdyn_lib_init(device, &xdyn_lib_cb_if);
  if (xdyn_lib_status != XDYN_LIB_OK) {
    printf("PROC%d: xdyn_lib_init failed\n", XT_RSR_PRID());
    return -1;
  }

  status = xrp_user_create_queues(2, main_priority);

  xos_start(0);

  return 0;
}
