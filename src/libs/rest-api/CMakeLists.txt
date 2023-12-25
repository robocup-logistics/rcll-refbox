find_package(Boost REQUIRED COMPONENTS filesystem) # Adjust with the required boost libraries
find_package(PkgConfig REQUIRED)

pkg_search_module(CLIPSMM REQUIRED clipsmm-1.0)
include_directories(${CLIPSMM_INCLUDE_DIRS})

pkg_check_modules(APRUTIL REQUIRED apr-util-1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CLIPSMM_INCLUDE_DIRS})

add_library(refbox-rest-api SHARED
    webview_server.cpp
    service_browse_handler.cpp
    rest_processor.cpp
    clips-rest-api/clips-rest-api.cpp
    clips-rest-api/model/RingSpec.cpp
    clips-rest-api/model/Machine.cpp
    clips-rest-api/model/GameState.cpp
    clips-rest-api/model/Environment.cpp
    clips-rest-api/model/Robot.cpp
    clips-rest-api/model/Order.cpp
    clips-rest-api/model/Points.cpp
    clips-rest-api/model/SlotValue.cpp
    clips-rest-api/model/Fact.cpp
)

target_link_libraries(refbox-rest-api
  ${APRUTIL_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CLIPSMM_LIBRARIES}
)

