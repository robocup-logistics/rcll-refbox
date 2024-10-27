add_library(refbox-protobuf-rebroadcaster SHARED rebroadcaster.cpp)
find_package(Protobuf REQUIRED)
find_package(ProtobufComm REQUIRED)
target_link_libraries(refbox-protobuf-rebroadcaster protobuf::libprotobuf ProtobufComm::protobuf_comm
  refbox-config
  refbox-logging
)

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-protobuf-rebroadcaster FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-protobuf-rebroadcaster
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
