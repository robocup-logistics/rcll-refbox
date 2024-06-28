find_package(Protobuf REQUIRED)
set(PROTO_FILES
    rcll-protobuf-msgs/AgentTask.proto
    rcll-protobuf-msgs/AttentionMessage.proto
    rcll-protobuf-msgs/BeaconSignal.proto
    rcll-protobuf-msgs/ExplorationInfo.proto
    rcll-protobuf-msgs/GameInfo.proto
    rcll-protobuf-msgs/GameState.proto
    rcll-protobuf-msgs/MachineCommands.proto
    rcll-protobuf-msgs/MachineDescription.proto
    rcll-protobuf-msgs/MachineInfo.proto
    rcll-protobuf-msgs/MachineInstructions.proto
    rcll-protobuf-msgs/MachineReport.proto
    rcll-protobuf-msgs/NavigationChallenge.proto
    rcll-protobuf-msgs/OrderInfo.proto
    rcll-protobuf-msgs/Pose2D.proto
    rcll-protobuf-msgs/ProductColor.proto
    rcll-protobuf-msgs/RingInfo.proto
    rcll-protobuf-msgs/RobotCommands.proto
    rcll-protobuf-msgs/RobotInfo.proto
    rcll-protobuf-msgs/SimTimeSync.proto
    rcll-protobuf-msgs/Team.proto
    rcll-protobuf-msgs/Time.proto
    rcll-protobuf-msgs/VersionInfo.proto
    rcll-protobuf-msgs/WorkpieceInfo.proto
    rcll-protobuf-msgs/Zone.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
add_library(rcll-protobuf-msgs SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(rcll-protobuf-msgs ${Protobuf_LIBRARIES})
target_include_directories(rcll-protobuf-msgs
    PUBLIC
    ${CMAKE_BINARY_DIR}/src/
)
install(FILES ${PROTO_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rcll-protobuf-msgs)
install(TARGETS rcll-protobuf-msgs
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY .
    DESTINATION ${SHAREDIR}/msgs/
    FILES_MATCHING PATTERN "*.proto"
)
