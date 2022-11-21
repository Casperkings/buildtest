add_subdirectory(lib/runtime_sim)
add_subdirectory(lib/xrp_sim)
add_subdirectory(lib/xdynlib_sim)
if (HOST_LINUX_CC)
  add_subdirectory(lib/runtime_native)
  add_subdirectory(lib/xrp_native)
  add_subdirectory(lib/xdynlib_native)
endif()
if (XTENSA_HOST_CORE)
  add_subdirectory(lib/runtime_xtensa_host_xos)
  add_subdirectory(lib/xrp_xtensa_host_xos)
  add_subdirectory(lib/xdynlib_xtensa_host_xos)
endif()
add_subdirectory(lib/tools)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/CL
        DESTINATION ${CMAKE_INSTALL_PREFIX}/host/include)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/runtime.h
        ${CMAKE_SOURCE_DIR}/include/xocl_kernel_info.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/host/include)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xrp.dts.o
  COMMAND dtc -O dtb
          ${CMAKE_SOURCE_DIR}/Subsystems/${XTENSA_OCL_SUBSYSTEM}/xrp.dts
          -o ${CMAKE_CURRENT_BINARY_DIR}/xrp.dtb
  COMMAND ${CMAKE_C_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/xrp.dtb.s -c -o
                              ${CMAKE_CURRENT_BINARY_DIR}/xrp.dts.o
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xrp.dtb.s
          ${CMAKE_SOURCE_DIR}/Subsystems/${XTENSA_OCL_SUBSYSTEM}/xrp.dts
  )

if (XTENSA_HOST_CORE)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xrp.xtensa_host.dts.o
    COMMAND dtc -O dtb
            ${CMAKE_SOURCE_DIR}/Subsystems/${XTENSA_OCL_SUBSYSTEM}/xrp.xtensa_host.dts
            -o ${CMAKE_CURRENT_BINARY_DIR}/xrp.xtensa_host.dtb
    COMMAND ${XTENSA_HOST_CC} --xtensa-core=${XTENSA_HOST_CORE}
            ${CMAKE_CURRENT_SOURCE_DIR}/xrp.xtensa_host.dtb.s -c -o
                              ${CMAKE_CURRENT_BINARY_DIR}/xrp.xtensa_host.dts.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xrp.dtb.s
            ${CMAKE_SOURCE_DIR}/Subsystems/${XTENSA_OCL_SUBSYSTEM}/xrp.xtensa_host.dts
    )
endif()

add_custom_target(xrp_dts ALL DEPENDS
                  ${CMAKE_CURRENT_BINARY_DIR}/xrp.dts.o)

if (XTENSA_HOST_CORE)
  add_custom_target(xrp_xtensa_host_dts ALL DEPENDS
                    ${CMAKE_CURRENT_BINARY_DIR}/xrp.xtensa_host.dts.o)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xrp.dts.o
        DESTINATION ${CMAKE_INSTALL_PREFIX}/host/lib)
if (XTENSA_HOST_CORE)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xrp.xtensa_host.dts.o
          DESTINATION ${CMAKE_INSTALL_PREFIX}/host/lib)
endif()
