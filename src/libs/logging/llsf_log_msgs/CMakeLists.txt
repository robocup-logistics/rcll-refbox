find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})


protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS LogMessage.proto)

# add_library(refbox-log-msgs ${PROTO_SRCS} ${PROTO_HDRS})
