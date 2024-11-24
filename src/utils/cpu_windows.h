/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * CPU usage on windows.
 */

#ifndef __UTILS_CPU_WINDOWS_H__
#define __UTILS_CPU_WINDOWS_H__

#ifdef _WIN32

#  ifdef __cplusplus
extern "C" {
#  endif

/**
 * Return the CPU usage as a percentage/
 *
 * @param pid The process ID to check, or -1 to check
 *   the current process.
 */
int
cpu_windows_get_usage (int pid);

#  ifdef __cplusplus
}
#  endif

#endif // _WIN32

#endif
