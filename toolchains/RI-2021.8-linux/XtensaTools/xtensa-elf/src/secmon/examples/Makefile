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

# Makefile -- Rules and commands for building Secure Monitor tests/examples.


# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

ALL_OVERRIDE	= 1
SEC_OVERRIDE	= 1
NSM_OVERRIDE	= 1
RUN_OVERRIDE	= 1
CLEAN_OVERRIDE	= 1

include Makefile.inc

EXAMPLE_LIST	=			\
	alloca_test			\
	coproc_disable			\
	exc_intr_nest			\
	idma_secure_mem			\
	intr_nest			\
	mon_secure_mem			\
	mpu_init			\
	mpu_init_nsmwren		\
	mpu_invalid			\
	nonsecure_exc_handler		\
	nonsecure_int_handlers		\
	nonsecure_int_dereg		\
	nonsecure_medpri_int_handlers	\
	secure_exc_noredirect		\
	secure_int_handlers		\
	secure_medpri_int_handlers	\
	skip_unpack			\
	syscall_test			\
	xmon
ifeq ($(SECMON_IN_ROM),1)
EXAMPLE_LIST	+= self_unpack_irom
endif
ifeq ($(HAVE_WWDT),1)
EXAMPLE_LIST	+= wwdt
endif

EXAMPLE_DIRS	= $(addprefix $(EXEDIR)$(S),$(EXAMPLE_LIST))
EXAMPLE_LOGS	= $(addsuffix $(S)$(LOG_TGT),$(EXAMPLE_DIRS))
EXAMPLE_MRKS	= $(addsuffix $(S).runlog,$(EXAMPLE_DIRS))

# Detect relocatable vector support
XSTR		= $(shell $(GREP) RelocatableVectors $(CONFIGDIR)$(S)config$(S)default-params)
RELOC_VEC	= $(word 3, $(XSTR))

ifeq ($(RELOC_VEC),1)
ifndef TEN4_INTERNAL
BYPASS_SECMON	= 1
endif
endif

# Targets

ifeq ($(BYPASS_SECMON),1)

all : 
	@echo WARNING: Relocatable vectors not supported in Secmon. Skipping build.

clean : all

test : all

else

all : mkdir makesecmon
ifeq ($(HOSTTYPE),win)
	for %%a in ($(EXAMPLE_MRKS)) do if exist "%%~a" $(RM) "%%~a"
else
	-$(RM_R) $(EXAMPLE_MRKS)
endif
	$(MAKE) examples

test : all

mkdir : $(addsuffix $(S).exists,$(EXAMPLE_DIRS))

%.exists :
	$(MKPATH) $(dir $@)
	@echo "" > $@

ifeq ($(MODE),CUSTOM)
makesecmon : make_custom_secmon
else
makesecmon :
endif

examples : $(EXAMPLE_MRKS)
	@echo
	@$(GREPI) "TEST_OVERALL_RESULT:\ FAILED" $(EXAMPLE_LOGS) && echo "Overall Results: Test(s) FAILED" || echo "Overall Results: Tests PASSED"
	@echo

# mpu and xmon tests require custom builds; serialize them
$(EXEDIR)$(S)xmon$(S).runlog : mkdir $(EXEDIR)$(S)mpu_invalid$(S).runlog
	echo Building xmon
	$(MAKE) -C xmon --no-print-directory TGTDIR=$(subst /,$(S),$(dir $(abspath $@))) TESTDIR=$(subst /,$(S),$(abspath xmon))
	@echo "" > $@

$(EXEDIR)$(S)mpu_invalid$(S).runlog : mkdir $(EXEDIR)$(S)mpu_init$(S).runlog
	echo Building mpu_invalid
	$(MAKE) -C mpu_invalid --no-print-directory TGTDIR=$(subst /,$(S),$(dir $(abspath $@))) TESTDIR=$(subst /,$(S),$(abspath mpu_invalid))
	@echo "" > $@

$(EXEDIR)%.runlog : mkdir
	echo Building $(subst $(S),,$*)
	$(MAKE) -C $(subst $(S),,$*) --no-print-directory TGTDIR=$(subst /,$(S),$(dir $(abspath $@))) TESTDIR=$(subst /,$(S),$(abspath $(subst $(S),,$*)))
	@echo "" > $@

clean:
	-$(RM_R) $(EXEDIR)

.PHONY : all clean makesecmon mkdir examples

endif

