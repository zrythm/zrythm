// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Environment variable helper.
 */

#ifndef __UTILS_ENV_H__
#define __UTILS_ENV_H__

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns an int for the given environment variable
 * if it exists and is valid, otherwise returns the
 * default int.
 *
 * @param def Default value to return if not found.
 */
int
env_get_int (const char * key, int def);

/**
 * @}
 */

#endif
