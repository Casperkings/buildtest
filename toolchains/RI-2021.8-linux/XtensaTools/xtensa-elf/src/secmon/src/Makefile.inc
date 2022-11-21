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

# Makefile.inc -- rules and commands to build the Secure Monitor


# Include platform-specific commands
CONFIGDIR = $(shell xt-clang --show-config=config)

CSTD			= -std=c11

ROOTDIR			= $(subst /,$(S),$(abspath ..))
ifndef XTOSSRCDIR  
XTOSSRCDIR		= $(ROOTDIR)$(S)..$(S)xtos
endif
BINDIR			?= $(ROOTDIR)$(S)bin
BINLIBDIR		= $(BINDIR)$(S)lib
BINMONDIR		= $(BINDIR)$(S)mon
BINMONXTOSDIR		= $(BINDIR)$(S)mon$(S)xtos-sm
BINMONXTOS2DIR		= $(BINDIR)$(S)mon$(S)xtos-sm$(S)xea2
BINMISCDIR		= $(BINDIR)$(S)misc
INCDIR			?= $(ROOTDIR)$(S)include
XINCDIR			= $(INCDIR)$(S)xtensa
SRCINCDIR		= $(ROOTDIR)$(S)src$(S)include$(S)xtensa
ifdef TEN4_INTERNAL
INTSRCDIR		= $(ROOTDIR)$(S)internal$(S)src
endif
OUTDIRLIST		= $(BINDIR) $(BINLIBDIR) $(BINMONDIR) $(BINMONXTOSDIR) $(BINMONXTOS2DIR) $(BINMISCDIR) $(INCDIR) $(XINCDIR)

CC			= xt-clang
ifeq ($(HOSTTYPE),win)
CC_EXISTS		= $(shell $(CC) --help 2> NUL || echo "fail")
else
CC_EXISTS		= $(shell $(CC) --help 2> /dev/null || echo "fail")
endif
ifeq ($(CC_EXISTS),fail)
CC			= xt-xcc
endif


# Warning options
WARNOPT			?= -Werror -Wall -Wextra -Wunused -Wundef -Wstrict-prototypes -Wpointer-arith \
			   -Wcast-qual -Wformat-security -Wmissing-prototypes -Wshadow -Wsign-compare -pedantic

# Extra warning flags for use with xt-clang
XWARN			?= -Wshadow-all -Wunreachable-code-aggressive -Wduplicate-enum \
			   -Wcompare-distinct-pointer-types -Warray-bounds-pointer-arithmetic -Wvla

ifeq ($(CC),xt-clang)
CFLAGS			= $(CSTD) $(WARNOPT) $(XWARN)
else
CFLAGS			= -clang $(CSTD) $(WARNOPT)
endif

# Common for all modes
IFLAGS			= -I$(INCDIR) -I$(XTOSSRCDIR)
CFLAGS			+= -O2 -g -mlongcalls -mno-l32r-flix $(IFLAGS)
ASFLAGS			+= -O2 -g -mlongcalls -mno-l32r-flix $(IFLAGS)

# Detect relocatable vector support
XSTR			= $(shell $(GREP) RelocatableVectors $(CONFIGDIR)$(S)config$(S)default-params)
RELOC_VEC		= $(word 3, $(XSTR))

XSTR2			= $(shell $(GREP) config-prefix $(CONFIGDIR)$(S)config$(S)default-params)
CORENAME		= $(notdir $(patsubst %$(S)Tensilica,%,$(realpath $(word 3, $(XSTR2)))))

ifeq ($(RELOC_VEC),1)
ifndef TEN4_INTERNAL
BYPASS_SECMON		= 1
endif
endif

SMLSP_PATH_SIM		= secmon-sim
SMLSP_PATH_BRD		= secmon-board

NSMAFLAGS		= -D_BUILD_NSM
NSMCFLAGS		= -D_BUILD_NSM

SMAFLAGS		= -D_BUILD_SM -D__XTENSA_CALL0_ABI__ -UXCHAL_HAVE_WINDOWED -DXTOS_SUBPRI=0
SMCFLAGS		= $(SMAFLAGS) -mabi=call0 -mno-coproc
SMLFLAGS_SIM		+= $(SMLFLAGS_EXTRA) -L$(BINDIR) -mlsp=$(SMLSP_PATH_SIM)
SMLFLAGS_BRD		+= $(SMLFLAGS_EXTRA) -L$(BINDIR) -mlsp=$(SMLSP_PATH_BRD)

