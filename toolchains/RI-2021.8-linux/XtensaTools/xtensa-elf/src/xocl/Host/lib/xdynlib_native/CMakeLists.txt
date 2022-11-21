SET(CMAKE_C_FLAGS "-Wall -Werror -fPIC")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXDYN_LIB_DEBUG")
endif()

set(CMAKE_C_COMPILER ${HOST_LINUX_CC})
set(CMAKE_AR         ${HOST_LINUX_AR})
set(CMAKE_RANLIB     ${HOST_LINUX_RANLIB})

set(XDYNLIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src/xdyn_lib_host.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../xdynlib_src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src)

add_library(xdyn-lib-host-native ${XDYNLIB_SRCS})
add_library(xdyn-lib-host-native-shared SHARED ${XDYNLIB_SRCS})

install(TARGETS xdyn-lib-host-native
        ARCHIVE DESTINATION host/lib)
install(TARGETS xdyn-lib-host-native-shared
        ARCHIVE DESTINATION host/lib)
