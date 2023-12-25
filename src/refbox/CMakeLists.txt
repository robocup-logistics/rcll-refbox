find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_library(GECODE_LIBRARIES NAMES
  gecodeint gecodekernel gecodesupport gecodesearch gecodedriver
  gecodeminimodel PATHS /usr/lib64)
find_package(ProtobufComm REQUIRED)
pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
pkg_search_module(LIBMHD REQUIRED libmicrohttpd)
# find_package(LibMicroHTTPD REQUIRED)

include_directories(${CLIPSMM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/)
include_directories(${GECODE_INCLUDE_DIRS})
# include_directories(${LIBMICROHTTPD_INCLUDE_DIR})
include_directories(${LIBMHD_INCLUDE_DIRS})

include_directories(/home/timwendt/rcll-refbox/build/src/)
include_directories(/home/timwendt/rcll-refbox/build/src/libs/logging/llsf_log_msgs)


pkg_search_module(AVAHI REQUIRED avahi-client)

if(AVAHI_FOUND)
    message(STATUS "Found Avahi: ${AVAHI_LIBRARIES}")
    add_definitions(${CFLAGS_AVAHI})
    include_directories(${AVAHI_INCLUDE_DIRS})
    link_directories(${AVAHI_LIBRARY_DIRS})
    set(LIBS_AVAHI ${AVAHI_LIBRARIES})
else()
    message(WARNING "Avahi not found: mDNS/DNS-SD support will be omitted")
    set(OMIT_OBJECTS "dns-sd/%")
endif()

add_executable(refbox clips_logger.cpp
    refbox.cpp)

target_link_libraries(refbox
  ${CLIPSMM_LIBRARIES}
  msgs
  stdc++
  protobuf_clips
  refbox-utils
  refbox-netcomm
  mps_comm
  refbox_mps_placing_clips
  refbox-webview
  mongodb-log
  protobuf_comm
gecodeminimodel gecodedriver gecodesearch gecodesupport gecodekernel gecodeint
  refbox-rest-api
  refbox-config
  refbox-logging)
target_link_libraries(refbox ${PROTOBUF_LIBRARIES} ${GECODE_LIBRARIES})
target_link_libraries(refbox ${PROTOBUFCOMM_LIBRARIES})

# target_link_libraries(refbox ${LIBMICROHTTPD_LIBRARIES})
target_link_libraries(refbox ${LIBMHD_LIBRARIES})

target_link_libraries(refbox ${AVAHI_LIBRARIES})