# Anything compiling code referring to __xt_mpu_init_* variables must disable
# flix instructions, as they reside in IRAM, specifically .ResetHandler.text.
SM_MPU_CFLAGS		= -mno-flix

# When building the reset vector, leave the literals in
# the text section, so that the reset vector can be placed
# at address 0 (there's no lower address for the literals).
# (Can't do that on Xtensa TX without load/store to IRAM/IROM.)
# Same with dispatch code, allowing use of MOVI rather than
# explicit L32R (and thus allow alternate literal options).

ifneq ($(XPAL_HAVE_IMEM_LOADSTORE),0)
RVFLAGS			= -mtext-section-literals
endif

NSMLIB			= $(BINDIR)$(S)libsec.a

NSMLIBOBJS		=		\
	debug-vector-nsm.o		\
	user-vector-nsm.o		\
	xtsm_handlers.o

PERLEVEL_NSM_INTHANDLER_OBJS	= $(foreach N,2 3 4 5 6,int-handler-level$(N)-nsm.o)

NSMLIBOBJS		+= $(PERLEVEL_NSM_INTHANDLER_OBJS)
XNSMLIBOBJS		= $(addprefix $(BINLIBDIR)$(S),$(NSMLIBOBJS))

MONEXE_NAMES		= $(BINDIR)$(S)secmon.exe

MONEXE_SIM_TGTS		= $(patsubst %.exe,%-sim.exe,$(MONEXE_NAMES))
MONEXE_BRD_TGTS		= $(patsubst %.exe,%-board.exe,$(MONEXE_NAMES))
MONEXE_TARGETS		= $(MONEXE_SIM_TGTS) $(MONEXE_BRD_TGTS)

OBJS			=		\
	crt1-sm.o			\
	trap-sm.o

XOBJS			= $(addprefix $(BINMONDIR)$(S),$(OBJS))

XOBJS_BRD		= $(XOBJS)
XOBJS_SIM		= $(XOBJS:.o=-sim.o)

MONOBJS_INT		=		\
	cpdisable-handler-sm.o		\
	debug-vector-sm.o		\
	exc-c-wrapper-handler-sm.o	\
	exc-return-sm.o			\
	exc-syscall-handler-sm.o	\
	exit-sm.o			\
	int-lowpri-dispatcher-sm.o	\
	memerror-vector-sm.o		\
	mpu_init_final.o		\
	mpu_inittab.o			\
	syscall.o			\
	tiny-refs-sm.o			\
	unpack.o			\
	user-vector-sm.o

XMONOBJS		= $(addprefix $(BINMONDIR)$(S),$(MONOBJS_INT))

MONXTOSOBJS		=		\
	shared-reset-vector.o		\
	memctl_default.o		\
	xtos-set-inthandler.o		\
	xtos-interrupt-enable.o		\
	xtos-interrupt-enabled.o	\
	xtos-interrupt-disable.o	\
	xtos-interrupt-clear.o		\
	xtos-interrupt-trigger.o	\
	xtos-set-exchandler.o		\
	memep-enable.o			\
	xea2$(S)exc-table.o		\
	xea2$(S)interrupt-table.o	\
	xea2$(S)nmi-vector.o

XMONXTOSOBJS		= $(addprefix $(BINMONXTOSDIR)$(S),$(MONXTOSOBJS))

MONXTOS_SIMBOARD_OBJS	=		\
	double-vector.o			\
	xea2$(S)kernel-vector.o		\
	memep-initrams.o

XMONXTOS_SIMBOARD_OBJS	= $(addprefix $(BINMONXTOSDIR)$(S),$(MONXTOS_SIMBOARD_OBJS))

XMONXTOS_BOARD_OBJS	= $(XMONXTOS_SIMBOARD_OBJS)
XMONXTOS_SIM_OBJS	= $(XMONXTOS_SIMBOARD_OBJS:.o=-sim.o)

RVEC_OBJ		= $(BINMONDIR)$(S)reset-vector.o
RVEC_UNPACK_OBJ		= $(BINMONDIR)$(S)reset-vector-unpack.o
RVEC_SRC		= reset-vector-xea2-sm.S

# Installed secure object
MONOBJ			= $(BINDIR)$(S)_vectors-sm.o

