# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED

cmake_minimum_required(VERSION 3.11)
enable_testing()
include(GNUInstallDirs)

set(HWBLIB_VERSION_MAJOR 0)
set(HWBLIB_VERSION_MINOR 1)
set(HWBLIB_VERSION_PATCH 0)
set(HWBLIB_VERSION_STRING ${HWBLIB_VERSION_MAJOR}.${HWBLIB_VERSION_MINOR}.${HWBLIB_VERSION_PATCH})

project(libFJhwb
	VERSION ${HWBLIB_VERSION_STRING}
	DESCRIPTION "FUJITSU HARDWARE BARRIER LIBRARY"
	LANGUAGES C)

option(BUILD_STATIC_LIB "build static library" OFF)
option(BUILD_STATIC "build tests/examples with static library" OFF)
option(BUILD_TESTS "build tests" ON)
option(BUILD_EXAMPLES "build examples" ON)

set(CMAKE_C_FLAGS "-Wall -Wextra -g -O2")
set(HWBLIB "FJhwb")

add_subdirectory(src)

if (BUILD_STATIC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
	set(HWBLIB "FJhwb-static")
endif()
if (BUILD_TESTS)
	add_subdirectory(tests)
endif()
if (BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.h")
