/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Error case test for fhwb_get_pe_info
 */

#include <fujitsu_hwb.h>

#include <stdio.h>

int main()
{
	int ret;

	printf("test1: check fhwb_get_pe_info fails with NULL argument\n");
	ret = fhwb_get_pe_info(NULL);
	if (!ret) {
		fprintf(stderr, "fhwb_get_pe_info should return error with NULL arg\n");
		return -1;
	}

	return 0;
}