# Installed secure libraries
MONLIB			= $(BINDIR)$(S)libsm.a
MONLIB_SIM		= $(patsubst %.a,%-sim.a,$(MONLIB))
MONLIB_BRD		= $(patsubst %.a,%-board.a,$(MONLIB))
MONLIB_SIM_UNPACK	= $(patsubst %.a,%-unpack.a,$(MONLIB_SIM))
MONLIB_BRD_UNPACK	= $(patsubst %.a,%-unpack.a,$(MONLIB_BRD))

INCLUDES		=		\
	secmon-common.h			\
	secmon-defs.h			\
	secmon-secure.h			\
	secmon.h
XPORT_INCS		= $(addprefix $(SRCINCDIR)$(S),$(INCLUDES))

PERLEVEL_INTHANDLER_OBJS= $(foreach N,2 3 4 5 6,$(BINMONDIR)$(S)int-handler-level$(N).o)
PERLEVEL_INTVECTOR_OBJS	= $(foreach N,2 3 4 5 6,$(BINMONDIR)$(S)int-vector-level$(N).o)
PERLEVEL_INTINITLEV_OBJS= $(foreach N,1 2 3 4 5 6,$(BINMONXTOSDIR)$(S)xea2$(S)int-init-level$(N).o)

XMONOBJS		+= $(PERLEVEL_INTHANDLER_OBJS)
XMONXTOSOBJS		+= $(PERLEVEL_INTVECTOR_OBJS) $(PERLEVEL_INTINITLEV_OBJS)

GENMPU_OUT		= $(BINMONDIR)$(S)mpu_inittab.c


# Targets

ifeq ($(BYPASS_SECMON),1)

all : 
	@echo WARNING: Relocatable vectors not supported in Secmon. Skipping build.

mon : all

lib : all

inc : all

clean : all

else

# Conditional is present to differentiate between native Windows and Cygwin.
# This allows hostenv.mk variables to remain set correctly for sub-make 
# processes, and remove a warning from xt-make on native Windows.
ifeq ($(HOSTTYPE),win)

all : 
	@echo Building $(CORENAME) secmon
	$(MAKE) inc
	$(MAKE) mon lib

else

all : 
	@echo Building $(CORENAME) secmon
	$(MAKE) -f Makefile.inc inc
	$(MAKE) -f Makefile.inc mon lib

endif   # (GREP != findstr)

mkdir : $(addsuffix $(S).exists,$(OUTDIRLIST))

%$(S).exists :
	-$(MKPATH) $(dir $@)
	@echo "" > $@

inc : mkdir $(addsuffix $(S).cpinc,$(XINCDIR))

%$(S).cpinc : %$(S).exists $(XPORT_INCS)
	$(CP) $(SRCINCDIR)$(S)*.h $(dir $@)
	@echo "" > $@

# Build secmon exes as examples only; src/ builds .a/.o targets only.
# mon : mkdir $(GENMPU_OUT) $(MONEXE_TARGETS)

mon : mkdir $(GENMPU_OUT) $(MONLIB_SIM) $(MONLIB_BRD) $(MONLIB_SIM_UNPACK) $(MONLIB_BRD_UNPACK) $(MONOBJ)

lib : mkdir $(NSMLIB)

clean:
	-$(RM_R) $(BINDIR)
	-$(RM_R) $(INCDIR)

endif   # (BYPASS_SECMON != 1)


$(GENMPU_OUT) :
	cd $(dir $@) && xt-genmpu --merge --secmon --no-fill --iram-wp -o $@

.SECONDARY : $(MONOBJ) $(XOBJS_SIM) $(XOBJS_BRD)

$(NSMLIB) : $(XNSMLIBOBJS)
	-$(RM) $@
	xt-ar rs $@ $^

$(MONLIB_SIM) : $(XMONOBJS) $(XOBJS_SIM) $(XMONXTOSOBJS) $(XMONXTOS_SIM_OBJS) $(RVEC_OBJ)
	-$(RM) $@
	xt-ar rs $@ $^
	xt-objcopy -W xt_mpu_init_cacheadrdis -W xt_mpu_init_table_size -W xt_mpu_init_table $@ $@

$(MONLIB_BRD) : $(XMONOBJS) $(XOBJS_BRD) $(XMONXTOSOBJS) $(XMONXTOS_BOARD_OBJS) $(RVEC_OBJ)
	-$(RM) $@
	xt-ar rs $@ $^
	xt-objcopy -W xt_mpu_init_cacheadrdis -W xt_mpu_init_table_size -W xt_mpu_init_table $@ $@

