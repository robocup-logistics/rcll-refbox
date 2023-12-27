find_package(PkgConfig REQUIRED)
pkg_search_module(LIBMHD REQUIRED libmicrohttpd)

include_directories(${LIBMHD_INCLUDE_DIRS})
add_library(refbox-webview SHARED
    page_footer_generator.cpp
    nav_manager.cpp
    request_manager.cpp
    file_reply.cpp
    rest_api_manager.cpp
    error_reply.cpp
    user_verifier.cpp
    request.cpp
    reply.cpp
    access_log.cpp
    url_manager.cpp
    server.cpp
    request_dispatcher.cpp
    page_header_generator.cpp
    rest_api.cpp
    redirect_reply.cpp
    page_reply.cpp)

target_link_libraries(refbox-webview stdc++ refbox-core refbox-utils
    refbox-logging ${LIBMHD_LIBRARIES})
