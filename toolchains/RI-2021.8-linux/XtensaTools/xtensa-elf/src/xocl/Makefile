# Use from PATH or relative to XTENSA_SYSTEM
XT_RUN := $(shell PATH=$(XTENSA_SYSTEM)/../bin:$(PATH) which xt-run)

$(info PATH set to $(PATH))
$(info XTENSA_SYSTEM set to $(XTENSA_SYSTEM))
$(info XTENSA_CORE set to $(XTENSA_CORE))
$(info XTENSA_HOST_CORE set to $(XTENSA_HOST_CORE))
$(info XTENSA_SUBSYSTEM set to $(XTENSA_SUBSYSTEM))
$(info XTENSA_OPENCL_NUM_DSPS set to $(XTENSA_OPENCL_NUM_DSPS))
$(info XTENSA_OPENCL_USE_NUM_DSPS set to $(XTENSA_OPENCL_USE_NUM_DSPS))
$(info XTENSA_OCL_RT set to $(XTENSA_OCL_RT))
$(info XTENSA_LLVM_INSTALL set to $(XTENSA_LLVM_INSTALL))

XTOOLS_DIR=$(shell $(XT_RUN) --show-config=xttools)
XTCONFIG_DIR=$(shell $(XT_RUN) --show-config=config)

# Path to pre-built target libraries
XOCL_TARGET_DIR=$(XTCONFIG_DIR)/xtensa-elf/lib/xocl

SUBSYSTEM_BUILD_DIR=Subsystems/$(XTENSA_SUBSYSTEM)

# Paths to subsystem spec files for xt-mbuild. These are generated at
# config build time.
# Subsystem spec with X86 host + multi-core DSP subsystem
ifneq (,$(wildcard Subsystems/base/${XTENSA_SUBSYSTEM}/subsys.xld))
  DSP_XLD=Subsystems/base/${XTENSA_SUBSYSTEM}/subsys.xld
else
  DSP_XLD=$(XOCL_TARGET_DIR)/subsys.xld
endif
# Subsystem spec with Xtensa host + multi-core DSP subsystem
ifneq (,$(wildcard Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/subsys.xld))
  DSP_XTENSA_HOST_XLD=Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/subsys.xld
else
  DSP_XTENSA_HOST_XLD=$(XOCL_TARGET_DIR)/subsys.xtensa_host.xld
endif
# Subsystem spec with DC_233L Xtensa/Linux host + multi-core DSP subsystem
DSP_XTENSA_LINUX_HOST_XLD=Subsystems/xtensa_linux_host/${XTENSA_SUBSYSTEM}/subsys.xld
# Subsystem spec with Android host + multi-core DSP subsystem
DSP_ANDROID_HOST_XLD=Subsystems/android_host/${XTENSA_SUBSYSTEM}/subsys.xld

# Paths to subsystem spec files for xt-sysbuilder. These are generated at
# config build time.
# Subsystem spec with X86 host + multi-core DSP subsystem
ifneq (,$(wildcard Subsystems/base/${XTENSA_SUBSYSTEM}/subsys.yml))
  DSP_YML=Subsystems/base/${XTENSA_SUBSYSTEM}/subsys.yml
else
  DSP_YML=$(XOCL_TARGET_DIR)/subsys.yml
endif
# Subsystem spec with Xtensa host + multi-core DSP subsystem
ifneq (,$(wildcard Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/subsys.yml))
  DSP_XTENSA_HOST_YML=Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/subsys.yml
else
  DSP_XTENSA_HOST_YML=$(XOCL_TARGET_DIR)/subsys.xtensa_host.yml
endif
# Subsystem spec with DC_233L Xtensa/Linux host + multi-core DSP subsystem
DSP_XTENSA_LINUX_HOST_YML=Subsystems/xtensa_linux_host/${XTENSA_SUBSYSTEM}/subsys.yml
# Subsystem spec with Android host + multi-core DSP subsystem
DSP_ANDROID_HOST_YML=Subsystems/android_host/${XTENSA_SUBSYSTEM}/subsys.yml

# The pre-built target libraries are copied in here
DEVICE_LIB_DIR=Device/lib/$(XTENSA_CORE)

# XRP for host
HOST_XRP_SRC=Host/lib/xrp_src

# XDYNLIB for host/devvice
HOST_XDYN_LIB_SRC=Host/lib/xdynlib_src
DEV_XDYN_LIB_SRC=Device/lib/xdynlib_src

