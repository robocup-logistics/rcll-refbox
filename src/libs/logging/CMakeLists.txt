include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)

include_directories(llsf_log_msgs)
add_subdirectory(llsf_log_msgs)

find_package(Boost 1.81 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Protobuf REQUIRED)
set(PROTO_FILES
llsf_log_msgs/LogMessage.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
add_library(rcll-log-msgs SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(rcll-log-msgs ${Protobuf_LIBRARIES})
target_include_directories(rcll-log-msgs
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/
)

add_library(refbox-logging SHARED
    network.cpp
    cache.cpp
    logger.cpp
    websocket.cpp
    console.cpp
    multi.cpp
    multi.h
    file.cpp)

target_link_libraries(refbox-logging
 refbox-websocket
 refbox-log-msgs
  ${Boost_LIBRARIES} refbox-core)
target_link_libraries(refbox-logging ${Protobuf_LIBRARIES})
target_include_directories(refbox-logging PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(refbox-logging PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(refbox-logging PRIVATE ${Boost_INCLUDE_DIRS})

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-logging FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-logging
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
