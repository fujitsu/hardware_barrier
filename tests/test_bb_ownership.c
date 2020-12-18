/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Test bb allocated by different process cannot be used.
 * Use clone(2) without any flags to spawn child and then parent allocate bb.
 * After that, check child cannot use parent's bd.
 */

#define _GNU_SOURCE

#include <fujitsu_hwb.h>
#include "util.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* cheat: parent should allocate bb 0 from CMG 0 */
#define parent_bd 0

static int child(void *arg __attribute__((unused)))
{
	cpu_set_t set;
	int ret;
	int bd;

	/* wait parent setup bd */
	sleep(1);
	printf("start child\n");

	/* setup bb on child side */
	ret = fill_cpumask_for_cmg(0, &set);
	ASSERT_SUCCESS(ret);
	ret = fhwb_init(sizeof(cpu_set_t), &set);
	ASSERT_VALID_BD(ret);
	ASSERT(ret != parent_bd);
	bd = ret;

	/* Try to use parent's bd */
	ret = fhwb_fini(parent_bd);
	ASSERT_FAIL(ret);

	/* Try to use child's bd */
	ret = fhwb_fini(bd);
	ASSERT_SUCCESS(ret);

	return 0;
}

#define STACK_SIZE (1024 * 1024)
int main()
{
	struct hwb_hwinfo hwinfo = {0};
	cpu_set_t set;
	pid_t pid;
	char *stack;
	int status;
	int ret;

	ret = get_hwb_hwinfo(&hwinfo);
	ASSERT_SUCCESS(ret);

	printf("test1: check bd allocated by different process cannot be used\n");

	/* allocate child stack and clone without any flags (no sharing) */
	stack = malloc(STACK_SIZE);
	ASSERT(stack != NULL);
	pid = clone(child, stack + STACK_SIZE, SIGCHLD, NULL);
	ASSERT(pid > 0);

	/* setup bb on parent side */
	ret = fill_cpumask_for_cmg(0, &set);
	ASSERT_SUCCESS(ret);
	if (CPU_COUNT(&set) < 2) {
		fprintf(stderr, "cannot perform test\n");
		return -1;
	}

	ret = fhwb_init(sizeof(cpu_set_t), &set);
	/* assume bb 0 from CMG 0 is allocated */
	ASSERT(ret == parent_bd);

	/* wait child process */
	printf("wait child\n");
	ret = waitpid(pid, &status, 0);
	ASSERT((ret == pid));
	ASSERT_SUCCESS(WEXITSTATUS(status));

	/* cleanup */
	ret = fhwb_fini(parent_bd);
	ASSERT_SUCCESS(ret);
	free(stack);

	/* check if everything is clean */
	ret = check_sysfs_status();
	ASSERT_SUCCESS(ret);

	return 0;
}
