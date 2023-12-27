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

cmake_minimum_required(VERSION 3.21)
project(rcll-refbox)

# Project name and version
project(rcll-refbox VERSION 1.0 DESCRIPTION "rcll-refbox" LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DBASEDIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DBINDIR="${PROJECT_SOURCE_DIR}/bin")
add_definitions(-DLIBDIR="${CMAKE_SOURCE_DIR}/lib")
add_definitions(-DPLUGINDIR="${CMAKE_SOURCE_DIR}/plugins")
add_definitions(-DIFACEDIR="${CMAKE_SOURCE_DIR}/lib/interfaces")
add_definitions(-DCONFDIR="${CMAKE_SOURCE_DIR}/cfg")
add_definitions(-DUSERDIR=".fawkes")
add_definitions(-DLOGDIR="${CMAKE_SOURCE_DIR}/log")
add_definitions(-DRESDIR="${CMAKE_SOURCE_DIR}/res")
add_definitions(-DTMPDIR="/tmp")
add_definitions(-DSRCDIR="${CMAKE_SOURCE_DIR}/src/libs/websocket")
add_definitions(-DSHAREDIR="${CMAKE_SOURCE_DIR}/src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(src)
