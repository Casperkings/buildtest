TOP_DIR=../..
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = LocalLaplacian_main.cpp LUT.cpp ../Common/utils.cpp ${TOP_DIR}/Native//LocalLaplacian_ref/LocalLaplacian_ref.cpp
DEVICE_SRC_CL_FILES = LocalLaplacian.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = LocalLaplacian.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv Input.yuv *.o
