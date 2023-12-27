find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(FreeOpcUa REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(/home/timwendt/rcll-refbox/build/src/)
include_directories(${SPDLOG_INCLUDE_DIR})

add_definitions(-DHAVE_MOCKUP)
add_definitions(-DHAVE_FREEOPCUA)
find_package(Boost REQUIRED COMPONENTS system program_options filesystem thread)
add_library(mps_comm SHARED
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
    opcua/ring_station.cpp
    opcua/cap_station.cpp
    opcua/opc_utils.cpp
    opcua/machine.cpp
    opcua/delivery_station.cpp
    opcua/base_station.cpp
    opcua/storage_station.cpp
    machine_factory.cpp
    opcua/ring_station.h
    opcua/cap_station.h
    opcua/opc_utils.h
    opcua/machine.h
    opcua/delivery_station.h
    opcua/base_station.h
    opcua/storage_station.h
    machine_factory.h)
target_compile_options(mps_comm PRIVATE -fPIC)
target_link_libraries(mps_comm PRIVATE stdc++ m
   pthread msgs Boost::system Boost::program_options Boost::thread
  ${Protobuf_LIBRARIES} fmt::fmt
  spdlog::spdlog opcuaclient opcuacore opcuaprotocol)
