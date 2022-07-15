/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Curl utilities.
 */

#ifndef __UTILS_CURL_H__
#define __UTILS_CURL_H__

#include <stdbool.h>

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @param timeout Timeout in seconds.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents (const char * url, int timeout);

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents_default (const char * url);

/**
 * Posts the given JSON to the URL without any
 * authentication.
 *
 * @param timeout Timeout, in seconds.
 * @param ... Optional files to send as
 *   multi-part mime objects. Each object must
 *   contain 3 strings: name, filepath, mimetype.
 *   The list must end in NULL.
 *
 * @return Non-zero if error.
 */
int
z_curl_post_json_no_auth (
  const char * url,
  const char * data,
  int          timeout,
  GError **    error,
  ...) G_GNUC_NULL_TERMINATED;

/**
 * @}
 */

#endif
