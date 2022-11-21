# Needs XTENSA_SYSTEM, XTENSA_CORE to be set
# Eg: 
#   export XTENSA_CORE=visionp6_ao
#   export XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/tools/<release>-linux/XtensaTools/config

if [ -z ${XTENSA_SYSTEM+x} ]
then
  echo "Environment variable XTENSA_SYSTEM is not defined"
  return 1
fi

if [ -z ${XTENSA_CORE+x} ]
then
  echo "Environment variable XTENSA_CORE is not defined"
  return 1
fi

# Use xt-run from PATH or relative to XTENSA_SYSTEM
if [ -x "$(command -v xt-run)" ]; then
  XT_RUN=xt-run
else
  XT_RUN=${XTENSA_SYSTEM}/../bin/xt-run
fi

XTTOOLS=`${XT_RUN} --show-config=xttools`
TOOLS=`${XT_RUN} --show-config=xttools`/Tools

# Optionally set buildroot for Xtensa/Linux host
# export XTENSA_LINUX_BUILDROOT=<path to buildroot>

# Optionally set for Android hosts.
# export ANDROID_BUILD=<path to android-ndk>/toolchains/llvm/prebuilt/linux-x86_64
# Android host architecture - x86_64 or aarch64
# export ANDROID_HOST=x86_64
# Set API version based on Android version
# export ANDROID_API_LEVEL=29

export XTENSA_TOOLS=${TOOLS}
export XTENSA_SUBSYSTEM=${XTENSA_CORE}
export XTENSA_HOST_CORE=${XTENSA_CORE}
# Number of DSPs (compute-units) in the multi-core subsystem. Must be
# consistent with the number of DSPs in the subsystem.
export XTENSA_OPENCL_NUM_DSPS=2
# Number of DSPs (compute-units) to use by the Xtensa OpenCL run-time. 
# Can be between 1 and XTENSA_OPENCL_NUM_DSPS
export XTENSA_OPENCL_USE_NUM_DSPS=${XTENSA_OPENCL_NUM_DSPS}
export XTENSA_LLVM_INSTALL=${XTTOOLS}
export XTENSA_OCL_RT=${PWD}/install_xt_ocl_rt
export XTENSA_OCL_TESTS_DIR=${PWD}/Tests
# Note, if the system tools (gcc, cmake, etc.) are newer than the ones under
# ${TOOLS}/bin, add ${TOOLS}/bin to end of $PATH.
export PATH=${TOOLS}/bin:$PATH
