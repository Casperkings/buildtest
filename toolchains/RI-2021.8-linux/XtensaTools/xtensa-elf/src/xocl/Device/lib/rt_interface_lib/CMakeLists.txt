SET(CMAKE_C_COMPILER ${XTENSA_CLANG})
SET(CMAKE_C_FLAGS    "--xtensa-core=${XTENSA_CORE} -W -Wall -DXOCL_NUM_DSPS=${XOCL_NUM_DSPS} -mlongcalls -g")

set(RT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../rt_interface_src)

SET(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -DXOCL_DYN_LIB -DXDYN_LIB_XTOS")

if (${XOCL_TIME_KERNEL})
  SET(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -DXOCL_TIME_KERNEL")
endif()
if (${XOCL_TRACE_EXEC})
  SET(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} -DXOCL_TRACE_EXEC")
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXOCL_DEBUG")
endif()

set(CMAKE_AR         ${XTENSA_AR})
set(CMAKE_RANLIB     ${XTENSA_RANLIB})

set(XDYN_LIB_SRCS ${CMAKE_SOURCE_DIR}/Device/lib/xdynlib_src/xdyn_lib_dev.c)
set(SRCS ${RT_SRC_DIR}/main.c)
set(MISC_SRCS ${RT_SRC_DIR}/lmem.c ${RT_SRC_DIR}/dma.c ${RT_SRC_DIR}/sync.c
              ${RT_SRC_DIR}/sched.c ${RT_SRC_DIR}/sys.c)

include_directories(${CMAKE_SOURCE_DIR}/include/xrp 
                    ${CMAKE_SOURCE_DIR}/include/device
                    ${CMAKE_SOURCE_DIR}/Device/lib/xdynlib_src
                    ${CMAKE_SOURCE_DIR}/include)

add_library(xocl-device-rt-lib-main ${SRCS})
add_library(xocl-device-rt-lib-misc ${MISC_SRCS})
add_library(xdyn-lib-dev ${XDYN_LIB_SRCS})

install(TARGETS xocl-device-rt-lib-main
        ARCHIVE DESTINATION device/lib)
install(TARGETS xocl-device-rt-lib-misc 
        ARCHIVE DESTINATION device/lib)
install(TARGETS xdyn-lib-dev
        ARCHIVE DESTINATION device/lib)
