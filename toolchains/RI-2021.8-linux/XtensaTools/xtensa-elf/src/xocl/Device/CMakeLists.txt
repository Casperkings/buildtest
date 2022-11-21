add_subdirectory(lib/rt_interface)
add_subdirectory(lib/rt_interface_lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/krl_translate
        DESTINATION ${CMAKE_INSTALL_PREFIX}/device/include)

install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/pretreat.bc
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/foltreat.bc
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/KinfoRequiredDefs.bc
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/libbuildins.bc
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/libxocl-device-rt.a
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/gen_xt_ocl_register_kernels.pl
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${XTENSA_DEVICE}/xt_ocl_register_all_kernels_template.ll
        DESTINATION ${CMAKE_INSTALL_PREFIX}/device/lib)
