TOP_DIR=../..

HOST_SRC_CPP_FILES = vecAdd.cpp ../Common/utils.cpp
DEVICE_SRC_CL_FILES = vecAdd.cl

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif
