/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Test to check fhwb_init succeeds num_bb times and fails at (num_bb + 1)
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct hwb_hwinfo hwinfo = {0};
	cpu_set_t set;
	int ret;
	int bd[128];
	int i;

	ret = get_hwb_hwinfo(&hwinfo);
	ASSERT_SUCCESS(ret);

	printf("test1: check fhwb_init fails at (num_bb + 1)\n");
	/* use CMG 0 */
	ret = fill_cpumask_for_cmg(0, &set);
	ASSERT_SUCCESS(ret);

	printf("CMG: 0, CPU_COUNT: %d\n", CPU_COUNT(&set));
	if (CPU_COUNT(&set) < 2) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}

	/* call fhwb_init num_bb times */
	for (i = 0; i < hwinfo.num_bb; i++) {
		ret = fhwb_init(sizeof(cpu_set_t), &set);
		ASSERT_VALID_BD(ret);
		bd[i] = ret;
	}

	/* expect failure */
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	ASSERT_FAIL(ret);

	/* cleanup */
	for (i = 0; i < hwinfo.num_bb; i++) {
		ret = fhwb_fini(bd[i]);
		ASSERT_SUCCESS(ret);
	}

	/* check if everything is clean */
	ret = check_sysfs_status();
	ASSERT_SUCCESS(ret);

	return 0;
}
