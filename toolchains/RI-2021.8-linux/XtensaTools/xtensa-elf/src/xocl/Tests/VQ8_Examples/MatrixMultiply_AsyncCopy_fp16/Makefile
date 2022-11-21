TOP_DIR=../..

HOST_SRC_CPP_FILES = mm.cpp ../Common/utils.cpp
DEVICE_SRC_CL_FILES = mm.cl
HOST_CMD_ARGS?=256 256 256 1 1

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif
