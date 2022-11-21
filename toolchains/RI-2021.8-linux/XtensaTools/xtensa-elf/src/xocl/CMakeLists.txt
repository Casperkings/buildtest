project(XOCL)
cmake_minimum_required(VERSION 3.4.3)

INCLUDE(CheckIncludeFiles)

if (NOT DEFINED ENV{XTENSA_SYSTEM})
  message(FATAL_ERROR "XTENSA_SYSTEM environment variable not defined")
endif()

if (NOT DEFINED ENV{XTENSA_CORE})
  message(FATAL_ERROR "XTENSA_CORE environment variable not defined")
endif()

if (NOT DEFINED ENV{XTENSA_LLVM_INSTALL})
  message(FATAL_ERROR "XTENSA_LLVM_INSTALL environment variable not defined")
endif()

if (NOT DEFINED ENV{XTENSA_LINUX_BUILDROOT})
  message("XTENSA_LINUX_BUILDROOT environment variable not defined")
endif()

if (NOT DEFINED ENV{ANDROID_BUILD})
  message(STATUS "ANDROID_BUILD environment variable not defined")
endif()

if (NOT XTENSA_HOST_CORE)
  message(STATUS "XTENSA_HOST_CORE environment variable not defined")
endif()
  
set(XTENSA_CORE          $ENV{XTENSA_CORE})
set(XTENSA_HOST_CORE     $ENV{XTENSA_HOST_CORE})
set(XTENSA_SYSTEM        $ENV{XTENSA_SYSTEM})
set(ANDROID_API_LEVEL    $ENV{ANDROID_API_LEVEL})
set(ANDROID_HOST         $ENV{ANDROID_HOST})
set(XTENSA_OCL_SUBSYSTEM ${XTENSA_SUBSYSTEM})
set(XTENSA_DEVICE        ${XTENSA_CORE})

find_program(XTENSA_CLANG     xt-clang   $ENV{XTENSA_LLVM_INSTALL}/bin)
find_program(XTENSA_CLANGXX   xt-clang++ $ENV{XTENSA_LLVM_INSTALL}/bin)

find_program(XTENSA_HOST_CC      xt-clang   PATHS $ENV{XTENSA_SYSTEM}/../bin)
find_program(XTENSA_HOST_CXX     xt-clang++ PATHS $ENV{XTENSA_SYSTEM}/../bin)
find_program(XTENSA_AR           xt-ar      PATHS $ENV{XTENSA_SYSTEM}/../bin)
find_program(XTENSA_RANLIB       xt-ranlib  PATHS $ENV{XTENSA_SYSTEM}/../bin)

if (DEFINED ENV{XTENSA_LINUX_BUILDROOT})
  find_program(HOST_LINUX_CC    xtensa-linux-gcc 
               PATHS $ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin)
  find_program(HOST_LINUX_CXX    xtensa-linux-g++ 
               PATHS $ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin)
  find_program(HOST_LINUX_AR     xtensa-linux-ar 
             PATHS $ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin)
  find_program(HOST_LINUX_RANLIB xtensa-linux-ranlib 
               PATHS $ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin)

  if (NOT HOST_LINUX_CC)
    message(FATAL_ERROR "$ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin does not have xtensa-linux-gcc")
  endif()

  if (NOT HOST_LINUX_CXX)
    message(FATAL_ERROR "$ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin does not have xtensa-linux-g++")
  endif()

  if (NOT HOST_LINUX_AR)
    message(FATAL_ERROR "$ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin does not have xtensa-linux-ar")
  endif()

  if (NOT HOST_LINUX_RANLIB)
    message(FATAL_ERROR "$ENV{XTENSA_LINUX_BUILDROOT}/output/host/usr/bin does not have xtensa-linux-ranlib")
  endif()
endif()

