
# Makefile for libidma examples. See README for more information.
#
# Copyright (c) 2014-2020 Cadence Design Systems. ALL RIGHTS RESERVED.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#----------------------------------------------------------------------
# Include platform-specific commands.
#----------------------------------------------------------------------
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

#----------------------------------------------------------------------
# Detect whether config has IDMA. Used to define build targets.
#----------------------------------------------------------------------
XSTR1     = $(shell $(GREP) XCHAL_HAVE_IDMA $(CONFIGDIR)$(S)src$(S)config$(S)core.h)
HAVE_IDMA = $(word 3, $(XSTR1))


ROOTDIR       ?= .
BINDIR        ?= $(ROOTDIR)$(S)bin
SRCDIR        ?= .
CC_FOR_TARGET ?= xt-clang
CFLAGS        ?= -O2 -g -Wall -Werror

ifeq ($(GREP),)
ifeq ($(HOSTTYPE),win)
GREP = findstr
else
GREP = grep
endif
endif

vpath %.c $(SRCDIR)


EXAMPLES = 				\
  test_task.c				\
  test_task_2D.c			\
  test_task_poll.c			\
  test_buf_err.c			\
  test_Dbuf_in.c		        \
  test_Dbuf_in_hw_schedule.c            \
  test_Dbuf_in_fast_schedule.c          \
  test_Dbuf_out_callback.c		\
  test_Dbuf_out_poll.c			\
  test_2ch_simultaneous.c               \
  test_pause_resume.c			\
  test_task_add_64B.c                   \
  test_task_copy_64B.c                  \
  test_desc_64B.c                       \
  test_task_add_multi_64B.c             \
  test_multichannel.c                   \
  test_wait_all.c			\
  test_axi2axi.c

CUSTOMOSFILES =

SHARED = 			\
	idma_tests.h

XTOSLIB_D = -lidma-debug-xtos
IDMALIB_D = -lidma-debug
XTOSLIB   = -lidma-xtos
IDMALIB   = -lidma

XTOSTESTPROGS    = $(addprefix $(BINDIR)/,$(EXAMPLES:.c=-xtos.test))
OSTESTPROGS      = $(addprefix $(BINDIR)/,$(EXAMPLES:.c=-os.test))
XTOSTESTPROGS_D  = $(addprefix $(BINDIR)/,$(EXAMPLES:.c=-debug-xtos.test))
OSTESTPROGS_D    = $(addprefix $(BINDIR)/,$(EXAMPLES:.c=-debug-os.test))


all : all-target

$(BINDIR)/%-debug-xtos.test : %.c $(SHARED)
	$(CC_FOR_TARGET) $(CFLAGS) -o $@ $< $(XTOSLIB_D) -DIDMA_DEBUG  -D_XTOS -lxtutil

$(BINDIR)/%-debug-os.test : %.c $(SHARED)
	$(CC_FOR_TARGET) $(CFLAGS) -o $@ $< $(CUSTOMOSFILES) $(IDMALIB_D) -DIDMA_DEBUG -lxtutil

$(BINDIR)/%-xtos.test : %.c $(SHARED)
	$(CC_FOR_TARGET) $(CFLAGS) -o $@ $< $(XTOSLIB)  -D_XTOS -lxtutil

$(BINDIR)/%-os.test : %.c $(SHARED)
	$(CC_FOR_TARGET) $(CFLAGS) -o $@ $< $(CUSTOMOSFILES) $(IDMALIB) -lxtutil

$(BINDIR)/test_pso_xtos_faked.test : test_pso_xtos_faked.c  $(SHARED)
	$(CC_FOR_TARGET) $(CFLAGS) -o $@ $< $(XTOSLIB)  -D_XTOS -lxtutil


ifneq ($(HAVE_IDMA),1)
TARGETS = 
else
TARGETS = $(OSTESTPROGS)  $(OSTESTPROGS_D)  $(XTOSTESTPROGS)  $(XTOSTESTPROGS_D)
endif

targets : $(TARGETS)

mkdir : $(BINDIR)$(S).exists

%$(S).exists :
	$(MKPATH) $(dir $@)
	@echo "" > $@

all-target : mkdir
	@$(MAKE) --no-print-directory -f Makefile targets

clean :
	-$(RM_R) $(BINDIR)

.PHONY: all clean all-target mkdir targets

