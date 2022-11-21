# Copyright (c) 2015-2021 Cadence Design Systems, Inc.
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

# Makefile.inc -- Rules and commands for building Secure Monitor tests/examples.


# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

# Examples can be built in STANDARD mode, using libraries/objects from config build,
# or in CUSTOM mode, using libraries/objects from inline build.

MODE			?= STANDARD

# Determine memory configuration
DIGITS			= 0 1 2 3 4 5 6 7 8 9
IRAM_STR		= $(shell $(GREP) InstRAMSecureSegCount $(CONFIGDIR)$(S)config$(S)default-params)
SECURE_IRAM		= $(findstring $(word 4, $(IRAM_STR)),$(DIGITS))
IROM_STR		= $(shell $(GREP) InstROMSecureSegCount $(CONFIGDIR)$(S)config$(S)default-params)
SECURE_IROM		= $(findstring $(word 4, $(IROM_STR)),$(DIGITS))
DROM_STR		= $(shell $(GREP) DataROMSecureSegCount $(CONFIGDIR)$(S)config$(S)default-params)
SECURE_DROM		= $(findstring $(word 4, $(DROM_STR)),$(DIGITS))
SROM_STR		= $(shell $(GREP) XCHAL_HAVE_SECURE_SROM $(CONFIGDIR)$(S)xtensa-elf$(S)arch$(S)include$(S)xtensa$(S)config$(S)secure.h)
SECURE_SROM		= $(findstring $(word 3, $(SROM_STR)),$(DIGITS))
WWDT_STR		= $(shell $(GREP) XCHAL_HAVE_WWDT $(CONFIGDIR)$(S)xtensa-elf$(S)arch$(S)include$(S)xtensa$(S)config$(S)core-isa.h)
HAVE_WWDT		= $(findstring $(word 3, $(WWDT_STR)),$(DIGITS))

# Configs running from secure ROM must have secure SROM or secure DROM (for .rom.store table).
ifneq ($(SECURE_DROM),)
SECMON_IN_ROM		= 1
endif
ifeq ($(SECURE_SROM),1)
SECMON_IN_ROM		= 1
endif

CSTD			= -std=c11

ifndef SECMON_ROOT
SECMON_ROOT		= $(CURDIR)$(S)..
endif

SECMON_ROOT_FIX		= $(subst /,$(S),$(SECMON_ROOT))


SRCDIR			= $(SECMON_ROOT_FIX)$(S)src
BINDIR			?= $(SECMON_ROOT_FIX)$(S)bin
EXEDIR			= $(BINDIR)$(S)examples
INCDIR			?= $(SECMON_ROOT_FIX)$(S)include
ifndef XTOSSRCDIR
XTOSSRCDIR		= $(SECMON_ROOT_FIX)$(S)..$(S)xtos
endif

CC			= xt-clang
CXX			= xt-clang++
ifeq ($(HOSTTYPE),win)
CC_EXISTS		= $(shell $(CC) --help 2> NUL || echo "fail")
else
CC_EXISTS		= $(shell $(CC) --help 2> /dev/null || echo "fail")
endif
ifeq ($(CC_EXISTS),fail)
CC			= xt-xcc
CXX			= xt-xc++
endif
ELF2ROM			= xt-elf2rom
OBJDUMP			= xt-objdump
ifeq ($(HOSTTYPE),win)
GREPI			= $(GREP) /I
CAT			= type
else
GREPI			= $(GREP) -i
CAT			= cat
endif

# Warning options
WARNOPT			= -Werror -Wall -Wextra -Wunused -Wundef -Wstrict-prototypes -Wpointer-arith \
			  -Wcast-qual -Wformat-security -Wmissing-prototypes -Wshadow -Wsign-compare -pedantic

# Extra warning flags for use with xt-clang
XWARN			= -Wshadow-all -Wunreachable-code-aggressive -Wduplicate-enum \
			  -Wcompare-distinct-pointer-types -Warray-bounds-pointer-arithmetic -Wvla

