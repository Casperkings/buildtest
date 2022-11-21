
// xos_once.c - Implements the once() function.

// Copyright (c) 2018-2020 Cadence Design Systems, Inc.
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
// Single global mutex used to serialize the once() process. Could lead to some
// contention but presumably most applications don't do a lot of once-inits.
//-----------------------------------------------------------------------------
static XosMutex once_mtx;


//-----------------------------------------------------------------------------
// Init the mutex before first use.
//-----------------------------------------------------------------------------
void
xos_once_init(void)
{
    (void) xos_mutex_create(&once_mtx, XOS_MUTEX_WAIT_PRIORITY, 0);
}


//-----------------------------------------------------------------------------
// Handle once() processing.
//-----------------------------------------------------------------------------
static int32_t
xos_once_do(uint32_t * once_control, void (*init_routine)(void))
{
    int32_t ret;

    // Attempt to lock the mutex. If we fail, some other thread is
    // executing an init routine and we must wait.

    ret = xos_mutex_lock(&once_mtx);
    if (ret != XOS_OK) {
        return ret;
    }

    // Once we get here, our once_control is not contended. It must
    // be either in INIT or DONE state. If it were INPROGRESS then
    // the mutex would have been locked.

    if (*once_control == XOS_ONCE_INIT) {
        *once_control = XOS_ONCE_INPROGRESS;
        init_routine();
        *once_control = XOS_ONCE_DONE;
    }
    else if (*once_control == XOS_ONCE_DONE) {
        // Nothing to do.
    }
    else {
        xos_fatal_error(XOS_ERR_INTERNAL_ERROR, XOS_NULL);
    }

    return xos_mutex_unlock(&once_mtx);
}


//-----------------------------------------------------------------------------
// Parameter validation and quick check. If init done already we don't need to
// try and lock the mutex.
//-----------------------------------------------------------------------------
int32_t
xos_once(uint32_t * once_control, void (*init_routine)(void))
{
    if ((once_control == XOS_NULL) || (init_routine == XOS_NULL)) {
        return XOS_ERR_INVALID_PARAMETER;
    }

    if (*once_control == XOS_ONCE_DONE) {
        return XOS_OK;
    }

    return xos_once_do(once_control, init_routine);
}

