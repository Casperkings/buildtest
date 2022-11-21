# Copyright (c) 2003-2015 Cadence Design Systems, Inc.
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
#

# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

CC = $(XTTOOLS)/bin/xt-clang
AR = $(XTTOOLS)/bin/xt-ar
INCLUDES = 
BUILD_DIR ?= ./bin

default: all

CFLAGS = -g -Wall -Werror -mlongcalls

ifeq ($(BUILD_SUBSYS),Custom)
  CFLAGS += -DXIPC_CUSTOM_SUBSYSTEM
endif

ifeq ($(BUILD_TARGET),DEBUG)
  CFLAGS += -O0 -DXIPC_DEBUG -DXIPC_VERIFY
else
  CFLAGS += -O2 -mno-serialize-volatile
endif

ifeq ($(BUILD_PROFILE),1)
	CFLAGS += -DXIPC_PROFILE
endif

CFILES = \
  xipc_rpc.c \
  xipc_counted_event.c \
  xipc_pkt_channel.c \
  xipc_msg_channel.c \
  xipc_cqueue.c \
  xipc_mutex.c \
  xipc_barrier.c \
  xipc_cond.c \
  xipc_init.c \
  xipc_channel_init.c \
  xipc_sem.c

ifeq ($(BUILD_SUBSYS),Custom)
  CFILES += xipc_custom_addr.c
  CFILES += xipc_custom_intr.c
else
  CFILES += xipc_addr.c
  CFILES += xipc_intr.c
endif

CFILES_XTOS = xipc_xtos.c
CFILES_XOS = xipc_xos.c

OBJ_XTOS_FILES = $(CFILES:.c=.xtos.o) $(CFILES_XTOS:.c=.o)
OBJ_XOS_FILES = $(CFILES:.c=.xos.o) $(CFILES_XOS:.c=.o)
OBJ_FILES = $(CFILES:.c=.o)

DEP_XTOS_FILES = $(CFILES:.c=.xtos.d) $(CFILES_XTOS:.c=.d)
DEP_XOS_FILES = $(CFILES:.c=.xos.d) $(CFILES_XOS:.c=.d)
DEP_FILES = $(CFILES:.c=.d)

OBJS_XTOS = $(addprefix $(BUILD_DIR)/,$(OBJ_XTOS_FILES))
OBJS_XOS = $(addprefix $(BUILD_DIR)/,$(OBJ_XOS_FILES))
OBJS = $(addprefix $(BUILD_DIR)/,$(OBJ_FILES))

DEPS_XTOS = $(addprefix $(BUILD_DIR)/,$(DEP_XTOS_FILES))
DEPS_XOS = $(addprefix $(BUILD_DIR)/,$(DEP_XOS_FILES))
DEPS = $(addprefix $(BUILD_DIR)/,$(DEP_FILES))

LIB_XTOS = libxipc-xtos.a
LIB_XOS = libxipc-xos.a
LIB = libxipc.a

$(BUILD_DIR)/$(LIB_XTOS) : $(OBJS_XTOS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XOS) : $(OBJS_XOS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB) : $(OBJS)
	$(AR) rc $@ $^

mkbuild_dir:
	$(MKPATH) $(BUILD_DIR)

all: mkbuild_dir $(BUILD_DIR)/$(LIB_XTOS) $(BUILD_DIR)/$(LIB_XOS) $(BUILD_DIR)/$(LIB) $(DEPS_FILE)

clean:
	$(RM_R) $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR)/%.xtos.o : %.c
	$(CC) $(INCLUDES) -DXIPC_USE_XTOS $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) -DXIPC_USE_XTOS $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.xtos.d

$(BUILD_DIR)/%.xos.o : %.c
	$(CC) $(INCLUDES) -DXIPC_USE_XOS $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) -DXIPC_USE_XOS $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.xos.d

$(BUILD_DIR)/%.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.d


.PHONY: default all clean
