
# Makefile  --  rules and commands to build the XOS library --

# Copyright (c) 2015-2020 Cadence Design Systems, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Makefile options:
#
#    MODE=DBG  -- build for debug
#    MODE=OPT  -- build for speed
#    MODE=DEV  -- build for development, midway between debug and opt
#    MODE=SIZE -- build for minimum size
#    MODE=FAST -- build for speed (disables some features)

# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

# Detect C library
XSTR2   = $(shell $(GREP) SW_CLibrary $(CONFIGDIR)$(S)config$(S)default-params)
CLIB    = $(word 3, $(XSTR2))

ifeq ($(CLIB), xclib)
CSTD    = -std=c11
else
CSTD    = -std=c99
endif

# Detect XEA version
XSTR1   = $(shell $(GREP) XEA_VERSION $(CONFIGDIR)$(S)src$(S)config$(S)core.h)
XEAVER  = $(word 3, $(XSTR1))

ROOTDIR = ..
BINDIR  = $(ROOTDIR)$(S)bin
INCDIR  = $(ROOTDIR)$(S)include$(S)xtensa
LIBXOS  = libxos.a
CC      = xt-clang
MODE   ?= DBG

# Warning options
WARNOPT = -Werror -Wall -Wextra -Wunused -Wundef -Wstrict-prototypes -Wpointer-arith \
	  -Wcast-qual -Wformat-security -Wmissing-prototypes -Wshadow -Wsign-compare -pedantic

# Extra warning flags for use with xt-clang
XWARN   = -Wshadow-all -Wunreachable-code-aggressive -Wduplicate-enum \
	  -Wcompare-distinct-pointer-types -Warray-bounds-pointer-arithmetic -Wvla

ifeq ($(CC),xt-clang)
CFLAGS  = $(CSTD) $(WARNOPT) $(XWARN)
else
CFLAGS  = -clang $(CSTD) $(WARNOPT)
endif

# Common for all modes
CFLAGS  += -mno-coproc -mlongcalls -ffunction-sections -mno-l32r-flix -fno-builtin
ASFLAGS += -mno-coproc -mlongcalls -ffunction-sections -mno-l32r-flix -Wa,--schedule

ifeq ($(MODE), OPT)
CFLAGS  += -O3 -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
ASFLAGS += -O3 -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
endif

ifeq ($(MODE), DEV)
CFLAGS  += -Os -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
ASFLAGS += -Os -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
endif

ifeq ($(MODE), DBG)
CFLAGS  += -Os -g -DXOS_OPT_STATS=1 -DXOS_DEBUG_ALL=1 $(CFLAGS_EXTRA)
ASFLAGS += -Os -g -DXOS_OPT_STATS=1 -DXOS_DEBUG_ALL=1 $(CFLAGS_EXTRA)
endif

# This option will build a minimum size version.
ifeq ($(MODE), SIZE)
OFLAGS   = -DXOS_INT_STACK_SIZE=4096 -DXOS_OPT_STACK_CHECK=0 -DXOS_OPT_THREAD_SAFE_CLIB=0 -DXOS_OPT_STATS=0
CFLAGS  += -Os -g $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -Os -g $(OFLAGS) $(CFLAGS_EXTRA)
endif

# This option will build the fastest version. Note the XOS option flags.
ifeq ($(MODE), FAST)   
OFLAGS   = -DXOS_OPT_STACK_CHECK=0 -DXOS_OPT_THREAD_SAFE_CLIB=0 -DXOS_OPT_STATS=0
CFLAGS  += -O3 $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -O3 $(OFLAGS) $(CFLAGS_EXTRA)
endif


# File-specific flags
FLAGS_xos_vectors_xea3 = -mtext-section-literals


OBJS    = \
	xos.o           \
	xos_barrier.o   \
	xos_blockmem.o  \
	xos_clib.o      \
	xos_cond.o      \
	xos_coproc_handler.o \
	xos_display.o   \
	xos_event.o     \
	xos_exc_handler.o \
	xos_hwprofile.o \
	xos_hwprofile_asm.o \
	xos_idma.o      \
	xos_int_table.o \
	xos_msgq.o      \
	xos_mutex.o     \
	xos_once.o      \
	xos_semaphore.o \
	xos_sm.o \
	xos_start_main.o \
	xos_stats.o     \
	xos_system_check.o \
	xos_system_timer.o \
	xos_thread.o    \
	xos_timeofday.o \
	xos_timer.o     \
	xos_tls.o       \
	xos_usermode.o

ifeq ($(XEAVER), 2)
OBJS    += \
	xos_handlers.o    \
	xos_int_stack.o   \
	xos_vectors.o     \
	xos_window_vectors.o
endif

ifeq ($(XEAVER), 3)
OBJS    += \
	xos_handlers_xea3.o \
	xos_vectors_xea3.o
endif

XOBJS   = $(addprefix $(BINDIR)/,$(OBJS))

XPORT_INCS = $(addprefix $(INCDIR)/,$(wildcard *.h))

# Targets

all : lib inc

mkdir : $(BINDIR)$(S).exists $(INCDIR)$(S).exists

%$(S).exists :
	$(MKPATH) $(dir $@)
	@echo "" > $@

lib : mkdir $(BINDIR)$(S)$(LIBXOS)

inc : mkdir $(XPORT_INCS)

$(INCDIR)/%.h : %.h
	$(CP) $< $(INCDIR)$(S)$<

$(BINDIR)$(S)$(LIBXOS) : $(XOBJS)
	-$(RM) $@
	xt-ar rs $@ $^

clean:
	-$(RM_R) $(BINDIR)
	-$(RM_R) $(INCDIR)


# Include dependency rules (generated using -MD)
-include $(wildcard $(BINDIR)/*.d)

$(BINDIR)/%.o: %.S
	$(CC) $(ASFLAGS) $(FLAGS_$*) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(FLAGS_$*) -MD -MF $(subst .o,.d,$@) -c -o $@ $<


.PHONY : all clean mkdir lib inc

