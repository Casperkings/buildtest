
# Makefile  --  Rules and commands for building XOS tests/examples.

# Copyright (c) 2015 Cadence Design Systems, Inc.
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

#------------------------------------------------------------------------------
# See README.TXT for more information.
#------------------------------------------------------------------------------

# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

# Detect C library
XSTR2   = $(shell $(GREP) SW_CLibrary $(CONFIGDIR)$(S)config$(S)default-params)
CLIB    = $(word 3, $(XSTR2))

ifeq ($(CLIB), xclib)
CSTD    = -std=c11
CXXSTD  = -std=c++11
else
CSTD    = -std=c99
CXXSTD  = -std=c++98
endif

ROOTDIR = ..
SRCDIR  = $(ROOTDIR)$(S)src
BINDIR  = $(ROOTDIR)$(S)bin
INCDIR  = $(ROOTDIR)$(S)include
CC      = xt-clang $(CSTD)
CXX     = xt-clang++ $(CXXSTD)
MODE   ?= DBG
TARGET ?= SIM


# Common for all modes
CFLAGS  = -Wall -Werror -mlongcalls -ffunction-sections -I$(INCDIR)
ASFLAGS = -Wall -Werror -mlongcalls -ffunction-sections -Wa,--schedule -I$(INCDIR)
LDFLAGS = -Wl,--gc-sections

ifeq ($(CC),xt-xcc)
CFLAGS += -clang
endif

ifeq ($(MODE), OPT)
CFLAGS  += -O3 -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
ASFLAGS += -O3 -g -DXOS_OPT_STATS=1
endif

ifeq ($(MODE), DEV)
CFLAGS  += -Os -g -DXOS_OPT_STATS=1 $(CFLAGS_EXTRA)
ASFLAGS += -Os -g -DXOS_OPT_STATS=1
endif

ifeq ($(MODE), DBG)
CFLAGS  += -Os -g -DXOS_OPT_STATS=1 -DXOS_DEBUG_ALL=1 $(CFLAGS_EXTRA)
ASFLAGS += -Os -g -DXOS_OPT_STATS=1 -DXOS_DEBUG_ALL=1
endif

ifeq ($(MODE), SIZE)
OFLAGS   = -DXOS_INT_STACK_SIZE=4096 -DXOS_OPT_STACK_CHECK=0 -DXOS_OPT_THREAD_SAFE_CLIB=0 -DXOS_OPT_STATS=0
CFLAGS  += -Os -g $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -Os -g $(OFLAGS)
endif

ifeq ($(MODE), FAST)
OFLAGS   = -DXOS_OPT_STACK_CHECK=0 -DXOS_OPT_THREAD_SAFE_CLIB=0 -DXOS_OPT_STATS=0
CFLAGS  += -O3 $(OFLAGS) $(CFLAGS_EXTRA)
ASFLAGS += -O3 $(OFLAGS)
endif

ifeq ($(TARGET), SIM)
LSP = sim
endif

ifeq ($(TARGET), ML605)
CFLAGS += -DBOARD=1
LSP     = xtml605-rt
endif

ifeq ($(TARGET), KC705)
CFLAGS += -DBOARD=1
LSP     = xtkc705-rt
endif


VPATH = ..


all : abort blockmem clib cond coproc cpp event exc malloc msg multi mutex perf sem log timeout timer tod tls

abort : makexos $(BINDIR)/abort_test

blockmem : makexos $(BINDIR)/blockmem_test

clib : makexos $(BINDIR)/clib_test

cond : makexos $(BINDIR)/cond_test

coproc : makexos $(BINDIR)/coproc_test

cpp : makexos $(BINDIR)/cpp_test

event : makexos $(BINDIR)/event_test

exc : makexos $(BINDIR)/exception_test

log : makexos $(BINDIR)/log_test

malloc : makexos $(BINDIR)/malloc_test

msg : makexos $(BINDIR)/msg_test

multi : makexos $(BINDIR)/multi_test

mutex : makexos $(BINDIR)/mutex_test

perf : makexos $(BINDIR)/perf_test

sem : makexos $(BINDIR)/sem_test

small : makexos $(BINDIR)/small_test

timeout : makexos $(BINDIR)/timeout_test

timer : makexos $(BINDIR)/timer_test

tod : makexos $(BINDIR)/tod_test

tls : makexos $(BINDIR)/tls_test


### Needs XOS_OPT_STATS ###
stats : makexos $(BINDIR)/stats_test

$(BINDIR)/cpp_test : cpp_test.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil

$(BINDIR)/perf_test : perf_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mno-serialize-volatile -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil

$(BINDIR)/stats_test : stats_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil

$(BINDIR)/coproc_test : coproc_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil

$(BINDIR)/small_test : small_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos

$(BINDIR)/multi_test : multi_test.c xos_display_tiny.c $(SRCDIR)/mpu_init.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil -lm

$(BINDIR)/% : %.c xos_display_tiny.c
	$(CC) $(CFLAGS) $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(BINDIR) -lxos -lxtutil


makexos :
	cd $(SRCDIR) && xt-make --no-print-directory all MODE=$(MODE)


$(BINDIR)/%.o: %.S
	$(CC) $(ASFLAGS) $(FLAGS_$*) -o $@ -c $<

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS)  $(FLAGS_$*) -o $@ -c $<

clean:
	cd $(SRCDIR) && xt-make --no-print-directory clean
	-$(RM) *.o

check :
	xt-run $(BINDIR)$(S)abort_test
	xt-run $(BINDIR)$(S)blockmem_test
	xt-run $(BINDIR)$(S)clib_test
	xt-run $(BINDIR)$(S)cond_test
	xt-run $(BINDIR)$(S)coproc_test
	xt-run $(BINDIR)$(S)cpp_test
	xt-run $(BINDIR)$(S)event_test
	xt-run $(BINDIR)$(S)exception_test
	xt-run $(BINDIR)$(S)msg_test
	xt-run $(BINDIR)$(S)mutex_test
	xt-run $(BINDIR)$(S)perf_test
	xt-run $(BINDIR)$(S)sem_test
	xt-run $(BINDIR)$(S)log_test
	xt-run $(BINDIR)$(S)timeout_test
	xt-run $(BINDIR)$(S)timer_test

.PHONY : all clean makexos check

