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
CLEAN_OVERRIDE	= 1

include $(SECMON_ROOT)/examples/Makefile.inc

MPU_TABLE_ORIG	= $(BINDIR)$(S)mon$(S)mpu_inittab.c
MPU_TABLE_NEW	= $(TGTDIR)mpu_table.c

$(MPU_TABLE_ORIG) : make_custom_secmon

# Update MPU table to remove PS.RING==1 bits (i.e. rwx)
$(MPU_TABLE_NEW) : $(MPU_TABLE_ORIG)
ifeq ($(HOSTTYPE),win)
	$(shell $(PERL) -pe "s/XTHAL_AR_([RWX]*)[rwx]*,/XTHAL_AR_$$1,/g" $^ > $@)
else
	$(shell $(PERL) -pe 's/XTHAL_AR_([RWX]*)[rwx]*,/XTHAL_AR_$$1,/g' $^ > $@)
endif

$(TGTDIR)$(SECSIM_TGT) : $(MPU_TABLE_NEW) $(SECMON_SRC) $(SECMON_SIM_O) $(SECMON_SIM_A)
	$(CC) $(CFLAGS) $(SMCFLAGS_SIM) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_SIM) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

$(TGTDIR)$(SECBRD_TGT) : $(MPU_TABLE_NEW) $(SECMON_SRC) $(SECMON_BRD_O) $(SECMON_BRD_A)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$*) $^ $(LFLAGS_EXTRA) $(SMLFLAGS_BRD) -o $@
	$(OBJDUMP) -D $@ > $(patsubst %.exe,%.dis,$@)

clean :
	-$(RM) -rf *.exe *.bin *.dis *.log $(MPU_TABLE_NEW)

