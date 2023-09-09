// SPDX-FileCopyrightText: © 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
z_curl_get_page_contents (const char * url, int timeout, GError ** error);

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

char *
z_curl_get_page_contents_finish (GAsyncResult * res, GError ** error);

void
z_curl_get_page_contents_async (
  const char *        url,
  int                 timeout_sec,
  GAsyncReadyCallback callback,
  gpointer            callback_data);

/**
 * @}
 */

#endif
