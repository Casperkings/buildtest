set(CMAKE_CXX_FLAGS  "--xtensa-core=${XTENSA_HOST_CORE} -std=c++11 -DXOS -Wall -Werror -DXOCL_NUM_DSPS=${XOCL_NUM_DSPS}")

SET(CMAKE_CXX_COMPILER ${XTENSA_HOST_CXX})
set(CMAKE_AR           ${XTENSA_AR})
set(CMAKE_RANLIB       ${XTENSA_RANLIB})

if (${USE_OPENCL_DYNLIB})
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DUSE_OPENCL_DYNLIB")
endif()

set(RT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../runtime_src)

set(RT_SRCS ${RT_SRC_DIR}/oclwrapper.cpp ${RT_SRC_DIR}/xt_device.cpp
            ${RT_SRC_DIR}/kernel.cpp ${RT_SRC_DIR}/md5.cpp
            ${RT_SRC_DIR}/xt_kernel.cpp ${RT_SRC_DIR}/command_queue.cpp
            ${RT_SRC_DIR}/mem.cpp ${RT_SRC_DIR}/event.cpp 
            ${RT_SRC_DIR}/device.cpp ${RT_SRC_DIR}/context.cpp
            ${RT_SRC_DIR}/thread/xocl_thread_xos.cc)

include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/include/xrp
                    ${CMAKE_SOURCE_DIR}/Host/include
                    ${CMAKE_SOURCE_DIR}/Host/lib/xdynlib_src
                    ${CMAKE_SOURCE_DIR}/Host/include/thread)

add_library(xocl-host-rt-xtensa-xos ${RT_SRCS})

install(TARGETS xocl-host-rt-xtensa-xos
        ARCHIVE DESTINATION host/lib)
