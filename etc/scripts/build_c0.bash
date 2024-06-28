#! /bin/bash
# Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

#
# build_c0.bash fakes a series of agent tasks to build a C0
#
# Copyright (C) 2022 Matteo Tschesche <matteo.tschesche@rwth-aachen.de>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  Read the full text in the LICENSE.GPL file in the doc directory.

# This script is used to simulate an agent building a C0 by sending Protobuf messages (Beacon Signals) including
# Agent Task messages, which inform the Refbox about the currently executing tasks by the simulated agent, and
# prepare-machine actions. It uses the fake-robot script for sending the Protobuf messages. The script can be
# found in /src/tools.

# The input parameters are the following:
#   <name> <team> <robot-nr> <order-id> <workpiece-colors> <task-type> <task-id> where
#   workpiece-colors are specified as <base-color> <ring1-color> <ring2-color> <ring3-color> <cap-color>
# parameter are specific for the agent-task type:
#   Move:  <waypoint> <machine_point>
#   Retrieve:  <machine_id> <machine_point>
#   Deliver:  <waypoint> <machine_point>
#   BufferStation:  <machine_id> <shelf_number>
#   ExploreWaypoint:  <waypoint> <machine_id> <machine_point>

# This script may help to understand how to build a C0, which messages to send to do so, and when to send Agent Task
# messages.


#prepare cap
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 1 C-CS1 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE CAP_GREY BufferStation 2 C-CS1 1 & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 RETRIEVE_CAP & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 3 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Retrieve 4 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Move 5 C_Z18 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Deliver 6 C_Z18 INPUT & sleep 10 ; kill $!

#get base
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 7 C-BS INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-BS INPUT BASE_BLACK & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Retrieve 8 C-BS INPUT & sleep 10 ; kill $!

#mount cap
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Move 9 C-CS1 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Deliver 10 C-CS1 INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 MOUNT_CAP & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 11 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE CAP_GREY Retrieve 12 C-CS1 OUTPUT & sleep 10 ; kill $!

#deliver
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE CAP_GREY Move 13 C-DS INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE CAP_GREY Deliver 14 C-DS INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-DS 3 & sleep 10 ; kill $!
