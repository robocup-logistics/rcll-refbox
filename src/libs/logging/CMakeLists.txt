include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})

include_directories(llsf_log_msgs)
add_subdirectory(llsf_log_msgs)

find_package(Boost 1.81 REQUIRED)
find_package(Protobuf REQUIRED)

# link_directories(${CLIPSMM_LIBRARY_DIRS})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# include_directories(${Boost_INCLUDE_DIRS})
# include_directories(${Protobuf_INCLUDE_DIRS})

# add_library(refbox-logging SHARED
#     network.cpp
#     cache.cpp
#     logger.cpp
#     websocket.cpp
#     console.cpp
#     multi.cpp
#     file.cpp)

# target_link_libraries(refbox-logging ${CLIPSMM_LIBRARIES} ${Boost_LIBRARIES} refbox-core refbox-log-msgs)
