SET(CMAKE_C_FLAGS "-W -Wall")

SET(CMAKE_C_COMPILER ${HOST_LINUX_CC})
SET(CMAKE_C_FLAGS    "-W -Wall -DHAVE_ATOMIC")
set(CMAKE_AR         ${HOST_LINUX_AR})
set(CMAKE_RANLIB     ${HOST_LINUX_RANLIB})

if (DEFINED ENV{ANDROID_BUILD})
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -DHAVE___U8 -DHAVE___U32 -DHAVE___U64 -fPIE -fPIC")
endif()

set(XRP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/xrp_threaded_queue.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/xrp_host_common.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/hosted/xrp_linux.c)

include_directories(${CMAKE_SOURCE_DIR}/include/xrp
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/thread-pthread
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/hosted
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-common
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-kernel)

add_library(xrp-linux-native ${XRP_SRCS})

install(TARGETS xrp-linux-native 
        ARCHIVE DESTINATION host/lib)
