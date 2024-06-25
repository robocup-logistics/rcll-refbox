find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(PROTO_FILES LogMessage.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(refbox-log-msgs SHARED ${PROTO_SRCS})
target_include_directories(refbox-log-msgs
    PUBLIC
    ${CMAKE_BINARY_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}/../
)

install(FILES ${PROTO_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-log-msgs)
install(TARGETS refbox-log-msgs
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${PROTO_FILES}
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/msgs/
)
