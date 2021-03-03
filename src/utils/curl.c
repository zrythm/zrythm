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

#include "zrythm-config.h"

#ifdef PHONE_HOME

#include "utils/curl.h"

#include <glib.h>

#ifdef PHONE_HOME
#include <curl/curl.h>
#endif

static size_t
curl_to_string (
  void * ptr,
  size_t size,
  size_t nmemb,
  void * data)
{
  if (size * nmemb == 0)
    return 0;

  GString * page_str = (GString *) data;

  page_str =
    g_string_append_len (
      page_str, ptr, (gssize) (size * nmemb));

  return size * nmemb;
}

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents (
  const char * url)
{
  CURL * curl = curl_easy_init ();
  g_return_val_if_fail (curl, NULL);

  GString * page_str = g_string_new (NULL);

  CURLcode res;
  curl_easy_setopt (
    curl, CURLOPT_URL,
    "https://www.zrythm.org/releases/?C=M;O=D");
  curl_easy_setopt (
    curl, CURLOPT_WRITEFUNCTION, curl_to_string);
  curl_easy_setopt (
    curl, CURLOPT_WRITEDATA, page_str);
  curl_easy_setopt (
    curl, CURLOPT_USERAGENT,
    "zrythm-daw/" PACKAGE_VERSION);

  char * page = NULL;
  res = curl_easy_perform (curl);
  if (res != CURLE_OK)
    {
      g_warning (
        "curl_easy_perform() failed: %s",
        curl_easy_strerror (res));

      g_string_free (page_str, true);
    }
  else
    {
      page = g_string_free (page_str, false);
    }

  curl_easy_cleanup (curl);

  g_return_val_if_fail (page, NULL);

  return page;
}
#endif /* PHONE_HOME */
