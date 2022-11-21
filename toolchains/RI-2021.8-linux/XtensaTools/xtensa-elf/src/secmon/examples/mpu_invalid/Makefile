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

# Force custom build so we can pull generated MPU table
MODE		= CUSTOM

SEC_OVERRIDE	= 1
RUN_OVERRIDE	= 1
CLEAN_OVERRIDE	= 1

include $(SECMON_ROOT)/examples/Makefile.inc

MPU_TABLE_ORIG	= $(BINDIR)$(S)mon$(S)mpu_inittab.c
MPU_TABLE_NEW1	= $(TGTDIR)mpu_table1.c
MPU_TABLE_NEW2	= $(TGTDIR)mpu_table2.c

$(MPU_TABLE_ORIG) : make_custom_secmon

# Update MPU table to make secure IRAM cacheable (to trigger error)
$(MPU_TABLE_NEW1) : $(MPU_TABLE_ORIG)
ifeq ($(HOSTTYPE),win)
	$(shell $(PERL) -pe "s/x, XTHAL_MEM_NON_CACHEABLE/x, XTHAL_MEM_WRITEBACK/g" $^ > $@)
else
	$(shell $(PERL) -pe 's/x, XTHAL_MEM_NON_CACHEABLE/x, XTHAL_MEM_WRITEBACK/g' $^ > $@)
endif

# Update MPU table to make last secure region unlocked (to trigger error)
$(MPU_TABLE_NEW2) : $(MPU_TABLE_ORIG)
ifeq ($(HOSTTYPE),win)
	$(shell $(PERL) -pe "s/_secure(.*) = (.*);/_secure$$1 = $$2 - 2;/g" $^ > $@)
else
	$(shell $(PERL) -pe 's/_secure(.*) = (.*);/_secure$$1 = $$2 - 2;/g' $^ > $@)
endif

$(TGTDIR)$(SECSIM_TGT) : $(TGTDIR)$(SECSIM_TGT)_1 $(TGTDIR)$(SECSIM_TGT)_2

$(TGTDIR)$(SECBRD_TGT) : $(TGTDIR)$(SECBRD_TGT)_1 $(TGTDIR)$(SECBRD_TGT)_2

$(TGTDIR)$(SECSIM_TGT)_1 : $(MPU_TABLE_NEW1) $(SECMON_SRC) $(SECMON_SIM_O) $(SECMON_SIM_A)
	$(CC) $(CFLAGS) $(SMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_SIM) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe_1,%.dis_1,$@)

$(TGTDIR)$(SECBRD_TGT)_1 : $(MPU_TABLE_NEW1) $(SECMON_SRC) $(SECMON_BRD_O) $(SECMON_BRD_A)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_BRD) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe_1,%.dis_1,$@)

$(TGTDIR)$(SECSIM_TGT)_2 : $(MPU_TABLE_NEW2) $(SECMON_SRC) $(SECMON_SIM_O) $(SECMON_SIM_A)
	$(CC) $(CFLAGS) $(SMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_SIM) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe_2,%.dis_2,$@)

$(TGTDIR)$(SECBRD_TGT)_2 : $(MPU_TABLE_NEW2) $(SECMON_SRC) $(SECMON_BRD_O) $(SECMON_BRD_A)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_BRD) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe_2,%.dis_2,$@)

test : $(LOG_LIST)

$(LOG_LIST) : $(TGTDIR)$(LOG_TGT)_1 $(TGTDIR)$(LOG_TGT)_2
	@$(CAT) $^ > $@
	@$(GREPI) "FAILED" $@ && echo "TEST_OVERALL_RESULT: FAILED" >> $@ || echo "TEST_OVERALL_RESULT: PASSED" >> $@

$(TGTDIR)$(LOG_TGT)_% : $(TGTDIR)$(SECSIM_TGT)_% $(TGTDIR)$(NSECSIM_TGT)
	-cd $(dir $@) && xt-run $(RUNFLAGS) --loadbin=$(TGTDIR)$(NSECSIM_BIN)@$(NSM_LOC) $< unpack=$(NSM_LOC) > $@ 2>&1
	@$(GREPI) "Unhandled\ user\ exception\|Unhandled\ double\ exception" $@ && echo "TEST_PERLOG_RESULT: PASSED" >> $@ || echo "TEST_PERLOG_RESULT: FAILED" >> $@

clean :
	$(RM) *.exe *.exe_? *.bin *.dis *.dis_? *.log *.log_? $(MPU_TABLE_NEW1) $(MPU_TABLE_NEW2)

