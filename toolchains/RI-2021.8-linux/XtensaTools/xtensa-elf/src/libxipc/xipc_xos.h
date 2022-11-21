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
#ifndef __XIPC_OS_H__
#define __XIPC_OS_H__

#include <xtensa/xos.h>

extern XosCond xipc_xos_cond;
typedef int32_t (xipcCondFunc)(void *arg, int32_t val, void *thread);

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_disable_interrupts()
{
  return xos_disable_interrupts();
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_enable_interrupts(uint32_t level)
{
  xos_restore_interrupts(level);
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_thread_block()
{
  xos_cond_wait(&xipc_xos_cond, 0, 0);
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_cond_thread_block(xipcCondFunc *cond_func, void *cond_arg)
{
  xos_cond_wait(&xipc_xos_cond, (XosCondFunc *)cond_func, cond_arg);
}

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_get_my_thread_id()
{
  return (uint32_t)xos_thread_id() >> 6;
}

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_get_thread_id(void *thread)
{
  return (uint32_t)thread >> 6;
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_disable_preemption()
{
  xos_preemption_disable();
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_enable_preemption()
{
  xos_preemption_enable();
}

/* Helper function to wakeup threads on this processor that could be waiting
 * on the xipc condition. Useful when the xipc objects can be used across
 * threads from the same processor
 */
__attribute__((unused)) static __attribute__((always_inline)) void
xipc_self_notify()
{
  xos_cond_signal(&xipc_xos_cond, 0);
}

#endif /* __XIPC_OS_H__ */
