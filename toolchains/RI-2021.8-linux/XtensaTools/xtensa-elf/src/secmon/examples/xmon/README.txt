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

Secmon / XMON example application


Overview
--------

While OCD can be used to debug secure or nonsecure code, OCD is an invasive
debug mechanism.  It is impossible to fully prevent an external agent with 
OCD connectivity from accessing secure regions, even while debugging 
nonsecure code.

As such, XMON is the recommended solution for those wishing to expose debug 
access to nonsecure code while keeping secure regions inaccessible.  Please
see the XMON chapter of the Xtensa Debug Guide for more details on XMON.

This example integrates the XMON library into a nonsecure application in
order to demonstrate how to provide nonsecure debug access without the use
of OCD.


Building
--------

As with all Secmon examples, this XMON example builds executables and 
binaries for 2 targets: ISS and a KC705-hosted remote target.  However, 
running this example is more involved, and differs somewhat between targets.
The example is built as usual, by running either "make" or "make MODE=CUSTOM".

NOTE: The UART implementation in the xmon-main-serial.c example is different
from the typical KC705 UART.  The typical xtkc705 BSP can be used if needed.

When running on a remote target, this example assumes the UART registers are
located at UART_LITE_REGBASE and a UART interrupt at UART_LITE_INTNUM (see
xtbsp-uart-axi-lite_2v0.h for more details).  Furthermore, the UART registers
are assumed to already be mapped as uncacheable device memory through the MPU.
If not, a new MPU entry can be added by calling "xt-genldscripts -r" on the 
nonsecure LSP and using <LSP>/mpu_table.c as input to secmon's gen-mpu_inittab
utility, e.g.:

    xt-regenlsps -mlsp=app-board -b app-board-uart
    xt-genldscripts -b app-board-uart -r memmap-uart.xmm
    xt-run --exit_with_target_code --nosummary ../../bin/misc/gen-mpu_inittab.exe
        --app=app-board-uart/mpu_table.c --sec=../../bin/mon/mpu_table.c.sec >
        mpu_table-uart.c

The resulting mpu_table-uart.c should then be used to rebuild secmon-board.exe.


Running on ISS
--------------

To run the example on ISS, two processes are required: (A) one xt-gdb or xt-run
session to load and launch the secure monitor and nonsecure binary, and (B) one
xt-gdb session to debug (via remote target) the nonsecure application via XMON.

An example of running this example on ISS is as follows:

(A) xt-run --mem_model --nosummary --loadbin=./nonsecure_app-sim.bin@0x00025880
           secmon-sim.exe unpack=0x00025880 port=20123

(B) xt-gdb nonsecure_app-sim.exe
        (xt-gdb) target remote localhost:20123
        (xt-gdb) b execute_count
        (xt-gdb) continue

Alternately, session (A) can be replaced with a second xt-gdb session connecting
to the simulator:

(A) xt-gdb secmon-sim.exe
        (xt-gdb) target iss --mem_model --nosummary 
                            --loadbin=./nonsecure_app-sim.bin@0x00025880
        (xt-gdb) run unpack=0x00025880 port=20123


Running on Remote Targets
-------------------------

Remote target execution follows the simulation pattern of using 2 xt-gdb 
sessions.  However, the second process uses a serial port to connect the
debug session, as follows:

(A) xt-gdb secmon-board.exe
        (xt-gdb) target remote localhost:20000 0x100000
        (xt-gdb) reset
        (xt-gdb) restore nonsecure_app-board.bin binary 0x110000
        (xt-gdb) load
        (xt-gdb) set-args --address=0x120000 unpack=0x110000
        (xt-gdb) continue
        (xt-gdb) <Ctrl-C>   # once application is running
        (xt-gdb) detach     # to allow XMON connection

(B) xt-gdb nonsecure_app-board.exe
        (xt-gdb) set serial baud 38400
        (xt-gdb) target remote COM4 (or /dev/ttyS4)
        (xt-gdb) b execute_count
        (xt-gdb) continue

Note that, in RI-2021.6, the "set-args" step requires code at _start to be 
writable, which it is not by default if _start is in secure IRAM.  This can 
be achieved by modifying "artype[MEM_SEC_IRAM]" in src/misc/gen-mpu_inittab.c 
and rebuilding secmon with MODE=CUSTOM.  RI-2021.7 is expected to support 
"set-args" using hardware breakpoints, obviating this step.


XMON Debugging Limitations
--------------------------

XMON is built into the nonsecure application, and has no visibility into secure
memory regions.  If a debug request to access secure memory occurs, it will 
trigger an exception that cannot be routed back to nonsecure code.  As such,
XMON will hang, and the user will no longer be able to interact with the system.

If OCD is available, it can be used to debug the exception location and cause;
otherwise, it is recommended that the user turn on remote debug in xt-gdb via
"set debug remote 1" and inspect the most recent debug request for invalid or 
secure addresses.


