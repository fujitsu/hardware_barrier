/* SPDX-License-Identifier: LGPL-3.0-only */
/* Copyright 2020 FUJITSU LIMITED */

#ifndef _FHWB_UTIL_H

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT(expr) do { \
	if (!(expr)) { \
		fflush(stdout); \
		fprintf(stderr, "[%s:%d] ASSERT(%s) fails\n", __func__, __LINE__, #expr); \
		fflush(stderr); \
		exit(1); \
	} \
} while(0)

struct hwb_hwinfo {
	int num_cmg;
	int num_bb;
	int num_bw;
	int max_pe_per_cmg;
};

/* Get barrier hwinfo of running system from sysfs and set to @hwinfo */
int get_hwb_hwinfo(struct hwb_hwinfo *hwinfo);

/* Make cpumask of all PEs in a @cmg */
int fill_cpumask_for_cmg(int cmg, cpu_set_t *mask);

/*
 * Get next cpu value from @set (not incuding @cpu).
 * If @cpu is -1, then get the first cpu.
 *
 * Return -1 if there is no cpu
 */
int get_next_cpu(cpu_set_t *set, int cpu);

/*
 * Check sysfs values of bb_bmap/bw_bmap/init_sync.
 *
 * Return 0 if they are all 0 (i.e. all resource is available)
 */
int check_sysfs_status();

#endif /*_FHWB_UTIL_H */
