/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Basic function test for fhwb_init/fhwb_fini
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
	cpu_set_t set, temp;
	int cpu;
	int ret;
	int bd;
	int i;

	ret = get_hwb_hwinfo(&hwinfo);
	if (ret)
		return -1;

	printf("test1: check init/fini by all PEs works in each CMG\n");
	for (i = 0; i < hwinfo.num_cmg; i++) {
		ret = fill_cpumask_for_cmg(i, &set);
		if (ret)
			return -1;

		printf("CMG: %d, CPU_COUNT: %d\n", i, CPU_COUNT(&set));
		if (CPU_COUNT(&set) < 2) {
			printf("barrier needs at least 2 PE. Skip test for this CMG\n");
			continue;
		}

		ret = fhwb_init(sizeof(cpu_set_t), &set);
		if (ret < 0)
			return -1;
		bd = ret;

		ret = fhwb_fini(bd);
		if (ret < 0)
			return -1;

		/* in the end, sysfs entries should be clean */
		ret = check_sysfs_status();
		if (ret < 0)
			return -1;
	}

	printf("test2: check init/fini by 2 PEs works in each CMG\n");
	for (i = 0; i < hwinfo.num_cmg; i++) {
		ret = fill_cpumask_for_cmg(i, &temp);
		if (ret)
			return -1;

		/* Pick 2 cpus in this CMG */
		CPU_ZERO(&set);
		cpu = get_next_cpu(&temp, -1);
		CPU_SET(cpu, &set);
		cpu = get_next_cpu(&temp, cpu);
		CPU_SET(cpu, &set);
		if (CPU_COUNT(&set) != 2) {
			fprintf(stderr, "unexpectedly CPU_COUNT is not 2\n");
			return -1;
		}

		ret = fhwb_init(sizeof(cpu_set_t), &set);
		if (ret < 0)
			return -1;
		bd = ret;

		ret = fhwb_fini(bd);
		if (ret < 0)
			return -1;

		/* in the end, sysfs entries should be clean */
		ret = check_sysfs_status();
		if (ret < 0)
			return -1;
	}

	return 0;
}
