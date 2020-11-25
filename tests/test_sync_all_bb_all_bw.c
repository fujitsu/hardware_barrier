/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Perform assign/sync/unassign in loop using all BB and BW in a specified CMG.
 * This program assumes A64FX has 6 BB per CMG and 4 BW per PE.
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

static int _loop;
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
	int i, j;

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

	for(i = 0; i < _loop; i++) {
		if ((i % (_loop / 2)) == 0)
			printf("sync %d: thread %ld, cmg: %d, bb: %d, cpuid: %d\n",
						i, info->thread_id, cmg, bb, info->cpuid);

		/* There is no need for each PE to use the same window number */
		ret = fhwb_assign(info->bd, -1);
		if (ret < 0) {
			info->ret = ret;
			pthread_exit(NULL);
		}

		for (j = 0; j < 3; j++)
			fhwb_sync(ret);

		ret = fhwb_unassign(info->bd);
		if (ret) {
			info->ret = ret;
			pthread_exit(NULL);
		}
	}

	info->ret = 0;
	pthread_exit(NULL);
}

/*
 * A64FX has 6 BB per CMG, 4 BW per PE, 12 or 13 PE per CMG.
 * create 3 groups of 8PE (half overlapped) and assign 2 BB for each group
 * to utilize all BB/BW in a CMG
 */
int main(int argc, char *argv[])
{
	struct hwb_hwinfo hwinfo = {0};
	struct thread_info *th_info;
	cpu_set_t set, group[3], sync_group[3];;
	int num_threads = 0;
	int cpu;
	int cmg;
	int ret, tmp_ret;
	int bd[6];
	int i, j, k;

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

	/* check environment */
	ret = get_hwb_hwinfo(&hwinfo);
	if (ret)
		return -1;
	if (hwinfo.num_bb != 6 || hwinfo.num_bw != 4) {
		fprintf(stderr, "cannot run test as this assumes 6 BB per CMG and 4 BW per PE\n");
		return -1;
	}

	/* check available PEs */
	fill_cpumask_for_cmg(cmg, &set);
	printf("CMG: %d, CPU_COUNT: %d\n", cmg, CPU_COUNT(&set));

	/* needs 12 PE */
	if (CPU_COUNT(&set) < 12) {
		fprintf(stderr, "cannot run test as this needs 12 PE per CMG\n");
		return -1;
	}

	/* create num_bw(4) threads per PE */
	num_threads = 12 * 4;
	th_info = calloc(num_threads, sizeof(struct thread_info));
	if (!th_info) {
		perror("calloc");
		return -1;
	}

	cpu = -1;
	/* divide 12 PEs into 3 groups of 4 PEs (exclusive) */
	for (i = 0; i < 3; i++) {
		CPU_ZERO(&group[i]);
		for (j = 0; j < 4; j++) {
			cpu = get_next_cpu(&set, cpu);
			if (cpu < 0) {
				printf("fail get_next_cpu unexpectedlyE\n");
				return -1;
			}
			CPU_SET(cpu, &group[i]);
		}
	}

	/* create 3 group of 8 PEs (half overlapped) */
	CPU_ZERO(&sync_group[0]);
	CPU_ZERO(&sync_group[1]);
	CPU_ZERO(&sync_group[2]);
	CPU_OR(&sync_group[0], &group[0], &group[1]);
	CPU_OR(&sync_group[1], &group[0], &group[2]);
	CPU_OR(&sync_group[2], &group[1], &group[2]);

	/* call fhwb_init 2 times per sync_group (total num_bb(6) times) */
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 2; j++) {
			ret = fhwb_init(sizeof(cpu_set_t), &sync_group[i]);
			if (ret < 0)
				goto out;
			bd[i*2 + j] = ret;
		}
	}

	/* create threads and wait they finish */
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 2; j++) {
			cpu = -1;
			for (k = 0; k < 8; k++) {
				int th_index = (i*16 + j*8 + k);

				cpu = get_next_cpu(&sync_group[i], cpu);
				if (cpu < 0) {
					ret = -1;
					printf("failed to get cpu number\n");
					goto out;
				}

				th_info[th_index].cpuid = cpu;
				th_info[th_index].bd = bd[i*2 + j];

				ret = pthread_create(&th_info[th_index].thread_id, NULL, &worker, &th_info[th_index]);
				if (ret < 0) {
					perror("pthread_create");
					goto out;
				}
			}
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

	for (i = 0; i < 6; i++) {
		ret = fhwb_fini(bd[i]);
		if (!ret && tmp_ret)
			ret = tmp_ret;
	}

out:
	free(th_info);

	return ret;
}
