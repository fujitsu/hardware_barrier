/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Error route test for fhwb_init/fhwb_fini
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
	cpu_set_t set, temp1, temp2;
	int ret;
	int bd;

	ret = get_hwb_hwinfo(&hwinfo);
	if (ret)
		return -1;

	printf("test1: check fhwb_init fails with 0 bit set\n");
	CPU_ZERO(&set);
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (!ret) {
		fprintf(stderr, "fhwb_init succeeds unexpectedly\n");
		return -1;
	}

	printf("test2: check fhwb_init fails with ony 1 bit set\n");
	CPU_SET(0, &set);
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (!ret) {
		fprintf(stderr, "fhwb_init succeeds unexpectedly\n");
		return -1;
	}

	printf("test3: check fhwb_init fails when several CMG are used\n");
	ret = fill_cpumask_for_cmg(0, &temp1);
	if (ret)
		return -1;
	ret = fill_cpumask_for_cmg(1, &temp2);
	if (ret)
		return -1;

	printf("CMG: 0, CPU_COUNT: %d, CMG: 1, CPU_COUNT: %d\n",
			CPU_COUNT(&temp1), CPU_COUNT(&temp2));
	if (CPU_COUNT(&temp1) < 2 || CPU_COUNT(&temp2) < 2) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}
	CPU_OR(&set, &temp1, &temp2);
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (!ret) {
		fprintf(stderr, "fhwb_init succeeds unexpectedly\n");
		return -1;
	}

	printf("test4: check fhwb_fini fails without fhwb_init\n");
	ret = fhwb_fini(0);
	if (!ret) {
		fprintf(stderr, "fhwb_init succeeds unexpectedly\n");
		return -1;
	}

	printf("test5: check fhwb_fini fails at second time\n");
	ret = fill_cpumask_for_cmg(0, &set);
	if (ret)
		return -1;

	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (ret)
		return -1;
	bd = ret;

	ret = fhwb_fini(bd);
	if (ret)
		return -1;

	ret = fhwb_fini(bd);
	if (!ret) {
		fprintf(stderr, "fhwb_fini succeeds unexpectedly\n");
		return -1;
	}

	/* check if everything is clean */
	ret = check_sysfs_status();

	return ret;
}
