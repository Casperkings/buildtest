SET(CMAKE_C_FLAGS "--xtensa-core=${XTENSA_HOST_CORE} -Wall -Werror -fPIC")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXDYN_LIB_DEBUG")
endif()

set(CMAKE_C_COMPILER ${XTENSA_HOST_CC})
set(CMAKE_AR         ${XTENSA_AR})
set(CMAKE_RANLIB     ${XTENSA_RANLIB})

set(XDYNLIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src/xdyn_lib_host.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src)

add_library(xdyn-lib-host-xtensa-xos ${XDYNLIB_SRCS})

install(TARGETS xdyn-lib-host-xtensa-xos
        ARCHIVE DESTINATION host/lib)
