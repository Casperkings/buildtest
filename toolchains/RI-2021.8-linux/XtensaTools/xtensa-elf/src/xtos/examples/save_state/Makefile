# Makefile for XTOS "xtos_save_system_state" test.

# Copyright (c) 2006-2008 Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.

CC = xt-clang
CFLAGS = -g -O0 -I../../../../Xtensa/OS/xtos

TARGET = save_state
OBJS = main.o set_reg_values.o


.PHONY: run clean


run: $(TARGET)
	xt-run $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -rf $(TARGET) $(OBJS) ram_image.bin load_cmds.txt
