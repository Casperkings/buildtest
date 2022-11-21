// Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

// secure_user_init.c -- Secure monitor example code


#include "xtensa/secmon-secure.h"
#include "xtensa/secmon-common.h"

// example entry point; replace as needed
#define NONSECURE_START         0xcafec0de


// no access to libc in secure code
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


// Hook into secure monitor init. argc/argv can be passed in either as:
//
// argc==1, from set-args, with entry point in argv[0], or
// argc==2, from ISS, with executable name in argv[0] and entry point in argv[1]
//
// NOTE: an invalid entry point may result in undefined behavior.
uint32_t secmon_unpack(int32_t argc, char **argv)
{
    uint32_t start_param = (argc > 0) ? my_strtoul(argv[argc - 1]) : 0;
    return start_param ? start_param : NONSECURE_START;
}

