/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Error route test for fhwb_init when pemask contains offline cpu
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define PATH_FORMAT "/sys/devices/system/cpu/cpu%d/online"
#define PATH_MAX 256
int cpu_online(int cpu, int status)
{
	char *path;
	char on[] = "1\0";
	char off[] = "0\0";
	int fd;
	int ret;

	path = malloc(PATH_MAX);
	if (!path) {
		perror("malloc");
		return -1;
	}

	ret = snprintf(path, PATH_MAX, PATH_FORMAT, cpu);
	if (ret < 0) {
		perror("snprintf");
		goto out;
	}

	printf("sysfs path: %s\n", path);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		perror("open");
		goto out;
	}

	ret = write(fd, (status == 1 ? on : off), 2);
	if (ret < 0) {
		perror("write");
	} else {
		printf("%s cpu %d\n", (status == 1 ? "online" : "offline"), cpu);
		ret = 0;
	}

	close(fd);
out:
	free(path);

	return ret;
}

int main()
{
	cpu_set_t set;
	int cpu, offline_cpu;
	int ret;

	/* skip test for non-root */
	if (getuid() != 0) {
		printf("This test requires root privilege. Skip the test\n");
		return 77;
	}

	printf("test1: check fhwb_init fails when offline cpu is in pemask\n");
	ret = fill_cpumask_for_cmg(0, &set);
	if (CPU_COUNT(&set) < 3) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}

	/* cpu 0 may not be offlined, use 2nd cpu */
	cpu = get_next_cpu(&set, -1);
	offline_cpu = get_next_cpu(&set, cpu);

	/* offline it */
	ret = cpu_online(offline_cpu, 0);
	ASSERT_SUCCESS(ret);

	/* fhwb_init shold fail with offline cpu */
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	ASSERT_FAIL(ret);

	/* restore cpu status */
	ret = cpu_online(offline_cpu, 1);
	ASSERT_SUCCESS(ret);

	/* check if everything is clean */
	ret = check_sysfs_status();
	ASSERT_SUCCESS(ret);

	return 0;
}
