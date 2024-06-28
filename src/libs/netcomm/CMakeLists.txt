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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(refbox-netcomm SHARED
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
    socket/datagram.cpp
    utils/acceptor_thread.h
    utils/resolver.h
    utils/ntoh64.h
    utils/resolver_thread.h
    utils/exceptions.h
    utils/incoming_connection_handler.h
    service_discovery/service_browser.h
    service_discovery/service.h
    service_discovery/service_publisher.h
    service_discovery/browse_handler.h
    service_discovery/dummy_service_browser.h
    service_discovery/dummy_service_publisher.h
    dns-sd/avahi_resolver_handler.h
    dns-sd/avahi_thread.h
    socket/datagram.h
    socket/socket.h
    socket/datagram_broadcast.h
    socket/stream.h
    socket/datagram_multicast.h)
target_link_libraries(refbox-netcomm stdc++ m refbox-core refbox-utils)

if(USE_AVAHI)
  target_link_libraries(refbox-netcomm ${AVAHI_LIBRARIES})
endif()
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-netcomm FILES_MATCHING PATTERN "*.h")
install(TARGETS refbox-netcomm
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include_directories(qa)
add_subdirectory(qa)
