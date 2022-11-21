// overlay_override.c -- Overlay manager user-overridable routines.
// $Id$

// Copyright (c) 2013 Tensilica Inc.
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


#include <stdlib.h>
#include <string.h>

#include <xtensa/hal.h>
#include "overlay.h"


#if defined (__XTENSA_WINDOWED_ABI__)

// The following functions are declared weak to allow user overrides.
#pragma weak xt_overlay_start_map
#pragma weak xt_overlay_is_mapping
#pragma weak xt_overlay_fatal_error


// The following functions can be overridden to implement custom mapping
// processes, e.g. DMA-based mapping.


// Starts the overlay map process. For synchronous mapping, everything
// could be done here, or in the xt_overlay_is_mapping() function.
// Returns zero on success.
int xt_overlay_start_map(void * dst, void * src, unsigned int len, int ov_id)
{
    xthal_memcpy(_ovly_table[ov_id].vma,
                 _ovly_table[ov_id].lma,
                 _ovly_table[ov_id].size);

    // Since we just changed code, we must flush the dcache (pending
    // changes from memcpy) and invalidate the corresponding region
    // in the icache.
    xthal_dcache_region_writeback(_ovly_table[ov_id].vma,
                                  _ovly_table[ov_id].size);
    xthal_icache_region_invalidate(_ovly_table[ov_id].vma,
                                   _ovly_table[ov_id].size);

    return 0;
}


// Returns nonzero if mapping is in progress, zero if not. For asynchronous
// mapping, this function must complete any activity required to complete 
// the map before it returns zero. Should call xt_overlay_fatal_error() if
// any error occurs.
int xt_overlay_is_mapping(int ov_id)
{
    return 0;
}


// Called if an overlay map fails. By default, calls exit() with code -1.
// Override this function to provide your own error handling.
void xt_overlay_fatal_error(int ov_id)
{
    exit(-1);
}

#endif // (__XTENSA_WINDOWED_ABI__)

