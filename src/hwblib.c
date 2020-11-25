/* SPDX-License-Identifier: LGPL-3.0-only */
/* Copyright 2020 FUJITSU LIMITED */

#define _GNU_SOURCE

#include "fujitsu_hwb.h"
#include "fujitsu_hpc_ioctl.h"
#include "internal.h"

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>

/* Global lock for __fd management */
static pthread_mutex_t fhwb_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * File descriptor of device file for ioctl
 *
 * Barrier driver requires all threads joining synchronization shares the same fd.
 * So, library opens file at fhwb_init() and close it fhwb_fini().
 */
static int __fd = -1;
static int open_count = 0;

/* Open device file for ioctl if not currently opened */
static int open_dev_file()
{
	pthread_mutex_lock(&fhwb_mutex);

	if (__fd > 0) {
		open_count++;
	} else {
		fhwb_debug("open device file");
		__fd = open(FHWB_DEV_FILE, O_RDONLY);
		if (__fd > 0)
			open_count++;
	}

	pthread_mutex_unlock(&fhwb_mutex);

	return __fd;
}

static void close_dev_file()
{
	pthread_mutex_lock(&fhwb_mutex);

	open_count--;
	if (open_count == 0) {
		int _errno;

		fhwb_debug("close device file");
		/* Keep original errno in case close() fails */
		_errno = errno;
		close(__fd);
		__fd = -1;
		errno = _errno;
	}

	pthread_mutex_unlock(&fhwb_mutex);
}

static inline int get_fd()
{
	return __fd;
}

/* Make barrier descriptor(bd) from bb/cmg num */
static inline int make_bd(int cmg, int bb)
{
	int bd = 0;

	bd |= (cmg << FHWB_BD_CMG_SHIFT);
	bd |= (bb  << FHWB_BD_BB_SHIFT);

	return bd;
}

int fhwb_get_cmg_from_bd(int bd)
{
	return ((bd >> FHWB_BD_CMG_SHIFT) & FHWB_BD_CMG_MASK);
}

int fhwb_get_bb_from_bd(int bd)
{
	return ((bd >> FHWB_BD_BB_SHIFT) & FHWB_BD_BB_MASK);
}

int fhwb_init(size_t pemask_size, cpu_set_t *pemask)
{
	struct fujitsu_hwb_ioc_bb_ctl ioc_bb_ctl = {0};
	int fd = -1;
	int bd = 0;
	int ret = 0;

	if (pemask == NULL || pemask_size == 0) {
		fhwb_error("pemask is NULL or pemask_size is 0");
		return -EINVAL;
	}

	fd = open_dev_file();
	if (fd < 0) {
		fhwb_error("open_dev_file failed: %m");
		return -errno;
	}

	ioc_bb_ctl.size = pemask_size;
	ioc_bb_ctl.pemask = (unsigned long *)pemask;
	ret = ioctl(fd, FUJITSU_HWB_IOC_BB_ALLOC, &ioc_bb_ctl);
	if (ret < 0) {
		fhwb_error("ioctl FUJITSU_HWB_IOC_BB_ALLOC failed: %m");
		close_dev_file();
		return -errno;
	}

	bd = make_bd(ioc_bb_ctl.cmg, ioc_bb_ctl.bb);
	fhwb_debug("Allocate BB. CMG: %u, BB: %u, bd: 0x%x", ioc_bb_ctl.cmg, ioc_bb_ctl.bb, bd);

	/* Do not call close_dev_file() as it will be called in fhwb_fini() */

	return bd;
}

