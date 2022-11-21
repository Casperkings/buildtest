// overlay_manager.c -- Overlay manager C routines.
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

#include <xtensa/xtruntime.h>
#include "overlay.h"


// These functions are called by the overlay manager to manage access
// to shared overlay data. These should be overridden by OS-specific 
// implementations if a pre-emptive multitasking OS is present.
extern void xt_overlay_init_os(void);
extern void xt_overlay_lock(void);
extern void xt_overlay_unlock(void);


// Defined by linker script.
extern unsigned int _overlay_vma;
extern unsigned int _overlay_vma_len;
extern unsigned int _NUM_OVERLAYS;

// Required for gdb.
int _novlys  = (int) &_NUM_OVERLAYS;

// Defined in overlay_asm.S
// ID of overlay currently mapped, if any, else -1.
extern short _ovly_id;
// ID of overlay currently being mapped, if any, else -1.
extern short _mapping_id;

// Static - local flag to track one-time init
static int ovly_os_init_done;

// Required for gdb, doesn't have to do anything. Must be called every
// time the active overlay changes.
//
__attribute__((noinline))
void _ovly_debug_event(void)
{
 __asm__ __volatile__("");
}


#if defined (__XTENSA_WINDOWED_ABI__)

// Returns the ID of the overlay currently being mapped, or -1 if no map
// is in progress.
//
int xt_overlay_map_in_progress(void)
{
    return _mapping_id;
}


// Updates the overlay table to indicate the current mapped overlay.
// Also calls the debug event hook that gdb monitors.
//
static void overlay_update_table(int id)
{
    int i;
    unsigned int flags;

    for (i = 0; i < _novlys; ++i) {
        _ovly_table[i].mapped = (i == id);
    }

    flags = XTOS_SET_INTLEVEL(15);
    _ovly_id = id;
    _mapping_id = -1;
    XTOS_RESTORE_INTLEVEL(flags);

    _ovly_debug_event();
}


// Starts async map of requested overlay. Calls out to do the actual mapping.
// The callouts can be overridden to implement your own map mechanism. See
// xt_overlay_start_map() and xt_overlay_is_mapping() for examples.
//
// This function calls the function xt_overlay_fatal_error() if there is an
// error mapping an overlay. It returns -
//
//     0 - Completed, requested overlay is mapped
//     1 - Map started, in progress
//    -1 - Error
//
// NOTE: This function must be protected against being re-entered, or else we
// could have two threads mapping into the overlay area. It is the caller's
// job to prevent this. See the use of xt_overlay_lock/unlock.
//
static int xt_overlay_map_internal(int ov_id)
{
    static unsigned int ov_sig = 0x4F563030;
    unsigned int * p           = (unsigned int *) &_overlay_vma;
    int ret;

    if ((ov_id < 0) || (ov_id >= _novlys)) {
        xt_overlay_fatal_error(ov_id);
        return -1;
    }
    
    // Wait for any pending maps to finish. We assume that this call
    // will return false only after taking all actions required to
    // complete the map.
    if (_mapping_id >= 0) {
        while (xt_overlay_is_mapping(_mapping_id))
            ;
        overlay_update_table(_mapping_id);
    }
    
    // Best to check directly. If this matches and no map is ongoing,
    // we are guaranteed to have the correct overlay.
    if (*p == (ov_sig + ov_id)) {
        return 0;
    }

#if DEBUG
    printf("xt_overlay_map_internal: %d %p %p %d\n",
        ov_id,
        _ovly_table[ov_id].vma,
        _ovly_table[ov_id].lma,
        _ovly_table[ov_id].size);
#endif

    // NOTE: We must invalidate _ovly_id before we start any map
    // operation, else another thread might get a false hit on the
    // ID and jump in while the overlay is being written into.
    _ovly_id = -1;
    ret = xt_overlay_start_map(_ovly_table[ov_id].vma,
                               _ovly_table[ov_id].lma,
                               _ovly_table[ov_id].size,
                               ov_id);
    if (!ret) {
        _mapping_id = ov_id;
    }

    if (ret) {
        xt_overlay_fatal_error(ov_id);
        return -1;
    }

    return 1;
}


// Maps the requested overlay. This call is synchronous and is thread
// safe, i.e. it locks access to the overlay while the mapping is in
// progress. With proper implementation of the OS hooks, we can prove
// that while one thread is mapping an overlay, no other map can be
// started by any other thread.
//
// This function does not return a value. Instead it calls the function
// xt_overlay_fatal_error() if there is an error mapping an overlay.
//
void xt_overlay_map(int ov_id)
{
    int ret;

    // OS init is done only once - at this time also clear the sig
    // location at the VMA to avoid false matches.
    if (!ovly_os_init_done) {
        unsigned int * p = (unsigned int *) &_overlay_vma;

        *p = 0;
        xt_overlay_init_os();
        ovly_os_init_done = 1;
    }

    // Lock access to the overlay
    xt_overlay_lock();

    ret = xt_overlay_map_internal(ov_id);

    if (ret == 1) {
        // This means mapping started and in progress.
        while (xt_overlay_is_mapping(ov_id))
            ;
        overlay_update_table(ov_id);
    }

    xt_overlay_unlock();
}


// Begins an asynchronous map operation. This operation is currently
// NOT protected for multi-threading and therefore should not be used
// in a multithreading/multitasking environment. It returns -
//
//     0 - Completed, requested overlay is mapped
//     1 - Map started, in progress
//    -1 - Error
//
int xt_overlay_map_async(int ov_id)
{
    // OS init is done only once - at this time also clear the sig
    // location at the VMA to avoid false matches.
    if (!ovly_os_init_done) {
        unsigned int * p = (unsigned int *) &_overlay_vma;

        *p = 0;
        xt_overlay_init_os();
        ovly_os_init_done = 1;
    }

    return xt_overlay_map_internal(ov_id);
}

#endif // (__XTENSA_WINDOWED_ABI__)