ifeq ($(CC),xt-clang)
CFLAGS			= $(CSTD) $(WARNOPT) $(XWARN)
else
CFLAGS			= -clang $(CSTD) $(WARNOPT)
endif

ifeq ($(MODE),CUSTOM)
# Inline (CUSTOM) build options
IFLAGS			?= -I$(INCDIR) -I$(XTOSSRCDIR)
SM_LIBPATH		?= -L$(BINDIR)
else
# Installed (STANDARD) build options
IFLAGS			?=
SM_LIBPATH		?=
endif

# Common for all modes
CFLAGS			+= -O2 -g -mlongcalls -mno-l32r-flix $(IFLAGS)
ASFLAGS			+= -O2 -g -mlongcalls -mno-l32r-flix $(IFLAGS)
RUNFLAGS		= --mem_model --nosummary

SMLSP_SIM		?= secmon-sim
SMLSP_BRD		?= secmon-board
NSMLSP_SIM		= app-sim
NSMLSP_BRD		= app-board

NSMAFLAGS		= -D_BUILD_NSM
NSMCFLAGS		= -D_BUILD_NSM -mcoproc
NSMCFLAGS_SIM		= $(NSMCFLAGS) -DSIMULATOR
NSM_LFLAGS		?= $(SM_LIBPATH) -lc -lgdbio -lsec
NSM_SIM_LFLAGS		= $(NSM_LFLAGS) -mlsp=$(NSMLSP_SIM)
NSM_BRD_LFLAGS		= $(NSM_LFLAGS) -mlsp=$(NSMLSP_BRD)

SMAFLAGS		= -D_BUILD_SM -D__XTENSA_CALL0_ABI__ -UXCHAL_HAVE_WINDOWED -DXTOS_SUBPRI=0
SMCFLAGS		= $(SMAFLAGS) -mabi=call0 -mno-coproc
SMCFLAGS_SIM		= $(SMCFLAGS) -DSIMULATOR
SMLFLAGS_SIM		+= $(SMLFLAGS_EXTRA) $(SM_LIBPATH) -mlsp=$(SMLSP_SIM)
SMLFLAGS_BRD		+= $(SMLFLAGS_EXTRA) $(SM_LIBPATH) -mlsp=$(SMLSP_BRD)

SECMON_SRC		?= secure_user_init.c
NONSEC_SRC		?= nonsecure_app.c

SECSIM_TGT		= secmon-sim.exe
SECBRD_TGT		= secmon-board.exe
NSECSIM_TGT		= nonsecure_app-sim.exe
NSECBRD_TGT		= nonsecure_app-board.exe
NSECSIM_BIN		= $(patsubst %.exe,%.bin,$(NSECSIM_TGT))
NSECBRD_BIN		= $(patsubst %.exe,%.bin,$(NSECBRD_TGT))
LOG_TGT			= test-sim.log

TGTS			= $(SECSIM_TGT) $(SECBRD_TGT) $(NSECSIM_TGT) $(NSECBRD_TGT)

TARGET_LIST		= $(addprefix $(TGTDIR),$(TGTS))
LOG_LIST		= $(TGTDIR)$(LOG_TGT)

# Place nonsecure application binary image at stack sentry for boot-loader to unpack
NSM_LOC_CMD		= xt-nm $(TGTDIR)$(NSECSIM_TGT) | $(GREP) _stack_sentry
NSM_LOC			= 0x$(word 1, $(shell $(NSM_LOC_CMD)))

ifeq ($(MODE),CUSTOM)
LIBSECMON_A		= $(BINDIR)$(S)libsec.a

SECMON_COMM_A_		= \

SECMON_PLFM_A_		= \
	libsm.a

SECMON_COMM_O_		= \
	_vectors-sm.o

SECMON_PLFM_O_		= \

