#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-only
# Copyright 2020 FUJITSU LIMITED
#
# Usage: ./check_cleanup.sh <program> <args>
# Run <program> and then check barrier's sysfs status

$@

error=$?
if [[ $error -ne 0 ]]; then
	echo exit status of \"$@\": $error
	exit $error
fi

# in the end, sysfs should be clean regardless program performing cleanup or not
./check_sysfs_status
