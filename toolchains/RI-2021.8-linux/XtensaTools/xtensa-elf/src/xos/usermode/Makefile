
CC   = xt-clang
ISS  = xt-run

ISSFLAGS = --mem_model --nosummary
CFLAGS   = -std=c11 -Os -g -Wall -mlongcalls
PILFLAGS = -Wl,--multi-section-pil -Wl,--shared-pagesize=128 -Wl,--no-relax -Wl,-pie -mlsp=pimultload
LIBS_PIL = -lxtutil -lsim -lmloader -lc -lhal

CFLAGS_EXE = -ffunction-sections -I../include -DXOS_OPT_STATS=1 -DXOS_DEBUG_ALL=1 -DXOS_OPT_UM=1 -mlsp=sim
LIBS_EXE   = -L../bin -lxos -lxtutil -lxmem -lmloader

# CORE has to be defined, and a matching MPU setup file provided.

CORE ?= sample_controller

# List of tests

TESTS = \
    example_basic \
    example_sem \
    example_mutex \
    example_thread

TEST_DLS = $(addsuffix .dynlib,$(TESTS))


# Targets

all : um_test.exe $(TEST_DLS)

idma : idma_test.exe example_idma.dynlib

# Special rules for the idma targets

example_idma.o : example_idma.c
	$(CC) $(CFLAGS) -c -o $@ $^ -DIDMA_USERMODE

example_idma.lib : example_idma.o
	$(CC) $(CFLAGS) $(PILFLAGS) -o $@ $^ $(LIBS_PIL) -lidma-um

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.lib : %.o
	$(CC) $(CFLAGS) $(PILFLAGS) -o $@ $^ $(LIBS_PIL)

%.dynlib : %.lib
	xt-pkg-loadlib -v -p -e $* -s $@ -o $*.p $^
	rm $*.p

um_test.exe : um_test.c $(CORE)_mpu.c
	$(CC) $(CFLAGS) $(CFLAGS_EXE) -o $@ $^ $(LIBS_EXE)

idma_test.exe : idma_test.c $(CORE)_mpu.c
	$(CC) $(CFLAGS) $(CFLAGS_EXE) -o $@ $^ $(LIBS_EXE)

clean :
	rm -rf *.exe *.o *.lib *.dynlib *.p

.PHONY : all clean

