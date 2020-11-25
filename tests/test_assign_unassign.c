/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Basic function test for fhwb_assign/fhwb_unassign
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

struct thread_info {
	pthread_t thread_id;
	int cpuid;
	int bd;
	int ret;
};

static void *worker(void * arg)
{
	struct thread_info *info = (struct thread_info *)arg;
	cpu_set_t set;
	int ret;
	int cmg;
	int bb;
	int i;

	/* bind to target PE */
	CPU_ZERO(&set);
	CPU_SET(info->cpuid, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret) {
		perror("sched_setaffinity\n");
		info->ret = ret;
		pthread_exit(NULL);
	}

	cmg = fhwb_get_cmg_from_bd(info->bd);
	bb = fhwb_get_bb_from_bd(info->bd);
	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d\n", info->thread_id, cmg, bb, info->cpuid);

	ret = fhwb_assign(info->bd, -1);
	if (ret < 0) {
		info->ret = ret;
		pthread_exit(NULL);
	}

	for(i = 0; i < 10; i++)
		fhwb_sync(ret);

	ret = fhwb_unassign(info->bd);
	if (ret) {
		info->ret = ret;
		pthread_exit(NULL);
	}

	info->ret = 0;
	pthread_exit(NULL);
}

int test_sync(cpu_set_t *set, int bd)
{
	struct thread_info *th_info;;
	int num_threads;
	int cpu;
	int ret, tmp_ret;
	int i;

	num_threads = CPU_COUNT(set);
	th_info = calloc(num_threads, sizeof(struct thread_info));
	if (!th_info) {
		perror("calloc");
		return -1;
	}

	/* create threads and wait they finish */
	cpu = -1;
	for (i = 0; i < num_threads; i++) {
		cpu = get_next_cpu(set, cpu);
		if (cpu < 0) {
			ret = -1;
			printf("failed to get cpu number\n");
			goto out;
		}

		th_info[i].cpuid = cpu;
		th_info[i].bd = bd;

		ret = pthread_create(&th_info[i].thread_id, NULL, &worker, &th_info[i]);
		if (ret < 0) {
			perror("pthread_create");
			goto out;
		}
	}

	tmp_ret = 0;
	for (i = 0; i < num_threads; i++) {
		ret = pthread_join(th_info[i].thread_id, NULL);
		if (ret) {
			perror("pthread_join");
			goto out;
		}
		if (th_info[i].ret != 0) {
			fprintf(stderr, "thread returns error\n");
			tmp_ret = -1;
		}
	}
	if (tmp_ret)
		ret = tmp_ret;

out:
	free(th_info);

	return ret;
}

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

	printf("test1: check init/assign/sync/unassign/fini by all PEs works in each CMG\n");
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

		ret = test_sync(&set, bd);
		if (ret < 0)
			/* leave cleanup to kernel */
			return -1;

		ret = fhwb_fini(bd);
		if (ret < 0)
			return -1;

		/* in the end, sysfs entries should be clean */
		ret = check_sysfs_status();
		if (ret < 0)
			return -1;
	}

	printf("test2: check init/assign/sync/unassign/fini by 2 PEs works in each CMG\n");
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

		ret = test_sync(&set, bd);
		if (ret < 0)
			/* leave cleanup to kernel */
			return -1;

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
