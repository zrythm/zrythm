// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Date and time utils.
 */

#ifndef __UTILS_DATETIME_H__
#define __UTILS_DATETIME_H__

#include "./types.h"

namespace zrythm::utils::datetime
{

/**
 * Returns the current datetime as a string.
 */
std::string
get_current_as_string ();

std::string
epoch_to_str (qint64 epoch, const std::string &format = "yyyy-MM-dd hh:mm:ss");

/**
 * Get the current datetime to be used in filenames, eg, for the log file.
 */
std::string
get_for_filename ();

}; // zrythm::utils::datetime

#endif
