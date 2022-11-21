# Copyright (c) 2006-2011 Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.
#
# Build examples in this dir (which are single C files) for your board.
#   make BOARD=<your-board> [XTENSA_BOARDS=<package-root>] ... [<example>]
# BOARD specifies which supported board to build for, and is always required.
# XTENSA_BOARDS is the absolute path to the root of an external board package.
#   It may be passed explicitly or set in your environment.
#   Not required if your Xtensa core package supports BOARD internally.
# XTENSA_CORE is required for Xtensa tools (usually set in your environment)
#   unless your tools installation was configured with a default core.
# Other variables may also be overridden via command line (eg. CFLAGS, LSP).

# Specify target board (edit this or set it via command line)
BOARD	    = your-board

# Specify compilation flags (edit this or override it via command line)
CFLAGS	    = -g -O0

# Build Locations, Flags and Commands
ifdef XTENSA_BOARDS
  # External board package (need explicit paths)
  COMM_XELF = $(XTENSA_BOARDS)/common/xtensa-elf
  IFLAGS    = -I$(COMM_XELF)/include
  LSP	    = $(XTENSA_BOARDS)/$(XTENSA_CORE)/xtensa-elf/lib/$(BOARD)-rt
else
  # Board support internal to core package (use standard paths)
  COMM_XELF = $(XTENSA_TOOLS_ROOT)/xtensa-elf
  IFLAGS    =
  LSP	    = $(BOARD)-rt
endif
ifdef XTENSA_CORE
  CORE_FLGS = --xtensa-core=$(XTENSA_CORE)
else
  CORE_FLGS =
endif
VPATH	    = $(COMM_XELF)/src/boards/examples/xtbsp
CC	    = xt-xcc $(CORE_FLGS)
RM	    = rm -f

# Board-Independent Examples
EXAMPLES  = count console announce

# Targets and Pattern Rules
.PHONY: all clean

all:	$(EXAMPLES)

clean:
	$(RM) $(EXAMPLES)

% : %.c
	$(CC) $(CFLAGS) $(IFLAGS) -o $@ $< -mlsp=$(LSP)

