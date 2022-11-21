// Copyright (c) 2004-2021 Cadence Design Systems, Inc.
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

// unpack.c -- Secure Monitor: unpack nonsecure application - reference solution


#include <xtensa/xtruntime.h>
#include <xtensa/config/system.h>
#include <xtensa/xt-elf2rom.h>

#include <errno.h>

#include "xtensa/secmon-common.h"
#include "xtensa/secmon-secure.h"
#include "xtensa/secmon-defs.h"


// Unpack offset within only SYSRAM is from the end.
#define UNPACK_OFFSET_SYSRAM        (1024 * 1024)

#define UNUSED(x) ((void)(x))


//-----------------------------------------------------------------------------
//  Prototypes
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  memcpy(): no access to libc/libxtutil in secure code
//-----------------------------------------------------------------------------
static void * my_memcpy(void *dst, void *src, uint32_t len)
{
    if (((uint32_t)dst & 3) || ((uint32_t)src & 3)) {
        char *dstc = (char *)dst;
        char *srcc = (char *)src;
        for (uint32_t i = 0; i < len; i++) {
            *dstc++ = *srcc++;
        }
    } else {
        /* 4-byte writes required for IRAM entries, and is faster.
         * Round up length to a multiple of 4 since byte writes would fail.
         */
        uint32_t *dstp = (uint32_t *)dst;
        uint32_t *srcp = (uint32_t *)src;
        for (uint32_t i = 0; i < len; i+=4) {
            *dstp++ = *srcp++;
        }
    }
    return dst;
}


//-----------------------------------------------------------------------------
//  strtoul(): no access to libc in secure code
//-----------------------------------------------------------------------------
static uint32_t my_strtoul(char *s)
{
    uint32_t val = 0;
    uint32_t dig, ndig = 0;

    if ((s == NULL) || (s[0] == '\0')) {
        return 0;
    }
    if (!((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))) {
        // hex input string required
        return 0;
    }
    s += 2;
    while ((*s != '\0') && (++ndig <= 8)) {
        if ((*s >= '0') && (*s <= '9')) {
            dig = *s - '0';
        } else if ((*s >= 'a') && (*s <= 'f')) {
            dig = 10 + *s - 'a';
        } else if ((*s >= 'A') && (*s <= 'F')) {
            dig = 10 + *s - 'A';
        }
        else return 0;
        val = (val << 4) + dig;     // avoid multiply on configs without MUL instr
        s++;
    }
    return val;
}


//-----------------------------------------------------------------------------
//  strncmp(): no access to string functions in secure code
//-----------------------------------------------------------------------------
static int32_t my_strncmp(char *s, const char *c, size_t n)
{
    if (!s || !c || (n == 0)) {
        return -EINVAL;
    }
    for (uint32_t i = 0; i < n; i++) {
        int32_t ret = (c[i] - s[i]);
        if (ret) {
            return ret;
        }
    }
    return 0;
}


//-----------------------------------------------------------------------------
//  Unpack a nonsecure application and return its entry point.
//  Detect and skip any memory segments with either source or destination 
//  addresses residing (fully or partially) in secure memory ranges.
//
//  Current implementation requires user to pre-load memory with a binary image
//  of the nonsecure executable, packed with xt-elf2rom.  The location can be
//  arbitrary, but the image must not be located within its own unpack regions.
//
//  The image address is provided as an "unpack=0xXXXXXXXX" argument to this 
//  function, either through ISS or through xt-gdb's "set-args" command.  The 
//  address must be a text string formatted as above or it will be ignored.
//
//  NOTE: Customer-defined routine(s) can be added here to validate integrity of 
//  nonsecure executable prior to running, if needed.
//
//  \param     argc            Argument count.
//
//  \param     argv            Argument list.
//
//  \return    On success: non-zero address indicating non-secure entry point.
//             On failure: zero.
//-----------------------------------------------------------------------------
uint32_t __attribute__((weak)) secmon_unpack(int32_t argc, char **argv)
{
    uint32_t nonsec_addr = 0;
    uint32_t ret = 0;

    for (int32_t i = 0; i < argc; i++) {
        if (my_strncmp(argv[i], "unpack=", 7) == 0) {
            nonsec_addr = my_strtoul(argv[i]+7);
            break;
        }
    }

    if (nonsec_addr > 0) {
        xt_rom_img_hdr *   phdr  = (xt_rom_img_hdr *)nonsec_addr;
        xt_elf_seg_entry * entry = (xt_elf_seg_entry *) (nonsec_addr + sizeof(xt_rom_img_hdr));

        // Check signature
        for (int32_t i = 0; i < 4; i++) {
            if (phdr->signature[i] != xt_rom_file_sig[i]) {
                // ROM image signature mismatch
                return 0;
            }
        }

        // Validate entry point
        if (xtsm_secure_addr_overlap(phdr->entry_addr, (phdr->entry_addr + 3))) {
            // Entry point must point to nonsecure memory
            return 0;
        }
        ret = phdr->entry_addr;

        // Validate and unpack
        for (; ; entry++) {
            char * dst = (char *) entry->dst_address;
            char * src = (char *) (nonsec_addr + entry->file_offset);
            uint32_t size = entry->file_size;

            if ((size != 0) && 
                (xtsm_secure_addr_overlap((uint32_t)src, (uint32_t)(src + size - 1)) ||
                 xtsm_secure_addr_overlap((uint32_t)dst, (uint32_t)(dst + size - 1)))) {
                // Unpack src/dst addresses must all reside in nonsecure memory
                return 0;
            }

            my_memcpy(dst, src, entry->file_size);

            if (entry->seg_flags & 0x80000000) {
                break;
            }
        }

        // Nonsecure MPU entries not yet programmed; nonsecure code is uncached
        // so no cache writeback is required.
    }

    return ret;
}