int fhwb_fini(int bd)
{
	struct fujitsu_hwb_ioc_bb_ctl ioc_bb_ctl = {0};
	int fd = -1;
	int ret = 0;

	fd = get_fd();
	if (fd < 0) {
		fhwb_error("get_fd failed. fhwb_init() is not called?");
		return -EINVAL;
	}

	ioc_bb_ctl.cmg = fhwb_get_cmg_from_bd(bd);
	ioc_bb_ctl.bb = fhwb_get_bb_from_bd(bd);
	ret = ioctl(fd, FUJITSU_HWB_IOC_BB_FREE, &ioc_bb_ctl);
	if (ret < 0) {
		fhwb_error("ioctl FUJITSU_HWB_IOC_BB_FREE failed: %m, CMG: %u, BB: %u, bd: 0x%x",
							ioc_bb_ctl.cmg, ioc_bb_ctl.bb, bd);
		/* Something is wrong, do not close fd */
		return -errno;
	}

	fhwb_debug("Free BB. CMG: %u, BB: %u, bd: 0x%x", ioc_bb_ctl.cmg, ioc_bb_ctl.bb, bd);
	close_dev_file();

	return 0;
}

int fhwb_assign(int bd, int window)
{
	struct fujitsu_hwb_ioc_bw_ctl ioc_bw_ctl = {0};
	int fd = -1;
	int ret = 0;

	fd = get_fd();
	if (fd < 0) {
		fhwb_error("get_fd failed. fhwb_init() is not called?");
		return -EINVAL;
	}

	ioc_bw_ctl.bb = fhwb_get_bb_from_bd(bd);
	ioc_bw_ctl.window = window;
	ret = ioctl(fd, FUJITSU_HWB_IOC_BW_ASSIGN, &ioc_bw_ctl);
	if (ret < 0) {
		fhwb_error("ioctl FUJITSU_HWB_IOC_BW_ASSIGN failed: %m, CMG: %u, BB: %u, window: %u, bd: 0x%x",
					fhwb_get_cmg_from_bd(bd), fhwb_get_bb_from_bd(bd), ioc_bw_ctl.window, bd);
		return -errno;
	}

	fhwb_debug("Assign window. CMG: %u, BB: %u, window: %u, bd: 0x%x",
			fhwb_get_cmg_from_bd(bd), fhwb_get_bb_from_bd(bd), ioc_bw_ctl.window, bd);

	return ioc_bw_ctl.window;
}

int fhwb_unassign(int bd)
{
	struct fujitsu_hwb_ioc_bw_ctl ioc_bw_ctl = {0};
	int fd = -1;
	int ret = 0;

	fd = get_fd();
	if (fd < 0) {
		fhwb_error("get_fd failed. fhwb_init() is not called?");
		return -EINVAL;
	}

	ioc_bw_ctl.bb = fhwb_get_bb_from_bd(bd);
	ret = ioctl(fd, FUJITSU_HWB_IOC_BW_UNASSIGN, &ioc_bw_ctl);
	if (ret < 0) {
		fhwb_error("ioctl FUJITSU_HWB_IOC_BW_UNASSIGN faied: %m, CMG: %u, BB: %u, bd: 0x%x",
						fhwb_get_cmg_from_bd(bd), fhwb_get_bb_from_bd(bd), bd);
		return -errno;
	}

	fhwb_debug("Unassign window. CMG: %u, BB: %u, bd: 0x%x",
			fhwb_get_cmg_from_bd(bd), fhwb_get_bb_from_bd(bd), bd);

	return 0;
}

#define SYNC(reg, num) \
	asm volatile(\
			"mrs x1, " #reg "\n\t" /* read LBSY bit */\
			"mvn x1, x1\n\t" /* negate */\
			"and x1, x1, #1\n\t" /* clear other than first-bit */\
			"msr " #reg ", x1\n\t" /* update BST bit */ \
\
			"sevl\n\t" \
\
		"fhwb_loop"#num":\n\t"\
			"wfe\n\t"\
			"mrs x2, " #reg "\n\t" /* read LBSY bit */\
			"and x2, x2, #1\n\t" /* clear other than first-bit */\
			"cmp x1, x2\n\t"\
			"bne fhwb_loop"#num"\n\t" /* loop until LBSY bit changes */\
		:\
		:\
		:"x1", "x2")

