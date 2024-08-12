// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Date and time utils.
 */

#ifndef __UTILS_DATETIME_H__
#define __UTILS_DATETIME_H__

#include <string>

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns the current datetime as a string.
 */
std::string
datetime_get_current_as_string (void);

std::string
datetime_epoch_to_str (
  gint64             epoch,
  const std::string &format = "%Y-%m-%d %H:%M:%S");

/**
 * Get the current datetime to be used in filenames, eg, for the log file.
 */
std::string
datetime_get_for_filename (void);

/**
 * @}
 */

#endif