if (DEFINED ENV{ANDROID_BUILD})
  if (NOT DEFINED ENV{ANDROID_HOST})
    message(FATAL_ERROR "ANDROID_HOST variable not defined. Required to be either x86_64 or aarch64")
  endif()
  if (NOT DEFINED ENV{ANDROID_API_LEVEL})
    message(FATAL_ERROR "ANDROID_API_LEVEL variable not defined")
  endif()
  find_program(HOST_LINUX_CC       ${ANDROID_HOST}-linux-android${ANDROID_API_LEVEL}-clang
               PATHS $ENV{ANDROID_BUILD}/bin)
  find_program(HOST_LINUX_CXX      ${ANDROID_HOST}-linux-android${ANDROID_API_LEVEL}-clang++
               PATHS $ENV{ANDROID_BUILD}/bin)
  find_program(HOST_LINUX_AR       ${ANDROID_HOST}-linux-android-ar
               PATHS $ENV{ANDROID_BUILD}/bin)
  find_program(HOST_LINUX_RANLIB   ${ANDROID_HOST}-linux-android-ranlib
               PATHS $ENV{ANDROID_BUILD}/bin)

  if (NOT HOST_LINUX_CC)
    message(FATAL_ERROR "$ENV{ANDROID_BUILD}/bin does not have ${ANDROID_HOST}-linux-android${ANDROID_API_LEVEL}-clang")
  endif()

  if (NOT HOST_LINUX_CXX)
    message(FATAL_ERROR "$ENV{ANDROID_BUILD}/bin does not have ${ANDROID_HOST}-linux-android${ANDROID_API_LEVEL}-clang++")
  endif()

  if (NOT HOST_LINUX_AR)
    message(FATAL_ERROR "$ENV{ANDROID_BUILD}/bin does not have ${ANDROID_HOST}-linux-android-ar")
  endif()

  if (NOT HOST_LINUX_RANLIB)
    message(FATAL_ERROR "$ENV{ANDROID_BUILD}/bin does not have ${ANDROID_HOST}-linux-android-ranlib")
  endif()
endif()

if (NOT XTENSA_CLANG)
  message(FATAL_ERROR "$ENV{XTENSA_LLVM_INSTALL}/bin does not have xt-clang")
endif()

if (NOT XTENSA_CLANGXX)
  message(FATAL_ERROR "$ENV{XTENSA_LLVM_INSTALL}/bin does not have xt-clang++")
endif()

if (NOT XTENSA_AR)
  message(FATAL_ERROR "$ENV{XTENSA_SYSTEM}/../bin does not have xt-ar")
endif()

if (NOT XTENSA_RANLIB)
  message(FATAL_ERROR "$ENV{XTENSA_SYSTEM}/../bin does not have xt-ranlib")
endif()

message("Using XTENSA_SYSTEM      ${XTENSA_SYSTEM}")
message("Using Device XTENSA_CORE ${XTENSA_CORE}")
message("Using Host XTENSA_CORE   ${XTENSA_HOST_CORE}")
message("Using XTENSA_CLANG       ${XTENSA_CLANG}")
message("Using XTENSA_HOST_CC     ${XTENSA_HOST_CC}")
message("Using XTENSA_HOST_CXX    ${XTENSA_HOST_CXX}")
message("Using XTENSA_AR          ${XTENSA_AR}")
message("Using XTENSA_RANLIB      ${XTENSA_RANLIB}")
message("Using HOST_LINUX_CC      ${HOST_LINUX_CC}")
message("Using HOST_LINUX_CXX     ${HOST_LINUX_CXX}")
message("Using HOST_LINUX_AR      ${HOST_LINUX_AR}")
message("Using HOST_LINUX_RANLIB  ${HOST_LINUX_RANLIB}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    message(FATAL_ERROR "GCC version must be at least 4.9!")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
    message(FATAL_ERROR "Clang version must be at least 3.4!")
  endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has
only been tested with Clang and GCC.")
endif()

#CHECK_INCLUDE_FILES(libfdt.h HAVE_LIBFDT_PKG)
#if (NOT HAVE_LIBFDT_PKG)
#  message(FATAL_ERROR "Needs libfdt package in your system")
#endif()

if (${USE_OPENCL_DYNLIB})
  CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_PKG)
  if (NOT HAVE_UUID_PKG)
    message(FATAL_ERROR "Needs uuid-dev package in your system")
  endif()
endif()

add_subdirectory(Device)
add_subdirectory(Host)

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/Subsystems/${XTENSA_OCL_SUBSYSTEM}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/Subsystem)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Host/lib/xdynlib_src/xdyn_lib.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/device/xdyn_lib_callback_if.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/device/idma_ext.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/xrp/xrp_opencl_ns.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
