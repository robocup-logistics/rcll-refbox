include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})

link_directories(${CLIPSMM_LIBRARY_DIRS})

add_library(refbox-protobuf-clips SHARED communicator.cpp)
target_link_libraries(refbox-protobuf-clips refbox-core m stdc++)
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-protobuf-clips FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-protobuf-clips
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
