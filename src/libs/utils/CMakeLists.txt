add_library(refbox-utils SHARED
    llsf/machines.cpp
    time/time.cpp
    time/simts.cpp
    time/watch.cpp
    time/clock.cpp
    time/wait.cpp
    time/tracker.cpp
    system/filetype.cpp
    system/hostinfo.cpp
    system/argparser.cpp
    system/file.cpp
    misc/string_conversions.cpp
    misc/string_compare.cpp)
target_link_libraries(refbox-utils stdc++ m refbox-core)
if(UNIX AND NOT APPLE)
    target_link_libraries(refbox-utils rt)
endif()
if(LINUX)
    target_link_libraries(refbox-utils dl pthread)
endif()
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-utils FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-utils
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
