include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})


find_package(Boost REQUIRED COMPONENTS graph)
find_library(GECODE_LIBRARIES NAMES
  gecodeint gecodekernel gecodesupport gecodesearch gecodedriver
  gecodeminimodel PATHS /usr/lib64)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CLIPSMM_INCLUDE_DIRS})

link_directories(${CLIPSMM_LIBRARY_DIRS})

if(Boost_FOUND AND CLIPSMM_FOUND AND GECODE_LIBRARIES)
    # Create the library
    add_library(refbox_mps_placing_clips SHARED mps_placing_clips.cpp)
    target_link_libraries(refbox_mps_placing_clips stdc++ m refbox-core
      ${Boost_LIBRARIES} ${CLIPSMM_LIBRARIES} ${GECODE_LIBRARIES})
else()
    message(WARNING "Cannot build mps_placing_clips library (one or more dependencies not found)")
endif()
