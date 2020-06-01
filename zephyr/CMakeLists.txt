if(CONFIG_HSM)

zephyr_interface_library_named(Hsm)
zephyr_include_directories(../include)

target_include_directories(Hsm INTERFACE ../include)
target_link_libraries(Hsm INTERFACE zephyr_interface)

endif()
