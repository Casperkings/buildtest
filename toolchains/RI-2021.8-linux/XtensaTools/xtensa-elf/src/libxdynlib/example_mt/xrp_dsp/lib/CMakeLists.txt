SET(CMAKE_C_COMPILER xt-clang)
SET(CMAKE_C_FLAGS    "-W -Wall")

set(XRP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/xrp_src/xrp_dsp_hw_simple_xos_custom.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../xrp_host/lib/xrp_src/xrp-common
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../xrp_host/lib/xrp_src/xrp-kernel)

add_library(xrp-dsp-hw-simple-xos-custom ${XRP_SRCS})

install(TARGETS xrp-dsp-hw-simple-xos-custom
        ARCHIVE DESTINATION dsp/lib)