SECMON_COMM_A		= $(addprefix $(BINDIR)$(S),$(SECMON_COMM_A_))
SECMON_COMM_O		= $(addprefix $(BINDIR)$(S),$(SECMON_COMM_O_))

SECMON_PLFM_A		= $(addprefix $(BINDIR)$(S),$(SECMON_PLFM_A_))
SECMON_PLFM_O		= $(addprefix $(BINDIR)$(S),$(SECMON_PLFM_O_))

ifeq ($(SMLSP_SIM),secmon-sim-rom)
SECMON_SIM_A		= $(patsubst %.a,%-sim-unpack.a,$(SECMON_PLFM_A)) $(SECMON_COMM_A)
else
SECMON_SIM_A		= $(patsubst %.a,%-sim.a,$(SECMON_PLFM_A)) $(SECMON_COMM_A)
endif

ifeq ($(SMLSP_BRD),secmon-board-rom)
SECMON_BRD_A		= $(patsubst %.a,%-board-unpack.a,$(SECMON_PLFM_A)) $(SECMON_COMM_A)
else
SECMON_BRD_A		= $(patsubst %.a,%-board.a,$(SECMON_PLFM_A)) $(SECMON_COMM_A)
endif

SECMON_SIM_O		= $(patsubst %.o,%-sim.o,$(SECMON_PLFM_O)) $(SECMON_COMM_O)
SECMON_BRD_O		= $(SECMON_PLFM_O) $(SECMON_COMM_O)
endif

# Export hosteenv.mk for sub-make processes
export RM
export RM_R
export CP
export GREP
export MKPATH
export S
export PERL


# Default target
def : 
	$(MAKE) all
	$(MAKE) test

ifeq ($(MODE),CUSTOM)
$(LIBSECMON_A) $(SECMON_SIM_A) $(SECMON_SIM_O) $(SECMON_BRD_A) $(SECMON_BRD_O) : make_custom_secmon
endif

make_custom_secmon :
	$(MAKE) -C $(SRCDIR) --no-print-directory all

ifndef ALL_OVERRIDE
all : $(TARGET_LIST)
endif

ifndef SEC_OVERRIDE
$(TGTDIR)$(SECSIM_TGT) : $(SECMON_SRC) $(SECMON_SIM_O) $(SECMON_SIM_A)
	$(CC) $(CFLAGS) $(SMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_SIM) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(SECBRD_TGT) : $(SECMON_SRC) $(SECMON_BRD_O) $(SECMON_BRD_A)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_BRD) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)
endif

ifndef NSM_OVERRIDE
$(TGTDIR)$(NSECSIM_TGT) : $(NONSEC_SRC) $(LIBSECMON_A)
	$(CC) $(CFLAGS) $(NSMCFLAGS_SIM) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_SIM_LFLAGS) -o $@
	$(ELF2ROM) $@ $(patsubst %.exe,%.bin,$@)
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(NSECBRD_TGT) : $(NONSEC_SRC) $(LIBSECMON_A)
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_BRD_LFLAGS) -o $@
	$(ELF2ROM) $@ $(patsubst %.exe,%.bin,$@)
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)
endif

ifndef RUN_OVERRIDE
test : $(LOG_LIST)

$(TGTDIR)$(LOG_TGT) : $(TGTDIR)$(SECSIM_TGT) $(TGTDIR)$(NSECSIM_TGT)
	-cd $(dir $@) && xt-run $(RUNFLAGS) --loadbin=$(TGTDIR)$(NSECSIM_BIN)@$(NSM_LOC) $< unpack=$(NSM_LOC) > $@ 2>&1
	-$(GREPI) "Passed" $@ && echo "TEST_OVERALL_RESULT: PASSED" >> $@ || echo "TEST_OVERALL_RESULT: FAILED" >> $@
endif

ifndef CLEAN_OVERRIDE
clean :
	-$(RM) *.exe *.bin *.dis *.log
endif

.PHONY : all test def clean

