find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(FreeOpcUa REQUIRED)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(/home/timwendt/rcll-refbox/build/src/)
include_directories(${SPDLOG_INCLUDE_DIR})

find_package(Boost REQUIRED COMPONENTS system program_options filesystem thread)
add_library(mps_comm SHARED
    mockup/ring_station.cpp
    mockup/cap_station.cpp
    mockup/machine.cpp
    mockup/delivery_station.cpp
    mockup/base_station.cpp
    mockup/storage_station.cpp
    time_utils.cpp
    opcua/ring_station.cpp
    opcua/cap_station.cpp
    opcua/opc_utils.cpp
    opcua/machine.cpp
    opcua/delivery_station.cpp
    opcua/base_station.cpp
    opcua/storage_station.cpp
    machine_factory.cpp)
target_compile_options(mps_comm PRIVATE -fPIC)
target_link_libraries(mps_comm PRIVATE stdc++ m refbox-core
  refbox-core pthread msgs Boost::system Boost::program_options Boost::thread
  ${Protobuf_LIBRARIES} fmt::fmt
  spdlog::spdlog opcuaclient opcuacore opcuaprotocol)
