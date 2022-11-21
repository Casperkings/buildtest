TOP_DIR=../../
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = sobel_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native/Sobel5x5_ref/sobel_ref.cpp 
DEVICE_SRC_CL_FILES = sobel.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = sobel.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt -mllvm -ten-imap-always-cheap=true

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf Output_dx.yuv Output_dy.yuv *gmon* *.ll *.s *.tmp.bc *.i *.o *.d
