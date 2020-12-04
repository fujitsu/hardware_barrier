# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED

add_executable(list_all_pe_info list_all_pe_info.c)
target_link_libraries(list_all_pe_info ${HWBLIB})

add_executable(sync_1cmg sync_1cmg.c)
target_link_libraries(sync_1cmg ${HWBLIB} pthread)
