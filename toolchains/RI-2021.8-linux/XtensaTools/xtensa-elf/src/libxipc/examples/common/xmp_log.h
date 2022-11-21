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
#ifndef __XMP_LOG_H__
#define __XMP_LOG_H__

#include <xtensa/system/xmp_subsystem.h>
#include <xtensa/xtutil.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/config/core-isa.h>
#if XCHAL_HAVE_CCOUNT
#include <xtensa/tie/xt_timer.h>
#endif

#if XCHAL_HAVE_PRID
#define XMP_GET_PRID() XT_RSR_PRID()
#else
#define XMP_GET_PRID() (0)
#endif

#if XCHAL_HAVE_CCOUNT
#define XMP_GET_CLOCK() XT_RSR_CCOUNT()
#else
#define XMP_GET_CLOCK() (0)
#endif

static const char *xmp_proc_names[] = { XMP_PROC_ARRAY_P(NAME_STR) };

__attribute__((unused)) static void
xmp_log(const char *fmt, ...)
{
  int l = 0;
  char _lbuf[1024];

  l += xt_sprintf(&_lbuf[l], "(%10d) ", (int)XMP_GET_CLOCK());

  char _tbuf[256];
  int i;
  for (i = 0; i < XMP_GET_PRID()*10; i++)
    _tbuf[i] = ' ';
  _tbuf[i] = '\0';
  l += xt_sprintf(&_lbuf[l], "%s", _tbuf);

  l += xt_sprintf(&_lbuf[l], "XMP_LOG: %s: ",
                  xmp_proc_names[XMP_GET_PRID()]);

  va_list argp;
  va_start(argp, fmt);
  xt_vsprintf(&_lbuf[l], fmt, argp);
  va_end(argp);
  xt_printf("%s", _lbuf);
}
#endif /* __XMP_LOG_H__ */
