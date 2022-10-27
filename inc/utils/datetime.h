// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Date and time utils.
 */

#ifndef __UTILS_DATETIME_H__
#define __UTILS_DATETIME_H__

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns the current datetime as a string.
 *
 * Must be free()'d by caller.
 */
char *
datetime_get_current_as_string (void);

char *
datetime_epoch_to_str (gint64 epoch, const char * format);

/**
 * Get the current datetime to be used in filenames,
 * eg, for the log file.
 */
char *
datetime_get_for_filename (void);

/**
 * @}
 */

#endif
