/* SPDX-License-Identifier: LGPL-3.0-only */
/* Copyright 2020 FUJITSU LIMITED */

#ifndef _FUJITSU_HWB_INTERNAL_H
#define _FUJITSU_HWB_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>

#define FHWB_BD_BB_SHIFT  0
#define FHWB_BD_CMG_SHIFT 8
#define FHWB_BD_BB_MASK  0xFF
#define FHWB_BD_CMG_MASK 0xFF

/* fujitsu_hwb driver will create following device file upon module load */
#define FHWB_DEV_FILE "/dev/fujitsu_hwb"

/* Macro for error message */
#define fhwb_error(fmt, ...) do { \
	fflush(stdout); \
	fprintf(stderr, "libFJhwb: ERROR: %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
	fflush(stderr); \
} while(0)

/* Macro for debug message (only will be shown when FHWB_DEBUG_ENV_NAME is set) */
#define fhwb_debug(fmt, ...) do { \
	if (getenv(FHWB_DEBUG_ENV_NAME) != NULL) { \
		fflush(stdout); \
		fprintf(stderr, "libFJhwb: DEBUG: %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
		fflush(stderr); \
	} \
} while(0)

#endif /* _FUJITSU_HWB_INTERNAL_H */
