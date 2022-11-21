# This directory contains examples that demonstrates usage of the
# xmem library API

.PHONY: all test clean

default: all

CC = xt-clang --xtensa-core=$(XTENSA_CORE)

CFLAGS = -g -Wall -O2

LIBS = -lxmem-bank-xtos -lxmem-xtos
LIBS_XOS = -lxmem-bank-xos -lxmem-xos -lxos

EXAMPLES = stack heap bank l2ram
XOS_EXAMPLES = bank_wait bank_thread

CFILES = $(addsuffix .c,$(EXAMPLES $(XOS_EXAMPLES)))
OBJS = $(addsuffix .o,$(EXAMPLES) $(XOS_EXAMPLES))
EXES = $(addsuffix .exe,$(EXAMPLES))
XOS_EXES = $(addsuffix .exe,$(XOS_EXAMPLES))

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(EXES): %.exe: %.o
	$(CC) $< $(LIBS) -o $@

$(XOS_EXES): %.exe: %.o
	$(CC) $< $(LIBS_XOS) -o $@

all: test

test: $(EXES) $(XOS_EXES)
	$(foreach exe,$(EXES) $(XOS_EXES),echo "Running $(exe)"; xt-run --xtensa-core=$(XTENSA_CORE) $(exe); echo;)

clean:
	-rm -f *.o *.exe
