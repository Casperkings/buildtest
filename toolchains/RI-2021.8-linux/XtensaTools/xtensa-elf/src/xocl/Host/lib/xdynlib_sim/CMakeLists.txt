SET(CMAKE_C_FLAGS "-Wall -Werror -fPIC")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXDYN_LIB_DEBUG")
endif()

set(XDYNLIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src/xdyn_lib_host.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src)

add_library(xdyn-lib-host-sim ${XDYNLIB_SRCS})

install(TARGETS xdyn-lib-host-sim
        ARCHIVE DESTINATION host/lib)
