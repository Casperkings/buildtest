# Copyright (c) 2020-2021 Cadence Design Systems, Inc.
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

# Makefile -- Rules and commands for building example


SECMON_ROOT		= $(abspath $(CURDIR)/../..)

# Force custom build so we can regenerate MPU table
MODE			= CUSTOM

CXXFLAGS		= -O2 -g -mlongcalls -mno-l32r-flix -Wall -Werror

XMON_NSEC_O		=		\
	nonsecure_app_wrapper.o

XMON_NSEC_SIM_O		=		\
	xmon-iss-asm_S.o		\
	xmon-main-iss.o

XMON_NSEC_BRD_O		=		\
	xtbsp-uart-axi-lite_2v0.o	\
	xmon-main-serial.o

SECMON_SRC		= secure_user_init.c

ALL_OVERRIDE		= 1
SEC_OVERRIDE		= 1
NSM_OVERRIDE		= 1
RUN_OVERRIDE		= 1
CLEAN_OVERRIDE		= 1

include $(SECMON_ROOT)/examples/Makefile.inc

ifndef TGTDIR
TGTDIR			= .$(S)
endif

LIBXMON_NSM		= $(TGTDIR)libxmon-nsm.a
LIBXMON_IN		= $(CONFIGDIR)/xtensa-elf/arch/lib/libxmon.a

TESTDIR   		?= .
GDBCMDFILE		= $(TESTDIR)/gdbcmds.txt 
GDBPORT			= 20123
CYCLE_LIMIT		= 20000000

XTTOOLS			= $(shell xt-clang --show-config=xttools)
CFLAGS			+= -I$(XTTOOLS)/xtensa-elf/include/xtensa/xtkc705 


all : $(TARGET_LIST)

$(LIBXMON_NSM) : $(LIBXMON_IN)
	xt-objcopy -p --rename-section .DebugExceptionVector.text=.text.DebugExceptionVector $< $@

$(TGTDIR)$(SECSIM_TGT) : $(SECMON_SRC) $(SECMON_SIM_O) $(SECMON_SIM_A)
	$(CC) $(CFLAGS) $(SMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_SIM) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(SECBRD_TGT) : $(SECMON_SRC) $(SECMON_BRD_O) $(SECMON_BRD_A)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_BRD) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(NSECSIM_TGT) : $(TGTDIR)$(XMON_NSEC_O) $(addprefix $(TGTDIR),$(XMON_NSEC_SIM_O)) $(LIBSECMON_O) $(LIBSECMON_A) $(LIBXMON_NSM)
	$(CC) $(CFLAGS) $(NSMCFLAGS_SIM) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_SIM_LFLAGS) -o $@
	$(ELF2ROM) $@ $(patsubst %.exe,%.bin,$@)
	xt-objdump -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(NSECBRD_TGT) : $(TGTDIR)$(XMON_NSEC_O) $(addprefix $(TGTDIR),$(XMON_NSEC_BRD_O)) $(LIBSECMON_O) $(LIBSECMON_A) $(LIBXMON_NSM)
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_BRD_LFLAGS) -o $@
	$(ELF2ROM) $@ $(patsubst %.exe,%.bin,$@)
	xt-objdump -D $@ > $(patsubst %.exe,%.dis,$@)

ifeq ($(HOSTTYPE),win)
$(TGTDIR)$(S)%_S.o : %.S
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) -c $^ -o $@

$(TGTDIR)$(S)%.o : %.c
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) -c $^ -o $@
else
$(TGTDIR)%_S.o : %.S
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) -c $^ -o $@

$(TGTDIR)%.o : %.c
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) -c $^ -o $@
endif

test : $(TGTDIR)$(LOG_TGT)

$(TGTDIR)$(LOG_TGT) : $(TGTDIR)$(NSECSIM_TGT) $(TGTDIR)$(SECSIM_TGT)
ifeq ($(HOSTTYPE),win)
	@echo Test skipped on Windows... >> $@
	@echo TEST_OVERALL_RESULT: PASSED >> $@
else
	-cd $(dir $@) && xt-gdb --command=$(GDBCMDFILE) $< &
	-cd $(dir $@) && xt-run $(RUNFLAGS) --cycle_limit=$(CYCLE_LIMIT) --loadbin=$(TGTDIR)$(NSECSIM_BIN)@$(NSM_LOC) $(TGTDIR)$(SECSIM_TGT) unpack=$(NSM_LOC) port=$(GDBPORT) > $(abspath $@) 2>&1
	@$(GREPI) "Passed" $@ && echo "TEST_OVERALL_RESULT: PASSED" >> $@ || echo "TEST_OVERALL_RESULT: FAILED" >> $@
endif

clean :
	-$(RM) *.exe *.bin *.o *.dis *.log
	-$(RM_R) $(LIBXMON_NSM)

