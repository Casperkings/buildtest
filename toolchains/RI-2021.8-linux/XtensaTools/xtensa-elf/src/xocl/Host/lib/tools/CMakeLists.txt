add_definitions("-Werror -std=c++11")

set(TOOL_SRCS tools.cpp ${CMAKE_SOURCE_DIR}/Host/lib/runtime_src/md5.cpp)

include_directories(${CMAKE_SOURCE_DIR}/Host/include)

add_library(ngtool ${TOOL_SRCS})

install(TARGETS ngtool
        ARCHIVE DESTINATION host/lib)