# Platform specific cache attributes + XRP communication base address
PLATFORM_MAKEFILE=Tests/Makefile.platform
PLATFORM_XTENSA_XOS_MAKEFILE=Tests/Makefile.xtensa_xos_host.platform
PLATFORM_ANDROID_HOST_MAKEFILE=Tests/Makefile.android_host.platform
PLATFORM_XTENSA_HOST_MAKEFILE=Tests/Makefile.xtensa_host.platform

# Xtensa OpenCL runtime build/install dir
XOCL_BUILD=build_xt_ocl_rt
XOCL_INSTALL=install_xt_ocl_rt

# Set below for internal builds only
ifeq ($(TEN4_INTERNAL),1)
  TCTOOLS=-tctools $(XTOOLS_DIR)/../tctools
endif

# Number of DSPs as defined in the subsystem. Set to '1' if making single-core
# performance measurements. XTENSA_OPENCL_USE_NUM_DSPS has to be
# 1 and XTENSA_OPENCL_NUM_DSPS
ifeq ($(XTENSA_OPENCL_USE_NUM_DSPS),)
  NUM_DSPS=$(XTENSA_OPENCL_NUM_DSPS)
else
  NUM_DSPS=$(XTENSA_OPENCL_USE_NUM_DSPS)
endif

# Internal timing. Profiling is the preferred way to measure performance
ifeq ($(XTENSA_OPENCL_TIME_KERNEL),1)
  TIME_KERNEL="On"
else
  TIME_KERNEL="Off"
endif

# Process the build options for clBuildProgram. The -D options are appended
# to the kernel source. It is expected that the CL file passed via offline
# compilation flow to the device compiler does not have the options embedded
# in the file.
ifeq ($(XTENSA_OPENCL_PROCESS_BUILD_OPTIONS),1)
  PROCESS_OPENCL_BUILD_OPTIONS="On"
else
  PROCESS_OPENCL_BUILD_OPTIONS="Off"
endif

# Emit OpenCL kernel source file processed by the host. 
# If PROCESS_OPENCL_BUILD_OPTIONS is enabled, the generated source file will
# include #defines of the clBuildProgram options
ifeq ($(XTENSA_OPENCL_EMIT_KERNEL_SRC),1)
  EMIT_OPENCL_KERNEL_SRC="On"
else
  EMIT_OPENCL_KERNEL_SRC="Off"
endif

# Build host side to for dynamically loading device libraries
ifeq ($(XTENSA_USE_OPENCL_DYNLIB),1)
  USE_OPENCL_DYNLIB="On"
else
  USE_OPENCL_DYNLIB="Off"
endif

.PHONY: all buildroot mkdirs clean spotless

all: mkdirs $(SUBSYSTEM_BUILD_DIR)/LSP $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host $(XOCL_INSTALL)

# Check if Android subsystem files for the target DSPs are present
$(DSP_ANDROID_HOST_XLD):
	@if [ ! -f $(DSP_ANDROID_HOST_XLD) ]; \
	then \
		echo "File $(DSP_ANDROID_HOST_XLD) for Android DSP subsystem does not exist!"; \
	fi;

$(DSP_ANDROID_HOST_YML):
	@if [ ! -f $(DSP_ANDROID_HOST_YML) ]; \
	then \
		echo "File $(DSP_ANDROID_HOST_YML) for Android DSP subsystem does not exist!"; \
	fi;

# Check if Xtensa/Linux subsystem files for the target DSPs are present
$(DSP_XTENSA_LINUX_HOST_XLD):
	@if [ ! -f $(DSP_XTENSA_LINUX_HOST_XLD) ]; \
	then \
		echo "File $(DSP_XTENSA_LINUX_HOST_XLD) for Xtensa/Linux DSP subsystem does not exist!"; \
	fi;

$(DSP_XTENSA_LINUX_HOST_YML):
	@if [ ! -f $(DSP_XTENSA_LINUX_HOST_YML) ]; \
	then \
		echo "File $(DSP_XTENSA_LINUX_HOST_YML) for Xtensa/Linux DSP subsystem does not exist!"; \
	fi;

# Setup for buildroot
buildroot: mkdirs $(SUBSYSTEM_BUILD_DIR)/LSP

