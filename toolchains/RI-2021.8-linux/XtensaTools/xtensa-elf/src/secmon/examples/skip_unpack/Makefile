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

NSM_OVERRIDE		= 1
RUN_OVERRIDE		= 1

include $(SECMON_ROOT)/examples/Makefile.inc

ifeq ($(HOSTTYPE),win)
SKIP_NSM_LOC_CMD	= xt-nm $(TGTDIR)$(NSECSIM_TGT) | $(GREP) /r "\<_start\>"
else
SKIP_NSM_LOC_CMD	= xt-nm $(TGTDIR)$(NSECSIM_TGT) | $(GREP) -w _start
endif
SKIP_NSM_LOC		= 0x$(word 1, $(shell $(SKIP_NSM_LOC_CMD)))

# skip_unpack does not generate .bin files as full ELF files are loaded by the user

$(TGTDIR)$(NSECSIM_TGT) : $(NONSEC_SRC)
	$(CC) $(CFLAGS) $(NSMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(NSM_SIM_LFLAGS) -o $@
	xt-objdump -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(NSECBRD_TGT) : $(NONSEC_SRC)
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(NSM_BRD_LFLAGS) -o $@
	xt-objdump -D $@ > $(patsubst %.exe,%.dis,$@)

# skip_unpack specifies non-secure entry point as command-line parameter and does not unpack
# anything

test : $(TGTDIR)$(LOG_TGT)

$(TGTDIR)$(LOG_TGT) : $(TGTDIR)$(NSECSIM_TGT) $(TGTDIR)$(SECSIM_TGT)
	-cd $(dir $@) && xt-run $(RUNFLAGS) --load=$< $(filter-out $<,$^) $(SKIP_NSM_LOC) > $@ 2>&1
	@$(GREPI) "Passed" $@ && echo "TEST_OVERALL_RESULT: PASSED" >> $@ || echo "TEST_OVERALL_RESULT: FAILED" >> $@

