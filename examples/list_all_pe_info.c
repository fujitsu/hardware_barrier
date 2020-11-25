/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Print list of PEs with CPUID/CMG/PPE number
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct fhwb_pe_info *info = NULL;
	int entry_num = 0;
	int ret;
	int i;

	ret = fhwb_get_all_pe_info(&info, &entry_num);
	if (ret < 0)
		return -1;

	for (i = 0; i < entry_num; i++) {
		if (info[i].cmg == FHWB_INVALID_CMG)
			printf("cpuid: %d, INVALID (offline or cgroup restriction?)\n", i);
		else
			printf("cpuid: %d, cmg: %u, ppe: %u\n", i, info[i].cmg, info[i].physical_pe);
	}

	free(info);

	return 0;
}
