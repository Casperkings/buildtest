default: all

include ../Makefile.common

XTENSA_TOOLS = `xt-run --xtensa-core=$(CORE) --show-config=xttools`
TC_TOOLS = `xt-run  --xtensa-core=$(CORE) --show-config=tctools`

CC = xt-clang --xtensa-core=$(CORE)

INCLUDES = -I../sys-x86_64-linux/subsys.xt_sysbuild/include -I../common

CFLAGS = -g -O2 -Wall

LAST_CORE_ID=$(shell echo "$(NUM_CORES)-1" | bc)
CORE_IDS=$(shell seq 0 $(LAST_CORE_ID))

CORES = $(addprefix DSP_,$(CORE_IDS))

CFILES = core_template.c
OBJS = $(CFILES:.c=.o)
EXE = a.out
EXES = $(foreach $(EXE),$(CORES),$(EXE))

XTSC_RUN_BINARY_DEF = $(addprefix --define=,$(join $(addsuffix _BINARY=,$(CORES)), $(EXES)))

%.o : %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

../common/xmp_mpu_attr.o: ../common/xmp_mpu_attr.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

$(EXE): $(OBJS) ../common/xmp_mpu_attr.o
	$(CC) $< ../common/xmp_mpu_attr.o -L../sys-x86_64-linux/subsys.xt_sysbuild/xipc_sys -lxipc-xtos -lxipc_sys.DSP_0 $(LIB_IDMA) -lxtutil -o $@ -mlsp=sim-mc

all: $(EXE)

xtsc-run: $(EXE)
	xtsc-run $(XTSC_RUN_BINARY_DEF) --include=../sys-x86_64-linux/subsys.xt_sysbuild/xtsc-run/TestSubSys.inc

clean:
	rm -f ../common/*.o *.o a.out

.PHONY: default all clean build
