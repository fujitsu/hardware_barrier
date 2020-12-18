# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED

# support source
add_executable(check_sysfs_status check_sysfs_status.c util.c)
target_link_libraries(check_sysfs_status ${HWBLIB})

add_executable(test_util test_util.c util.c)
target_link_libraries(test_util ${HWBLIB})

# test source
## for basic function test
add_executable(test_init_fini test_init_fini.c util.c)
target_link_libraries(test_init_fini ${HWBLIB})
add_executable(test_assign_unassign test_assign_unassign.c util.c)
target_link_libraries(test_assign_unassign ${HWBLIB} pthread)

add_executable(test_get_pe_info test_get_pe_info.c util.c)
target_link_libraries(test_get_pe_info ${HWBLIB})
add_executable(test_get_all_pe_info test_get_all_pe_info.c util.c)
target_link_libraries(test_get_all_pe_info ${HWBLIB})

## for error case test
add_executable(test_call_init_num_bb_times test_call_init_num_bb_times.c util.c)
target_link_libraries(test_call_init_num_bb_times ${HWBLIB})
add_executable(test_init_fini_error test_init_fini_error.c util.c)
target_link_libraries(test_init_fini_error ${HWBLIB})

add_executable(test_call_assign_num_bw_times test_call_assign_num_bw_times.c util.c)
target_link_libraries(test_call_assign_num_bw_times ${HWBLIB})
add_executable(test_assign_unassign_error test_assign_unassign_error.c util.c)
target_link_libraries(test_assign_unassign_error ${HWBLIB})

add_executable(test_get_pe_info_error test_get_pe_info_error.c util.c)
target_link_libraries(test_get_pe_info_error ${HWBLIB})
add_executable(test_get_all_pe_info_error test_get_all_pe_info_error.c util.c)
target_link_libraries(test_get_all_pe_info_error ${HWBLIB})

add_executable(test_illegal_sync test_illegal_sync.c util.c)
target_link_libraries(test_illegal_sync ${HWBLIB})
add_executable(test_bb_ownership test_bb_ownership.c util.c)
target_link_libraries(test_bb_ownership ${HWBLIB})

## for cleanup test
add_executable(test_exit_program_without_cleanup test_exit_program_without_cleanup.c util.c)
target_link_libraries(test_exit_program_without_cleanup ${HWBLIB} pthread)

## for stress test
add_executable(test_sync_1cmg test_sync_1cmg.c util.c)
target_link_libraries(test_sync_1cmg ${HWBLIB} pthread)
add_executable(test_sync_1cmg_error test_sync_1cmg_error.c util.c)
target_link_libraries(test_sync_1cmg_error ${HWBLIB} pthread)
add_executable(test_sync_all_bb_all_bw test_sync_all_bb_all_bw.c util.c)
target_link_libraries(test_sync_all_bb_all_bw ${HWBLIB} pthread)

# test definitions
## unittests for util functions
add_test(NAME util_test COMMAND $<TARGET_FILE:test_util>)

## basic function test
add_test(NAME init/fini COMMAND $<TARGET_FILE:test_init_fini>)
add_test(NAME assign/unassign COMMAND $<TARGET_FILE:test_assign_unassign>)
add_test(NAME get_pe_info COMMAND $<TARGET_FILE:test_get_pe_info>)
add_test(NAME get_all_pe_info COMMAND $<TARGET_FILE:test_get_all_pe_info>)

## error case test
add_test(NAME init_fini_error COMMAND $<TARGET_FILE:test_init_fini_error>)
add_test(NAME call_init_num_bb_times COMMAND $<TARGET_FILE:test_call_init_num_bb_times>)
add_test(NAME assign_unassign_error COMMAND $<TARGET_FILE:test_assign_unassign_error>)
add_test(NAME call_assign_num_bw_times COMMAND $<TARGET_FILE:test_call_assign_num_bw_times>)
add_test(NAME get_pe_info_error COMMAND $<TARGET_FILE:test_get_pe_info_error>)
add_test(NAME get_all_pe_info_error COMMAND $<TARGET_FILE:test_get_all_pe_info_error>)
add_test(NAME illegal_sync COMMAND $<TARGET_FILE:test_illegal_sync>)
add_test(NAME bb_ownership COMMAND $<TARGET_FILE:test_bb_ownership>)

## cleanup test
find_program (BASH bash)
# check all resources will be freed even when program itself does not call unassign before fini
add_test(NAME check_cleanup1 COMMAND ./test_exit_program_without_cleanup 1)
# check all resources will be freed even when program itself does not call neither unassign/fini
add_test(NAME check_cleanup2
	COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/check_cleanup.sh ./test_exit_program_without_cleanup 0)

## stress test (loop assign - sync - unassign in each thread)
# run 1 sync process per CMG in parallel (which uses 1 bb for all PEs in a CMG)
add_test(NAME stress_test1
	COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/multi_process.sh ./test_sync_1cmg 0 1 10000)
# run num_bw sync process per CMG in parallel (which uses 1 bb for all PEs in a CMG)
add_test(NAME stress_test2
	COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/multi_process.sh ./test_sync_1cmg 1 1 300)

# run 1 sync process per CMG in parallel (which uses all bb and all bw in a CMG)
add_test(NAME stress_test3
	COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/multi_process.sh ./test_sync_all_bb_all_bw 0 1 300)

# run num_bw sync process per CMG in parallel which abort operation on the way,
# then check barrier resources will be cleaned up correctly
add_test(NAME stress_test_error_case
	COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/multi_process.sh ./test_sync_1cmg_error 1 0 300)
