/* Copyright (c) 2021 Cadence Design Systems, Inc.
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

#ifndef __XDYN_LIB_MISC_H__
#define __XDYN_LIB_MISC_H__

#include <stdint.h>
#include <stdlib.h>

/* On Xtensa device platforms */
#ifdef __XTENSA__
#include <xtensa/xtutil.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/tie/xt_core.h>

#if XCHAL_NUM_DATARAM > 1
  #if XCHAL_DATARAM1_PADDR < XCHAL_DATARAM0_PADDR
  #  define DRAM_ATTR __attribute__((section(".dram1.data")))
  #else
  #  define DRAM_ATTR __attribute__((section(".dram0.data")))
  #endif
#else
  #  define DRAM_ATTR __attribute__((section(".dram0.data")))
#endif

#ifdef XDYN_LIB_DEBUG
#define XDYN_LIB_INLINE
#else
#define XDYN_LIB_INLINE  __attribute__((always_inline))
#endif

#if XCHAL_HAVE_PRID
__attribute__((unused)) static XDYN_LIB_INLINE uint32_t
xdyn_lib_get_proc_id()
{
  return XT_RSR_PRID();
}
#else
__attribute__((unused)) static XDYN_LIB_INLINE uint32_t
xdyn_lib_get_proc_id()
{
  return 0;
}
#endif // XCHAL_HAVE_PRID

#define XT_PRINTF(...)   xt_printf(__VA_ARGS__)
#define XT_SPRINTF(...)  xt_sprintf(__VA_ARGS__)
#define XT_VSPRINTF(...) xt_vsprintf(__VA_ARGS__)
#define XT_FLUSH()

#define PRAGMA_FREQ_HINT_NEVER _Pragma("frequency_hint NEVER")

#else // !__XTENSA__

#include <stdio.h>
#include <stdarg.h>

#define XT_PRINTF(...)   printf(__VA_ARGS__)
#define XT_SPRINTF(...)  sprintf(__VA_ARGS__)
#define XT_VSPRINTF(...) vsprintf(__VA_ARGS__)
#define XT_FLUSH()       fflush(stdout);

#define PRAGMA_FREQ_HINT_NEVER
#define XDYN_LIB_INLINE

#endif // __XTENSA__

#ifdef XDYN_LIB_DEBUG
static void
xdyn_lib_log(const char *fmt, ...) {
  int l = 0;
  char _lbuf[1024];
#ifdef __XTENSA__
  l += XT_SPRINTF(&_lbuf[l], "XDYN_LIB_LOG: PROC%d: TID%d ", 
                  xdyn_lib_get_proc_id(), xdyn_lib_thread_id());
#else
  l += XT_SPRINTF(&_lbuf[l], "XDYN_LIB_LOG: ");
#endif
  va_list argp;
  va_start(argp, fmt);
  XT_VSPRINTF(&_lbuf[l], fmt, argp);
  va_end(argp);
  XT_PRINTF("%s", _lbuf);
  XT_FLUSH();
}

static const char *
xdyn_lib_get_uuid_str(const unsigned char *uuid) {
  static char s[128] = {0,};
  char *p = s;
  XT_SPRINTF(p, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
             "%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3],
             uuid[4], uuid[5],
             uuid[6], uuid[7],
             uuid[8], uuid[9],
             uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
  return s;
}

#define XDYN_LIB_LOG(...)          xdyn_lib_log(__VA_ARGS__);
#define XDYN_LIB_ABORT(...)        { xdyn_lib_log(__VA_ARGS__); exit(-1); }
#define XDYN_LIB_ASSERT(C, ...)    { if (!(C)) XDYN_LIB_ABORT(__VA_ARGS__); }
#define XDYN_LIB_CHECK(C, R, ...)  { if (!(C)) {                              \
                                       XDYN_LIB_LOG(__VA_ARGS__);             \
                                       return R;                              \
                                     }                                        \
                                   }
#define XDYN_LIB_CHECK_STATUS(C, S, R, label, ...) {                          \
    if (!(C)) {                                                               \
      XDYN_LIB_LOG(__VA_ARGS__);                                              \
      S = R;                                                                  \
      goto label;                                                             \
    }                                                                         \
}

#else
#define XDYN_LIB_LOG(...)
#define XDYN_LIB_ABORT(...)                 { exit(-1); }
#define XDYN_LIB_ASSERT(C, ...)   { if (!(C)) {                               \
                                      PRAGMA_FREQ_HINT_NEVER                  \
                                      XDYN_LIB_ABORT(__VA_ARGS__);            \
                                    }                                         \
                                  }
#define XDYN_LIB_CHECK(C, R, ...) { if (!(C)) {                               \
                                      PRAGMA_FREQ_HINT_NEVER                  \
                                      return R;                               \
                                    }                                         \
                                  }
#define XDYN_LIB_CHECK_STATUS(C, S, R, label, ...) {                          \
    if (!(C)) {                                                               \
      PRAGMA_FREQ_HINT_NEVER                                                  \
      S = R;                                                                  \
      goto label;                                                             \
    }                                                                         \
}

#endif // XDYN_LIB_DEBUG

#endif // __XDYN_LIB_MISC_H__
