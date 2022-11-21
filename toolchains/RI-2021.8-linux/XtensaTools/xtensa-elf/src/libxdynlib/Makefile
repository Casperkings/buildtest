# Copyright (c) 2021 Cadence Design Systems, Inc.
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
CONFIGDIR = $(shell xt-run --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

TOOLS = $(shell xt-run --show-config=xttools)/Tools

HOST_CC ?= $(TOOLS)/bin/gcc
HOST_AR ?= $(TOOLS)/bin/ar

CC = $(XTTOOLS)/bin/xt-clang
AR = $(XTTOOLS)/bin/xt-ar

INCLUDES = 
BUILD_DIR ?= ./bin
INSTALL_DIR ?= ./install

$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(INSTALL_DIR))

default: all

CFLAGS = -g -std=c99 -Wall -Werror -mlongcalls -fno-jump-tables -DXDYN_LIB_XTOS
CFLAGS_XOS = $(CFLAGS) -UXDYN_LIB_XTOS -DXDYN_LIB_XOS
HOST_CFLAGS = -g -std=c99 -Wall -Werror -I $(XTTOOLS)/xtensa-elf/include

ifeq ($(DEBUG),1)
  CFLAGS += -O0 -DXDYN_LIB_DEBUG
  HOST_CFLAGS += -O0 -DXDYN_LIB_DEBUG
else
  CFLAGS += -O2
  HOST_CFLAGS += -O2
endif

XDYN_LIB_HEADERS = xdyn_lib.h

XDYN_LIB_HOST_CFILES = \
  xdyn_lib_host.c

XDYN_LIB_DEV_CFILES = \
  xdyn_lib_dev.c

XDYN_LIB_HOST_OBJ_FILES = $(XDYN_LIB_HOST_CFILES:.c=.host.o)
XDYN_LIB_HOST_DEP_FILES = $(XDYN_LIB_HOST_CFILES:.c=.d)
XDYN_LIB_HOST_OBJS = $(addprefix $(BUILD_DIR)/,$(XDYN_LIB_HOST_OBJ_FILES))
LIB_XDYN_LIB_HOST = libxdyn-lib-host.a
$(BUILD_DIR)/$(LIB_XDYN_LIB_HOST) : $(XDYN_LIB_HOST_OBJS)
	$(HOST_AR) rc $@ $^

XDYN_LIB_DEV_OBJ_FILES = $(XDYN_LIB_DEV_CFILES:.c=.dev.o)
XDYN_LIB_DEV_DEP_FILES = $(XDYN_LIB_DEV_CFILES:.c=.d)
XDYN_LIB_DEV_OBJS = $(addprefix $(BUILD_DIR)/,$(XDYN_LIB_DEV_OBJ_FILES))
LIB_XDYN_LIB_DEV = libxdyn-lib-dev.a
$(BUILD_DIR)/$(LIB_XDYN_LIB_DEV) : $(XDYN_LIB_DEV_OBJS)
	$(AR) rc $@ $^

XDYN_LIB_DEV_XOS_OBJ_FILES = $(XDYN_LIB_DEV_CFILES:.c=.dev.xos.o)
XDYN_LIB_DEV_XOS_DEP_FILES = $(XDYN_LIB_DEV_CFILES:.c=.xos.d)
XDYN_LIB_DEV_XOS_OBJS = $(addprefix $(BUILD_DIR)/,$(XDYN_LIB_DEV_XOS_OBJ_FILES))
LIB_XDYN_LIB_DEV_XOS = libxdyn-lib-dev-xos.a
$(BUILD_DIR)/$(LIB_XDYN_LIB_DEV_XOS) : $(XDYN_LIB_DEV_XOS_OBJS)
	$(AR) rc $@ $^

all: $(BUILD_DIR)/$(LIB_XDYN_LIB_HOST) $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV) \
     $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV_XOS)

install: $(BUILD_DIR)/$(LIB_XDYN_LIB_HOST) $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV) \
         $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV_XOS)
	cp -rf $(BUILD_DIR)/$(LIB_XDYN_LIB_HOST) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XDYN_LIB_DEV_XOS) $(INSTALL_DIR)
	cp -rf $(XDYN_LIB_HEADERS) $(INSTALL_DIR)

clean:
	$(RM_R) $(BUILD_DIR)
	$(RM_R) $(INSTALL_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR)/%.host.o : %.c
	$(HOST_CC) $(INCLUDES) $(HOST_CFLAGS) -c -o $@ $<
	$(HOST_CC) $(INCLUDES) $(HOST_CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.dev.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.dev.xos.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS_XOS) -c -o $@ $<
	$(CC) $(INCLUDES) $(CFLAGS_XOS) -MM $< -MT $@ > $(BUILD_DIR)/$*.xos.d

.PHONY: default all clean install
