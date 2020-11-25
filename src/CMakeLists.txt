# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED

add_library(FJhwb SHARED hwblib.c)
target_link_libraries(FJhwb pthread)

set_target_properties(FJhwb PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(FJhwb PROPERTIES SOVERSION ${HWBLIB_VERSION_MAJOR})

target_include_directories(FJhwb PUBLIC ${PROJECT_SOURCE_DIR}/include)

install(TARGETS FJhwb
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
