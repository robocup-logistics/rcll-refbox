find_package(Boost REQUIRED COMPONENTS system thread)
find_package(bsoncxx REQUIRED)
find_package(mongocxx REQUIRED)

add_library(mongodb-log mongodb_log_logger.cpp mongodb_log_protobuf.cpp)
target_link_libraries(mongodb-log refbox-logging refbox-core ${Boost_LIBRARIES})
target_link_libraries(mongodb-log mongo::mongocxx_shared)
target_link_libraries(mongodb-log mongo::bsoncxx_shared)
