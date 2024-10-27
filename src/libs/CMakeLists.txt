# *****************************************************************************
# CMake Build System for rcll-refbox
# -------------------
# Copyright (C) 2023 by Tim Wendt
#
# *****************************************************************************
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#
# *****************************************************************************

include_directories(config)
add_subdirectory(config)
include_directories(core)
add_subdirectory(core)
include_directories(logging)
add_subdirectory(logging)
include_directories(mongodb_log)
add_subdirectory(mongodb_log)
include_directories(mps_comm)
add_subdirectory(mps_comm)
include_directories(mps_placing_clips)
add_subdirectory(mps_placing_clips)
include_directories(netcomm)
add_subdirectory(netcomm)
include_directories(protobuf_clips)
add_subdirectory(protobuf_clips)
include_directories(protobuf_rebroadcaster)
add_subdirectory(protobuf_rebroadcaster)
include_directories(utils)
add_subdirectory(utils)
include_directories(websocket)
add_subdirectory(websocket)
