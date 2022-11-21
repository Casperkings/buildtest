
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
//  xt_putchar - print one character on stdout
//-----------------------------------------------------------------------------
int
xt_putchar(int c)
{
    char x = (char) c;

    _xt_output(0, &x, 1);
    return (int) x;
}


//-----------------------------------------------------------------------------
//  xt_puts - print a string to stdout. Returns the number of bytes written.
//-----------------------------------------------------------------------------
int
xt_puts(const char *s)
{
    const char * t = s;
    int          n = 0;

    if (t) {
        while (*t) {
            _xt_output(0, t, 1);
            t++;
            n++;
        }
        _xt_output(0, "\n", 1);
    }

    return n;
}


//-----------------------------------------------------------------------------
//  xt_putn - Print a number to stdout. The number is treated as unsigned.
//-----------------------------------------------------------------------------
void
xt_putn(unsigned int n)
{
    // avoids division
    int digit, i, seen = 0;

    if (n == 0) {
        xt_putchar('0');
        return;
    }

    for (digit = 0; n >= 1000000000; digit++)
        n -= 1000000000;

    for (i = 9;;) {
        seen += digit;
        if (seen) {
            xt_putchar('0' + digit);
        }
        for (digit = 0; n >= 100000000; digit++) {
            n -= 100000000;
        }
        if (--i == 0) {
            xt_putchar('0' + digit);
            return;
        }
        n = ((n << 1) + (n << 3));
    }
}


//-----------------------------------------------------------------------------
//  xt_atoi - Convert ASCII string to integer. Does not check overflow.
//  NOTE: should check for sign.
//-----------------------------------------------------------------------------
int
xt_atoi(const char * s)
{
    int c, n = 0;

    if (s == NULL) {
        return 0;
    }

    while ((c = *(const unsigned char *)s++) != 0) {
        c -= '0';
        if (c > 9)
            break;
        n = (n << 3) + (n << 1) + c;
    }

    return n;
}

