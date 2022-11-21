
// xos_idma.c - XOS interface for multi-thread libidma support.

// Copyright (c) 2019-2020 Cadence Design Systems, Inc.
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

//-----------------------------------------------------------------------------
//  XOS interface to the libidma OS functions. This allows XOS threads to use
//  iDMA channels safely. If you wish to provide your own implementation then
//  this one is easily overridden from application code. However make sure to
//  provide a full replacement for the public API.
//-----------------------------------------------------------------------------


#include "xos.h"

#if XCHAL_HAVE_IDMA

#include <xtensa/idma.h>


//-----------------------------------------------------------------------------
//  Allow one buffer per channel per thread. Normally this is in excess of what
//  is actually needed, so adjust as necessary. Most threads will end up using
//  only one channel at a time, and not all threads in an application even use
//  iDMA.
//  Thread-local storage is currently not used here because the assumption is
//  that most threads will not use iDMA, so it is not useful to tie up one or
//  more TLS slots in every thread.
//-----------------------------------------------------------------------------

typedef struct idma_buf_info {
    XosThread *  thread;
    idma_buf_t * buf_list[XCHAL_IDMA_NUM_CHANNELS];
} idma_buf_info;

//-----------------------------------------------------------------------------
//  Statically allocate some number of thread buffer info structs. Adjust this
//  as needed.
//-----------------------------------------------------------------------------

#define MAX_THREADS     8

static ALIGNDCACHE idma_buf_info buf_info[MAX_THREADS];


//-----------------------------------------------------------------------------
//  idma_register_interrupts
//-----------------------------------------------------------------------------
int32_t
idma_register_interrupts(int32_t ch, os_handler done_handler, os_handler err_handler)
{
#if XCHAL_HAVE_INTERRUPTS
    if (ch < XCHAL_IDMA_NUM_CHANNELS) {
        uint32_t doneint = (uint32_t) XCHAL_IDMA_CH0_DONE_INTERRUPT + (uint32_t) ch;
        uint32_t errint  = (uint32_t) XCHAL_IDMA_CH0_ERR_INTERRUPT + (uint32_t) ch;
        int32_t  ret     = 0;

        ret += xos_register_interrupt_handler(doneint, done_handler, cvt_int32_to_voidp(ch));
        ret += xos_interrupt_enable(doneint);

        ret += xos_register_interrupt_handler(errint, err_handler, cvt_int32_to_voidp(ch));
        ret += xos_interrupt_enable(errint);

        return (ret == XOS_OK) ? 0 : -1;
    }

    return -1;
#else
    return -1;
#endif
}


//-----------------------------------------------------------------------------
//  idma_disable_interrupts
//-----------------------------------------------------------------------------
uint32_t
idma_disable_interrupts(void)
{
    return xos_disable_interrupts();
}


//-----------------------------------------------------------------------------
//  idma_enable_interrupts
//-----------------------------------------------------------------------------
void
idma_enable_interrupts(uint32_t level)
{
    xos_restore_interrupts(level);
}


//-----------------------------------------------------------------------------
//  idma_thread_id
//-----------------------------------------------------------------------------
void *
idma_thread_id(void)
{
    return xos_thread_id();
}


//-----------------------------------------------------------------------------
//  idma_thread_block
//-----------------------------------------------------------------------------
void
idma_thread_block(void * thread)
{
    if (thread != NULL) {
        (void) xos_thread_suspend((XosThread *) thread); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Type conversion dictated by API"
    }
}


//-----------------------------------------------------------------------------
//  idma_thread_unblock
//-----------------------------------------------------------------------------
void
idma_thread_unblock(void * thread)
{
    if (thread != NULL) {
        (void) xos_thread_resume((XosThread *) thread); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Type conversion dictated by API"
    }
}


//-----------------------------------------------------------------------------
//  idma_chan_buf_set
//-----------------------------------------------------------------------------
void
idma_chan_buf_set(int32_t ch, idma_buf_t * buf)
{
    if (ch < XCHAL_IDMA_NUM_CHANNELS) {
        XosThread * thread = xos_thread_id();
        int32_t     j      = -1;
        int32_t     i;
        uint32_t    ps;

        // Find a free slot and allocate it. This requires interrupts
        // disabled because we're manipulating global shared data.
        ps = xos_disable_interrupts();

        for (i = 0; i < MAX_THREADS; i++) {
            if (buf_info[i].thread == thread) {
                break;
            }
            if ((buf_info[i].thread == NULL) && (j < 0)) {
                j = i;
            }
        }
        if (i < MAX_THREADS) {
            buf_info[i].buf_list[ch] = buf;
        }
        else if (j >= 0) {
            buf_info[j].thread = thread;
            buf_info[j].buf_list[ch] = buf;
        }
        else {
            xos_fatal_error(XOS_ERR_LIMIT, "idma_chan_buf_set");
        }

        xos_restore_interrupts(ps);
    }
}


//-----------------------------------------------------------------------------
//  idma_chan_buf_get
//-----------------------------------------------------------------------------
idma_buf_t *
idma_chan_buf_get(int32_t ch)
{
    if (ch < XCHAL_IDMA_NUM_CHANNELS) {
        XosThread * thread = xos_thread_id();
        int32_t     i;

        for (i = 0; i < MAX_THREADS; i++) {
            if (buf_info[i].thread == thread) {
                return buf_info[i].buf_list[ch];
            }
        }
    }

    return NULL;
}

#endif // XCHAL_HAVE_IDMA

