all: test

SYSPKG = ./syspackage
SYSCORES = $(SYSPKG)/cores
XTENSA_TOOLS_ROOT = $(shell xt-run --show-config=xttools)
MBUILD = $(XTENSA_TOOLS_ROOT)/libexec/xt-mbuild
LSP = sim

.PHONY: all test submake clean
#  Fancy submake because xt-mbuild generates a makefile fragment that we include:
ifndef SYSDEFS
all $(filter-out clean,$(MAKECMDGOALS)): submake
submake: .mbuild
	$(MAKE) SYSDEFS=$(SYSPKG)/sys-defs.mk $(MAKECMDGOALS)
.mbuild: system.xld Makefile
	$(MBUILD) -syspkg $(SYSPKG) -sysbuild ./sysbuild -lsps $(LSP) \
		-socfile system.xld -logfile mbuild.log
	touch $@
else
include $(SYSDEFS)

CC = xt-clang
CFLAGS = -g -I $(SYSPKG)/xtensa-elf/include
CFLAGS_PERCORE =  -I $(SYSCORES)/$*/xtensa-elf/include
ALLPROGS = $(CORES:%=%.out)
LIBS =

test: $(ALLPROGS)
	xtsc-run --xtensa-system=$(SYSPKG)/config --xtensa-core=core0 --include=$(SYSPKG)/xtsc-run/SYSTEM.inc

#  All use the same source file, for simple example:
$(ALLPROGS): common.c

%.out:
	$(CC) -o $@ $(filter %.c %.o %.S,$^) $(CFLAGS) $(CFLAGS_PERCORE) \
			--xtensa-core=$(CORE_$*_CONFIG)  \
			$(LIBS)  -mlsp=$(SYSCORES)/$*/xtensa-elf/lib/$(LSP)
endif

clean:
	-rm -rf *.o *.out *.partial $(SYSPKG) ./sysbuild .mbuild mbuild.log
