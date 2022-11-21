TOP_DIR=../..

NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = Median3x3_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//Median3x3_ref/medianblur_ref.cpp
DEVICE_SRC_CL_FILES = Median3x3.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include
ifneq ($(NATIVE_KERNEL),0)
DEVICE_SRC_C_FILES = Median3x3.c
DEVICE_C_OPTFLAGS = -I ../../libxi/include  
endif
DEVICE_OPTFLAGS = -mllvm -tensilica-misaligned-ldst-opt


ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv Input.yuv *.o *.bc *.cbe.c
