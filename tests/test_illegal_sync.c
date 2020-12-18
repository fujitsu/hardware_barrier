/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Test fhwb_sync without assign invoke SIGILL
 */

#include <fujitsu_hwb.h>
#include "util.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char msg[16] = "receive sigill\n\0";
void sigill_handler(int sig __attribute__((unused)))
{
	/* expect sigill, exit with return value 0 */
	write(STDOUT_FILENO, msg, 16);
	_exit(0);
}

int main()
{
	struct sigaction sa = {0};
	int ret;

	printf("test1: check fhwb_sync without assign invoke SIGILL\n");
	sa.sa_handler = sigill_handler;
	ret = sigaction(SIGILL, &sa, NULL);
	ASSERT_SUCCESS(ret);

	fhwb_sync(0);

	fprintf(stderr, "fhwb_sync unexpectedly succeed without assign operation\n");

	return -1;
}
