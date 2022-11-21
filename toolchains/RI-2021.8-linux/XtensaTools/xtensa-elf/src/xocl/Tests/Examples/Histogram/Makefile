TOP_DIR=../..
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = histogram_main.cpp ../Common/utils.cpp 
DEVICE_SRC_CL_FILES = histogram.cl reduce.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL}

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = histogram.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

# Override --xt-async-opt
reduce.bc: reduce.cl
	$(XTENSA_LLVM_INSTALL)/bin/xt-clang -x mxpa -DXTENSA \
                             $(DEVICE_CLFLAGS) \
                             -m32 $< -o $@

clean::
	rm -rf *.o *.s *gmon*
