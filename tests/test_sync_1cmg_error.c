/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Basically the same as the test_sync_1cmg.c but some
 * threads call fhwb_fini during operation and then exit the program
 *
 * Usage: ./a.out <cmg_num> <loop_num>
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

	cmg = fhwb_get_cmg_from_bd(_bd);
	bb = fhwb_get_bb_from_bd(_bd);
	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d\n", info->thread_id, cmg, bb, info->cpuid);

	/* loop assign/unasign */
	for(i = 0; i < _loop; i++) {
		if ((i % (_loop / 2)) == 0)
			printf("sync %d: thread %ld, cmg: %d, bb: %d, cpuid: %d\n",
						i, info->thread_id, cmg, bb, info->cpuid);

		/* at the half of loop, some threads call fhwb_fini and exit */
		if ((i == (_loop / 2)) && (info->cpuid % 2) == 0) {
			printf("call bar_fini: %d at %d\n", info->cpuid, i);
			ret = fhwb_fini(_bd);
			/*
			 * wait 1 sec for other threads to chceck multiple fhwb_fini() call or
			 * fhwb_[un]assign() after fhwb_fini() does not cause bad thing
			 */
			sleep(1);
			info->ret = ret;
			pthread_exit(NULL);
		}

		ret = fhwb_assign(_bd, -1);
		if (ret < 0) {
			info->ret = ret;
			pthread_exit(NULL);
		}

		/* After mid point, remaining threads do not call fhwb_sync() as it may hung */
		if (i < (_loop / 2))
			fhwb_sync(ret);

		ret = fhwb_unassign(_bd);
		if (ret < 0) {
			info->ret = ret;
			pthread_exit(NULL);
		}
	}

	info->ret = 0;
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	struct thread_info *th_info;
	cpu_set_t set;
	int num_threads = 0;
	int cpu;
	int cmg;
	int ret, tmp_ret;
	int i;

	/* get arguments */
	if (argc < 2) {
		fprintf(stderr, "perform sync by all PEs in specified CMG\n");
		fprintf(stderr, "usage: ./a.out <cmg_num> <loop_num>\n");
		return -1;
	}

	cmg = atoi(argv[1]);
	if (cmg < 0) {
		fprintf(stderr, "invalid cmg number\n");
		return -1;
	}
	_loop = atoi(argv[2]);
	if (_loop < 0) {
		fprintf(stderr, "invalid loop number\n");
		return -1;
	}

	fill_cpumask_for_cmg(cmg, &set);
	num_threads = CPU_COUNT(&set);

	printf("CMG: %d, CPU_COUNT: %d\n", cmg, num_threads);
	if (num_threads < 2) {
		fprintf(stderr, "cannot run test since barrier needs at least 2 PE\n");
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
			fprintf(stderr, "thread returns error\n");
			tmp_ret = -1;
		}
	}

	ret = fhwb_fini(_bd);
	if (!ret && tmp_ret)
		ret = tmp_ret;

out:
	free(th_info);

	return ret;
}
