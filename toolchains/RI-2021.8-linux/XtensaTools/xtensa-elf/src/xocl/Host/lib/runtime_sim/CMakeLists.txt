set(CMAKE_CXX_FLAGS  "-std=c++11 -DXRP_SIM -fPIC -Wall -Werror -DXOCL_NUM_DSPS=${XOCL_NUM_DSPS} -DHAVE_UUID")

if (${EMIT_OPENCL_KERNEL_SRC})
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DEMIT_OPENCL_KERNEL_SRC")
endif()

if (${PROCESS_OPENCL_BUILD_OPTIONS})
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DPROCESS_OPENCL_BUILD_OPTIONS")
endif()

if (${USE_OPENCL_DYNLIB})
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DUSE_OPENCL_DYNLIB")
endif()

set(RT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../runtime_src)

set(RT_SRCS ${RT_SRC_DIR}/oclwrapper.cpp ${RT_SRC_DIR}/xt_device.cpp
            ${RT_SRC_DIR}/kernel.cpp ${RT_SRC_DIR}/md5.cpp
            ${RT_SRC_DIR}/xt_kernel.cpp ${RT_SRC_DIR}/command_queue.cpp
            ${RT_SRC_DIR}/mem.cpp ${RT_SRC_DIR}/event.cpp 
            ${RT_SRC_DIR}/device.cpp ${RT_SRC_DIR}/context.cpp
            ${RT_SRC_DIR}/thread/xocl_thread.cc)

include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/include/xrp
                    ${CMAKE_SOURCE_DIR}/Host/lib/xrp_src
                    ${CMAKE_SOURCE_DIR}/Host/lib/xdynlib_src
                    ${CMAKE_SOURCE_DIR}/Host/include
                    ${CMAKE_SOURCE_DIR}/Host/include/thread)

add_library(xocl-host-rt-sim ${RT_SRCS})
add_library(xocl-host-rt-sim-shared SHARED ${RT_SRCS})

install(TARGETS xocl-host-rt-sim
        ARCHIVE DESTINATION host/lib)
install(TARGETS xocl-host-rt-sim-shared
        DESTINATION host/lib)
