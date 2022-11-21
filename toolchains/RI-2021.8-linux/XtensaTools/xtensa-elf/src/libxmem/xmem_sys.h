/* Copyright (c) 2020 Cadence Design Systems, Inc.
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
#ifndef __XMEM_SYS_H__
#define __XMEM_SYS_H__

#include <stdint.h>
#include <xtensa/xtruntime.h>
#ifdef XMEM_XOS
#include <xtensa/xos.h>
#endif

/* Override the below functions if used in a MP/mult-threaded enviroment */

// In a bare-metal/XTOS enviroment, the below functions are not required,
// else the user needs to provide implementations of these weakly defined
// functions.
#ifdef XMEM_XTOS
#define xmem_disable_preemption()
#define xmem_enable_preemption()
#define xmem_get_thread_id()      (uint32_t)0
#elif XMEM_XOS
#define xmem_disable_preemption() xos_preemption_disable()
#define xmem_enable_preemption()  xos_preemption_enable()
#define xmem_get_thread_id()      ((uint32_t)xos_thread_id()&0xffff)
#else
__attribute__((weak)) void
xmem_disable_preemption() {}

__attribute__((weak)) void
xmem_enable_preemption() {}

__attribute__((weak)) uint32_t
xmem_get_thread_id() { return 0; }
#endif // XMEM_XTOS

#endif // __XMEM_SYS_H__
