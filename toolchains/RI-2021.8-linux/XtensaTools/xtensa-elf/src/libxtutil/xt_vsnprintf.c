
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
//  xt_vsnprintf - Replacement for vsnprintf.
//  For format specifications see the documentation for printf().
//-----------------------------------------------------------------------------
int
xt_vsnprintf(char * str, size_t size, const char * format, va_list ap)
{
    int n;
    int s;

    if (str == NULL) {
        return 0;
    }

    s = (int)(size - 1);
    n = _xt_vprint(0, str, format, ap, s);

    if (n < s) {
        str[n] = 0;
    }
    else {
        str[s] = 0;
    }

    return n;
}

