TOP_DIR=../..

HOST_SRC_CPP_FILES = mm.cpp ../Common/utils.cpp
DEVICE_SRC_CL_FILES = mm.cl
HOST_CMD_ARGS?=128 128 128 1 1

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  ifeq (${TARGET}, xtensa-xos)
    include ../Makefile.xtensa-xos-host
  else
    include ../Makefile.x86_64-host
  endif
endif
