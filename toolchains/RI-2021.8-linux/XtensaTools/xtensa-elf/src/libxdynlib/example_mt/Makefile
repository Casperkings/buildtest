# Assumes XTENSA_CORE=XRC_Vision_Q7_AO. For a subsystem based on an alternate
# core, modify the subsys.yml and subsys.xld files included in this 
# directory
#
# Steps to build and run
#
# 1) make spotless (do make clean, if you only want to update just the app 
#                   and not the system builds and XRP host libs)
# 2) make xrp_setup
#    This copies over the XRP host source files from XtensaTools
# 3) make build 
#    This generates the XTSC model and the MP LSPs
# 4) make stmp-xrp 
#    This generates the XRP libraries for the host
# 5) make run
#    Builds the binaries for the host and the 2 DSPs and runs them

# Include platform-specific commands
CONFIGDIR = $(shell xt-run --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

TOOLS = $(shell xt-run --show-config=xttools)/Tools
XTENSA_TOOLS=$(shell xt-run --show-config=xttools)
ifeq ($(TEN4_INTERNAL),1)
TCTOOLS = -tctools $(XTTOOLS)/../tctools
endif

XDYN_LIB_INSTALL=../install

SYSBUILD_DIR = sysbuild_dir

# Below are specific to the subsystem
SUBSYS_YML = subsys.yml
SUBSYS_XLD = subsys.xld
#SUBSYS_YML = subsys.2p.yml
#SUBSYS_XLD = subsys.2p.xld
CACHE_ATTR=0x44441140
DSP0_COMM_BASE_MAGIC=0x90000000
DSP1_COMM_BASE_MAGIC=0x90001000

HOST_CC ?= $(TOOLS)/bin/gcc
HOST_AR ?= $(TOOLS)/bin/ar

CC = $(XTTOOLS)/bin/xt-clang
AR = $(XTTOOLS)/bin/xt-ar
XT_PKG_LOADLIB = $(XTTOOLS)/bin/xt-pkg-loadlib

LSP0=${SYSBUILD_DIR}/sys-x86_64-linux/${SUBSYS_XLD}/package/cores/CORE0/xtensa-elf/lib/sim
LSP1=${SYSBUILD_DIR}/sys-x86_64-linux/${SUBSYS_XLD}/package/cores/CORE1/xtensa-elf/lib/sim
SHARED_LSP=${SYSBUILD_DIR}/sys-x86_64-linux/${SUBSYS_XLD}/package/cores/shared/xtensa-elf/lib/shared

HOST_XRP_SRC=xrp_host/lib/xrp_src
DSP_XRP_SRC=xrp_dsp/lib/xrp_src

INCLUDES = -I ${XDYN_LIB_INSTALL}

DEVICE_LIB=device.lib
DEVICE_PKG_LIB=device.lib.pkg
HOST_EXE=host.exe
CORE0_EXE=core0.exe
CORE1_EXE=core1.exe
SHARED_EXE=shared.exe

default: all

CFLAGS = -g -Wall -Werror -mlongcalls
HOST_CFLAGS = -g -std=c99 -Wall -Werror -I $(XTTOOLS)/xtensa-elf/include

CFLAGS += -O2
HOST_CFLAGS += -O2

LIB_CFILES = \
  my_lib.c

HOST_CFILES = \
  host.c

DEV_CFILES = \
  dev.c

LIB_OBJ_FILES = $(LIB_CFILES:.c=.lib.o)
HOST_OBJ_FILES = $(HOST_CFILES:.c=.host.o)
DEV_OBJ_FILES = $(DEV_CFILES:.c=.dev.o)

HOST_LDFLAGS = \
  -Linstall/host/lib \
  -L${XDYN_LIB_INSTALL} \
  -L$(XTENSA_TOOLS)/Tools/lib \
  -lxrp-linux-sim -lpthread -lfdt -lrt -luuid \
  -Wl,-rpath=$(XTENSA_TOOLS)/Tools/lib \
  install/host/lib/xrp.dts.o \
  -lxdyn-lib-host

CORE0_LDFLAGS = -lxrp-dsp -L install/dsp/lib -lxrp-dsp-hw-simple-xos-custom \
                -lxrp-common \
                -L${XDYN_LIB_INSTALL} -lxdyn-lib-dev \
                -lidma-os -lxmem-bank-xos -lxmem-xos -lxos -lxtutil -lmloader \
                -Wl,--defsym,_memmap_cacheattr_reset=$(CACHE_ATTR) \
                -Wl,--defsym,xrp_dsp_comm_base_magic=$(DSP0_COMM_BASE_MAGIC) \
                -mlsp=$(LSP0)

CORE1_LDFLAGS = -lxrp-dsp -L install/dsp/lib -lxrp-dsp-hw-simple-xos-custom \
                -lxrp-common \
                -L${XDYN_LIB_INSTALL} -lxdyn-lib-dev \
                -lidma-os -lxmem-bank-xos -lxmem-xos -lxos -lxtutil -lmloader \
                -Wl,--defsym,_memmap_cacheattr_reset=$(CACHE_ATTR) \
                -Wl,--defsym,xrp_dsp_comm_base_magic=$(DSP1_COMM_BASE_MAGIC) \
                -mlsp=$(LSP1)

%.lib.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

%.host.o: %.c
	$(HOST_CC) $(INCLUDES) $(HOST_CFLAGS) -c -o $@ $<

%.dev.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
	$(CC) $(INCLUDES) $(CFLAGS) -MM $< -MT $@ > $*.d

$(DEVICE_LIB): $(LIB_OBJ_FILES)
	$(CC) $(CFLAGS) -mlsp=pimultload -Wl,--shared-pagesize=128 \
				-Wl,-pie -Wl,--no-relax -Wl,--multi-section-pil $^ \
        -ldynlib -lm -lc -o $@

$(DEVICE_PKG_LIB): $(DEVICE_LIB)
	$(XT_PKG_LOADLIB) -s $@ $^ -p

$(HOST_EXE): $(HOST_OBJ_FILES) ${XDYN_LIB_INSTALL}/libxdyn-lib-host.a
	$(HOST_CC) -o $@ $< $(HOST_LDFLAGS)

$(CORE0_EXE): $(DEV_OBJ_FILES) ${XDYN_LIB_INSTALL}/libxdyn-lib-dev.a $(SHARED_EXE)
	$(CC) $(CFLAGS) $< $(CORE0_LDFLAGS) -Wl,--just-symbols=$(SHARED_EXE) -o $@

$(CORE1_EXE): $(DEV_OBJ_FILES) ${XDYN_LIB_INSTALL}/libxdyn-lib-dev.a $(SHARED_EXE)
	$(CC) $(CFLAGS) $< $(CORE1_LDFLAGS) -Wl,--just-symbols=$(SHARED_EXE) -o $@

$(SHARED_EXE): shared.c
	$(CC) $(CFLAGS) $< -mlsp=$(SHARED_LSP) -o $@

# Copy over the XRP host sources
xrp_setup: 
	mkdir -p $(HOST_XRP_SRC)
	mkdir -p $(DSP_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp_api.h $(HOST_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-common $(HOST_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-host $(HOST_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-kernel $(HOST_XRP_SRC)
	cp -rf $(DSP_XRP_SRC)/../xrp_dsp_hw_simple_xos_custom.c $(DSP_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-dsp/xrp_dsp_hw.h $(DSP_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-dsp/xrp_dsp_interrupt.h $(DSP_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-dsp/xrp_dsp_sync.h $(DSP_XRP_SRC)
	cp -rf $(XTTOOLS)/xtensa-elf/src/xrp/xrp-dsp/xrp_dsp_user.h $(DSP_XRP_SRC)

stmp-xrp:
	rm -rf build install
	mkdir -p build install
	cd build && \
    CC=$(TOOLS)/bin/gcc CXX=$(TOOLS)/bin/g++ \
		CC=gcc CXX=g++  \
		$(TOOLS)/bin/cmake ..  \
			-DCMAKE_INSTALL_PREFIX=../install && \
	make VERBOSE=1 install && \
	cd ..
	rm -rf build
	touch stmp-xrp

build: $(SUBSYS_YML) $(SUBSYS_XLD)
	$(XTTOOLS)/libexec/xt-sysbuilder -subsys $(SUBSYS_YML) \
                                    -swtools $(XTENSA_TOOLS) \
                                    -build $(SYSBUILD_DIR)
	$(XTTOOLS)/libexec/xt-mbuild -root $(SYSBUILD_DIR) $(TCTOOLS) \
    -socfile $(SUBSYS_XLD) -preclean -build sw -lsps=sim
	cp -f $(SYSBUILD_DIR)/xrp/SubSystem.dts xrp_host/xrp.dts
	cp -f $(SYSBUILD_DIR)/xrp/XRP_Controller.lua \
       ${SYSBUILD_DIR}/xtsc-run/TestBenchMaster.vec

define run_pre
	rm -rf run_sim.sh; \
	cat run_prologue.sh > run_sim.sh; \
  echo "set -x" >> run_sim.sh; \
	echo "./$(1) $(2) & pids[0]=\$$!" >> run_sim.sh; \
	echo "sleep 1" >> run_sim.sh
endef 


run: all
	$(call run_pre,$(HOST_EXE))
	@echo "xtsc-run \
           --define=PROXY_POLL_INTERVAL=100 \
           --set_xtsc_parm=turbo=false \
           --set_xtsc_parm=turbo_max_relaxed_cycles=10000000 \
           --define=CORE0_BINARY=$(CORE0_EXE) \
           --define=CORE1_BINARY=$(CORE1_EXE) \
           --define=SHARED_RAM_L_NAME=SharedRAM_L.\$${pids[0]} \
           --define=SYSTEMRAM_DELAY=1 \
           --define=SYSTEMROM_DELAY=1 \
           --include=${SYSBUILD_DIR}/xtsc-run/SubSystem.inc & pids[1]=\$$!" >> run_sim.sh
	@echo "wait" >> run_sim.sh
	chmod 755 run_sim.sh
	./run_sim.sh

all: stmp-xrp $(DEVICE_PKG_LIB) $(HOST_EXE) $(CORE0_EXE) $(CORE1_EXE) $(SHARED_EXE)

spotless:
	$(RM_R) $(LIB_OBJ_FILES) $(DEVICE_LIB) $(DEVICE_PKG_LIB) $(HOST_OBJ_FILES) \
					$(DEV_OBJ_FILES) $(HOST_EXE) $(CORE0_EXE) $(CORE1_EXE) $(SHARED_EXE) \
					$(SYSBUILD_DIR) xrp_host/xrp.dts run_sim.sh \
					$(HOST_XRP_SRC) $(DSP_XRP_SRC) install stmp-xrp \
					*.d build install

clean:
	$(RM_R) $(LIB_OBJ_FILES) $(DEVICE_LIB) $(DEVICE_PKG_LIB) $(HOST_OBJ_FILES) \
					$(DEV_OBJ_FILES) $(HOST_EXE) $(CORE0_EXE) $(CORE1_EXE) $(SHARED_EXE) *.d

.phone: build
