/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Micro benchmark measuring hardware barrier synchronization time by all PEs in a specified CMG
 * This only measures time of one fhwb_sync() call in each thread after setup is done.
 *
 * Usage: ./a.out <cmg_num> <wait_us>
 * If wait_us is not 0, one thread sleep wait_us before performing fhwb_sync()
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
#include <time.h>
#include <unistd.h>

/* Read system clock counter directly from EL0 */
static inline unsigned long read_cntvct(void)
{
	unsigned long x;

	__asm__ __volatile__("isb \n\t"
		"mrs %[output], cntvct_el0"
		: [output]"=r"(x)
		:
		: "memory");

	return x;
}

static inline unsigned long read_cntfrq(void)
{
	unsigned long x;

	__asm__ __volatile__("isb \n\t"
		"mrs %[output], cntfrq_el0"
		: [output]"=r"(x)
		:
		: "memory");

	return x;
}

static int _bd;
struct thread_info {
	pthread_t thread_id;
	int cpuid;
	int ret;
	unsigned long wait_us;
};

static void *worker(void * arg)
{
	struct thread_info *info = (struct thread_info *)arg;
	struct timespec wait = {0, 0};
	unsigned long t1, t2, freq, scale;
	cpu_set_t set;
	int ret;
	int cmg = fhwb_get_cmg_from_bd(_bd);
	int bb = fhwb_get_bb_from_bd(_bd);

	/* Each thread must be bound to one PE during fhwb_assign() and fhwb_unassign() */
	CPU_ZERO(&set);
	CPU_SET(info->cpuid, &set);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
	if (ret) {
		perror("sched_setaffinity\n");
		info->ret = ret;
		pthread_exit(NULL);
	}

	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d, wait_us: %lu\n",
			info->thread_id, cmg, bb, info->cpuid, info->wait_us);
	if (info->wait_us) {
		wait.tv_sec  = (info->wait_us * 1000 ) / (1000UL * 1000 * 1000);
		wait.tv_nsec = (info->wait_us * 1000 ) % (1000UL * 1000 * 1000);
	}

	/* Setup window register */
	ret = fhwb_assign(_bd, -1);
	if (ret < 0) {
		info->ret = ret;
		pthread_exit(NULL);
	}

	/* Measure time of hardware barrier synchronization (fhwb_sync) */
	if (info->wait_us) {
		/* For adjusting start time in eacth thread */
		fhwb_sync(ret);

		t1 = read_cntvct();
		nanosleep(&wait, NULL);
		fhwb_sync(ret);
		t2 = read_cntvct();
	} else {
		fhwb_sync(ret);

		t1 = read_cntvct();
		fhwb_sync(ret);
		t2 = read_cntvct();
	}

	freq = read_cntfrq();
	scale = (1000UL * 1000 * 1000) / freq;

	printf("thread %ld, cmg: %d, bb: %d, cpuid: %d, t1: %lu, t2: %lu, freq: %lu, sync time %lu ns\n",
			info->thread_id, cmg, bb, info->cpuid, t1, t2, freq, (t2-t1)*scale);

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
	long wait_us = 0;
	int cpuids[MAX_THREADS]= {0};
	int entry_num = 0;
	int num_threads = 0;
	int cmg;
	int ret, tmp_ret;
	int i;

	/* Get arguments */
	if (argc < 3) {
		fprintf(stderr, "Micro benchmark measuring hardware barrier synchronization time by all PEs in a specified CMG\n\n");
		fprintf(stderr, "Usage: ./a.out <cmg_num> <wait_us>\n");
		fprintf(stderr, "If wait_us is not 0, one thread sleep wait_us before performing fhwb_sync()\n");
		return -1;
	}

	cmg = atoi(argv[1]);
	if (cmg < 0) {
		fprintf(stderr, "Invalid cmg number\n");
		return -1;
	}
	wait_us = atoi(argv[2]);
	if (wait_us < 0) {
		fprintf(stderr, "Invalid wait_us number\n");
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
		/* only first CPU will sleep */
		if (i == 0)
			th_info[i].wait_us = wait_us;

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
