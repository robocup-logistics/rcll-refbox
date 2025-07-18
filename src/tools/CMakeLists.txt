find_package(Boost REQUIRED COMPONENTS system system thread)
add_executable(rcll-show-peers llsf-show-peers.cpp)
find_package(ProtobufComm REQUIRED)
set(TOOL_DEPS refbox-core refbox-utils refbox-config rcll-protobuf-msgs ProtobufComm::protobuf_comm)
target_link_libraries(rcll-show-peers PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-show-peers
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-fake-robot llsf-fake-robot.cpp)
target_link_libraries(rcll-fake-robot PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-fake-robot
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-report-machine llsf-report-machine.cpp)
target_link_libraries(rcll-report-machine PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-report-machine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-prepare-machine rcll-prepare-machine.cpp)
target_link_libraries(rcll-prepare-machine PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-prepare-machine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-reset-machine rcll-reset-machine.cpp)
target_link_libraries(rcll-reset-machine PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-reset-machine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-set-machine-state rcll-set-machine-state.cpp)
target_link_libraries(rcll-set-machine-state PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-set-machine-state
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-machine-add-base rcll-machine-add-base.cpp)
target_link_libraries(rcll-machine-add-base PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-machine-add-base
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-set-machine-lights rcll-set-machine-lights.cpp)
target_link_libraries(rcll-set-machine-lights PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-set-machine-lights
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-refbox-instruct rcll-refbox-instruct.cpp)
target_link_libraries(rcll-refbox-instruct PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-refbox-instruct
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-workpiece rcll-workpiece.cpp)
target_link_libraries(rcll-workpiece PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-workpiece
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-proto-rebroadcaster rcll-proto-rebroadcaster.cpp)
target_link_libraries(rcll-proto-rebroadcaster PRIVATE refbox-core refbox-utils refbox-config refbox-protobuf-rebroadcaster ProtobufComm::protobuf_comm)
install(TARGETS rcll-proto-rebroadcaster
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rcll-agent-task-client rcll-agent-task-client.cpp)
target_link_libraries(rcll-agent-task-client PRIVATE ${TOOL_DEPS})
install(TARGETS rcll-agent-task-client
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
