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

set(CMAKE_C_FLAGS "-Wall -Wextra -g -O2")

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)

install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.h")
