/** @file */

// xos_barrier.h - XOS Barrier API interface and data structures.

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

// NOTE: Do not include this file directly in your application. Including
// xos.h will automatically include this file.

#ifndef XOS_BARRIER_H
#define XOS_BARRIER_H

#include "xos_semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
///
/// XosBarrier object.
///
//-----------------------------------------------------------------------------
typedef struct XosBarrier {
  XosSem                sem;    ///< Control semaphore
  uint32_t              cnt;    ///< Number of threads to sync
  uint32_t XOS_ATOMIC   rem;    ///< Number of threads yet to arrive
  uint32_t              sig;    ///< Signature indicates valid object
} XosBarrier;


//-----------------------------------------------------------------------------
///
///  Initialize a barrier object before first use. Attempts to use a barrier
///  without initializing it will result in undefined behavior.
///
///  \param     bar             Pointer to barrier object.
///
///  \param     count           Count of the number of threads the barrier will
///                             synchronize. Must be > 0.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_barrier_create(XosBarrier * bar, uint32_t count);


//-----------------------------------------------------------------------------
///
///  Destroy a barrier object. Must have been previously created by calling
///  xos_barrier_create(). Destroying a barrier that has waiting threads will
///  cause the waits to return from xos_barrier_wait() with an error code.
///
///  \param     bar             Pointer to barrier object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_barrier_delete(XosBarrier * bar);


//-----------------------------------------------------------------------------
///
///  Wait at the barrier until all required threads have arrived at it.
///  The barrier must have been initialized. The last thread to arrive will
///  trigger barrier completion and release all waiting threads.
///
///  \param     bar             Pointer to barrier object.
///
///  \return    Returns XOS_OK on success, XOS_ERR_BAR_DELETE if the barrier
///             was deleted, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_barrier_wait(XosBarrier * bar);


#ifdef __cplusplus
}
#endif

#endif  // XOS_BARRIER_H

