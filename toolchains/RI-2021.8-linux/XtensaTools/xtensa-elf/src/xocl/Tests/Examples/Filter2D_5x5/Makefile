TOP_DIR=../..
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = Filter2D5x5_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//Filter2D_ref/functions.cpp
DEVICE_SRC_CL_FILES = Filter2D.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = Filter2D.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
  HOST_COMPILE_COMMON_FLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL}
endif

ifneq ($(XT_OCL_EXT),)
  HOST_COMPILE_COMMON_FLAGS = -DXT_OCL_EXT=${XT_OCL_EXT}
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
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv Input.yuv *.o *.bc *.cbe.c
