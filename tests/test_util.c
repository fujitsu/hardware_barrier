/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Unittests for util functions
 */
# define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

void test_get_hwb_hwinfo()
{
	struct hwb_hwinfo hwinfo;
	int ret;

	ret = get_hwb_hwinfo(&hwinfo);
	ASSERT(ret == 0);
	/* Assume we can get valid (non-zero) numbers */
	ASSERT(hwinfo.num_cmg != 0);
	ASSERT(hwinfo.num_bb != 0);
	ASSERT(hwinfo.num_bw != 0);
	ASSERT(hwinfo.max_pe_per_cmg != 0);
}

void test_fill_cpumask_for_cmg()
{
	cpu_set_t set;
	int ret;

	CPU_ZERO(&set);
	/* Assume there are some online cpus in CMG 0 */
	ret = fill_cpumask_for_cmg(0, &set);
	ASSERT(ret == 0);
	ASSERT(CPU_COUNT(&set) != 0);
}

void test_get_next_cpu()
{
	cpu_set_t set;
	int cpu;

	CPU_ZERO(&set);
	cpu = get_next_cpu(&set, -1);
	ASSERT(cpu == -1);

	CPU_SET(3, &set);
	CPU_SET(5, &set);
	CPU_SET(7, &set);
	cpu = get_next_cpu(&set, -1);
	ASSERT(cpu == 3);
	cpu = get_next_cpu(&set, cpu);
	ASSERT(cpu == 5);
	cpu = get_next_cpu(&set, cpu);
	ASSERT(cpu == 7);
	cpu = get_next_cpu(&set, cpu);
	ASSERT(cpu == -1);
}

void test_check_sysfs_status()
{
	cpu_set_t set;
	int ret;
	int bd;

	/* at first it should be clean */
	ret = check_sysfs_status();
	ASSERT(ret == 0);

	/* allocate BB and check status is not clean */
	ret = fill_cpumask_for_cmg(0, &set);
	ASSERT(ret == 0);
	bd = fhwb_init(sizeof(cpu_set_t), &set);
	ASSERT(bd >= 0);
	ret = check_sysfs_status();
	ASSERT(ret != 0);

	/* free BB and check status is clean */
	ret = fhwb_fini(bd);
	ASSERT(ret == 0);
	ret = check_sysfs_status();
	ASSERT(ret == 0);
}

int main()
{

	printf("test1 get_hwb_hwinfo\n");
	test_get_hwb_hwinfo();

	printf("test2 fill_cpumask_for_cmg\n");
	test_fill_cpumask_for_cmg();

	printf("test3 get_next_cpu\n");
	test_get_next_cpu();

	printf("test4 check_sysfs_status\n");
	test_check_sysfs_status();

	return 0;
}
