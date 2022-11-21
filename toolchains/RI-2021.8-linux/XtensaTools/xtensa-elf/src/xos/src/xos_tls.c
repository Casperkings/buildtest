
// xos_tls.c - Thread Local Storage API.

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


#define XOS_INCLUDE_INTERNAL
#include "xos.h"


//-----------------------------------------------------------------------------
// TLS cleanup function pointer type.
//-----------------------------------------------------------------------------
typedef void (XosTLSFunc)(void * arg);


#if XOS_OPT_THREAD_LOCAL_STORAGE
static XosTLSFunc * tls_funcs[XOS_TLS_NUM_KEYS];
static uint32_t     kidx;
#endif


//-----------------------------------------------------------------------------
// Create TLS key and record destructor function.
//-----------------------------------------------------------------------------
int32_t
xos_tls_create(uint32_t * key, void (*destructor)(void * arg))
{
#if XOS_OPT_THREAD_LOCAL_STORAGE
    uint32_t ps;
    int32_t  ret;
#endif

    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

#if XOS_OPT_THREAD_LOCAL_STORAGE
    ps = xos_critical_enter();

    if (kidx < XOS_TLS_NUM_KEYS) {
        if (key != XOS_NULL) {
            *key = kidx;
            tls_funcs[kidx] = destructor;
            kidx++;
            ret = XOS_OK;
        }
        else {
            // Invalid key ptr
            ret = XOS_ERR_INVALID_PARAMETER;
        }
    }
    else {
        // No more keys
        ret = XOS_ERR_LIMIT;
    }

    xos_critical_exit(ps);
    return ret;
#else
    UNUSED(key);
    UNUSED(destructor);
    return XOS_ERR_FEATURE_NOT_PRESENT;
#endif
}


//-----------------------------------------------------------------------------
// Delete TLS key and remove destructor function.
//-----------------------------------------------------------------------------
int32_t
xos_tls_delete(uint32_t key)
{
    if (key < XOS_TLS_NUM_KEYS) {
        uint32_t ps;

        ps = xos_critical_enter();
        tls_funcs[key] = XOS_NULL;
        xos_critical_exit(ps);
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
}


//-----------------------------------------------------------------------------
// Return TLS value for this thread.
//-----------------------------------------------------------------------------
void *
xos_tls_get(uint32_t key)
{
    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

#if XOS_OPT_THREAD_LOCAL_STORAGE
    if (key < XOS_TLS_NUM_KEYS) {
        return xos_thread_id()->tls[key];
    }

    return XOS_NULL;
#else
    UNUSED(key);
    return XOS_NULL;
#endif
}


//-----------------------------------------------------------------------------
// Set TLS value for this thread.
//-----------------------------------------------------------------------------
int32_t
xos_tls_set(uint32_t key, void * value)
{
    // Can't call this from interrupt context.
    if (INTERRUPT_CONTEXT) {
        xos_fatal_error(XOS_ERR_INTERRUPT_CONTEXT, XOS_NULL);
    }

#if XOS_OPT_THREAD_LOCAL_STORAGE
    if (key < XOS_TLS_NUM_KEYS) {
        xos_thread_id()->tls[key] = value;
        return XOS_OK;
    }

    return XOS_ERR_INVALID_PARAMETER;
#else
    UNUSED(key);
    UNUSED(value);
    return XOS_ERR_FEATURE_NOT_PRESENT;
#endif
}


//-----------------------------------------------------------------------------
// Run TLS cleanup for exiting thread.
//-----------------------------------------------------------------------------
#if XOS_OPT_THREAD_LOCAL_STORAGE
void
xos_tls_cleanup(void)
{
    uint32_t    i;
    XosThread * thread = xos_thread_id();

    for (i = 0; i < kidx; i++) {
        if ((thread->tls[i] != XOS_NULL) && (tls_funcs[i] != XOS_NULL)) {
            // Clear out the stored value before the destructor is called.
            // But call the destructor with the original value.
            void * tmp = thread->tls[i];

            thread->tls[i] = XOS_NULL;
            (*tls_funcs[i])(tmp);
        }
    }
}
#endif

