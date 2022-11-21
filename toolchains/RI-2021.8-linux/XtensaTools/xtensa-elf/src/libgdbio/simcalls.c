/*
 Copyright (c) 2003-2017 Cadence Design Systems, Inc.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// Misc system calls supported by the simulator. These are passed
// through if libgdbio detects that we are running on the simulator.
// If not, they will do nothing and return an error if appropriate.

#include <xtensa/config/core.h>

#include "libio.h"

void 
xt_profile_init(void)
{
    // Does nothing.
}


void 
xt_profile_add_memory(void * buf, unsigned int buf_size)
{
    // Does nothing.
}


void
xt_profile_enable (void)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_profile_enable;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
		          : "=a" (ret_val), "=a" (ret_err)
		          : "a" (a2));
    }
}


void
xt_profile_disable (void)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_profile_disable;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2));
    }
}


void
xt_iss_trace_level (unsigned level)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_trace_level;
        register unsigned a3 __asm__ ("a3") = level;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2), "a" (a3));
    }
}


int
xt_iss_client_command(const char *client, const char *command)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_client_command;
        register unsigned a3 __asm__ ("a3") = (unsigned)client;
        register unsigned a4 __asm__ ("a4") = (unsigned)command;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2), "a" (a3), "a" (a4));
        return ret_val;
    }

    return -1;
}


unsigned long long
xt_iss_cycle_count()
{
    if (libio_mode == LIBIO_SIM) {
        register unsigned int a2_in  __asm__ ("a2") = SYS_cycle_count;
        register unsigned int a2_out __asm__ ("a2");
        register unsigned int a3_out __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (a2_out), "=a" (a3_out)
                          : "a" (a2_in));
#if XCHAL_HAVE_BE
        return (((unsigned long long)a2_out) << 32) | a3_out;
#else
        return (((unsigned long long)a3_out) << 32) | a2_out;
#endif
    }

    // Note this is 32 bits only, will roll over.
    return xthal_get_ccount();
}


int
xt_iss_switch_mode(int mode)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_sim_mode_switch;
        register int a3 __asm__ ("a3") = mode;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2), "a" (a3));
        return ret_val;
    }

    return -1;
}


void
xt_iss_event_fire(unsigned int event_id)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_event_fire;
        register unsigned int a3 __asm__ ("a3") = event_id;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2), "a" (a3));
    }
}


void
xt_iss_event_wait(unsigned int event_id)
{
    if (libio_mode == LIBIO_SIM) {
        register int a2 __asm__ ("a2") = SYS_event_stall;
        register unsigned int a3 __asm__ ("a3") = event_id;
        register int ret_val __asm__ ("a2");
        register int ret_err __asm__ ("a3");
        __asm__ volatile ("simcall"
                          : "=a" (ret_val), "=a" (ret_err)
                          : "a" (a2), "a" (a3));
    }
}


int
xt_profile_save_and_reset(void)
{
    if (libio_mode == LIBIO_SIM) {
        if (xt_iss_client_command("profile", "save") == 0)
            return xt_iss_client_command("profile", "reset");
    }

    return -1;
}


unsigned int 
xt_profile_get_frequency(void)
{
  return 1;
}


void 
xt_profile_set_frequency(unsigned int sample_frequency)
{
    // Does nothing.
}


int 
xt_profile_num_errors(void)
{
    // Always return 0.
    return 0;
}

