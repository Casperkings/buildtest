TOP_DIR=../..

HOST_SRC_CPP_FILES = main.cpp  ../Common/utils.cpp
DEVICE_SRC_CL_FILES = simpleMath.cl

ifeq (${TARGET}, xtensa)
  include ../Makefile.xtensa-host
else
  include ../Makefile.x86_64-host
endif
