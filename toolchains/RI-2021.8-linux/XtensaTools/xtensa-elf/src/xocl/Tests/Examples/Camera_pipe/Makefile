TOP_DIR=../..

NATIVE_KERNEL ?= 0

HOST_SRC_CPP_FILES = Camera_pipe_main.cpp ../Common/utils.cpp ${TOP_DIR}/Native//Camera_pipe_ref/camera_pipe_ref.cpp
DEVICE_SRC_CL_FILES = Camera_pipe.cl
DEVICE_CLFLAGS = -DNATIVE_KERNEL=${NATIVE_KERNEL} -I ../../libxi/include

ifneq ($(NATIVE_KERNEL),0)
  DEVICE_SRC_C_FILES = Camera_pipe.c
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

clean::
	rm -rf *gmon* *.ll *.s *.tmp.bc *.i Output.yuv Input.yuv *.o *.bc *.cbe.c
