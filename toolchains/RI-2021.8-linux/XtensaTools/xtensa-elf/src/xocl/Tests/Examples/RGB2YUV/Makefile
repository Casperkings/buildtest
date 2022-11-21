TOP_DIR=../..
NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = RGB2YUV_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//RGB2YUV_ref/cvtcolor_rgb2yuv_ref.cpp 
DEVICE_SRC_CL_FILES = RGB2YUV.cl 
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = RGB2YUV.c
  DEVICE_C_OPTFLAGS = -I ../../libxi/include
endif

DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt

ifeq ($(XT_OCL_EXT),1)
  DEVICE_CLFLAGS += -DXT_OCL_EXT
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i OutputY.yuv OutputU.yuv OutputV.yuv Input.yuv *.o
