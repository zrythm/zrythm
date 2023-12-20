// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * File utilities.
 */

#ifndef __UTILS_FILE_H__
#define __UTILS_FILE_H__

#include <stdio.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns 1 if the file/dir exists.
 */
#define file_exists(file) g_file_test (file, G_FILE_TEST_EXISTS)

char *
file_path_relative_to (const char * path, const char * base);

int
file_symlink (const char * old_path, const char * new_path);

/**
 * Do cp --reflink from \ref src to \ref dest.
 *
 * @return Non-zero on error.
 */
int
file_reflink (const char * dest, const char * src);

/**
 * @}
 */

#endif
