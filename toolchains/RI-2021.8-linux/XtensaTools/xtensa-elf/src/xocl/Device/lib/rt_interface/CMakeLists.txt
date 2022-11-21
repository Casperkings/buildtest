SET(CMAKE_C_COMPILER ${XTENSA_CLANG})
SET(CMAKE_C_FLAGS    "--xtensa-core=${XTENSA_CORE} -W -Wall -DXOCL_NUM_DSPS=${XOCL_NUM_DSPS} -mlongcalls -g")

set(RT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../rt_interface_src)

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

set(SRCS ${RT_SRC_DIR}/main.c)
set(MISC_SRCS ${RT_SRC_DIR}/lmem.c ${RT_SRC_DIR}/dma.c ${RT_SRC_DIR}/sync.c
              ${RT_SRC_DIR}/sched.c ${RT_SRC_DIR}/sys.c)

include_directories(${CMAKE_SOURCE_DIR}/include/xrp 
                    ${CMAKE_SOURCE_DIR}/include/device
                    ${CMAKE_SOURCE_DIR}/Device/lib/xdynlib_src
                    ${CMAKE_SOURCE_DIR}/include)

add_library(xocl-device-rt-main ${SRCS})
add_library(xocl-device-rt-misc ${MISC_SRCS})

install(TARGETS xocl-device-rt-main
        ARCHIVE DESTINATION device/lib)
install(TARGETS xocl-device-rt-misc 
        ARCHIVE DESTINATION device/lib)
