find_package(Protobuf REQUIRED)

# include_directories(${Protobuf_INCLUDE_DIRS})
# include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# install(FILES ${PROTO_HDRS} DESTINATION include/msg)

# message(STATUS "${PROTO_SRCS}=${${PROTO_SRCS}}")
# message(STATUS "${PROTO_HDRS}=${${PROTO_HDRS}}")
# add_library(msgs ${PROTO_SRCS} ${PROTO_HDRS})
add_library(msgs ${PROTO_FILES})
protobuf_generate(TARGET msgs)
# TARGET_LINK_LIBRARIES(msgs ${PROTOBUF_LIBRARIES})
# target_link_libraries(msgs ${PROTO_FILES})
