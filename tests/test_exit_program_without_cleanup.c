/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Exit program without proper cleanup (unassign/fini) after init/assign.
 * Even user program does not perfrom proper cleanup, kernel driver should
 * perform cleanup upon file close.
 *
 * Usage: ./a.out 0|1 (call fhwb_fini if 1)
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static int _bd;
struct thread_info {
	pthread_t thread_id;
	int cpuid;
	int ret;
};

static void *worker(void * arg)
{
	struct thread_info *info = (struct thread_info *)arg;
	cpu_set_t set;
	int ret;
	int cmg;
	int bb;

	/* bind to target PE */
	CPU_ZERO(&set);
	CPU_SET(info->cpuid, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret) {
		perror("sched_setaffinity\n");
		info->ret = ret;
		pthread_exit(NULL);
	}

	cmg = fhwb_get_cmg_from_bd(_bd);
	bb = fhwb_get_bb_from_bd(_bd);
	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d\n", info->thread_id, cmg, bb, info->cpuid);

	/* just do assign and exit */
	ret = fhwb_assign(_bd, -1);

	info->ret = ret;
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	struct thread_info *th_info;
	cpu_set_t set;
	int num_threads = 0;
	int call_fini;
	int cpu;
	int ret, tmp_ret;
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage: ./a.out 0|1 (call fhwb_fini if 1)\n");
		return -1;
	}
	call_fini = atoi(argv[1]);
	if (call_fini != 0 && call_fini != 1) {
		fprintf(stderr, "invalid argument: %d\n", call_fini);
		return -1;
	}

	/* use CMG 0 */
	ret = fill_cpumask_for_cmg(0, &set);
	if (ret)
		return -1;
	num_threads = CPU_COUNT(&set);

	printf("CMG: 0, CPU_COUNT: %d\n", num_threads);
	if (num_threads < 2) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}

	th_info = calloc(num_threads, sizeof(struct thread_info));
	if (!th_info) {
		perror("calloc");
		return -1;
	}

	/* initialize MASK */
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (ret < 0)
		goto out;
	_bd = ret;

	/* create threads and wait they finish */
	cpu = -1;
	for (i = 0; i < num_threads; i++) {
		cpu = get_next_cpu(&set, cpu);
		if (cpu < 0) {
			ret = -1;
			printf("failed to get cpu number\n");
			goto out;
		}

		th_info[i].cpuid = cpu;

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
			fprintf(stderr, "thread: %ld returns error\n", th_info[i].thread_id);
			tmp_ret = -1;
		}
	}
	if (tmp_ret) {
		ret = tmp_ret;
		goto out;
	}


	if (call_fini) {
		ret = fhwb_fini(_bd);
		if (ret)
			goto out;

		/* fhwb_fini should cleanup all unassigned resources */
		ret = check_sysfs_status();
	}
	/* even if fhwb_fini is not called, kernel should do the same cleanup upon fd release */

out:
	free(th_info);

	return ret;
}
