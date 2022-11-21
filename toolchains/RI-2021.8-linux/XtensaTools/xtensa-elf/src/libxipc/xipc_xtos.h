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

#include <xtensa/xtruntime.h>

typedef int32_t (xipcCondFunc)(void *arg, int32_t val, void *thread);

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_disable_interrupts()
{
  return xthal_disable_interrupts();
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_enable_interrupts(uint32_t level)
{
  xthal_restore_interrupts(level);
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_thread_block()
{
#if XCHAL_HAVE_INTERRUPTS
  XT_WAITI(0);
#endif
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_cond_thread_block(xipcCondFunc *cond_func, void *cond_arg)
{
#if XCHAL_HAVE_INTERRUPTS
  XT_WAITI(0);
#endif
}

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_get_my_thread_id()
{
  return 0;
}

__attribute__((unused)) static __attribute__((always_inline)) uint32_t
xipc_get_thread_id(void *thread)
{
  return 0;
}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_disable_preemption() {}

__attribute__((unused)) static __attribute__((always_inline)) void
xipc_enable_preemption() {}

/* A dummy version of the corresponding function in xipc_xos.h. For XTOS based
 * systems self notify is undefined.
 */
__attribute__((unused)) static __attribute__((always_inline)) void
xipc_self_notify() {}

#endif /* __XIPC_OS_H__ */
