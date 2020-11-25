/* SPDX-License-Identifier: LGPL-3.0-only */
/*
 * Copyright 2020 FUJITSU LIMITED
 *
 * Just check barrier's sysfs status (intended to be used in bash script)
 */

#include "util.h"

int main()
{
	return check_sysfs_status();
}
