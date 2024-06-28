find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_library(GECODE_LIBRARIES NAMES
  gecodeint gecodekernel gecodesupport gecodesearch gecodedriver
  gecodeminimodel PATHS /usr/lib64)
find_package(ProtobufComm REQUIRED)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
pkg_search_module(LIBMHD REQUIRED libmicrohttpd)

# include_directories(${GECODE_INCLUDE_DIRS})

pkg_search_module(AVAHI REQUIRED avahi-client)

add_executable(refbox main.cpp clips_logger.cpp
    refbox.cpp)
target_include_directories(refbox PRIVATE ${LIBMHD_INCLUDE_DIRS})
target_include_directories(refbox  PRIVATE ${CLIPSMM_INCLUDE_DIRS})

target_link_libraries(refbox
  refbox-protobuf-clips
  refbox-utils
  refbox-netcomm
  refbox-mps-comm
  refbox-mps-placing-clips
  refbox-mongodb-log
  protobuf_comm
gecodeminimodel gecodedriver gecodesearch gecodesupport gecodekernel gecodeint
  refbox-config
  refbox-logging
  rcll-protobuf-msgs)
target_link_libraries(refbox ${PROTOBUF_LIBRARIES} ${GECODE_LIBRARIES})
target_link_libraries(refbox ${PROTOBUFCOMM_LIBRARIES} )
target_link_libraries(refbox stdc++)

target_compile_options(refbox PRIVATE -DHAVE_WEBSOCKETS)
target_compile_options(refbox PRIVATE -DHAVE_MONGODB)
target_compile_options(refbox PRIVATE -DHAVE_AVAHI)

target_link_libraries(refbox ${LIBMHD_LIBRARIES})

target_link_libraries(refbox ${AVAHI_LIBRARIES})
set_target_properties(refbox PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
install(TARGETS refbox
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# create symlink for backwards compatibility
add_custom_command(
    TARGET refbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:refbox> $<TARGET_FILE_DIR:refbox>/llsf-refbox
    COMMENT "Creating symlink llsf-refbox -> refbox"
)
add_custom_target(llfs-refbox_symlink ALL DEPENDS refbox COMMENT "target to create symlink for old executable name")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink refbox ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/llsf-refbox)")
