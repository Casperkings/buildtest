TOP_DIR=../..
NATIVE_KERNEL ?= 0
HOST_SRC_CPP_FILES = Bilateral5x5_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//Bilateral5x5_ref/functions_vq8.cpp
DEVICE_SRC_CL_FILES = Bilateral5x5.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = Bilateral5x5.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include  
  HOST_COMPILE_COMMON_FLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL}
endif

DEVICE_OPTFLAGS = -mllvm -xtensa-misaligned-ldst-opt

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
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv Input.yuv *.o *.d
