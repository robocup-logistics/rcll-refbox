include(FindPkgConfig)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)

find_package(Boost 1.81.0 REQUIRED)
find_package(RapidJSON REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(refbox-websocket SHARED data.cpp client.cpp backend.cpp server.cpp data.h client.h backend.h server.h)
target_link_libraries(refbox-websocket stdc++ pthread refbox-core ${Boost_LIBRARIES})
target_compile_options(refbox-websocket PUBLIC -DHAVE_CLIPS ${CLIPSMM_CFLAGS})
target_link_libraries(refbox-websocket ${CLIPSMM_LDFLAGS})
target_include_directories(refbox-websocket PRIVATE ${Boost_INCLUDE_DIRS})
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-websocket FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-websocket
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/message_schemas
    DESTINATION ${SHAREDIR}/libs/websocket/
    FILES_MATCHING PATTERN "*.json"
)
