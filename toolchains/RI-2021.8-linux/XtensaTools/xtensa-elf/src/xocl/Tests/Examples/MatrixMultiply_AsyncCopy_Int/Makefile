TOP_DIR=../..
NATIVE_KERNEL ?= 0
HOST_SRC_CPP_FILES = mm.cpp ../../Native/matMul_int_ref/matrixmultiply_ref.cpp ../Common/utils.cpp
DEVICE_SRC_CL_FILES = mm.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = mmul.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
endif

DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt

HOST_CMD_ARGS?=128 128 128 1 1

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i *.o
