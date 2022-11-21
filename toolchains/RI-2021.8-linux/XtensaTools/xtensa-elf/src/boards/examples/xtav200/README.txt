Copyright (c) 2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

This directory contains examples specifc to the Avnet XT-AV200 board,
using information from the board-specific header xtav200.h and others.
These examples can be compiled and linked for XT-AV200 board by using 
an appropriate linker-support-package (LSP).

If your board-support is in an external package such as the RB-2007.1-xtav200
package for the Diamond cores:

    xt-xcc -g -I<xtav200_root>/common/xtensa-elf/include/xtensa/xtav200 \
           -I<xtav200_root>/common/xtensa-elf/include \
           <example>.c -mlsp=<LSP> -o <example>

    <xtav200_root> is the root of your XT-AV200 board package installation.
    <LSP> must be the full path to the desired LSP in the support package:
        <pkg_root>/<core>/xtensa-elf/lib/<lsp>
    <core> is the core name (eg. DC_212GP) from your $XTENSA_CORE
        environment variable or (if not set) that of your default core.
    <lsp> is the name of the desired LSP (eg. xtav200-rt)

If your board-support is internal to your core package such as one
downloaded from the XPG for Xtensa cores with tools version RB-2007.2:

    xt-xcc -g -I<xtensa_tools_root>/xtensa-elf/include/xtensa/xtav200 \
        <example>.c -mlsp=<lsp> -o <example>

    <lsp> is the name of the desired LSP (eg. xtav200-rt)

The LSPs named xtav200-rt[-rom] are appropriate and automatically cause 
the board-support library libxtav200.a to be linked.

The supplied Makefile may be used to build any or all of the examples by

    xt-make [XTENSA_BOARDS=<xtav200_root>] [<example>]

    XTENSA_BOARDS must be defined only if an external board package is used.
    You may add XTENSA_CORE=<core> to override the default core.

Then connect to the board using XOCD and xt-gdb as usual and run <example>.

