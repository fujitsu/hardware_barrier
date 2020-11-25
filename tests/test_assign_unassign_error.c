/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Test error path of assign/unassign
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
	cpu_set_t set, orig;
	int ret;
	int exclude_cpu;
	int cpu;
	int bd, bd2;

	ret = get_hwb_hwinfo(&hwinfo);
	if (ret)
		return -1;

	ret = sched_getaffinity(0, sizeof(cpu_set_t), &orig);
	if (ret < 0) {
		perror("getaffinity");
		return -1;
	}

	printf("test1: check fhwb_assign fails without CPU bind\n");
	/* use CMG 0 */
	ret = fill_cpumask_for_cmg(0, &set);
	if (ret)
		return -1;

	printf("CMG: 0, CPU_COUNT: %d\n", CPU_COUNT(&set));
	if (CPU_COUNT(&set) < 3) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}

	/* exclude first cpu from set for later test */
	exclude_cpu = get_next_cpu(&set, -1);
	cpu = get_next_cpu(&set, exclude_cpu);
	CPU_CLR(exclude_cpu, &set);

	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (ret < 0)
		return -1;
	bd = ret;

	/* allocate one more bd for later test */
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (ret < 0)
		return -1;
	bd2 = ret;

	/* call fhwb_assign without CPU bind */
	ret = fhwb_assign(bd, 0);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test2: check fhwb_assign fails if current PE is not in mask\n");
	/* bind to PE which does not join synchronization */
	CPU_ZERO(&set);
	CPU_SET(exclude_cpu, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret < 0) {
		perror("setaffinity");
		return -1;
	}

	ret = fhwb_assign(bd, 0);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test3: check fhwb_assign fails for unallocated bd\n");
	/* bind to PE which joins synchronization */
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret < 0) {
		perror("setaffinity");
		return -1;
	}

	ret = fhwb_assign(bd2+1 /* invalid value */, 0);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test4: check fhwb_assign fails for already used window number\n");
	ret = fhwb_assign(bd2, 0);
	if (ret) {
		fprintf(stderr, "assign fails unexpectedly\n");
		return -1;
	}

	/* try to assign already used window */
	ret = fhwb_assign(bd, 0);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test5: check fhwb_assign fails for invalid window number\n");
	ret = fhwb_assign(bd, hwinfo.num_bw);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test6: check fhwb_assign fails at second time\n");
	ret = fhwb_assign(bd, -1);
	if (ret < 0)
		return ret;
	ret = fhwb_assign(bd, -1);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	/* tests for unassign */
	printf("test7: check fhwb_unassign fails without CPU bind\n");
	/* restore original affinity */
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &orig);
	if (ret < 0) {
		perror("setaffinity");
		return -1;
	}

	ret = fhwb_unassign(bd);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test8: check fhwb_unassign fails at second time\n");
	/* bind again */
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret < 0) {
		perror("setaffinity");
		return -1;
	}

	ret = fhwb_unassign(bd);
	if (ret < 0)
		return ret;
	ret = fhwb_unassign(bd);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	printf("test9: check fhwb_unassign fails for unallocated bd\n");
	ret = fhwb_unassign(bd2+1 /* invalid value */);
	if (!ret) {
		fprintf(stderr, "assign succeeds unexpectedly\n");
		return -1;
	}

	/* cleanup */
	ret = fhwb_fini(bd);
	if (ret < 0)
		return -1;

	ret = fhwb_fini(bd2);
	if (ret < 0)
		return -1;

	/* check if everything is clean */
	ret = check_sysfs_status();

	return ret;
}
