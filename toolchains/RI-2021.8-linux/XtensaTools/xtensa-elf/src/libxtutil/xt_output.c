
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
//  Default output function.
//-----------------------------------------------------------------------------
extern int
_write_r(int *, int, const void *, int);


//-----------------------------------------------------------------------------
//  Output function pointer.
//-----------------------------------------------------------------------------
static xt_output_fn * xt_out = &_write_r;


//-----------------------------------------------------------------------------
//  xt_set_output_fn - Set the character output handler function.
//  Returns the previous handler function address.
//-----------------------------------------------------------------------------
xt_output_fn *
xt_set_output_fn(xt_output_fn *fn)
{
    xt_output_fn * old = xt_out;

    xt_out = fn;
    return old;
}


//-----------------------------------------------------------------------------
//  _xt_output - helper for xt_printf() etc.
//-----------------------------------------------------------------------------
void
_xt_output(void *closure, const char *buf, int len)
{
    int err;

    // Unused
    (void) closure;

    (*xt_out)(&err, 1, buf, len);
}

