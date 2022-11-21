# Copyright (c) 2019 Cadence Design Systems, Inc.
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
INSTALL_DIR ?= ./install

$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(INSTALL_DIR))

default: all

CFLAGS = -g -Wall -Werror -mlongcalls -mno-serialize-volatile -fno-jump-tables

ifeq ($(BUILD_TARGET),DEBUG)
  CFLAGS += -O0 -DXMEM_DEBUG -DXMEM_VERIFY
else
  CFLAGS += -O2
endif

XMEM_BANK_INC_HEADERS = xmem_bank.h xmem_bank_common.h
XMEM_INC_HEADERS = xmem.h

XMEM_BANK_CFILES = \
  xlmem.c          \
  xmem_bank.c      \
  xmem_l2ram.c

XMEM_CFILES =  \
  xmem_stack.c \
  xmem_heap.c

XMEM_BANK_OBJ_FILES = $(XMEM_BANK_CFILES:.c=.o)
XMEM_OBJ_FILES = $(XMEM_CFILES:.c=.o)

XMEM_BANK_XTOS_OBJ_FILES = $(XMEM_BANK_CFILES:.c=.xtos.o)
XMEM_XTOS_OBJ_FILES = $(XMEM_CFILES:.c=.xtos.o)

XMEM_BANK_XOS_OBJ_FILES = $(XMEM_BANK_CFILES:.c=.xos.o)
XMEM_XOS_OBJ_FILES = $(XMEM_CFILES:.c=.xos.o)

XMEM_BANK_DEP_FILES = $(XMEM_BANK_CFILES:.c=.d)
XMEM_DEP_FILES = $(XMEM_CFILES:.c=.d)

XMEM_BANK_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_BANK_OBJ_FILES))
XMEM_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_OBJ_FILES))

XMEM_BANK_XTOS_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_BANK_XTOS_OBJ_FILES))
XMEM_XTOS_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_XTOS_OBJ_FILES))

XMEM_BANK_XOS_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_BANK_XOS_OBJ_FILES))
XMEM_XOS_OBJS = $(addprefix $(BUILD_DIR)/,$(XMEM_XOS_OBJ_FILES))

LIB_XMEM_BANK = libxmem-bank.a
LIB_XMEM = libxmem.a
LIB_XMEM_BANK_XTOS = libxmem-bank-xtos.a
LIB_XMEM_XTOS = libxmem-xtos.a
LIB_XMEM_BANK_XOS = libxmem-bank-xos.a
LIB_XMEM_XOS = libxmem-xos.a

$(BUILD_DIR)/$(LIB_XMEM_BANK) : $(XMEM_BANK_OBJS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XMEM) : $(XMEM_OBJS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XMEM_BANK_XTOS) : $(XMEM_BANK_XTOS_OBJS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XMEM_XTOS) : $(XMEM_XTOS_OBJS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XMEM_BANK_XOS) : $(XMEM_BANK_XOS_OBJS)
	$(AR) rc $@ $^

$(BUILD_DIR)/$(LIB_XMEM_XOS) : $(XMEM_XOS_OBJS)
	$(AR) rc $@ $^

all: $(BUILD_DIR)/$(LIB_XMEM_BANK) $(XMEM_BANK_DEPS_FILE) \
     $(BUILD_DIR)/$(LIB_XMEM) $(XMEM_DEPS_FILE) \
     $(BUILD_DIR)/$(LIB_XMEM_BANK_XTOS) $(BUILD_DIR)/$(LIB_XMEM_XTOS) \
     $(BUILD_DIR)/$(LIB_XMEM_BANK_XOS) $(BUILD_DIR)/$(LIB_XMEM_XOS)

install: $(BUILD_DIR)/$(LIB_XMEM_BANK) $(BUILD_DIR)/$(LIB_XMEM) \
         $(BUILD_DIR)/$(LIB_XMEM_BANK_XTOS) $(BUILD_DIR)/$(LIB_XMEM_XTOS) \
         $(BUILD_DIR)/$(LIB_XMEM_BANK_XOS) $(BUILD_DIR)/$(LIB_XMEM_XOS)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM_BANK) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM_BANK_XTOS) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM_XTOS) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM_BANK_XOS) $(INSTALL_DIR)
	cp -rf $(BUILD_DIR)/$(LIB_XMEM_XOS) $(INSTALL_DIR)
	cp -rf $(XMEM_INC_HEADERS) $(INSTALL_DIR)
	cp -rf $(XMEM_BANK_INC_HEADERS) $(INSTALL_DIR)

clean:
	$(RM_R) $(BUILD_DIR)
	$(RM_R) $(INSTALL_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR)/%.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.xtos.o : %.c
	$(CC) $(INCLUDES) -DXMEM_XTOS $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) -DXMEM_XTOS $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.xtos.d

$(BUILD_DIR)/%.xos.o : %.c
	$(CC) $(INCLUDES) -DXMEM_XOS $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) -DXMEM_XOS $(CFLAGS) -MM $< -MT $@ > $(BUILD_DIR)/$*.xos.d

.PHONY: default all clean install
