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


SECMON_ROOT	= $(abspath $(CURDIR)/../..)

ALL_OVERRIDE	= 1
NSM_OVERRIDE	= 1
RUN_OVERRIDE	= 1

OBJCOPY		= xt-objcopy

# Use romable LSPs
SMLSP_SIM	= secmon-sim-rom
SMLSP_BRD	= secmon-board-rom

# Extract window vector location for IROM pre-load
WVEC_LOC_CMD	= xt-nm $(TGTDIR)$(NSECSIM_TGT) | $(GREP) WindowVectors_text_start
WVEC_LOC	= 0x$(word 1, $(shell $(WVEC_LOC_CMD)))

include $(SECMON_ROOT)/examples/Makefile.inc


test : $(TGTDIR)$(LOG_TGT)

ifeq ($(SECMON_IN_ROM),1)

all : $(TARGET_LIST)

$(TGTDIR)$(NSECSIM_TGT) : $(NONSEC_SRC) $(LIBSECMON_A)
	$(CC) $(CFLAGS) $(NSMCFLAGS_SIM) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_SIM_LFLAGS) -o $@
	$(OBJCOPY) -R .WindowVectors.text $@ $(patsubst %.exe,%.nowinvec.exe,$@)
	$(OBJCOPY) -O binary -j .WindowVectors.text $@ $(patsubst %.exe,%.winvec.bin,$@)
	$(ELF2ROM) $(patsubst %.exe,%.nowinvec.exe,$@) $(patsubst %.exe,%.bin,$@)
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(NSECBRD_TGT) : $(NONSEC_SRC) $(LIBSECMON_A)
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) $(filter-out $(LIBSECMON_A),$^) $(LFLAGS_EXTRA) $(NSM_BRD_LFLAGS) -o $@
	$(OBJCOPY) -R .WindowVectors.text $@ $(patsubst %.exe,%.nowinvec.exe,$@)
	$(OBJCOPY) -O binary -j .WindowVectors.text $@ $(patsubst %.exe,%.winvec.bin,$@)
	$(ELF2ROM) $(patsubst %.exe,%.nowinvec.exe,$@) $(patsubst %.exe,%.bin,$@)
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(LOG_TGT) : $(TGTDIR)$(NSECSIM_TGT) $(TGTDIR)$(SECSIM_TGT)
	-cd $(dir $@) && xt-run $(RUNFLAGS) --loadbin=$(patsubst %.exe,%.bin,$<)@$(NSM_LOC) --loadbin=$(patsubst %.exe,%.winvec.bin,$<)@$(WVEC_LOC) $(filter-out $<,$^) unpack=$(NSM_LOC) > $@ 2>&1
	@$(GREPI) "Passed" $@ && echo "TEST_OVERALL_RESULT: PASSED" || echo "TEST_OVERALL_RESULT: FAILED"

else

all : 

$(TGTDIR)$(LOG_TGT) : 
	echo "TEST_OVERALL_RESULT (skipped): PASSED" >> $@

$(TGTDIR)$(NSECSIM_TGT) : 

$(TGTDIR)$(NSECBRD_TGT) : 

endif

