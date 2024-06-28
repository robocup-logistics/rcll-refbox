find_package(Boost REQUIRED COMPONENTS system thread)
find_package(bsoncxx REQUIRED)
find_package(mongocxx REQUIRED)

add_library(refbox-mongodb-log SHARED mongodb_log_logger.cpp mongodb_log_protobuf.cpp)
target_link_libraries(refbox-mongodb-log refbox-logging refbox-core ${Boost_LIBRARIES})
target_link_libraries(refbox-mongodb-log mongo::mongocxx_shared)
target_link_libraries(refbox-mongodb-log mongo::bsoncxx_shared)

install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mongodb-log FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-mongodb-log
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
