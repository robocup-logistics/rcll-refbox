include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})

link_directories(${CLIPSMM_LIBRARY_DIRS})

add_library(protobuf_clips communicator.cpp)
target_link_libraries(protobuf_clips refbox-core m stdc++)
