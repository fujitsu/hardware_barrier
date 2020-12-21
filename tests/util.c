/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Some misc functions to help writing test program
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FHWB_SYSFS_ROOT "/sys/class/misc/fujitsu_hwb"
#define PATH_MAX 256

#define UTIL_ERR(fmt, ...)\
	fprintf(stderr, "libFJlib util:%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);\

/* sysfs file size is always PAGE_SIZE */
size_t page_size;

static int __get_hwb_hwinfo(struct hwb_hwinfo *hwinfo, char *buf)
{
	char path[PATH_MAX];
	int ret;
	int fd;

	ret = snprintf(path, PATH_MAX, "%s/hwinfo", FHWB_SYSFS_ROOT);
	if (ret < 0) {
		UTIL_ERR("snprintf %m");
		return ret;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		UTIL_ERR("open %m");
		return -1;
	}

	ret = read(fd, buf, page_size);
	if (ret < 0) {
		UTIL_ERR("read %m");
		goto out;
	}

	buf[ret] = '\0';
	ret = sscanf(buf, "%d %d %d %d",
			&hwinfo->num_cmg, &hwinfo->num_bb, &hwinfo->num_bw, &hwinfo->max_pe_per_cmg);
	if (ret < 0) {
		UTIL_ERR("sscanf %m");
		goto out;
	}

	ret = 0;

out:
	close(fd);

	return ret;
}

int get_hwb_hwinfo(struct hwb_hwinfo *hwinfo)
{
	char *buf;
	int ret;

	if (hwinfo == NULL) {
		UTIL_ERR("hwinfo is NULL");
		return -1;
	}

	page_size = getpagesize();
	buf = malloc(page_size);
	if (!buf) {
		UTIL_ERR("malloc");
		return -1;
	}

	ret = __get_hwb_hwinfo(hwinfo, buf);
	free(buf);

	return ret;
}


static int check_used_bb_bmap(int cmg, char *buf)
{
	char path[PATH_MAX];
	int used_bb_bmap;
	int ret;
	int fd;

	ret = snprintf(path, PATH_MAX, "%s/CMG%d/used_bb_bmap", FHWB_SYSFS_ROOT, cmg);
	if (ret < 0) {
		perror("snprintf");
		return -1;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		UTIL_ERR("open %m");
		return -1;
	}

	ret = read(fd, buf, page_size);
	if (ret < 0) {
		UTIL_ERR("read %m");
		goto out;
	}

	buf[ret] = '\0';
	ret = sscanf(buf, "%x", &used_bb_bmap);
	if (ret < 0) {
		UTIL_ERR("sscanf %m");
		goto out;
	}

	ret = 0;
	if (used_bb_bmap != 0) {
		printf("used_bb_bmap CMG: %d\n", cmg);
		printf("%04x\n", used_bb_bmap);
		ret = -1;
	}

out:
	close(fd);

	return ret;
}


static int check_used_bw_bmap(int cmg)
{
	FILE *stream = NULL;
	ssize_t nread;
	size_t len = 0;
	char path[PATH_MAX];
	char *line = NULL;
	int used_bw_bmap;
	int cpuid;
	int clean;
	int ret;

	ret = snprintf(path, PATH_MAX, "%s/CMG%d/used_bw_bmap", FHWB_SYSFS_ROOT, cmg);
	if (ret < 0) {
		UTIL_ERR("snprintf %m");
		return -1;
	}

	stream = fopen(path, "r");
	if (stream == NULL) {
		UTIL_ERR("fopen %m");
		return -1;
	}

	clean = 0;
	while ((nread = getline(&line, &len, stream)) != -1) {
		ret = sscanf(line, "%d %x", &cpuid, &used_bw_bmap);
		if (ret < 0) {
			UTIL_ERR("sscanf %m");
			goto out;
		}

		if (used_bw_bmap != 0) {
			if (clean == 0)
				printf("used_bw_bmap CMG: %d\n", cmg);

			printf("%d %04x\n", cpuid, used_bw_bmap);
			clean = -1;
		}
	}
	ret = 0;

out:
	free(line);
	fclose(stream);

	if (ret)
		return ret;
	else
		return clean;
}

static int check_init_sync(int cmg, int bb, char* buf)
{
	char path[PATH_MAX];
	int mask, bst;
	int ret;
	int fd;

	ret = snprintf(path, PATH_MAX, "%s/CMG%d/init_sync_bb%d", FHWB_SYSFS_ROOT, cmg, bb);
	if (ret < 0) {
		UTIL_ERR("snprintf %m");
		return -1;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		UTIL_ERR("open %m");
		return -1;
	}

	ret = read(fd, buf, page_size);
	if (ret < 0) {
		UTIL_ERR("read %m");
		goto out;
	}

	if (ret == 0)
		/* empty file. there is no CMG for this PE */
		goto out;

	buf[ret] = '\0';
	ret = sscanf(buf, "%x\n%x", &mask, &bst);
	if (ret < 0) {
		UTIL_ERR("sscanf %m");
		goto out;
	}

	ret = 0;
	if (mask != 0 || bst != 0) {
		printf("init_sync CMG: %d BB: %d\n", cmg, bb);
		printf("%04x %04x\n", mask, bst);
		ret = -1;
	}

out:
	close(fd);

	return ret;
}

int check_sysfs_status()
{
	struct hwb_hwinfo hwinfo = {0};
	char *buf;
	int ret;
	int i, j;

	page_size = getpagesize();
	buf = malloc(page_size);
	if (!buf) {
		UTIL_ERR("malloc");
		return -1;
	}

	ret = __get_hwb_hwinfo(&hwinfo, buf);
	if (ret)
		goto out;

	for (i = 0; i < hwinfo.num_cmg; i++)
		ret |= check_used_bb_bmap(i, buf);

	for (i = 0; i < hwinfo.num_cmg; i++)
		ret |= check_used_bw_bmap(i);

	if (getuid() == 0) {
		for (i = 0; i < hwinfo.num_cmg; i++) {
			for (j = 0; j < hwinfo.num_bb; j++) {
				ret |= check_init_sync(i, j, buf);
			}
		}
	} else {
		printf("skip check_init_sync() as it requires root privilege\n");
	}

out:
	free(buf);

	if (ret)
		return -1;

	return 0;
}

int fill_cpumask_for_cmg(int cmg, cpu_set_t *mask)
{
	struct fhwb_pe_info *pe_info = NULL;
	int entry_num = 0;
	int ret;
	int i;

	/* get system's PE info */
	ret = fhwb_get_all_pe_info(&pe_info, &entry_num);
	if (ret < 0)
		return -1;

	/* Make cpumask of specified CMG */
	CPU_ZERO(mask);
	for (i = 0; i < entry_num; i++) {
		if (pe_info[i].cmg == cmg)
			CPU_SET(i, mask);
	}
	free(pe_info);

	return 0;
}

/* get next cpu value (not incuding @cpu) */
int get_next_cpu(cpu_set_t *set, int cpu)
{
	int i;

	for (i = cpu + 1; i < (int)(sizeof(cpu_set_t) * 8); i++) {
		if (CPU_ISSET(i, set))
			return i;
	}

	return -1;
}
