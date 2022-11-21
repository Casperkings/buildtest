SET(CMAKE_C_COMPILER ${XTENSA_HOST_CC})
SET(CMAKE_C_FLAGS    "--xtensa-core=${XTENSA_HOST_CORE} -W -Wall")
set(CMAKE_AR         ${XTENSA_AR})
set(CMAKE_RANLIB     ${XTENSA_RANLIB})


set(XRP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/xrp_threaded_queue.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/xrp_host_common.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/standalone/xrp_host.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/standalone/xrp_elf.c
             ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-kernel/xrp_alloc.c)

include_directories(${CMAKE_SOURCE_DIR}/include/xrp
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/thread-xos
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-host/standalone
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-common
                    ${CMAKE_CURRENT_SOURCE_DIR}/../xrp_src/xrp-kernel)

add_library(xrp-xtensa-host-xos ${XRP_SRCS})

install(TARGETS xrp-xtensa-host-xos
        ARCHIVE DESTINATION host/lib)
