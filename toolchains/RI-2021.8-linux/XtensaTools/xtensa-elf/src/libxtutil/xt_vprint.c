
// Copyright (c) 2001-2020 Cadence Design Systems, Inc.
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


#define XTUTIL_LIB
#include "xtutil.h"


//-----------------------------------------------------------------------------
//  For format specifications see the documentation for printf().
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  This function will print to stdout or into a string depending on the args.
//  Note that the pointer passed as "outargp" will be moved forward on write.
//  "lim" is the count of bytes remaining in the output buffer and is also
//  updated by this function.
//  Once "lim" reaches zero, no more output will be generated, and "outargp"
//  will not be advanced. An initial value of lim = -1 effectively disables
//  the bound checking.
//-----------------------------------------------------------------------------
static void
_output(xt_outbuf_fn *out, char **outargp, const char *inarg, int count, int *lim)
{
    if (out) {
        (*out)(*outargp, inarg, count);
    }
    else {
        while (count && *lim) {
            **outargp = *inarg;
            (*outargp)++; inarg++; count--; (*lim)--;
        }
    }
}


//-----------------------------------------------------------------------------
//  Base formatting routine, used internally.
//-----------------------------------------------------------------------------
int
_xt_vprint(xt_outbuf_fn * out, void * outarg, const char * fmt, va_list ap, int size)
{
    int           total = 0;
    int           lim   = (int) size;
    char          space = ' ';
    const  char * inp   = fmt;
    char *        outp  = (char *) outarg;
    char          buf[22];    // largest 64-bit integer output (octal)
    char          c;

    while ((c = *inp) != 0) {
        const char * tp = inp;
        int          tc = 0;

        // Send out as many characters as we can in one go
        while ((*tp != '%') && (*tp != 0)) {
            tp++;
            tc++;
        }
        _output(out, &outp, inp, tc, &lim);
        total += tc;
        inp += tc;

        // Now check the next character
        c = *inp++;
        if (c == 0) {
            break;
        }
        else {
            int width = 0, len = 1, rightjust = 1;
            int longlong = 0;
            uint32_t n;
            uint64_t nn;
            char *s = buf, *t, pad = ' ', sign = 0;

            while (1) {
                c = *inp++;
                switch (c) {
                case 'c':
                    buf[0] = va_arg(ap, int);
                    goto donefmt;

                case 's':
                    s = va_arg(ap, char*);
                    if (s == 0) {
                        s = "(null)";
                    }
                    for (t = s; *t; t++) ;
                    len = t - s;
                    goto donefmt;

                case 'l':
                    if (*inp == 'l') {
                        longlong = 1;
                        inp++;
                    }
                    break;

                case '#':    // ignore (not supported)
                case 'h':    // ignore (short; passed as int)
                case 'j':    // ignore (intmax_t or uintmax_t; same as int)
                case 'z':    // ignore (size_t or ssize_t; same as int)
                case 't':    // ignore (ptrdiff_t; same as int)
                    break;

                case ' ':    sign = ' ';        break;
                case '+':    sign = '+';        break;
                case '-':    rightjust = 0;     break;

                case 'i':    // FALLTHROUGH
                case 'd':
                    if (longlong) {
                        nn = va_arg(ap, int64_t);
                        if ((int64_t)nn < 0) {
                            sign = '-';
                            nn = -(int64_t)nn;
                        }
                    }
                    else {
                        n = va_arg(ap, int32_t);
                        if ((int32_t)n < 0) {
                            sign = '-';
                            n = -(int32_t)n;
                        }
                    }
                    if (sign) {
                        if (rightjust && pad == ' ') {
                            *s++ = sign;
                        }
                        else {
                            _output(out, &outp, &sign, 1, &lim);
                            width--;
                            total++;
                        }
                    }
                    goto do_decimal;

                case 'u':
                    if (longlong) {
                        nn = va_arg(ap, int64_t);
                    }
                    else {
                        n = va_arg(ap, int32_t);
                    }
do_decimal:
                    if (longlong) {
                        // (avoids division or multiplication)
                        int digit, i, seen = 0;

                        for (digit = 0; nn >= 10000000000000000000ULL; digit++) {
                            nn -= 10000000000000000000ULL;
                        }
                        for (i = 19;;) {
                            if (!seen && digit != 0) {
                                seen = i;
                            }
                            if (seen) {
                                *s++ = '0' + digit;
                            }
                            for (digit = 0; nn >= 1000000000000000000ULL; digit++) {
                                nn -= 1000000000000000000ULL;
                            }
                            if (--i == 0) {
                                *s++ = '0' + digit;
                                len = s - buf;
                                s = buf;
                                goto donefmt;
                            }
                            nn = ((nn << 1) + (nn << 3));
                        }
                    }
                    else {
                        // (avoids division or multiplication)
                        int digit, i, seen = 0;

                        for (digit = 0; n >= 1000000000; digit++) {
                            n -= 1000000000;
                        }
                        for (i = 9;;) {
                            if (!seen && digit != 0) {
                                seen = i;
                            }
                            if (seen) {
                                *s++ = '0' + digit;
                            }
                            for (digit = 0; n >= 100000000; digit++) {
                                n -= 100000000;
                            }
                            if (--i == 0) {
                                *s++ = '0' + digit;
                                len = s - buf;
                                s = buf;
                                goto donefmt;
                            }
                            n = ((n << 1) + (n << 3));
                        }
                    }
                    // NOTREACHED

                case 'p':
                    // Emulate "%#x"
                    _output(out, &outp, "0x", 2, &lim);
                    total += 2;
                    rightjust = 1;
                    pad = '0';
                    width = sizeof(void *) * 2;
                    // FALLTHROUGH

                case 'X':
                    if (longlong) {
                        nn = va_arg(ap, unsigned long long);
                        s = buf + 16;
                        do {
                            *--s = "0123456789ABCDEF"[nn & 0xF];
                            nn = nn >> 4;
                        } while (nn);
                        len = buf + 16 - s;
                    }
                    else {
                        n = va_arg(ap, unsigned);
                        s = buf + 8;
                        do {
                            *--s = "0123456789ABCDEF"[n & 0xF];
                            n = (unsigned)n >> 4;
                        } while (n);
                        len = buf + 8 - s;
                    }
                    goto donefmt;

                case 'x':
                    if (longlong) {
                        nn = va_arg(ap, unsigned long long);
                        s = buf + 16;
                        do {
                            *--s = "0123456789abcdef"[nn & 0xF];
                            nn = nn >> 4;
                        } while (nn);
                        len = buf + 16 - s;
                    }
                    else {
                        n = va_arg(ap, unsigned);
                        s = buf + 8;
                        do {
                            *--s = "0123456789abcdef"[n & 0xF];
                            n = (unsigned)n >> 4;
                        } while (n);
                        len = buf + 8 - s;
                    }
                    goto donefmt;

                case 0:
                    goto done;

                case '0':
                    if (width == 0) pad = '0';
                    // FALLTHROUGH

                default:
                    if (c >= '0' && c <= '9') {
                        width = ((width<<1) + (width<<3)) + (c - '0');
                    }
                    else {
                        buf[0] = c;    // handles case of '%'
                        goto donefmt;
                    }
                }
            }
            // NOTREACHED
donefmt:
            if (len < width) {
                total += width;
                if (rightjust) {
                    do {
                        _output(out, &outp, &pad, 1, &lim);
                    } while (len < --width);
                }
            }
            else {
                total += len;
            }
            _output(out, &outp, s, len, &lim);
            for (; len < width; len++) {
                _output(out, &outp, &space, 1, &lim);
            }
        }
    }
done:
    return total;
}

