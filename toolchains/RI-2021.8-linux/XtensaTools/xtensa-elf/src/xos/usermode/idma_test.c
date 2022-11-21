
// XOS example to demonstrate idma operation from user mode.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <xt_mld_loader.h>
#include <xtensa/xtutil.h>

// Need to use local version of XOS headers (and library)
#include "../include/xtensa/xos.h"

// Need this for ERACCESS read/write.
#include <xtensa/tie/xt_externalregisters.h>


// Assume a reasonable size for the thread stack.
// Also assume a default size for the dynamic library binary
// file to be read into memory.

#define STACK_SIZE    (1024*10)
#define FILE_SIZE     20000

// Test thread TCB and stack.

XosThread um_test_tcb;
uint8_t   um_test_stk[STACK_SIZE];


//-----------------------------------------------------------------------------
// Load a dynamic library image from file and return a pointer to the
// image data in memory.
//-----------------------------------------------------------------------------
static void *
load_dl_from_file(const char * filename)
{
    // Static buffer because we don't want to use malloc.
    static char file_buf[FILE_SIZE] __attribute__((aligned(4)));

    size_t fsize;
    size_t rsize;
    FILE * fp = fopen(filename, "rb");

    if (fp == NULL) {
        printf("Error: can't open file (%s) for loading.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    if (fsize >= FILE_SIZE) {
        printf("Error: file buf too small (%d) need (%d).\n", FILE_SIZE, fsize);
        return NULL;
    }

    rsize = fread(file_buf, 1, fsize, fp);
    if (rsize != fsize) {
        printf("Error: failed to read (%d) bytes from file (%s).\n", fsize, filename);
        return NULL;
    }

    fclose(fp);
    return file_buf;
}


//-----------------------------------------------------------------------------
// Allocate a block of memory from the user-mode pool. The block is composed
// of 3 chunks, of which the 3rd is assumed to be for stack and is aligned
// to 16 bytes. The others are also aligned to 16 for convenience.
//-----------------------------------------------------------------------------
static void
generate_address(void ** p1, uint32_t size1,
                 void ** p2, uint32_t size2,
                 void ** p3, uint32_t size3,
                 void ** pt, uint32_t * ptsize)
{
    uint32_t tsize;
    void *   ptr;
    int32_t  ret;

    // Round up to a multiple of the MPU region size.
    tsize = (size1 + size2 + size3 + XCHAL_MPU_ALIGN - 1) & -XCHAL_MPU_ALIGN;

    // Align allocation to MPU region size.
    ret = xos_um_private_mem_alloc(tsize, XCHAL_MPU_ALIGN, (void **)&ptr);
    if (ret || (ptr == XOS_NULL)) {
        printf("Error allocating um memory\n");
        exit(-1);
    }

    // Address and size of overall block.
    *pt = ptr;
    *ptsize = tsize;

    // Pointers to code/data/stack portions.
    *p3 = (void *) (((uint32_t)ptr + 15) & ~0xF);
    *p2 = (void *) (((uint32_t)(*p3) + size3 + 15) & ~0xF);
    *p1 = (void *) (((uint32_t)(*p2) + size2 + 15) & ~0xF);
}


//-----------------------------------------------------------------------------
// Create and run multiple user-mode threads (using the same dynamic library
// for all of them).
//-----------------------------------------------------------------------------
int32_t
um_test(void * arg, int32_t unused)
{
    char * dlname = (char *) arg;

    // See mpu_init.c
    extern void *   umem_start;
    extern uint32_t umem_size;
    extern uint32_t mpu_priv_mem_idx;

    // Pointer to library image in memory.
    xtmld_packaged_lib_t * plib;

    int32_t j;
    int32_t ret;

    xtmld_library_info_t info;
    xtmld_result_code_t  code;
    static xtmld_state_t state;
    static XosUThread *  ptcb;
    static char          name[16];

    void * data_memory[XTMLD_MAX_DATA_SECTIONS];
    void * code_memory[XTMLD_MAX_CODE_SECTIONS];
    void * stack_memory;
    void * vstart;

    XosThreadFunc * entry_func;

    printf("idma_test: start\n");

    // Init XOS user-mode support.
    xos_um_init(umem_start, umem_size);

    // Load image from file.
    plib = load_dl_from_file(dlname);
    if (plib == NULL) {
        printf("FAIL\n");
        exit(-1);
    }

    // Extract library info.
    code = xtmld_library_info(plib, &info);
    if (code != xtmld_success) {
        printf("xtmld_library_info failed: error %d\n", code);
        exit(-1);
    }

#if 0
    if ((info.number_of_code_sections != 1) || (info.number_of_data_sections != 1)) {
        printf("idma_test: expects a dyn library with 1 code and 1 data section\n");
        exit(-1);
    }
#endif

    void *   tmem;
    uint32_t tsize;

    // Allocate a new set of memory regions to load the library into.
    generate_address(&code_memory[0], info.code_section_size[0],
                     &data_memory[0], info.data_section_size[0],
                     &stack_memory, STACK_SIZE,
                     &tmem, &tsize);

    printf("idma_test: load library:\n"
           "  allocated 0x%x bytes @ 0x%08x\n"
           "  code  @ 0x%08x - 0x%08x\n  data  @ 0x%08x - 0x%08x\n  stack @ 0x%08x - 0x%08x\n",
           tsize, tmem,
           code_memory[0], code_memory[0] + info.code_section_size[0] - 1,
           data_memory[0], data_memory[0] + info.data_section_size[0] - 1,
           stack_memory, stack_memory + STACK_SIZE);

    // Hack for dataram. Assume that there are 2 data sections in the library
    // and the second one is the dataram section. Point the load address to
    // the start of free dram0.
    extern char _memmap_mem_dram0_max;
    data_memory[1] = &_memmap_mem_dram0_max;

    // Load the library.
    code = xtmld_load(plib, code_memory, data_memory, &state, &vstart);
    entry_func = (XosThreadFunc *) vstart;

    if ((code != xtmld_success) || (entry_func == NULL)) {
        printf("xtmld_load failed or no entry point\n");
        exit(-1);
    }

    printf("library load OK\n");

    // Grant access to idma channel 0. Enable ERI access to device 1
    // by writing ERACCESS, and set the channel's userpriv register
    // to allow it to be visible from user mode _and_ restrict all
    // transfers to user privilege.

    XT_WSR_ERACCESS(0xFF);

    XT_WER(0x80000003, 0x00110018); // hardcoded ch0 register address and value

    // Here we should call idma_init() for channel 0 and set things up.
    // For now, we can afford to skip it because the default state of
    // the channel is usable.

    // Allocate TCB
    ptcb = xos_um_tcb_alloc();

    sprintf(name, "uthd0");

    ret = xos_uthread_create_p(ptcb,
                               entry_func,
                               (void *)0, // idma chan num
                               0,
                               0,
                               name,
                               stack_memory,
                               STACK_SIZE,
                               1,
                               0,
                               tmem,
                               tsize,
                               mpu_priv_mem_idx);
    if (ret) {
        xos_fatal_error(-1, "Error creating user threads\n");
    }

    ret = xos_thread_join(xos_uthread_thread(ptcb), &j);
    printf("User thread returned %d\n", j);

    xos_uthread_delete_p(ptcb);
    xos_um_tcb_free(ptcb);

    // Unload.
    printf("unloading library\n");
    xtmld_unload(&state);

    exit(0);
}


//-----------------------------------------------------------------------------
//  Main.
//-----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
    int32_t ret;
    char *  dlname;

    dlname = (argc > 1) ? argv[1] : "example_idma.dynlib";

#if XCHAL_HAVE_MPU
    extern int32_t mpu_init(void);

    if (mpu_init() != 0) {
        printf("MPU init failed\n");
        return -1;
    }
#endif

    ret = xos_thread_create(&um_test_tcb,
                            0,
                            um_test,
                            dlname,
                            "um_test",
                            um_test_stk,
                            STACK_SIZE,
                            7,
                            0,
                            0);
    if (ret != XOS_OK) {
        printf("Test thread create failed\n");
        return -1;
    }

    xos_set_clock_freq(XOS_CLOCK_FREQ);
    ret = xos_start_system_timer_ex(-1, 10000, 1000);
    if (ret != XOS_OK) {
        printf("Start system timer failed\n");
        return -1;
    }

    xos_start(0);
    return -1; // should never get here
}

