TOP_DIR=../..
NATIVE_KERNEL ?= 0
HOST_SRC_CPP_FILES = fft2d_main.cpp fft2d_twidlefactors.cpp ../Common/utils.cpp ${TOP_DIR}/Native//FFT2D_ref/fft2d_ref.cpp
DEVICE_SRC_CL_FILES = fft2d.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = fft2d.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include  -I ${TOP_DIR}/Native/FFT2D_ref
  HOST_COMPILE_COMMON_FLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL}
endif

DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
  HOST_COMPILE_COMMON_FLAGS += -DXT_OCL_EXT
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv *.o *.d
