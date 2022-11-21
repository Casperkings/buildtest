/* Copyright (c) 2003-2015 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems, Inc. They may not be modified, copied, reproduced, 
 * distributed, or disclosed to third parties in any manner, medium, or form, 
 * in whole or in part, without the prior written consent of Cadence Design 
 * Systems Inc.
 */

#include <xtensa/xipc/xipc_primitives.h>
#include "xmp_log.h"
#include "shared.h"

/* Put an integer element to queue */
void put(buffer_t *buffer, int x)
{
  xipc_mutex_acquire(&buffer->lock);
  XIPC_INVALIDATE_ELEMENT(&buffer->count);
  // Wait if the queue is full on the not_full condition object */
  while (buffer->count == NUM_ITEMS) {
    xipc_cond_wait(&buffer->not_full, &buffer->lock);
    XIPC_INVALIDATE_ELEMENT(&buffer->count);
  }
  XIPC_INVALIDATE_ELEMENT(buffer);
  buffer->data[buffer->put_ptr] = x;
  // Wrap around circular buffer
  if (++buffer->put_ptr == NUM_ITEMS)
    buffer->put_ptr = 0;
  buffer->count++;
  XIPC_WRITE_BACK_ELEMENT(buffer);
  // Notify consumer
  xipc_cond_signal(&buffer->not_empty);
  xipc_mutex_release(&buffer->lock);
}

int take(buffer_t *buffer)
{
  int x;
  xipc_mutex_acquire(&buffer->lock);
  XIPC_INVALIDATE_ELEMENT(&buffer->count);
  // Wait if the queue is empty on the not_empty condition object */
  while (buffer->count == 0) {
    xipc_cond_wait(&buffer->not_empty, &buffer->lock);
    XIPC_INVALIDATE_ELEMENT(&buffer->count);
  }
  XIPC_INVALIDATE_ELEMENT(buffer);
  x = buffer->data[buffer->take_ptr];
  // Wrap around circular buffer
  if (++buffer->take_ptr == NUM_ITEMS)
    buffer->take_ptr = 0;
  buffer->count--;
  XIPC_WRITE_BACK_ELEMENT(buffer);
  // Notify consumer
  xipc_cond_signal(&buffer->not_full);
  xipc_mutex_release(&buffer->lock);
  return x;
}