void fhwb_sync(int window)
{
	switch (window) {
	case 0:
		SYNC(s3_3_c15_c15_0, 0);
		break;
	case 1:
		SYNC(s3_3_c15_c15_1, 1);
		break;
	case 2:
		SYNC(s3_3_c15_c15_2, 2);
		break;
	case 3:
		SYNC(s3_3_c15_c15_3, 3);
		break;
	default:
		fhwb_error("window number is invalid: %d", window);
		break;
	}
}

int fhwb_get_pe_info(struct fhwb_pe_info *info)
{
	struct fujitsu_hwb_ioc_pe_info ioc_info = {0};
	int fd = -1;
	int ret = 0;

	if (info == NULL) {
		fhwb_error("pe_info is NULL");
		return -EINVAL;
	}

	fd = open_dev_file();
	if (fd < 0) {
		fhwb_error("open_dev_file failed: %m");
		return -errno;
	}

	ret = ioctl(fd, FUJITSU_HWB_IOC_GET_PE_INFO, &ioc_info);
	if (ret < 0) {
		fhwb_error("ioctl FUJITSU_HWB_IOC_GET_PE_INFO failed: %m");
		close_dev_file();
		return -errno;
	}

	info->cmg = ioc_info.cmg;
	info->physical_pe = ioc_info.ppe;

	fhwb_debug("PE info (CPU %u) ... CMG: %u, Physical PE: %u",
				sched_getcpu(), ioc_info.cmg, ioc_info.ppe);
	close_dev_file();

	return 0;
}

int fhwb_get_all_pe_info(struct fhwb_pe_info **list, int *entry_num)
{
	struct fujitsu_hwb_ioc_pe_info ioc_info = {0};
	struct fhwb_pe_info *result = NULL;
	cpu_set_t orig;
	cpu_set_t set;
	int online_pe;
	int num_pe;
	int ret;
	int fd;
	int i;

	if (list == NULL || entry_num == NULL) {
		fhwb_error("list/entry_num is NULL");
		return -EINVAL;
	}

	fd = open_dev_file();
	if (fd < 0) {
		fhwb_error("open_dev_file failed: %m");
		return -errno;
	}

	/* Get number of PEs on the system (incl. offline PEs) */
	num_pe = get_nprocs_conf();
	result = calloc(num_pe, sizeof(struct fhwb_pe_info));
	if (!result) {
		fhwb_error("memory allocation failure");
		ret = -ENOMEM;
		goto out1;
	}

	/* Keep original affinity value */
	ret = sched_getaffinity(0, sizeof(cpu_set_t), &orig);
	if (ret < 0) {
		fhwb_error("sched_getaffinity failed\n");
		ret = -errno;
		goto out1;
	}

	online_pe = 0;
	/* Issue GET_PE_INFO ioctl on each available PE */
	for (i = 0; i < num_pe; i++) {
		CPU_ZERO(&set);
		CPU_SET(i, &set);
		ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
		if (ret < 0) {
			/* Should be offline or restricted by cgroup. Ignore it */
			result[i].cmg = FHWB_INVALID_CMG;
			result[i].physical_pe = FHWB_INVALID_PPE;
			continue;
		}

		ret = ioctl(fd, FUJITSU_HWB_IOC_GET_PE_INFO, &ioc_info);
		if (ret < 0) {
			fhwb_error("ioctl FUJISU_HWB_IOC_GET_PE_INFO failed: %m");
			ret = -errno;
			goto out2;
		}

		result[i].cmg = ioc_info.cmg;
		result[i].physical_pe = ioc_info.ppe;

		online_pe++;
	}

	*entry_num = num_pe;
	*list = result;
	fhwb_debug("Get %d/%d PE info", online_pe, num_pe);

out2:
	/* Restore affinity */
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &orig);
	if (ret < 0)
		fhwb_error("failed to restore cpu affinity\n");

out1:
	close_dev_file();
	if (ret)
		free(result);

	return ret;
}
