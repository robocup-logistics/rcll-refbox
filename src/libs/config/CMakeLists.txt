find_package(yaml-cpp REQUIRED)
add_library(refbox-config SHARED config.cpp yaml.cpp)

target_link_libraries(refbox-config refbox-utils refbox-core yaml-cpp)

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-config FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-config
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
