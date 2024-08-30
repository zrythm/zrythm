// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * File utilities.
 */

#ifndef __UTILS_FILE_H__
#define __UTILS_FILE_H__

#include <cstdio>
#include <string>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns whether the file/dir exists.
 */
bool
file_path_exists (const std::string &path);

bool
file_dir_exists (const std::string &dir_path);

char *
file_path_relative_to (const char * path, const char * base);

int
file_symlink (const char * old_path, const char * new_path);

/**
 * Do cp --reflink from @ref src to @ref dest.
 *
 * @return Whether successful.
 */
[[nodiscard]] bool
file_reflink (const std::string &dest, const std::string &src);

/**
 * @}
 */

#endif
