/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Basic function test for fhwb_get_all_pe_info
 */

#include <fujitsu_hwb.h>

#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct fhwb_pe_info *info = NULL;
	int entry_num = 0;
	int valid_info;
	int ret;
	int i;

	printf("test1: check fhwb_get_all_pe_info\n");
	ret = fhwb_get_all_pe_info(&info, &entry_num);
	if (ret)
		return -1;

	valid_info = 0;
	for (i = 0; i < entry_num; i++) {
		if (info[i].cmg == FHWB_INVALID_CMG && info[i].physical_pe == FHWB_INVALID_PPE)
			continue;

		if (info[i].cmg == FHWB_INVALID_CMG || info[i].physical_pe == FHWB_INVALID_PPE) {
			fprintf(stderr, "fhwb_get_all_pe_info returns partial invalid data: %d/%d\n",
					info[i].cmg, info[i].physical_pe);
			free(info);
			return -1;
		}

		valid_info++;
	}
	free(info);

	if (valid_info == 0) {
		fprintf(stderr, "fhwb_get_all_pe_info should return with at lest 1 valid entry\n");
		return -1;
	}
	printf("get %d valid entries\n", valid_info);

	return 0;
}