# Build the LSPs for the DSP subsystem with X86 and Xtensa/Linux host
$(SUBSYSTEM_BUILD_DIR)/LSP: $(DSP_XLD)
	rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP $(SUBSYSTEM_BUILD_DIR)/LSP_build 
	mkdir $(SUBSYSTEM_BUILD_DIR)/LSP
	$(XTOOLS_DIR)/libexec/xt-mbuild -root $(SUBSYSTEM_BUILD_DIR)/LSP_build -syspkg $(SUBSYSTEM_BUILD_DIR)/LSP_build/package $(TCTOOLS) -socfile $(DSP_XLD) -preclean -build sw -lsps sim,sim-stacklocal
	cp -r $(SUBSYSTEM_BUILD_DIR)/LSP_build/package/cores/* $(SUBSYSTEM_BUILD_DIR)/LSP
	if [ ! -z $(XTENSA_LINUX_BUILDROOT) ]; \
	then \
		rm -rf $(XTENSA_LINUX_BUILDROOT)/board/xtsc/${XTENSA_SUBSYSTEM}/LSP; \
		mkdir $(XTENSA_LINUX_BUILDROOT)/board/xtsc/${XTENSA_SUBSYSTEM}/LSP; \
		cp -r $(SUBSYSTEM_BUILD_DIR)/LSP_build/package/cores/* $(XTENSA_LINUX_BUILDROOT)/board/xtsc/${XTENSA_SUBSYSTEM}/LSP; \
	fi;
	rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_build 

# Build the LSPs for the DSP subsystem with Xtensa host
$(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host: $(DSP_XTENSA_HOST_XLD)
	if [ ! -z $(XTENSA_HOST_CORE) ]; \
	then \
	  rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build; \
	  mkdir $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host; \
	  $(XTOOLS_DIR)/libexec/xt-mbuild -root $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build -syspkg $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build/package $(TCTOOLS) -socfile $(DSP_XTENSA_HOST_XLD) -preclean -build sw -lsps sim,sim-stacklocal; \
	  cp -r $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build/package/cores/* $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host; \
	  rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build; \
	fi;

# Build the LSPs for the DSP subsystem with Android host
$(SUBSYSTEM_BUILD_DIR)/LSP_android_host: $(DSP_ANDROID_HOST_XLD)
	if [ ! -z $(ANDROID_BUILD) ]; \
	then \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_android_host $(SUBSYSTEM_BUILD_DIR)/LSP_android_host_build; \
		mkdir $(SUBSYSTEM_BUILD_DIR)/LSP_android_host; \
		$(XTOOLS_DIR)/libexec/xt-mbuild -root $(SUBSYSTEM_BUILD_DIR)/LSP_android_host_build -syspkg $(SUBSYSTEM_BUILD_DIR)/LSP_android_host_build/package $(TCTOOLS) -socfile $(DSP_ANDROID_HOST_XLD) -preclean -build sw -lsps sim,sim-stacklocal; \
		cp -r $(SUBSYSTEM_BUILD_DIR)/LSP_android_host_build/package/cores/* $(SUBSYSTEM_BUILD_DIR)/LSP_android_host; \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_android_host_build; \
	fi;

# Build the xtsc model and xrp spec for the DSP subsystem with X86 host
$(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim: $(DSP_YML)
	rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim
	$(XTOOLS_DIR)/libexec/xt-sysbuilder -subsys $(DSP_YML) -swtools $(XTOOLS_DIR) -build $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild -xtensa-system $(XTENSA_SYSTEM)
	cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild/xtsc-run $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim
	cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild/xrp/XRP_Controller.lua $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim/TestBenchMaster.vec
	cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild/xrp/SubSystem.dts $(SUBSYSTEM_BUILD_DIR)/xrp.dts
	rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild

# Build the xtsc model and xrp spec for the DSP subsystem with Xtensa host
$(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host: $(DSP_XTENSA_HOST_YML)
	if [ ! -z $(XTENSA_HOST_CORE) ]; \
	then \
	  rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host; \
	  $(XTOOLS_DIR)/libexec/xt-sysbuilder -subsys ${DSP_XTENSA_HOST_YML} -swtools $(XTOOLS_DIR) -build $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild -xtensa-system $(XTENSA_SYSTEM); \
	  cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild/xtsc-run $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host; \
	  cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild/xrp/SubSystem.dts $(SUBSYSTEM_BUILD_DIR)/xrp.xtensa_host.dts; \
	  rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild; \
	fi;

# Build the xtsc model for the DSP subsystem with Xtensa linux/host.
$(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host: $(DSP_XTENSA_LINUX_HOST_YML)
	if [ ! -z $(XTENSA_LINUX_BUILDROOT) ]; \
	then \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_linux_host_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host; \
		$(XTOOLS_DIR)/libexec/xt-sysbuilder -subsys ${DSP_XTENSA_LINUX_HOST_YML} -swtools $(XTOOLS_DIR) -build $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_linux_host_sysbuild -xtensa-system $(XTENSA_SYSTEM); \
		cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_linux_host_sysbuild/xtsc-run $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host; \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_linux_host_sysbuild; \
	fi;

# Build the xtsc model for the DSP subsystem with Android host
$(SUBSYSTEM_BUILD_DIR)/xtsc-run.android_host: $(DSP_ANDROID_HOST_YML)
	if [ ! -z $(ANDROID_BUILD) ]; \
	then \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_android_host_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.android_host; \
		$(XTOOLS_DIR)/libexec/xt-sysbuilder -subsys ${DSP_ANDROID_HOST_YML} -swtools $(XTOOLS_DIR) -build $(SUBSYSTEM_BUILD_DIR)/dsp_android_host_sysbuild -xtensa-system $(XTENSA_SYSTEM); \
		cp -r $(SUBSYSTEM_BUILD_DIR)/dsp_android_host_sysbuild/xtsc-run $(SUBSYSTEM_BUILD_DIR)/xtsc-run.android_host; \
		rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_android_host_sysbuild; \
	fi;

# Copy over the XRP host sources
$(HOST_XRP_SRC): 
	rm -rf $(HOST_XRP_SRC)
	mkdir -p $(HOST_XRP_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/xrp/xrp_api.h $(HOST_XRP_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/xrp/xrp-common $(HOST_XRP_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/xrp/xrp-host $(HOST_XRP_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/xrp/xrp-kernel $(HOST_XRP_SRC)
	cp -rf include/xrp/xrp_atomic.h $(HOST_XRP_SRC)/xrp-host

# Copy over the XDYN_LIB sources
$(HOST_XDYN_LIB_SRC): 
	rm -rf $(HOST_XDYN_LIB_SRC)
	mkdir -p $(HOST_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib.h $(HOST_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_misc.h $(HOST_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_sys.h $(HOST_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_common.h $(HOST_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_host.c $(HOST_XDYN_LIB_SRC)

# Copy over the XDYN_LIB sources
$(DEV_XDYN_LIB_SRC): 
	rm -rf $(DEV_XDYN_LIB_SRC)
	mkdir -p $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib.h $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_misc.h $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_common.h $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_sys.h $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_dev.h $(DEV_XDYN_LIB_SRC)
	cp -rf $(XTOOLS_DIR)/xtensa-elf/src/libxdynlib/xdyn_lib_dev.c $(DEV_XDYN_LIB_SRC)

# Copy over the target device libraries
$(DEVICE_LIB_DIR): $(XOCL_TARGET_DIR)/libbuildins.bc $(XOCL_TARGET_DIR)/pretreat.bc $(XOCL_TARGET_DIR)/KinfoRequiredDefs.bc $(XOCL_TARGET_DIR)/foltreat.bc $(XOCL_TARGET_DIR)/libxocl-device-rt.a $(XOCL_TARGET_DIR)/xt_ocl_register_all_kernels_template.ll $(XOCL_TARGET_DIR)/gen_xt_ocl_register_kernels.pl
	rm -rf $(DEVICE_LIB_DIR)
	mkdir -p $(DEVICE_LIB_DIR)
	cp -r $(XOCL_TARGET_DIR)/*.bc $(DEVICE_LIB_DIR)
	cp -r $(XOCL_TARGET_DIR)/libxocl-device-rt*.a $(DEVICE_LIB_DIR)
	cp -r $(XOCL_TARGET_DIR)/xt_ocl_register_all_kernels_template.ll $(DEVICE_LIB_DIR)
	cp -r $(XOCL_TARGET_DIR)/gen_xt_ocl_register_kernels.pl $(DEVICE_LIB_DIR)

# Copy platform specific attributes
$(PLATFORM_MAKEFILE):
ifneq (,$(wildcard Subsystems/base/${XTENSA_SUBSYSTEM}/Makefile.attr)) 
	cp -f Subsystems/base/${XTENSA_SUBSYSTEM}/Makefile.attr $(PLATFORM_MAKEFILE)
else
	cp -f $(XOCL_TARGET_DIR)/Makefile.attr $(PLATFORM_MAKEFILE)
endif
ifneq (,$(wildcard Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/Makefile.attr)) 
	if [ ! -z $(XTENSA_HOST_CORE) ]; \
	then \
		cp -f Subsystems/xtensa_host/${XTENSA_SUBSYSTEM}/Makefile.attr $(PLATFORM_XTENSA_XOS_MAKEFILE); \
	fi;
else
	if [ ! -z $(XTENSA_HOST_CORE) ]; \
	then \
		cp -f $(XOCL_TARGET_DIR)/Makefile.attr $(PLATFORM_XTENSA_XOS_MAKEFILE); \
	fi;
endif

# Build the SDK
$(XOCL_INSTALL): $(SUBSYSTEM_BUILD_DIR)/LSP $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host $(SUBSYSTEM_BUILD_DIR)/LSP_android_host $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host $(SUBSYSTEM_BUILD_DIR)/xtsc-run.android_host $(DEVICE_LIB_DIR) $(PLATFORM_MAKEFILE) $(HOST_XRP_SRC) $(HOST_XDYN_LIB_SRC) $(DEV_XDYN_LIB_SRC)
	rm -rf $(XOCL_BUILD) $(XOCL_INSTALL)
	mkdir -p $(XOCL_BUILD)
	cd $(XOCL_BUILD) &&                           \
    CC=gcc CXX=g++ cmake ..                     \
      -DCMAKE_BUILD_TYPE=Release                \
      -DXOCL_TIME_KERNEL=$(TIME_KERNEL)         \
      -DXOCL_TRACE_EXEC=Off                     \
      -DXOCL_NUM_DSPS=$(NUM_DSPS)               \
      -DEMIT_OPENCL_KERNEL_SRC=$(EMIT_OPENCL_KERNEL_SRC)             \
      -DPROCESS_OPENCL_BUILD_OPTIONS=$(PROCESS_OPENCL_BUILD_OPTIONS) \
      -DUSE_OPENCL_DYNLIB=${USE_OPENCL_DYNLIB}                       \
      -DCMAKE_INSTALL_PREFIX=../$(XOCL_INSTALL) \
      -DXTENSA_SUBSYSTEM=$(XTENSA_SUBSYSTEM) && \
    make -j16 install                        && \
  cd ..
	rm -rf $(XOCL_BUILD)

mkdirs:
	mkdir -p $(SUBSYSTEM_BUILD_DIR)
	if [ ! -z $(XTENSA_LINUX_BUILDROOT) ]; \
	then \
		rm -rf $(XTENSA_LINUX_BUILDROOT)/board/xtsc/${XTENSA_SUBSYSTEM}; \
		cp -rf Subsystems/xtensa_linux_host/buildroot/board/xtsc/${XTENSA_SUBSYSTEM} $(XTENSA_LINUX_BUILDROOT)/board/xtsc; \
		cp -rf Subsystems/xtensa_linux_host/buildroot/configs/xtsc_${XTENSA_SUBSYSTEM}_xrp_defconfig $(XTENSA_LINUX_BUILDROOT)/configs; \
		cp -f Subsystems/xtensa_linux_host/${XTENSA_SUBSYSTEM}/Makefile.attr $(PLATFORM_XTENSA_HOST_MAKEFILE); \
	fi;
	if [ ! -z $(ANDROID_BUILD) ]; \
  then \
    cp -f Subsystems/android_host/${XTENSA_SUBSYSTEM}/Makefile.attr $(PLATFORM_ANDROID_HOST_MAKEFILE); \
  fi;

clean:
	rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP $(SUBSYSTEM_BUILD_DIR)/LSP_build
	rm -rf $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host $(SUBSYSTEM_BUILD_DIR)/LSP_xtensa_host_build
	rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.sim
	rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_host_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_host
	rm -rf $(SUBSYSTEM_BUILD_DIR)/dsp_xtensa_linux_host_sysbuild $(SUBSYSTEM_BUILD_DIR)/xtsc-run.xtensa_linux_host
	rm -rf $(SUBSYSTEM_BUILD_DIR)/xrp.dts $(SUBSYSTEM_BUILD_DIR)/xrp.xtensa_host.dts
	rm -rf $(XOCL_BUILD) $(XOCL_INSTALL)
	rm -rf $(DEVICE_LIB_DIR)
	rm -rf $(PLATFORM_MAKEFILE) $(PLATFORM_XTENSA_XOS_MAKEFILE)
	rm -rf $(PLATFORM_XTENSA_HOST_MAKEFILE) $(PLATFORM_ANDROID_HOST_MAKEFILE)
	rm -rf $(XOCL_BUILD) $(XOCL_INSTALL)
	rm -rf $(HOST_XRP_SRC)
	rm -rf $(HOST_XDYN_LIB_SRC)
	rm -rf $(DEV_XDYN_LIB_SRC)

spotless: clean
	rm -rf $(SUBSYSTEM_BUILD_DIR)
