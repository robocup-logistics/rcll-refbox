include_directories(${CLIPSMM_INCLUDE_DIRS})

add_library(refbox-config SHARED config.cpp yaml.cpp)

target_link_libraries(refbox-config refbox-utils refbox-core)
