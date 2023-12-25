# TODO all this packages seame to be dead so just delete them
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

include_directories(${BASEDIR}/src/libs)


# Create executables only if Avahi is found
if(AVAHI_FOUND)
    # add_library(qa_netcomm_avahi_publisher qa_avahi_publisher.cpp)
    # target_link_libraries(qa_netcomm_avahi_publisher refbox-netcomm refbox-util ${Avahi_LIBRARIES})

    # add_library(qa_netcomm_avahi_browser qa_avahi_browser.cpp)
    # target_link_libraries(qa_netcomm_avahi_browser refbox-netcomm refbox-util ${Avahi_LIBRARIES})

    # add_library(qa_netcomm_avahi_resolver qa_avahi_resolver.cpp)
    # target_link_libraries(qa_netcomm_avahi_resolver refbox-netcomm refbox-util ${Avahi_LIBRARIES})
endif()

add_library(qa_netcomm_socket_typeof qa_socket_typeof.cpp)
target_link_libraries(qa_netcomm_socket_typeof refbox-netcomm)

# add_library(qa_netcomm_worldinfo_msgsizes qa_worldinfo_msgsizes.cpp)
# target_link_libraries(qa_netcomm_worldinfo_msgsizes refbox-netcomm refbox-util)

# add_library(qa_netcomm_worldinfo qa_worldinfo.cpp)
# target_link_libraries(qa_netcomm_worldinfo refbox-netcomm refbox-util)

# add_library(qa_netcomm_worldinfo_encryption qa_worldinfo_encryption.cpp)
# target_link_libraries(qa_netcomm_worldinfo_encryption refbox-netcomm refbox-util)

# # add_library(qa_netcomm_socket_datagram_multicast qa_socket_datagram_multicast.cpp)
# # target_link_libraries(qa_netcomm_socket_datagram_multicast refbox-netcomm)

# # add_library(qa_netcomm_dynamic_buffer qa_dynamic_buffer.cpp)
# # target_link_libraries(qa_netcomm_dynamic_buffer refbox-netcomm)

# # add_library(qa_netcomm_socket_datagram qa_socket_datagram.cpp)
# # target_link_libraries(qa_netcomm_socket_datagram refbox-netcomm)

# # add_library(qa_netcomm_resolver qa_resolver.cpp)
# # target_link_libraries(qa_netcomm_resolver refbox-netcomm)

# # add_library(qa_netcomm_socket_datagram_broadcast qa_socket_datagram_broadcast.cpp)
# # target_link_libraries(qa_netcomm_socket_datagram_broadcast refbox-netcomm)

# # add_library(qa_netcomm_socket_stream qa_socket_stream.cpp)
# # target_link_libraries(qa_netcomm_socket_stream refbox-netcomm)
