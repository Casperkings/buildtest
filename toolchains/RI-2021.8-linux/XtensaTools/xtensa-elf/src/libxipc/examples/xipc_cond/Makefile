default: all

include ../Makefile.common

XTENSA_TOOLS = `xt-run --xtensa-core=$(CORE) --show-config=xttools`
TC_TOOLS = `xt-run  --xtensa-core=$(CORE) --show-config=tctools`

CC = xt-clang --xtensa-core=$(CORE)

INCLUDES = -I../sys-x86_64-linux/subsys.xt_sysbuild/include -I../common

CFLAGS = -g -O2 -Wall

SHARED_CFILES = shared.c

LAST_CORE_ID=$(shell echo "$(NUM_CORES)-1" | bc)
CORE_IDS=$(shell seq 0 $(LAST_CORE_ID))

CORES = $(addprefix DSP_,$(CORE_IDS))

CFILES = $(addsuffix .c,$(CORES))
OBJS = $(addsuffix .o,$(CORES))
EXES = $(addsuffix .exe,$(CORES))

XTSC_RUN_BINARY_DEF = $(addprefix --define=,$(join $(addsuffix _BINARY=,$(CORES)), $(EXES)))

SHARED_OBJS = $(SHARED_CFILES:.c=.o)

$(OBJS): %.o : %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

shared.o: shared.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

../common/xmp_mpu_attr.o: ../common/xmp_mpu_attr.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

shared_buf.o: shared_buf.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

shared.exe : $(SHARED_OBJS)
	$(CC) $(SHARED_OBJS) -o $@ -mlsp=../sys-x86_64-linux/$(XTSYS)/package/cores/shared/xtensa-elf/lib/shared

$(EXES): %.exe: %.o ../common/xmp_mpu_attr.o shared_buf.o shared.exe
	$(CC) $< ../common/xmp_mpu_attr.o shared_buf.o -L../sys-x86_64-linux/subsys.xt_sysbuild/xipc_sys -lxipc-xtos -lxipc_sys.$* $(LIB_IDMA) -lxtutil -o $@ -mlsp=../sys-x86_64-linux/$(XTSYS)/package/cores/$*/xtensa-elf/lib/sim -Wl,--just-symbols=shared.exe

all: shared.exe $(EXES)

build:
	for C in $(CFILES) ; do \
    cp core_template.c $$C; \
	done

xtsc-run:
	xtsc-run $(XTSC_RUN_BINARY_DEF) --include=../sys-x86_64-linux/subsys.xt_sysbuild/xtsc-run/TestSubSys.inc

clean:
	rm -f ../common/*.o *.o *.exe DSP*.c

.PHONY: default all clean build
