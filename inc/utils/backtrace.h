/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Backtrace utils.
 */

#ifndef __UTILS_BACKTRACE_H__
#define __UTILS_BACKTRACE_H__

#include <stdbool.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns the backtrace with \ref max_lines
 * number of lines and a string prefix.
 *
 * @param exe_path Executable path for running
 *   addr2line.
 * @param with_lines Whether to show line numbers.
 *   This is very slow.
 */
char *
_backtrace_get (
  const char * exe_path,
  const char * prefix,
  int          max_lines,
  bool         with_lines,
  bool         write_to_file);

#define backtrace_get(prefix, max_lines, write_to_file) \
  _backtrace_get (NULL, prefix, max_lines, false, write_to_file)

#define backtrace_get_with_lines(prefix, max_lines, write_to_file) \
  _backtrace_get ( \
    (ZRYTHM && ZRYTHM->exe_path) ? ZRYTHM->exe_path : NULL, prefix, max_lines, \
    ZRYTHM && ZRYTHM->exe_path ? true : false, write_to_file)

/**
 * @}
 */

#endif
