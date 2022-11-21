#  mbuild-defs.mk  --  Makefile definitions for building systems (eg. multicore)
#
# Copyright (c) 2005-2018 by Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.

#  The Xtensa Tools are assumed to be on the user's PATH
#  (i.e. $(XTENSA_TOOLS_ROOT)/bin which contains xt-clang, xt-ld, etc).
#  However, $(XTENSA_STANDARD_TOOLS)/bin are not assumed on the PATH.

#  Include $(XTENSA_TOOLS_ROOT)/misc/defs.mk if not done already:
ifndef SUBMAKEFILES
include $(XTENSA_TOOLS_ROOT)/misc/defs.mk
endif

#  Saved copy of $(SRC_SOCFILE) in XLD format:
SAVED_SOCFILE = $(XTENSA_SYSTEM_ROOT)/config/sysorig.xld
#  Expanded/processed version of $(SRC_SOCFILE), which most scripts/tools use:
SOCFILE       = $(XTENSA_SYSTEM_ROOT)/config/system.yml

export XTENSA_SYSTEM := $(XTENSA_SYSTEM_ROOT)/config

SYS_TYPES = $(XTENSA_TOOLS_ROOT)/src/system/systypes.xsysi

#  Set this to 1 to enable SYSTPP profiling:
TPP_PROFILING = 0

ifneq ($(TPP_PROFILING),1)
#SYSTPP   = $(XTENSA_TOOLS_ROOT)/libexec/tpp
SYSTPP   = $(PERL) $(XTENSA_TOOLS_ROOT)/libexec/tpp.pl
else
#  For profiling (need to find tmon.out files after run and call dprofpp on each of interest):
ifeq ($(ARCH_OSLIKE),unix)
SYSTPP   = PERL_DPROF_OUT_FILE_NAME=$(notdir $@).tmon.out $(PERL) -d:DProf $(XTENSA_TOOLS_ROOT)/libexec/tpp.pl
else
#  No easy way to set PERL_DPROF_OUT_FILE_NAME per file on Windows, without
#  changing all rules that invoke $(SYSTPP).  So last tpp to write tmon.out wins.
#  Doing:
#      set PERL_PROF_OUT_FILE_NAME=$(notdir $@).tmon.out & $(PERL) ...
#  is promising, but changes cmd parsing of quotes (using native cmd instead of make
#  presumably), so rules also need to change for that.
#  Note you have to convert tmon.out to Unix line-endings for dprofpp to work (!).
SYSTPP   = $(PERL) -d:DProf $(XTENSA_TOOLS_ROOT)/libexec/tpp.pl
#  This doesn't work, don't know why:
#SYSTPP   = -move tmon.out bef_$(notdir $@).tmon.out $N -mv tmon.out before_$(notdir $@).tmon.out $N $(PERL) -d:DProf $(XTENSA_TOOLS_ROOT)/libexec/tpp.pl
endif
endif

#  temporary, for transition -- to be deleted:
XCLTPP   = $(SYSTPP)

#XT_XCC   = xt-clang \
#		-isystem $(XTENSA_SYSTEM_ROOT)/xtensa-elf/include \
#		-L$(XTENSA_SYSTEM_ROOT)/xtensa-elf/config/$(XTENSA_CORE)/lib

