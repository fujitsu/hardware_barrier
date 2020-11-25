/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/* Copyright 2020 FUJITSU LIMITED */
#ifndef _UAPI_LINUX_FUJITSU_HPC_IOC_H
#define _UAPI_LINUX_FUJITSU_HPC_IOC_H

#include <linux/ioctl.h>
#include <asm/types.h>

#ifndef __user
#define __user
#endif

#define __FUJITSU_IOCTL_MAGIC 'F'

/* ioctl definitions for hardware barrier driver */
struct fujitsu_hwb_ioc_bb_ctl {
	__u8 cmg;
	__u8 bb;
	__u8 unused[2];
	__u32 size;
	unsigned long __user *pemask;
};

struct fujitsu_hwb_ioc_bw_ctl {
	__u8 bb;
	__s8 window;
};

struct fujitsu_hwb_ioc_pe_info {
	__u8 cmg;
	__u8 ppe;
};

#define FUJITSU_HWB_IOC_BB_ALLOC _IOWR(__FUJITSU_IOCTL_MAGIC, \
	0x00, struct fujitsu_hwb_ioc_bb_ctl)
#define FUJITSU_HWB_IOC_BW_ASSIGN _IOWR(__FUJITSU_IOCTL_MAGIC, \
	0x01, struct fujitsu_hwb_ioc_bw_ctl)
#define FUJITSU_HWB_IOC_BW_UNASSIGN _IOW(__FUJITSU_IOCTL_MAGIC, \
	0x02, struct fujitsu_hwb_ioc_bw_ctl)
#define FUJITSU_HWB_IOC_BB_FREE _IOW(__FUJITSU_IOCTL_MAGIC, \
	0x03, struct fujitsu_hwb_ioc_bb_ctl)
#define FUJITSU_HWB_IOC_GET_PE_INFO _IOR(__FUJITSU_IOCTL_MAGIC, \
	0x04, struct fujitsu_hwb_ioc_pe_info)

#endif /* _UAPI_LINUX_FUJITSU_HPC_IOC_H */
