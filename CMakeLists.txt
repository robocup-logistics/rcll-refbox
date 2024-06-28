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
include(GNUInstallDirs)
set(CONFDIR ${CMAKE_SOURCE_DIR}/cfg CACHE STRING "destination of configuration files")
set(SHAREDIR ${CMAKE_SOURCE_DIR}/src CACHE STRING "destination of assets needed during runtime")
set(BINDIR ${CMAKE_SOURCE_DIR}/bin CACHE STRING "destination of binaries")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib64")
# Function to check and update submodules
function(update_submodules)
  # Read the .gitmodules file
  file(READ "${CMAKE_SOURCE_DIR}/.gitmodules" gitmodules_content)

  # Initialize an empty list to store submodule paths
  set(submodule_paths "")

  # Use a regex to extract the paths of the submodules
  string(REGEX MATCHALL "path = ([^\n]+)" matches "${gitmodules_content}")

  # Iterate over the matches to extract the submodule paths
  foreach(match ${matches})
    string(REGEX REPLACE "path = " "" submodule_path "${match}")
    list(APPEND submodule_paths "${CMAKE_SOURCE_DIR}/${submodule_path}")
  endforeach()

  # Display the submodule paths for debugging purposes
  message(STATUS "Submodule paths: ${submodule_paths}")

  # Update each submodule
  foreach(submodule_path ${submodule_paths})
    if( NOT EXISTS "${submodule_path}/.git" )
      execute_process(
        COMMAND git submodule update --init --recursive ${submodule_path}
      )

      if (result)
        message(FATAL_ERROR "Error at submodule ${submodule_path}: ${error}")
      else()
        message(STATUS "Submodule ${submodule_path} updated successfully")
      endif()
    endif()
  endforeach()
endfunction()

update_submodules()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DBASEDIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DBINDIR="${BINDIR}")
add_definitions(-DLIBDIR="${CMAKE_SOURCE_DIR}/lib")
add_definitions(-DPLUGINDIR="${CMAKE_SOURCE_DIR}/plugins")
add_definitions(-DIFACEDIR="${CMAKE_SOURCE_DIR}/lib/interfaces")
add_compile_definitions(CONFDIR="${CONFDIR}")
add_compile_definitions(SHAREDIR="${SHAREDIR}")
add_definitions(-DUSERDIR=".fawkes")
add_definitions(-DLOGDIR="${CMAKE_SOURCE_DIR}/log")
add_definitions(-DRESDIR="${CMAKE_SOURCE_DIR}/res")
add_definitions(-DTMPDIR="/tmp")
add_definitions(-DSRCDIR="${CMAKE_SOURCE_DIR}/src/libs/websocket")

function(create_symlink source destination)
  get_filename_component(target ${source} NAME)
  create_symlink_custom_target(${target} ${source} ${destination}/${target})
install(FILES ${target} DESTINATION ${CMAKE_INSTALL_BINDIR}
)
endfunction()
# creates a symlink for a target
function(create_symlink_custom_target target source destination)
  add_custom_target(
    ${target} ALL
    COMMAND ${CMAKE_COMMAND} -E true
    COMMENT "Target ${target} for symlink")
  get_filename_component(source_name ${source} NAME)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_CURRENT_SOURCE_DIR}/${source} ${destination}
    COMMENT
      "${BOLD_BLUE}Created symlink: ${CMAKE_CURRENT_SOURCE_DIR}/${target} \
-> ${destination}${COLOR_RESET}"
  )
endfunction()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(src)
add_subdirectory(etc)
install(DIRECTORY cfg/ DESTINATION ${CONFDIR})