$(MONLIB_SIM_UNPACK) : $(XMONOBJS) $(XOBJS_SIM) $(XMONXTOSOBJS) $(XMONXTOS_SIM_OBJS) $(RVEC_UNPACK_OBJ)
	-$(RM) $@
	xt-ar rs $@ $^
	xt-objcopy -W xt_mpu_init_cacheadrdis -W xt_mpu_init_table_size -W xt_mpu_init_table $@ $@

$(MONLIB_BRD_UNPACK) : $(XMONOBJS) $(XOBJS_BRD) $(XMONXTOSOBJS) $(XMONXTOS_BOARD_OBJS) $(RVEC_UNPACK_OBJ)
	-$(RM) $@
	xt-ar rs $@ $^
	xt-objcopy -W xt_mpu_init_cacheadrdis -W xt_mpu_init_table_size -W xt_mpu_init_table $@ $@


# Include dependency rules (generated using -MD)
-include $(wildcard $(BINDIR)$(S)*.d)

FLAGS_memerror-vector-sm = -mtext-section-literals

# BINDIR nonsecure objects 
$(BINDIR)%.o: lib%.S
	$(CC) $(ASFLAGS) $(NSMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -c -o $@ $<

$(BINLIBDIR)%.o: lib%.S
	$(CC) $(ASFLAGS) $(NSMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINLIBDIR)%.o: lib%.c
	$(CC) $(CFLAGS) $(NSMCFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINLIBDIR)$(S)int-handler-level%-nsm.o: lib$(S)int-medpri-handlers-nsm.S
	$(CC) $(ASFLAGS) $(NSMAFLAGS) -D_INTERRUPT_LEVEL=$* -MD -MF $(subst .o,.d,$@) -c -o $@ $<

# BINDIR secure object is included directly during secure monitor link
$(BINDIR)%.o: mon%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

# BINMONDIR objects are packaged in secure libraries for secure monitor link
$(BINMONDIR)%.o: mon%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)%-sim.o: mon%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -DSIMULATOR -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)%.o: mon%.c
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)$(S)mpu_inittab.o: $(GENMPU_OUT)
	$(CC) $(CFLAGS) $(SMCFLAGS) $(SM_MPU_CFLAGS) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)$(S)mpu_init_final.o: mon$(S)mpu_init_final.c
	$(CC) $(CFLAGS) $(SMCFLAGS) $(SM_MPU_CFLAGS) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)$(S)int-handler-level%.o: mon$(S)int-handler-sm.S mon$(S)int-highpri-dispatcher-sm.S mon$(S)int-medpri-dispatcher-sm.S $(XTOSSRCDIR)$(S)xea2$(S)int-highpri-template.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) -D_INTERRUPT_LEVEL=$* -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONDIR)$(S)int-vector-level%.o: mon$(S)int-vector-sm.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) -D_INTERRUPT_LEVEL=$* -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(RVEC_OBJ): mon$(S)$(RVEC_SRC)
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(RVFLAGS) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(RVEC_UNPACK_OBJ): mon$(S)$(RVEC_SRC)
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(RVFLAGS) -DXTOS_UNPACK -MD -MF $(subst .o,.d,$@) -c -o $@ $<

# BINMONXTOSDIR objects are unmodified rebuilds of XTOS source
$(BINMONXTOSDIR)%-sim.o: $(XTOSSRCDIR)%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -DSIMULATOR -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)%-sim.o: $(XTOSSRCDIR)%.c
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$(subst $(S),,$*)) -DSIMULATOR -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)%-sim.o: $(XTOSSRCDIR)$(S)xea2%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -DSIMULATOR -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)%.o: $(XTOSSRCDIR)%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)%.o: $(XTOSSRCDIR)%.c
	$(CC) $(CFLAGS) $(SMCFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)%.o: $(XTOSSRCDIR)$(S)xea2%.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) $(FLAGS_$(subst $(S),,$*)) -MD -MF $(subst .o,.d,$@) -c -o $@ $<

$(BINMONXTOSDIR)$(S)xea2$(S)int-init-level%.o: $(XTOSSRCDIR)$(S)xea2$(S)int-initlevel.S
	$(CC) $(ASFLAGS) $(SMAFLAGS) -D_INTERRUPT_LEVEL=$* -MD -MF $(subst .o,.d,$@) -c -o $@ $<

.PHONY : all clean mkdir lib inc

