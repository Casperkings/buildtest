TOP_DIR=../..
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = IntegralU8U32_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//Integral_ref/Integral_ref.cpp 
DEVICE_SRC_CL_FILES = IntegralU8U32.cl 
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = IntegralU8U32.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

DEVICE_OPTFLAGS = -mllvm -xtensa-misaligned-ldst-opt -mllvm -mao-allow-byte-idx-group=true

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf Output.yuv Input.yuv *.o *.s *gmon*
