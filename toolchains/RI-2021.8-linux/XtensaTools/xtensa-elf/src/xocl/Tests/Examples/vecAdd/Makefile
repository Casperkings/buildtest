TOP_DIR=../../
HOST_SRC_CPP_FILES = vecAdd.cpp ../Common/utils.cpp
DEVICE_SRC_CL_FILES = vecAdd.cl

ifneq ($(XTENSA_OPENCL_USE_NUM_DSPS),)
  XOCL_NUM_DSPS=$(XTENSA_OPENCL_USE_NUM_DSPS)
else
  XOCL_NUM_DSPS=$(XTENSA_OPENCL_NUM_DSPS)
endif

ifeq ($(XT_OCL_HAVE_MAIN), 1)
  DEVICE_EXTERNAL_MAIN_SRC_C_FILES=main.c
  DEVICE_EXTERNAL_MAIN_CFLAGS=-DXOCL_NUM_DSPS=$(XOCL_NUM_DSPS) -I $(XTENSA_OCL_RT)/include
endif

ifeq ($(XT_OCL_HAVE_MAIN_XOS), 1)
  DEVICE_EXTERNAL_MAIN_SRC_C_FILES=main_xos.c
  DEVICE_EXTERNAL_MAIN_CFLAGS=-DXOCL_NUM_DSPS=$(XOCL_NUM_DSPS) -I $(XTENSA_OCL_RT)/include
endif

ifeq ($(XT_OCL_DYN_LIB), 1)
  DEVICE_EXTERNAL_MAIN_CFLAGS+=-DXOCL_DYN_LIB
endif

ifeq ($(XT_OCL_HAVE_MAIN_XOS), 1)
  DEVICE_LDFLAGS = -lxos -Wl,--defsym,xrp_user_queue_stack_size=16384
endif

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else 
  ifeq (${TARGET}, xtensa-xos)
    include ../Makefile.xtensa-xos-host
  else 
    ifeq (${TARGET}, android)
      include ../Makefile.android-host
    else
      include ../Makefile.x86_64-host
    endif
  endif
endif

clean::
	rm -rf $(DEVICE_EXTERNAL_MAIN_C_OBJS)
