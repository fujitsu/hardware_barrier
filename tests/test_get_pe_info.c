/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Basic function test for fhwb_get_pe_info
 */

#include <fujitsu_hwb.h>

#include <stdio.h>
#include <string.h>

int main()
{
	struct fhwb_pe_info info;
	int ret;

	printf("test1: check fhwb_get_pe_info fails with NULL argument\n");
	ret = fhwb_get_pe_info(NULL);
	if (!ret) {
		fprintf(stderr, "fhwb_get_pe_info should return error with NULL arg\n");
		return -1;
	}

	printf("test2: check fhwb_get_pe_info\n");
	/* fill invalid value */
	memset(&info, 0xFF, sizeof(info));
	ret = fhwb_get_pe_info(&info);
	if (ret)
		return -1;

	if (info.cmg == FHWB_INVALID_CMG || info.physical_pe == FHWB_INVALID_PPE) {
		fprintf(stderr, "fhwb_get_pe_info returns invalid CMG/PE number: %d/%d\n",
				info.cmg, info.physical_pe);
		return -1;
	}

	return 0;
}
