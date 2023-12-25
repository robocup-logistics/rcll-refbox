include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})

find_package(Boost 1.81.0 REQUIRED)
find_package(RapidJSON REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

link_directories(${CLIPSMM_LIBRARY_DIRS})

if(Boost_FOUND AND RapidJSON_FOUND)
    # Create the library
    add_library(refbox-websocket SHARED data.cpp client.cpp backend.cpp server.cpp)
    target_link_libraries(refbox-websocket stdc++ pthread refbox-core ${Boost_LIBRARIES})
else()
    message(WARNING "Cannot build llsfrbwebsocket library (one or more dependencies not found)")
endif()
