/*
 * Copyright (c) 2014 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef IDMA_DEBUG

#include "../idma.h"
#include "../idma_internal.h"
#include <stdint.h>
#include <stdarg.h>


/* parasoft-begin-suppress ALL "This file not MISRA checked." */

static void
xt_output(char **outargp, char *inarg, int32_t count, int32_t *lim)
{
  char *in = inarg;
  char** outp = outargp;
  int32_t cnt = count;
  while ((cnt > 0) && (*lim != 0)) {
    **outp = *in;
    (*outp)++; in++; cnt--; (*lim)--;
  }
}

/*
 * Base formatting routine, used internally.
 */
static int32_t
xt_vprint(void * outarg, const char * fmth, va_list ap, int32_t size)
{
    const char* fmt = fmth;
    int32_t    total = 0;
    int32_t    lim   = size;
    char   space = ' ';
    char * outp  = (char *) outarg;
    char   buf[11];    /* largest 32-bit integer output (octal) */
    char   c;

    while (fmt[0] != (char)0) {
        c = fmt[0];
        fmt++;
        if (c != '%') {
            xt_output(&outp, &c, 1, &lim);
            total++;
        }
        else {
            int32_t width = 0, len = 1;
            uint32_t n;
            char *s = buf, *t, pad = ' ', sign = (char)0;

            while (1) {
              c = fmt[0];
              fmt++;
              switch (c) {
		      
	        case 'l':    /* ignore (long; same as int) */
		  break;

                case 's':
                    s = va_arg(ap, char*);
                    if (s == (char*)0) {
                     s = (char*)"(null)";
                    }
                    for (t = s; (*t != (char)0); t++) {};
                    len = t - s;
                    goto donefmt;

                case 'd':
                    n = va_arg(ap, int32_t);
                    if ((int32_t)n < 0) {
                        sign = '-';
                        n = -(int32_t)n;
                    }
                    if (sign != 0) {
                        if (pad == ' ') {
                            *s = sign;
                            s++;
                        }
                        else {
                            xt_output(&outp, &sign, 1, &lim);
                            width--;
                            total++;
                        }
                    }
                    goto do_decimal;

                case 'u':
                    n = va_arg(ap, int32_t);
do_decimal:
                    {
                        /*  (avoids division or multiplication)  */
                        int32_t digit, i, seen = 0;

                        for (digit = 0; n >= 1000000000U; digit++) {
                            n -= 1000000000U;
                        }
                        i = 9;
                        while (1) {
                            if ((seen == 0) && (digit != 0)) {
                                seen = i;
                            }
                            if (seen != 0) {
                                *s = '0' + digit;
                                s++;
                            }
                            for (digit = 0; n >= 100000000U; digit++) {
                                n -= 100000000U;
                            }
                            i--;
                            if (i == 0) {
                                *s = '0' + digit;
                                s++;
                                len = s - buf;
                                s = buf;
                                goto donefmt;
                            }
                            n = ((n << 1) + (n << 3));
                        }
                    }
                    /*NOTREACHED*/

                case 'p':
                    /* Emulate "%#x" */
                    xt_output(&outp, (char*)"0x", 2, &lim);
                    total += 2;
                    pad = '0';
                    width = (int32_t)sizeof(void *) * 2;

                case 'x':
                    n = va_arg(ap, uint32_t);
                    s = &buf[8];
                    do {
                        --s;
                        *s = (char)"0123456789abcdef"[n & 0xFU];
                        n = (uint32_t)n >> 4;
                    } while (n != 0U);
                    len = &buf[8] - s;
                    goto donefmt;

                case (char)0:
                    goto done;

                default:
                    if ((c >= (char)'0') && (c <= (char)'9')) {
                        width = (int32_t)((width<<1)  + (width<<3) + (c - '0'));
                    }
                    else {
                        buf[0] = c;    /* handles case of '%' */
                        goto donefmt;
                    }
                    break;
                }
            }
            /*NOTREACHED*/
donefmt:
            if (len < width) {
                total += width;
                do {
                    xt_output(&outp, &pad, 1, &lim);
                } while (len < --width);
            }
            else {
                total += len;
            }
            xt_output(&outp, s, len, &lim);
            for (; len < width; len++) {
                xt_output(&outp, &space, 1, &lim);
            }
        }
    }
done:
    return total;
}

void
idma_print(int ch, const char* fmt,...)
{
  int32_t n = 0;
  va_list ap;

  if(!g_idma_cntrl[IDMA_CHANNEL_0].xlogh) {
    return;
  }
  
  g_idmalogbuf[0] = 'c';
  g_idmalogbuf[1] = 'h';
  g_idmalogbuf[3] = ':';
  g_idmalogbuf[4] = ' ';
  g_idmalogbuf[2] = (char) (ch + 0x30);

  va_start(ap, fmt);
  n = xt_vprint(g_idmalogbuf + 5, fmt, ap, IDMA_LOG_SIZE);
  va_end(ap);
  g_idmalogbuf[n+5] = (char)0;

  g_idma_cntrl[IDMA_CHANNEL_0].xlogh(g_idmalogbuf);
}

#else

// Need something here to avoid compiler warnings about empty translation unit.
int idma_dummy_;

#endif

/* parasoft-end-suppress ALL "This file not MISRA checked." */

