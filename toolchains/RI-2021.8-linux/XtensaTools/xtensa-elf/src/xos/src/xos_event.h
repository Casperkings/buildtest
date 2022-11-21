/** @file */

// xos_event.h - XOS Event API interface and data structures.

// Copyright (c) 2015-2020 Cadence Design Systems, Inc.
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

#ifndef XOS_EVENT_H
#define XOS_EVENT_H

#include "xos_types.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------
#define XOS_EVENT_BITS_ALL      0xFFFFFFFFU
#define XOS_EVENT_BITS_NONE     0U


//-----------------------------------------------------------------------------
// Event flags.
//-----------------------------------------------------------------------------
#define XOS_EVENT_AUTO_CLEAR    0x0010U ///< Auto-clear bits on thread signal
#define XOS_EVENT_WAIT_ALL      0x0001U
#define XOS_EVENT_WAIT_ANY      0x0002U


//-----------------------------------------------------------------------------
///
/// Event object.
///
//-----------------------------------------------------------------------------
typedef struct XosEvent {
  XosThreadQueue        waitq;          ///< Queue of waiters.
  uint32_t              bits;           ///< Event bits.
  uint32_t              mask;           ///< Specifies which bits are valid.
  uint16_t              flags;          ///< Properties.
  uint16_t              pad;            ///< Padding.
  uint32_t              sig;            // Valid signature indicates inited.
} XosEvent;


//-----------------------------------------------------------------------------
///
///  Initialize an event object before first use.
///
///  \param     event           Pointer to event object.
///
///  \param     mask            Mask of active bits. Only these bits can be signaled.
///
///  \param     flags           Creation flags:
///                             - XOS_EVENT_AUTO_CLEAR -- Automatically clear
///                               the set bits that signal a waiting thread.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_create(XosEvent * event, uint32_t mask, uint16_t flags);


//-----------------------------------------------------------------------------
///
///  Destroy an event object. Must have been previously created by calling
///  xos_event_create().
///
///  \param     event           Pointer to event object.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_delete(XosEvent * event);


//-----------------------------------------------------------------------------
///
///  Set the specified bits in the specified event. Propagates the bit states
///  to all waiting threads and wakes them if needed.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to set. Bits not set in the mask
///                             will not be modified by this call. To set all
///                             the bits in the event, use the constant
///                             XOS_EVENT_BITS_ALL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_set(XosEvent * event, uint32_t bits);


//-----------------------------------------------------------------------------
///
///  Clear the specified bits in the specified event. Propagates the bit states
///  to all waiting threads and wakes them if needed.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to clear. Every bit that is set in
///                             the mask will be cleared from the event. Bits
///                             not set in the mask will not be modified by this
///                             call. To clear all the bits in an event use the
///                             constant XOS_EVENT_BITS_ALL.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_clear(XosEvent * event, uint32_t bits);


//-----------------------------------------------------------------------------
///
///  Clear and set the specified bits in the specified event. The two steps are
///  combined into one update, so this is faster than calling xos_event_clear()
///  and xos_event_set() separately. Only one update is sent out to waiting
///  threads.
///
///  \param     event           Pointer to event object.
///
///  \param     clr_bits        Mask of bits to clear. The clear operation
///                             happens before the set operation.
///
///  \param     set_bits        Mask of bits to set.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_clear_and_set(XosEvent * event, uint32_t clr_bits, uint32_t set_bits);


//-----------------------------------------------------------------------------
///
///  Get the current state of the event object. This is a snapshot of the state
///  of the event at this time.
///
///  \param     event           Pointer to event object.
///
///  \param     pstate          Pointer to a uint32_t variable where the state
///                             will be returned.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_get(const XosEvent * event, uint32_t * pstate);


//-----------------------------------------------------------------------------
///
///  Wait until all the specified bits in the wait mask become set in the given
///  event object.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to test. Must not be all zeros.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_all(XosEvent * event, uint32_t bits);


//-----------------------------------------------------------------------------
///
///  Wait until all the specified bits in the wait mask become set in the given
///  event object, or the timeout expires.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to test. Must not be all zeros.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else
///             error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_all_timeout(XosEvent * event, uint32_t bits, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Wait until any of the specified bits in the wait mask become set in the
///  given event object.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to test. Must not be all zeros.
///
///  \return    Returns XOS_OK on success, else error code.   
///
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_any(XosEvent * event, uint32_t bits);


//-----------------------------------------------------------------------------
///
///  Wait until any of the specified bits in the wait mask become set in the
///  event object, or the timeout expires.
///
///  \param     event           Pointer to event object.
///
///  \param     bits            Mask of bits to test. Must not be all zeros.
///
///  \param     to_cycles       Timeout in cycles. Convert from time to cycles
///                             using the helper functions provided in xos_timer.
///                             A value of zero indicates no timeout.
///
///  \return    Returns XOS_OK on success, XOS_ERR_TIMEOUT on timeout, else
///             error code.
///
///  NOTE: If XOS_OPT_WAIT_TIMEOUT is not enabled, then the timeout value is
///  ignored, and no timeout will occur.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_wait_any_timeout(XosEvent * event, uint32_t bits, uint64_t to_cycles);


//-----------------------------------------------------------------------------
///
///  Set a specified group of bits, then wait for another specified group
///  of bits to become set.
///
///  \param     event           Pointer to event object.
///
///  \param     set_bits        Group of bits to set.
///
///  \param     wait_bits       Group of bits to wait on. All the bits in the
///                             group will have to get set before the wait is
///                             satisfied. Must not be all zeros.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_set_and_wait(XosEvent * event, uint32_t set_bits, uint32_t wait_bits);


//-----------------------------------------------------------------------------
///
///  Clear a specified group of bits, then wait for another specified group
///  of bits to become set.
///
///  \param     event           Pointer to event object.
///
///  \param     clear_bits      Group of bits to clear.
///
///  \param     wait_bits       Group of bits to wait on. All the bits in the
///                             group will have to get set before the wait is
///                             satisfied. Must not be all zeros.
///
///  \return    Returns XOS_OK on success, else error code.
///
//-----------------------------------------------------------------------------
int32_t
xos_event_clear_and_wait(XosEvent * event, uint32_t clear_bits, uint32_t wait_bits);


#ifdef XOS_INCLUDE_INTERNAL
XosEvent *
xos_eventq_to_event(XosThreadQueue * q);
#endif


#ifdef __cplusplus
}
#endif

#endif  //      XOS_EVENT_H

