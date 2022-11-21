-------------------------------------------------------------------------
Copyright (c) 2020-2021 Cadence Design Systems, Inc.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------

Secure Boot-loader / Secure Monitor executable and API library (libsec)


Build Details
-------------

The secure monitor / library code is built inline in the secmon source tree.

1. To build the monitor and library code, run "xt-make" in secmon/src/.
2. To build the examples / test code, run "xt-make" in secmon/examples/, which
   has a dependency on #1 and will invoke it automatically if needed.

Both builds can be parallelized.

It is important to note that secmon has a dependency on xtos, and must be able
to locate the xtos source directory at build-time.  If the secmon tree is 
copied elsewhere or is built outside of the usual location, the following 
modification is required:

  secmon/src/Makefile.inc - update "XTOSSRCDIR" to indicate xtos source location

After a successful build, the following manifest is generated:

Secure artifacts:
  1. bin/libsm-board.a              - Secure boot-loader/monitor library (board)
  2. bin/libsm-sim.a                - Secure boot-loader/monitor library (ISS)
  3. bin/libsm-board-unpack.a       - Self-unpacking version of above (board)
  4. bin/libsm-sim-unpack.a         - Self-unpacking version of above (ISS)
  5. bin/_vectors-sm.o              - Secure boot-loader/monitor vector anchors
  6. include/xtensa/secmon-secure.h - Header of secure customization hooks
  7. include/xtensa/secmon-common.h - Header of common secure/nonsecure routines
  8. include/xtensa/secmon-internal.h Shared internal header for mon, lib

Non-secure artifacts:
  9. bin/libsec.a                   - Non-secure library API to secure monitor
  10. include/xtensa/secmon.h       - Header of non-secure APIs in libsecmon.a
  11. include/xtensa/secmon-common.h  Header of common secure/nonsecure routines
  12 include/xtensa/secmon-internal.h Shared internal header for mon, lib

Example/test artifacts:
  13 bin/examples/<name>/nonsecure_app-board.exe - Non-secure test application
  14 bin/examples/<name>/nonsecure_app-iss.exe   - Non-secure test application
  15 bin/examples/<name>/secmon-board.exe        - Test-specific secure monitor
  16 bin/examples/<name>/secmon-iss.exe          - Test-specific secure monitor
  17 bin/examples/<name>/*.dis                   - Disassembly of ELF files
  18 bin/examples/<name>/nonsecure_app*.bin      - Packed/xt-elf2rom executables


Execution Details
-----------------

Two execution targets for secure monitor / library code are supported:

  A. ISS simulation target

     To run, invoke xt-run with the secure monitor, loading the nonsecure
     packed binary file at the temporary address <loadaddr> (unpacking and
     addressing discussed in more detail below, under Implementation Details):

     shell>   xt-elf2rom nonsecure_app-sim.exe nonsecure_app-sim.bin
     shell>   xt-run --loadbin=nonsecure_app-sim.bin@<loadaddr> --mem_model 
                     secmon-sim.exe unpack=<loadaddr>

  B. Board target (e.g. KC705)

     To run, start XOCD and invoke xt-gdb with the secure monitor:

     shell>   xt-gdb secmon-board.exe
     (xt-gdb) target remote localhost:20000 <spilladdr>
     (xt-gdb) reset
     (xt-gdb) restore nonsecure_app-board.bin binary <loadaddr> 
                                            # load nonsecure binary first
     (xt-gdb) load                          # load secure monitor ELF file second
     (xt-gdb) set-args --address=<argaddr> unpack=<loadaddr>
                                            # specify address of nonsecure binary 
     (xt-gdb) continue

     NOTES on addresses: 
     <loadaddr>  must be writeable and not overlap with nonsecure application
     <argaddr>   must have write and execute permissions for set-args to work
     <spilladdr> must have write and execute permissions for TIE read/write ops
     
     NOTE: set-args also requires the use of a hardware breakpoint, or write 
     access to the memory region containing _start.


Implementation Details
----------------------

After initializing the system, the secure monitor is responsible for locating 
and unpacking the nonsecure application, then launching it.  A reference 
implementation of this process is provided here, but can also be customized.  

In this environment, the non-secure application ELF file is packed into a 
binary format using the xt-elf2rom utility, which at a high level, provides a 
segment table for unpacking as well as an execution entry point.  No compression
is provided at this time.  The secure monitor contains logic to recognize, 
unpack, and launch the nonsecure application (see "secmon_unpack()" in 
src/mon/unpack.c).


Linker Support Packages
-----------------------

Secmon relies on the following standard sets of LSPs during its build process:
secmon-sim and secmon-board are used to link the secure boot-loader/monitor 
executables, and app-sim and app-board are used to link the nonsecure 
executables.  

Additionally, the secure boot-loader/monitor can be built to unpack itself
from a romable memory by linking against the secmon-sim-rom or secmon-boad-rom
LSPs while building example tests.  Secure LSPs can be overridden by setting
"SMLSP_SIM=secmon-sim-rom" or "SMLSP_BRD=secmon-board-rom" during an example 
build.


Secure Code Limitations
-----------------------

The secure monitor itself is entirely built using the call0 ABI, as the
Windowed Register option (if configured) is only usable by non-secure 
appplications.  Secmon code and its XTOS dependencies are recompiled with
"-mabi=call0", but all other libraries are not, most notably libhal and libc.  
As a result, the secure LSPs prohibit linking against these libraries, and 
any customizations to secure code must also not use them.  Macros and inline 
functions are permitted.

In the event that HAL, libc, GDBIO, or other library code is required
in the secure monitor, the user should generate a second parallel Xtensa 
configuration with the call0 ABI selected, and link secmon against those
libraries directly.  

Please see the Secure LX Programmer's Guide for additional information.

