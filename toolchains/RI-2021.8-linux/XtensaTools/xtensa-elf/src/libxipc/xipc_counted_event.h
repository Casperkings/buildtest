/* Copyright (c) 2003-2015 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __XIPC_COUNTED_EVENT_H__
#define __XIPC_COUNTED_EVENT_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xipc_common.h"

/*! Minimum size of a counted event object in bytes. */
#define XIPC_COUNTED_EVENT_STRUCT_SIZE (92)

#ifndef __XIPC_COUNTED_EVENT_INTERNAL_H__
#ifdef XIPC_CUSTOM_SUBSYSTEM
#include "xmp_custom_subsystem.h"
#else
#include <xtensa/system/xmp_subsystem.h>
#endif
struct xipc_counted_event_struct {
  char _[((XIPC_COUNTED_EVENT_STRUCT_SIZE+XMP_MAX_DCACHE_LINESIZE-1)/XMP_MAX_DCACHE_LINESIZE)*XMP_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned (XMP_MAX_DCACHE_LINESIZE)));
#endif

/*! 
 * Data structure used to setup a counted event. The minimum size of an 
 * object of this type is \ref XIPC_COUNTED_EVENT_STRUCT_SIZE. For a cached 
 * subsystem, the size is rounded and aligned to the maximum dcache line size 
 * across all cores in the subsystem. */
typedef struct xipc_counted_event_struct xipc_counted_event_t;

/*!
 * Initialize the event object. The counter is set to 0. The object needs to 
 * be initialized prior to its use.
 *
 * \param e         Pointer to event object.
 * \param wait_kind Spin-wait or sleep-wait when waiting on the event.
 * \return          XIPC_OK, if successful, else returns one of
 *                  - XIPC_ERROR_INTERNAL
 *                  - XIPC_ERROR_INVALID_ARG
 *                  - XIPC_ERROR_SUBSYSTEM_UNSUPPORTED
 */
extern xipc_status_t
xipc_counted_event_init(xipc_counted_event_t    *e,
                        xipc_wait_kind_t wait_kind);

/*!
 * This function sets an event by incrementing the event counter. Any core
 * that is sleep-waiting for the event count to be greater than or equal to the 
 * count on which they are waiting on gets notified using the \a XIPC-interrupt.
 *
 * \param e Pointer to event object.
 * \return  XIPC_OK, if successful, else returns XIPC_ERROR_EVENT_SET if event
 *          count overflows.
 */
extern xipc_status_t 
xipc_set_counted_event(xipc_counted_event_t *e);

/*!
 * The core can either do a \a spin-wait or \a sleep-wait waiting for the event 
 * counter's value to be greater than or equal to the parameter \a count. If 
 * sleep-waiting, any core that sets the event unblocks this core using the 
 * \a XIPC-interrupt if the event count has reached the value it is waiting for.
 * 
 * \param e     Pointer to event object.
 * \param count Wait for the event counter to reach this value.
 */
extern void 
xipc_wait_counted_event(xipc_counted_event_t *e, uint32_t count);

/*!
 * Wait for any event in a set of events to be set. The core can either do a 
 * \a spin-wait or \a sleep-wait waiting for the event 
 * counter's value to be greater than or equal to the corresponding \a count. 
 * If sleep-waiting, any core that sets the event unblocks this core using the 
 * \a XIPC-interrupt if the event count has reached the value it is waiting for.
 * Note, all events have to be either sleep or spin-waiting.
 * Note, the order of the events in the event set has to be the same when
 * called from multiple clients to avoid deadlocks.
 * 
 * \param events     Pointer to set of events.
 * \param counts     Wait for the event counter to reach this value 
 *                   for each event.
 * \param num_events Number of events in set.
 * \return           The index of the earliest event whose count is met.
 */
extern uint32_t 
xipc_wait_counted_event_any(xipc_counted_event_t *events[],
                            uint32_t counts[],
                            uint32_t num_events);

/*!
 * Read the current value of the event counter.
 *
 * \param e Pointer to event object.
 * \return  The current value of the event counter.
 */
extern uint32_t 
xipc_read_counted_event(xipc_counted_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* __XIPC_COUNTED_EVENT_H__ */
