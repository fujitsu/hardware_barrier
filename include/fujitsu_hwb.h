/* SPDX-License-Identifier: LGPL-3.0-only */
/* Copyright 2020 FUJITSU LIMITED */

#ifndef _FUJITSU_HWBLIB_H
#define _FUJITSU_HWBLIB_H

#define _GNU_SOURCE

#include <stdint.h>
#include <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FUJITSU_HWBLIB_VERSION_MAJOR 1
#define FUJITSU_HWBLIB_VERSION_MINOR 0
#define FUJITSU_HWBLIB_VERSION_PATCH 0

/* When this environment variable is set, debug message will be shown */
#define FHWB_DEBUG_ENV_NAME "FUJITSU_HWBLIB_DEBUG"

#define FHWB_WINDOW_0 0
#define FHWB_WINDOW_1 1
#define FHWB_WINDOW_2 2
#define FHWB_WINDOW_3 3

/* CMG/Physical PE number of a PE */
#define FHWB_INVALID_CMG 0xFF
#define FHWB_INVALID_PPE 0xFF
struct fhwb_pe_info {
	uint8_t cmg;         /* CMG number */
	uint8_t physical_pe; /* Physical PE number */
};

/**
 * Allocate barrier blade (bb) in @pemask's CMG and initialize its mask with @pemask.
 * Since hardware barrier can be used by PEs of the same CMG, PE in @pemask
 * must belong to the same CMG.
 *
 * @param[in] pemask_size size of @pemask in bytes
 * @param[in] pemask cpumask of PEs joining synchronization
 *
 * @return 0>= barrier descriptor (bd) which will be used in subsequent functions
 *         <0 error
 *            -ENOMEM ... failed to allocate memory
 *            -EFAULT ... failed to copy from/to kernel space
 *            -EBUSY  ... all barrier blade in the CMG is currently used
 *            -EINVAL ... value of @pemask is invalid (some PE belongs to the different CMG etc.)
 */
int fhwb_init(size_t pemask_size, cpu_set_t *pemask);

/**
 * Free allocated barrier blade.
 *
 * @param[in] bd barrier descriptor returned by fhwb_init()
 *
 * @return 0 success
 *        <0 error
 *           -EPERM  ... the bb is being freed
 *           -EFAULT ... failed to copy from/to kernel space
 *           -EINVAL ... @bd is invalid
 */
int fhwb_fini(int bd);

/**
 * Allocate barrier window (bw) and initialize it with given @bd.
 *
 * Barrier window is per PE resource and therefore this function needs to be called
 * once on each PE joining synchronization. The caller thread must be bound to one PE.
 *
 * @param[in] bd barrier descriptor returned by fhwb_init()
 * @param[in] window barrier window number to be used. If -1 is given,
 *            unallocated window will be automatically chosen.
 *
 * @return 0>= window number to be used
 *         <0 error
 *            -EPERM  ... the bb is being freed
 *            -EPERM  ... caller is not bound to one PE
 *            -EFAULT ... failed to copy from/to kernel space
 *            -EBUSY  ... all barrier window or specified barrier window is currently used
 *            -EINVAL ... @bd or @window is invalid
 *            -EINVAL ... the PE is already assigned to a barrier window
 *            -EINVAL ... the PE is not supposed to join synchronization
 */
int fhwb_assign(int bd, int window);

/**
 * Free allocated barrier window.
 *
 * This function needs to be called once on each PE after fhwb_assign.
 * The caller thread must be bound to one PE.
 *
 * @param[in] bd barrier descriptor returned by fhwb_init()
 *
 * @return 0 success
 *        <0 error
 *           -EPERM  ... the bb is being freed
 *           -EPERM  ... caller is not bound to one PE
 *           -EFAULT ... failed to copy from/to kernel space
 *           -EINVAL ... @bd is invalid
 *           -EINVAL ... the PE is not assigned to a window
 */
int fhwb_unassign(int bd);

/**
 * Perform synchronization using hardware barrier. This will block until
 * synchronization completes. This function can be called repeatedly.
 *
 * This function can only be used after calling fhwb_assign() (and before calling hwb_unassign());
 * Calling fhwb_sync() without fhwb_assign() raises SIGILL signal.
 *
 * The caller thread must be bound to one PE.
 *
 * @param[in] window barrier window number to be synced
 */
void fhwb_sync(int window);

/**
 * Get CMG/Physical PE number of PE on which this function is called.
 * The caller thread should be bound to one PE.
 *
 * @param[out] info CMG/Physical PE number of running PE
 *
 * @return 0 success
 *        <0 error
 *           -EFAULT ... failed to copy from kernel space
 *           -EINVAL ... @info is NULL
 */
int fhwb_get_pe_info(struct fhwb_pe_info *info);

/**
 * Get list of CMG/Physical PE number of current running system.
 *
 * Index of @list corresponds to the cpuid of each PE.
 * This is implemented by calling fhwb_get_pe_info() on each available
 * PEs of caller's process. If PE is offline or restricted by cgroup,
 * its CMG/Physical PE number is set to FHWB_INVALID_{CMG,PPE}.
 *
 * Library allocates memory for @list and the caller must free it.
 *
 * @param[out] list list of CMG/PE information
 * @param[out] entry_num entry size of @list
 *
 * @return 0 success
 *        <0 error
 *           -ENOMEM ... failed to allocate memory
 *           -EFAULT ... failed to copy from kernel space
 *           -EINVAL ... @list or @entry_num is NULL
 */
int fhwb_get_all_pe_info(struct fhwb_pe_info **list, int *entry_num);

/*
 * Get CMG number from bd.
 * This is only for debugging purpose to check which CMG is used by current
 * barrier operation.
 *
 * @param[in] bd barrier descriptor returned by fhwb_init()
 *
 * @return 0> CMG number of @bd
 */
int fhwb_get_cmg_from_bd(int bd);

/*
 * Get BB number from bd.
 * This is only for debugging purpose to check which BB is used by current
 * barrier operation.
 *
 * @param[in] bd barrier descriptor returned by fhwb_init()
 *
 * @return 0> BB number of @bd
 */
int fhwb_get_bb_from_bd(int bd);

#ifdef __cplusplus
}
#endif

#endif /* _FUJITSU_HWBLIB_H */
