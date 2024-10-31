include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)

find_package(Boost 1.81 REQUIRED)

add_library(refbox-logging SHARED
    cache.cpp
    logger.cpp
    websocket.cpp
    console.cpp
    multi.cpp
    multi.h
    file.cpp)

target_link_libraries(refbox-logging
 refbox-websocket
  ${Boost_LIBRARIES} refbox-core)
target_include_directories(refbox-logging PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(refbox-logging PRIVATE ${Boost_INCLUDE_DIRS})

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-logging FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-logging
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
