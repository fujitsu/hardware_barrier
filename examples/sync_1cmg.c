/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Perform hardware barrier synchronization in loop by all PEs in a specified CMG
 *
 * Usage: ./a.out <cmg_num> <loop_num>
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

static int _bd;
static int _loop;
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
	int cmg = fhwb_get_cmg_from_bd(_bd);
	int bb = fhwb_get_bb_from_bd(_bd);
	int i;

	/* Each thread must be bound to one PE during fhwb_assign() and fhwb_unassign() */
	CPU_ZERO(&set);
	CPU_SET(info->cpuid, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret) {
		perror("sched_setaffinity\n");
		info->ret = ret;
		pthread_exit(NULL);
	}

	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d\n", info->thread_id, cmg, bb, info->cpuid);

	/* Setup window register */
	ret = fhwb_assign(_bd, -1);
	if (ret < 0) {
		info->ret = ret;
		pthread_exit(NULL);
	}

	/* Repeat synchronization _loop times */
	for(i = 0; i < _loop; i++) {
		fhwb_sync(ret);
		if (i % 1000 == 0)
			printf("sync %d: thread %ld, cmg: %d, bb: %d, cpuid: %d\n",
					i, info->thread_id, cmg, bb, info->cpuid);
	}

	/* Release window register */
	ret = fhwb_unassign(_bd);

	info->ret = ret;
	pthread_exit(NULL);
}

#define MAX_THREADS 64
int main(int argc, char *argv[])
{
	struct fhwb_pe_info *pe_info = NULL;
	struct thread_info *th_info;
	cpu_set_t set;
	int cpuids[MAX_THREADS]= {0};
	int entry_num = 0;
	int num_threads = 0;
	int cmg;
	int ret, tmp_ret;
	int i;

	/* Get arguments */
	if (argc < 3) {
		fprintf(stderr, "Perform hardware barrier synchronization in loop by all PEs in a specified CMG\n");
		fprintf(stderr, "Usage: ./a.out <cmg_num> <loop_num>\n");
		return -1;
	}

	cmg = atoi(argv[1]);
	if (cmg < 0) {
		fprintf(stderr, "Invalid cmg number\n");
		return -1;
	}
	_loop = atoi(argv[2]);
	if (_loop < 0) {
		fprintf(stderr, "Invalid loop number\n");
		return -1;
	}

	/* Get system's PE info */
	ret = fhwb_get_all_pe_info(&pe_info, &entry_num);
	if (ret < 0)
		return -1;

	/* Make cpumask of a specified CMG */
	CPU_ZERO(&set);
	for (i = 0; i < entry_num; i++) {
		if (pe_info[i].cmg == cmg) {
			cpuids[num_threads] = i;
			CPU_SET(i, &set);
			num_threads++;
		}
	}
	free(pe_info);

	/* At least 2 PEs are needed to perform sync */
	if (num_threads < 2) {
		fprintf(stderr, "There are not enough PEs in CMG %d\n", cmg);
		return -1;
	}

	th_info = calloc(num_threads, sizeof(struct thread_info));
	if (!th_info) {
		perror("calloc");
		return -1;
	}

	/* Setup barrier blade */
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	if (ret < 0)
		goto out;
	/*
	 * Store barrier descriptor (bd) in global variable since all threads uses
	 * the same bd for fhwb_assign()/fhwb_unassign()
	 */
	_bd = ret;

	/* Create threads */
	for (i = 0; i < num_threads; i++) {
		th_info[i].cpuid = cpuids[i];

		ret = pthread_create(&th_info[i].thread_id, NULL, &worker, &th_info[i]);
		if (ret < 0) {
			perror("pthread_create");
			goto out;
		}
	}

	/* Wait each threads to finish */
	tmp_ret = 0;
	for (i = 0; i < num_threads; i++) {
		ret = pthread_join(th_info[i].thread_id, NULL);
		if (ret) {
			perror("pthread_join");
			goto out;
		}
		if (th_info[i].ret != 0) {
			fprintf(stderr, "Thread returns error\n");
			tmp_ret = -1;
		}
	}

	/* Release barrier blade */
	ret = fhwb_fini(_bd);
	if (!ret && tmp_ret)
		ret = tmp_ret;

out:
	free(th_info);

	return ret;
}
