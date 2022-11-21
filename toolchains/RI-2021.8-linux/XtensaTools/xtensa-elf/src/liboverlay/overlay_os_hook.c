// overlay_os.c -- Overlay manager OS hook routines.
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


// The overlay manager makes the following call-outs to any OS present.
// For no-OS cases, the default implementations below are sufficient.
// For cases where an OS is present, and pre-emptive task switching is
// enabled, these functions should be overridden to provide lock init,
// lock acquire, and lock release functionality, alogn with any other
// side effects that may be required.


#pragma weak xt_overlay_init_os
#pragma weak xt_overlay_lock
#pragma weak xt_overlay_unlock


// This function should be overridden to provide OS specific init such
// as the creation of a mutex lock that can be used for overlay locking.
// Typically this mutex would be set up with priority inheritance. See
// overlay manager documentation for more details.
void xt_overlay_init_os(void)
{
}


// This function locks access to shared overlay resources, typically
// by acquiring a mutex.
void xt_overlay_lock(void)
{
}


// This function releases access to shared overlay resources, typically
// by unlocking a mutex.
void xt_overlay_unlock(void)
{
}


