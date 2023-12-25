find_package(PkgConfig REQUIRED)

# Attempt to find Avahi using pkg-config
if(USE_AVAHI)
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
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(refbox-netcomm
    utils/exceptions.cpp
    utils/incoming_connection_handler.cpp
    utils/acceptor_thread.cpp
    utils/resolver.cpp
    utils/resolver_thread.cpp
    service_discovery/service_publisher.cpp
    service_discovery/dummy_service_browser.cpp
    service_discovery/service.cpp
    service_discovery/service_browser.cpp
    service_discovery/dummy_service_publisher.cpp
    dns-sd/avahi_resolver_handler.cpp
    dns-sd/avahi_thread.cpp
    socket/stream.cpp
    socket/datagram_multicast.cpp
    socket/datagram_broadcast.cpp
    socket/socket.cpp
    socket/datagram.cpp)
target_link_libraries(refbox-netcomm stdc++ m refbox-core refbox-utils)

if(USE_AVAHI)
  target_link_libraries(refbox-netcomm ${AVAHI_LIBRARIES})
endif()
install(TARGETS refbox-netcomm LIBRARY DESTINATION ${LIBDIR})

include_directories(qa)
add_subdirectory(qa)
