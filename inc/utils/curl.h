/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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

#ifdef PHONE_HOME

#include <stdbool.h>

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
z_curl_get_page_contents (
  const char * url,
  int          timeout);

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents_default (
  const char * url);

/**
 * @}
 */

#endif /* PHONE_HOME */
#endif
