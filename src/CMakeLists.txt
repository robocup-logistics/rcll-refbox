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

include_directories(msgs)
add_subdirectory(msgs)
include_directories(libs)
add_subdirectory(libs)
include_directories(refbox)
add_subdirectory(refbox)
include_directories(tools)
add_subdirectory(tools)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/games
    DESTINATION ${SHAREDIR}
)
