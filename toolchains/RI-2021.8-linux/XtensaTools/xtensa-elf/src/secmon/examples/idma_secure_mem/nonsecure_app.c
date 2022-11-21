// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

// nonsecure_app.c -- Nonsecure application example code


#include "xtensa/secmon.h"

#include <xtensa/config/secure.h>
#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <xtensa/idma.h>
#include <stdio.h>
#include <stdint.h>

#define UNUSED(x)   ((void)(x))

#if (XCHAL_HAVE_IDMA)

#define IDMA_XFER_SIZE   (32)

#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
#define SECURE_START    XCHAL_INSTRAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
#define SECURE_START    XCHAL_INSTRAM0_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTROM0) && XCHAL_HAVE_SECURE_INSTROM0
#define SECURE_START    XCHAL_INSTROM0_SECURE_START
#endif

#if (defined XCHAL_HAVE_SECURE_DATARAM1) && XCHAL_HAVE_SECURE_DATARAM1
#define SECURE_DSTART       XCHAL_DATARAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_DATARAM0) && XCHAL_HAVE_SECURE_DATARAM0
#define SECURE_DSTART       XCHAL_DATARAM0_SECURE_START
#else
#error Unable to locate secure segment start
#endif

#if (defined XCHAL_DATARAM1_VADDR) && (XCHAL_DATARAM1_VADDR != SECURE_DSTART)
#define NONSECURE_DSTART    XCHAL_DATARAM1_VADDR
#define NONSECURE_SEC       ".dram1.data"
#elif (defined XCHAL_DATARAM0_VADDR) && (XCHAL_DATARAM0_VADDR != SECURE_DSTART)
#define NONSECURE_DSTART    XCHAL_DATARAM0_VADDR
#define NONSECURE_SEC       ".dram0.data"
#else
#error Unable to locate nonsecure segment
#endif

// iDMA buffer
static IDMA_BUFFER_DEFINE(idma_buffer, 2, IDMA_1D_DESC);

static uint8_t sysmem_source_buf [IDMA_XFER_SIZE] __attribute__((section(".sram.data"))) = { 0 };
static uint8_t dram_nonsecure_dest_buf [IDMA_XFER_SIZE] __attribute__((section(NONSECURE_SEC)));

static int          do_idma_err_test = 0;
static volatile int idma_err_count = 0;

static void xtsm_idma_err_handler ( void* arg )
{
    UNUSED(arg);
}

static void idma_err_handler ( const struct idma_error_details_struct *err )
{
    UNUSED(err);

    printf("iDMA error encountered\n");

    // increment the error count
    ++idma_err_count;
}

static int idma_test(void)
{
    int idma_status;

    printf("Testing nonsecure iDMA ...\n");

    // iDMA init
    idma_init(0,                                // flags
              MAX_BLOCK_16,                     // max block size
              XCHAL_IDMA_MAX_OUTSTANDING_REQ,   // max outstanding bus requests
              0,                                // ticks per cycle
              0,                                // timeout ticks
              idma_err_handler);                // iDMA error handler
    
    idma_init_loop(idma_buffer,     // iDMA buffer
                   IDMA_1D_DESC,    // type of iDMA transfers
                   2,               // size of iDMA buffer
                   0,               // callback data 
                   0);              // callback routine   

    // 2 descriptors - one in non-secure, one in secure
    idma_add_desc(idma_buffer, (void *)dram_nonsecure_dest_buf, sysmem_source_buf, IDMA_XFER_SIZE, 0);
    idma_add_desc(idma_buffer, (void *)SECURE_DSTART, sysmem_source_buf, IDMA_XFER_SIZE, 0);

    // iDMA operations from system memory to non-secure local DRAM
    printf("iDMA transfer from system memory to non-secure local DRAM ...\n");

    while ((idma_status = idma_buffer_status()) > 0) {
    }
    
    idma_schedule_desc(1);
    
    while ((idma_status = idma_buffer_status()) > 0) {
    }
    
    if (idma_status == 0) {
        printf("iDMA transfer from system memory to non-secure local DRAM: SUCCESS\n");
    } else {
        printf("iDMA transfer from system memory to non-secure local DRAM: ERROR (%d)\n", idma_status);
    }
  
    // iDMA operations from system memory to non-secure local DRAM
    printf("iDMA transfer from system memory to secure local DRAM ...\n");

    while ((idma_status = idma_buffer_status()) > 0) {
    }
    
    idma_schedule_desc(1);
    
    while ((idma_status = idma_buffer_status()) > 0) {
    }
    
    if (idma_status == 0) {
        printf("iDMA transfer from system memory to secure local DRAM: ERROR (iDMA op returned success)\n");
    } else {
        printf("iDMA transfer from system memory to non-secure local DRAM: SUCCESS (iDMA correctly flagged error %d)\n", idma_status);
    }

    return 0;
}

#endif

int main(int argc, char **argv)
{
    int ret = 0;

    UNUSED(argc);
    UNUSED(argv);

#if (XCHAL_HAVE_IDMA)
    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Install non-secure interrupt handler for IDMA error for test purposes
    ret = xtsm_set_interrupt_handler(XCHAL_IDMA_CH0_ERR_INTERRUPT, xtsm_idma_err_handler, NULL);
    if (ret == 0) {
        do_idma_err_test = 1;

        // enable the iDMA error interrupt
        xtos_interrupt_enable(XCHAL_IDMA_CH0_ERR_INTERRUPT);
    }

    printf("Hello, nonsecure world\n");
    if ((Xthal_intlevel[XCHAL_IDMA_CH0_DONE_INTERRUPT] > XCHAL_EXCM_LEVEL) ||
        (Xthal_intlevel[XCHAL_IDMA_CH0_ERR_INTERRUPT] > XCHAL_EXCM_LEVEL)) {
        printf("IDMA test PASSED (skipping): DONE int level %d, ERR int level %d (must be <= EXCM_LEVEL, i.e. %d)\n",
                Xthal_intlevel[XCHAL_IDMA_CH0_DONE_INTERRUPT], 
                Xthal_intlevel[XCHAL_IDMA_CH0_ERR_INTERRUPT], 
                XCHAL_EXCM_LEVEL);
        return 0;
    }
    ret = idma_test();
    printf("Test %s : handled (%d) iDMA error interrupts\n", ret ? "FAILED" : "PASSED", idma_err_count);
#else
    printf("Skipping IDMA test (IDMA not configured); test PASSED\n");
#endif

    return ret;
}

