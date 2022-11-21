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
#include "xtensa/secmon-defs.h"

#include <xtensa/config/secure.h>
#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <stdio.h>


#define UNUSED(x)       ((void)(x))

#if (defined XCHAL_HAVE_SECURE_INSTRAM1) && XCHAL_HAVE_SECURE_INSTRAM1
#define SECURE_START    XCHAL_INSTRAM1_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTRAM0) && XCHAL_HAVE_SECURE_INSTRAM0
#define SECURE_START    XCHAL_INSTRAM0_SECURE_START
#elif (defined XCHAL_HAVE_SECURE_INSTROM0) && XCHAL_HAVE_SECURE_INSTROM0
#define SECURE_START    XCHAL_INSTROM0_SECURE_START
#endif


// Prototypes
int32_t issue_syscall(int32_t id);
int32_t run_test_window_spill(void);
int32_t test_window_spill(uint32_t a2, uint32_t a3, uint32_t a4);


// Must not be on stack
const uint32_t spill_key = 0x007eed1e;


int32_t issue_syscall(int32_t id)
{
    int32_t ret;
    __asm__ __volatile__ ( "or a2, %1, %1   \n\t \
                            syscall         \n\t \
                            or %0, a2, a2   \n\t" : "=a" (ret) : "a" (id) : "a2" );
    return ret;
}

int32_t test_window_spill(uint32_t a2, uint32_t a3, uint32_t a4)
{
    int32_t ret;
    int32_t *sp_traverse;

    extern int32_t __stack;
    int32_t *stack_top = &__stack;

    // ret + 12B locals + 16B extra save area + 64B issue_syscall stack frame
    int32_t *stack_bot = (int32_t *)((char *)&ret - 12 - 16 - 64);

    UNUSED(a2);
    UNUSED(a3);
    UNUSED(a4);

    // Confirm spill value doesn't exist on stack
    for (sp_traverse = stack_bot; sp_traverse < stack_top; sp_traverse++) {
        if (*sp_traverse == spill_key) {
            printf("Error: spill key found at %p (stack top %p, bottom %p)\n",
                    (void *)sp_traverse, (void *)stack_top, (void *)stack_bot);
            return -1;
        }
    }

    // Load key into a4 using inline assembly to ensure it's not on stack.
    // Can't use a0-a3 since they are not spilled as the active register block.
    __asm__ __volatile__ ( "mov a4, %0" :: "a"(spill_key));

    // Spill all registers, which will write key (parameter/a2) to stack
    ret = issue_syscall(0);
    if (ret) {
        printf("Error: syscall(0) returned %d\n", ret);
        return -2;
    }

    // Confirm spill value exists on stack
    for (sp_traverse = stack_bot; sp_traverse < stack_top; sp_traverse++) {
        if (*sp_traverse == spill_key) {
            printf("INFO: spill key found at %p (stack top %p, bottom %p)\n",
                    (void *)sp_traverse, (void *)stack_top, (void *)stack_bot);
            return 0;
        }
    }
    printf("Error: key not found after spill\n");
    return -3;
}

int32_t run_test_window_spill()
{
    printf("Testing register spill (syscall 0)\n");
    return test_window_spill(0, 0, 0);
}

int main(int argc, char **argv)
{
    int ret, ret_default, ret_nonsecure, ret_dereg, ret_spill = 0;

    UNUSED(argc);
    UNUSED(argv);

    printf("Testing secure/nonsecure syscall handling\n");
    ret = xtsm_init();
    if (ret != 0) {
        printf("Error: xtsm_init() FAILED (%d)\n", ret);
        return ret;
    }

    // Behavior of unhandled secure syscalls is to return -1
    printf("Testing illegal syscall handler\n");
    if (xtsm_set_syscall_handler((xtos_handler)SECURE_START) == 0) {
        printf("Error: Register nonsecure syscall should have failed but did not\n");
        return -1;
    }

    printf("Testing unhandled syscall \n");
    ret_default = (issue_syscall(3) == -1) ? 0 : 1;
    printf("Secure syscall handling: %s\n", (ret_default == 0) ? "OK" : "FAILED");


#if (defined __XTENSA_WINDOWED_ABI__)
    printf("Testing window spill syscall(0) \n");
    ret_spill = run_test_window_spill();
    printf("syscall(0) (window spill): %s\n", (ret_spill == 0) ? "OK" : "FAILED");
#endif

    // Test default (XTOS) nonsecure syscall handling
    ret_nonsecure = xtsm_set_syscall_handler(XTSM_SYSCALL_HANDLER_DEFAULT);
    printf("xtsm_set_syscall_handler(DEFAULT): %s\n", (ret_nonsecure == 0) ? "OK" : "Error");

    if (ret_nonsecure == 0) {
#if (defined __XTENSA_WINDOWED_ABI__)
        // For windowed ABIs, behavior of nonsecure syscall(0) (stack spill) is to return 0.
        ret_nonsecure |= (issue_syscall(0) == 0) ? 0 : 2;
#else
        printf("Call0 ABI requires manual intervention.  Add breakpoint to _SyscallException\n");
#endif
        printf("Nonsecure default (XTOS) syscall handling test: %s\n", 
                (ret_nonsecure == 0) ? "OK" : "FAILED");
    }


    // Test syscall deregistering 
    ret_dereg = xtsm_set_syscall_handler(NULL);
    printf("xtsm_set_syscall_handler(NULL): %s\n", (ret_dereg == 0) ? "OK" : "Error");
    if (ret_dereg == 0) {
        ret_dereg = (issue_syscall(0) == -1) ? 0 : 1;
        printf("Secure syscall handling (unregistered): %s\n", (ret_dereg == 0) ? "OK" : "FAILED");
    }

    printf("INFO: reserved syscall range: 0x%x - 0x%x\n", SECMON_SYSCALL_RSVD_FIRST, SECMON_SYSCALL_RSVD_LAST);
    printf("INFO: custom   syscall range: 0x%x - 0x%x\n", SECMON_SYSCALL_CUSTOM_FIRST, SECMON_SYSCALL_CUSTOM_LAST);
    printf("INFO: total    syscall range: 0x%x - 0x%x\n", SECMON_SYSCALL_FIRST, SECMON_SYSCALL_MAX);
    printf("INFO: syscall mask: 0x%x\n", SECMON_SYSCALL_MASK);

    printf("syscall() exception test: %s\n", 
            (ret_default || ret_nonsecure || ret_dereg || ret_spill) ? "FAILED" : "PASSED");
    return (ret_default || ret_nonsecure || ret_dereg || ret_spill);
}

