#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED
#
# Usage: ./multi_process.sh <program> <multi_bw> <check_error> <loop_num>
# Launch multiple <program> per CMG in parallel and wait them finish.
# If <multi_bw> is 1, launch num_bw process per CMG. otherwise 1 process per CMG.
# If <check_error> is 1, check each process's return value.

SYSFS_PATH=/sys/class/misc/fujitsu_hwb/hwinfo
LOOP=$4

# check specified program exists
if [ ! -f $1 ]; then
	echo file not found: $1
	exit 1
fi

# read sysfs file to get number of cmg/bb/bw
hwinfo=$(cat $SYSFS_PATH)
# convert to array
hwinfo=($hwinfo)

num_cmg=${hwinfo[0]}
num_bb=${hwinfo[1]}
num_bw=${hwinfo[2]}

echo hwinfo: ${num_cmg} ${num_bb} ${num_bw}

if [ $2 -eq 1 ]; then
	num_pr=${num_bw}
else
	num_pr=1
fi

echo launch ${num_pr} process per CMG
for ((cmg=0; cmg<${num_cmg}; cmg++)) do
	for ((i=0; i<${num_pr}; i++)) do
		$1 ${cmg} ${LOOP} &
		PIDS+=($!)
	done
done

# wait each process
for pid in ${PIDS[@]}; do
	wait ${pid}
	STATUS+=($?)
done

echo pids: ${PIDS[@]}
echo status: ${STATUS[@]}
# check each return code
if [ $3 -eq 1 ]; then
	for status in ${STATUS[@]}; do
		if [[ ${status} -ne 0 ]]; then
			echo "some tests failed"
			exit ${status}
		fi
	done
fi

# in the end, sysfs should be clean regardless program return value
./check_sysfs_status
