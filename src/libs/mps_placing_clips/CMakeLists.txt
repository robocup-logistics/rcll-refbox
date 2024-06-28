include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)

find_package(Boost REQUIRED COMPONENTS graph)
find_library(GECODE_LIBRARIES NAMES
  gecodeint gecodekernel gecodesupport gecodesearch gecodedriver
  gecodeminimodel PATHS /usr/lib64)

if(Boost_FOUND AND CLIPSMM_FOUND AND GECODE_LIBRARIES)
    # Create the library
    add_library(refbox-mps-placing-clips SHARED mps_placing_clips.cpp)
    target_link_libraries(refbox-mps-placing-clips stdc++ m refbox-core
      ${Boost_LIBRARIES} ${CLIPSMM_LIBRARIES} ${GECODE_LIBRARIES})
    target_include_directories(refbox-mps-placing-clips PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(refbox-mps-placing-clips PRIVATE ${Boost_INCLUDE_DIRS})
    target_include_directories(refbox-mps-placing-clips PRIVATE ${CLIPSMM_INCLUDE_DIRS})
    target_compile_options(refbox-mps-placing-clips PUBLIC ${CLIPSMM_CFLAGS})
    target_link_libraries(refbox-mps-placing-clips ${CLIPSMM_LDFLAGS})
    install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mps-placing-clips FILES_MATCHING PATTERN "*.h")
    install(TARGETS refbox-mps-placing-clips
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    message(WARNING "Cannot build mps_placing_clips library (one or more dependencies not found)")
endif()
