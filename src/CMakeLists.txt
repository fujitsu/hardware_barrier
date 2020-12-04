# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED

add_library(${HWBLIB} SHARED hwblib.c)
target_link_libraries(${HWBLIB} pthread)

set_target_properties(${HWBLIB} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${HWBLIB} PROPERTIES SOVERSION ${HWBLIB_VERSION_MAJOR})

target_include_directories(${HWBLIB} PUBLIC ${PROJECT_SOURCE_DIR}/include)

install(TARGETS ${HWBLIB}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (BUILD_STATIC_LIB)
	add_library(${HWBLIB}-static STATIC hwblib.c)
	target_link_libraries(${HWBLIB}-static pthread)

	target_include_directories(${HWBLIB}-static PUBLIC ${PROJECT_SOURCE_DIR}/include)
	install(TARGETS ${HWBLIB}-static
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
