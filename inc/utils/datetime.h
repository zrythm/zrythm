/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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
