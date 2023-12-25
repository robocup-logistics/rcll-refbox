find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})


# file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS LogMessage.proto)

add_library(mps-log-msgs SHARED LogMessage.proto)
protobuf_generate(TARGET mps-log-msgs LANGUAGE cpp)
# add_library(refbox-log-msgs ${PROTO_SRCS} ${PROTO_HDRS})
