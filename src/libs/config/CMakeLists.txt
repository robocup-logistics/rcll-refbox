include_directories(${CLIPSMM_INCLUDE_DIRS})
find_package(yaml-cpp REQUIRED)
add_library(refbox-config SHARED config.cpp yaml.cpp)

target_link_libraries(refbox-config refbox-utils refbox-core yaml-cpp)
