# Build the board-support library for a board BOARD.
#
# Makefile.inc - In board-generic source directory of core and board packages.
#
# Copyright (c) 2006-2007 Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.
#
# This is included by the board-specific Makefile in each $(BOARD) subdirectory.
# These and this Makefile.inc are installed in the released core and board
# packages and are intended to be used to (re)generate the board support library
# in the installed environment.
#
# The board-specific Makefile that includes this defines BOARD and MODS.
# BOARD identifies the board and names board-specific subdirectories.
# MODS is the list of basenames of module sources (no extension) for a board.
# Each module source will be found in the board-specific source directory if it
# exists, else in the common sources.
# XTENSA_CORE must be set for Xtensa tools (usually set in the environment)
# unless the tools installation was configured with a default core.
#
# To build the library, change to the board-specific subdirectory BOARD and
# use one of the following commands:
#
#   To (re)build and install inside the board package (the normal way):
#     xt-make [XTENSA_BOARDS=<package-root>] install
#   XTENSA_BOARDS may be inherited from your environment if defined.
#
#   To (re)build from sources inside a core package (and not install):
#     xt-make [XTENSA_TOOLS_ROOT=<tools-pkg-root>] [XTENSA_ROOT=<core-pkg-root>]
#   XTENSA_TOOLS_ROOT and XTENSA_ROOT may be inherited from the environment.
#
#   Intermediate files and the generated board library lib$(BOARD).a go to the
#   current working directory and may be removed after installation by:
#     xt-make clean
#
# Note: Any GNU 'make' can be used, it need not be xt-make.

# Location defaults (relative to board package root)
ifdef XTENSA_BOARDS
  # External board package (need explicit paths)
  CORE_XELF = $(XTENSA_BOARDS)/$(XTENSA_CORE)/xtensa-elf
  COMM_XELF = $(XTENSA_BOARDS)/common/xtensa-elf
  IFLAGS    = -I$(COMM_XELF)/include/xtensa/$(BOARD) -I$(COMM_XELF)/include
else
  # Board support internal to core package (use standard paths where possible)
  CORE_XELF = $(XTENSA_ROOT)/xtensa-elf
  COMM_XELF = $(XTENSA_TOOLS_ROOT)/xtensa-elf
  IFLAGS    = -I$(COMM_XELF)/include/xtensa/$(BOARD)
endif
COMM_SRC    = $(COMM_XELF)/src/boards
INST_DIR    = $(CORE_XELF)/arch/lib

# Name of the board library (this is the name the board-specific LSPs use)
LIB	= lib$(BOARD).a

# Object modules to be included in the library (from list of module basenames)
OBJS	= $(MODS:%=%.o)

# Source paths (board-specific sources override common sources with same name)
VPATH   = $(COMM_SRC)/$(BOARD) $(COMM_SRC)

# Fallback to sources internal to Tensilica (for non-released sources)
ifdef INTERN_SRC
IFLAGS	+= -I$(INTERN_SRC)/include/xtensa/$(BOARD)
VPATH   += $(INTERN_SRC)/boards/$(BOARD)
endif

# Tools and Flags
ifdef XTENSA_CORE
CFGFLG	= --xtensa-core=$(XTENSA_CORE)
else
CFGFLG	=
endif
CC	= xt-xcc $(CFGFLG)
COPT	= -g -O2
CFLAGS	= $(COPT) -mlongcalls
CP	= cp -p
AR	= xt-ar
RM	= rm -f

#
# Target Rules
#
.PHONY:	all install clean

all:	$(LIB)

$(LIB):	$(OBJS)
	$(AR) -rc $@ $^

install: all
	$(CP) $(LIB) $(INST_DIR)/$(LIB)

clean:
	$(RM) $(OBJS) $(LIB)

#
# Generic Rules
#

%.o: %.c
	$(CC) -c $(CFLAGS) $(IFLAGS) -o $@ $<

%.o: %.S
	$(CC) -c $(CFLAGS) $(IFLAGS) -o $@ $<

