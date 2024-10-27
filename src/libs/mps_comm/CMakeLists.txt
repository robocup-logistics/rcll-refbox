find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(FreeOpcUa)
find_package(PahoMqttCpp)
find_package(Boost REQUIRED COMPONENTS system program_options filesystem thread)

add_library(refbox-mps-comm SHARED
  machine_factory.cpp
  mockup/ring_station.cpp
  mockup/cap_station.cpp
  mockup/machine.cpp
  mockup/delivery_station.cpp
  mockup/base_station.cpp
  mockup/storage_station.cpp
  mockup/ring_station.h
  mockup/cap_station.h
  mockup/machine.h
  mockup/delivery_station.h
  mockup/base_station.h
  mockup/storage_station.h
  time_utils.cpp
  machine_factory.h
)

if(PahoMqttCpp_FOUND)
  target_compile_options(refbox-mps-comm PUBLIC -DHAVE_MQTT)
  target_sources(refbox-mps-comm PRIVATE
    mqtt/machine.cpp
    mqtt/base_station.cpp
    mqtt/mqtt_callback.cpp
    mqtt/mqtt_action_listener.cpp
    mqtt/mqtt_client_wrapper.cpp
    mqtt/cap_station.cpp
    mqtt/delivery_station.cpp
    mqtt/ring_station.cpp
    mqtt/storage_station.cpp
  )
  target_sources(refbox-mps-comm PRIVATE
    mqtt_legacy/machine.cpp
    mqtt_legacy/base_station.cpp
    mqtt_legacy/mqtt_utils.cpp
    mqtt_legacy/mqtt_callback.cpp
    mqtt_legacy/mqtt_action_listener.cpp
    mqtt_legacy/mqtt_client_wrapper.cpp
    mqtt_legacy/cap_station.cpp
    mqtt_legacy/delivery_station.cpp
    mqtt_legacy/ring_station.cpp
    mqtt_legacy/storage_station.cpp
  )
  target_link_libraries(refbox-mps-comm PRIVATE ${PAHO_MQTT_C_LIBRARIES} ${PAHO_MQTT_CPP_LIBRARIES}  paho-mqttpp3 paho-mqtt3as)
else()
  message(WARNING "Build without MQTT support (paho-cpp(-devel) missing")
endif()
if(FreeOpcUa_FOUND)
  target_compile_options(refbox-mps-comm PUBLIC -DHAVE_FREEOPCUA)
  target_sources(refbox-mps-comm PRIVATE
    opcua/ring_station.cpp
    opcua/cap_station.cpp
    opcua/opc_utils.cpp
    opcua/machine.cpp
    opcua/delivery_station.cpp
    opcua/base_station.cpp
    opcua/storage_station.cpp
    opcua/ring_station.h
    opcua/cap_station.h
    opcua/opc_utils.h
    opcua/machine.h
    opcua/delivery_station.h
    opcua/base_station.h
    opcua/storage_station.h
  )
  target_link_libraries(refbox-mps-comm PRIVATE opcuaclient opcuacore opcuaprotocol)
else()
  message(WARNING "Build without OPCUA support (freeopcua(-devel) missing")
endif()
target_compile_options(refbox-mps-comm PUBLIC -DHAVE_MOCKUP)
# target_include_directories(${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(refbox-mps-comm PUBLIC ${SPDLOG_INCLUDE_DIR})
# target_compile_options(refbox-mps-comm PRIVATE -fPIC)
target_link_libraries(refbox-mps-comm PRIVATE stdc++ m
   pthread Boost::system Boost::program_options Boost::thread
  ${Protobuf_LIBRARIES} fmt::fmt rcll-protobuf-msgs
  spdlog::spdlog)

install(TARGETS refbox-mps-comm
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mps-comm FILES_MATCHING PATTERN "*.h")

install(DIRECTORY mockup/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mps-comm/mockup FILES_MATCHING PATTERN "*.h")
if(FreeOpcUa_FOUND)
  install(DIRECTORY opcua/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mps-comm/opcua FILES_MATCHING PATTERN "*.h")
endif()
if(PahoMqttCpp_FOUND)
  install(DIRECTORY mqtt/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/refbox-mps-comm/mqtt FILES_MATCHING PATTERN "*.h")
endif()
