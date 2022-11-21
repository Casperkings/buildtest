Copyright (c) 2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

This directory contains examples specifc to the Xilinx XT-ML605 board,
using information from the board-specific header xtml605.h and others.
These examples can be compiled and linked for XT-ML605 board by using 
an appropriate linker-support-package (LSP).

For example:
    xt-xcc -g -I<xtensa_tools_root>/xtensa-elf/include/xtensa/xtml605 \
        <example>.c -mlsp=<lsp> -o <example>

    <lsp> is the name of the desired LSP (eg. xtml605-rt)

The LSPs named xtml605-rt[-rom] are appropriate and automatically cause 
the board-support library libxtml605.a to be linked.

The supplied Makefile may be used to build any or all of the examples by

    xt-make [XTENSA_BOARDS=<xtml605_root>] [<example>]

    You may add XTENSA_CORE=<core> to override the default core.

Then connect to the board using XOCD and xt-gdb as usual and run <example>.

