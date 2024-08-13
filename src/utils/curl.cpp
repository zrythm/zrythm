// clang-format off
// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright (c) 1996 - 2021, Daniel Stenberg, daniel@haxx.se, and many contributors, see the THANKS file.
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder.
 */
// clang-format on

#include "zrythm-config.h"

#include "utils/curl.h"
#include "utils/env.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib.h>

#include <curl/curl.h>

typedef enum
{
  Z_UTILS_CURL_ERROR_FAILED,
  Z_UTILS_CURL_ERROR_BAD_REQUEST,
  Z_UTILS_CURL_ERROR_CANNOT_INIT,
  Z_UTILS_CURL_ERROR_NON_2XX_RESPONSE,
} ZUtilsCurlError;

#define Z_UTILS_CURL_ERROR z_utils_curl_error_quark ()
GQuark
z_utils_curl_error_quark (void);
G_DEFINE_QUARK (
  z - utils - curl - error - quark,
  z_utils_curl_error)

static size_t
curl_to_string (void * ptr, size_t size, size_t nmemb, void * data)
{
  if (size * nmemb == 0)
    return 0;

  GString * page_str = (GString *) data;

  page_str = g_string_append_len (
    page_str, static_cast<const char *> (ptr), (gssize) (size * nmemb));

  return size * nmemb;
}

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @param timeout Timeout in seconds.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents (const char * url, int timeout, GError ** error)
{
  z_debug ("getting page contents for %s...", url);

  CURL * curl = curl_easy_init ();
  z_return_val_if_fail (curl, nullptr);

  GString * page_str = g_string_new (nullptr);

  CURLcode res;
  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curl_to_string);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, page_str);
  curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeout);
  curl_easy_setopt (curl, CURLOPT_USERAGENT, "zrythm-daw/" PACKAGE_VERSION);

  char * page = NULL;
  res = curl_easy_perform (curl);
  if (res == CURLE_OK)
    {
      page = g_string_free (page_str, false);
    }
  else
    {
      g_set_error (
        error, Z_UTILS_CURL_ERROR, Z_UTILS_CURL_ERROR_FAILED,
        "curl_easy_perform() failed: %s", curl_easy_strerror (res));
      g_string_free (page_str, true);
    }

  curl_easy_cleanup (curl);

  z_debug ("done getting page contents for %s", url);

  return page;
}

/**
 * Returns the contents of the page in a newly
 * allocated string.
 *
 * @return Newly allocated string or NULL if fail.
 */
char *
z_curl_get_page_contents_default (const char * url)
{
  z_error ("unsupported function");
  return NULL;
  int timeout = env_get_int ("Z_CURL_TIMEOUT", 0);

  if (timeout <= 0)
    {
      timeout = 1;
    }

  return z_curl_get_page_contents (url, timeout, nullptr);
}

struct WriteThis
{
  const char * readptr;
  size_t       sizeleft;
};

static size_t
read_callback (char * dest, size_t size, size_t nmemb, void * userp)
{
  struct WriteThis * wt = (struct WriteThis *) userp;
  size_t             buffer_size = size * nmemb;

  if (wt->sizeleft)
    {
      /* copy as much as possible from the source to the destination */
      size_t copy_this_much = wt->sizeleft;
      if (copy_this_much > buffer_size)
        copy_this_much = buffer_size;
      memcpy (dest, wt->readptr, copy_this_much);

      wt->readptr += copy_this_much;
      wt->sizeleft -= copy_this_much;
      z_debug ("copied %zu chars", copy_this_much);
      return copy_this_much; /* we copied this many bytes */
    }

  return 0; /* no more data left to deliver */
}

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
  ...)
{
  z_return_val_if_fail (
    url == NULL || data == NULL || error == NULL || *error == nullptr, -1);

  z_info ("sending data...");

  CURL * curl = curl_easy_init ();
  if (!curl)
    {
      g_set_error_literal (
        error, Z_UTILS_CURL_ERROR, Z_UTILS_CURL_ERROR_CANNOT_INIT,
        "Failed to init curl");
      return -1;
    }

#if 0
  struct WriteThis write_data;
  write_data.readptr = data;
  write_data.sizeleft = strlen (data);
  curl_easy_setopt (
    curl, CURLOPT_POSTFIELDSIZE,
    (curl_off_t) write_data.sizeleft);
#endif

  struct curl_slist * headers = NULL;
  headers = curl_slist_append (headers, "Accept: application/json");
  headers = curl_slist_append (headers, "Content-Type: multipart/form-data");
#if 0
  headers =
    curl_slist_append (
      headers, "Content-Type: application/json");
  headers =
    curl_slist_append (headers, "charset: utf-8");
#endif

  GString * response = g_string_new (nullptr);

  CURLcode res;
  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_POST, 1L);
  curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
  (void) read_callback;
#if 0
  curl_easy_setopt (
    curl, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt (
    curl, CURLOPT_READDATA, &write_data);
#endif
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curl_to_string);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, response);
  curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeout);
  curl_easy_setopt (curl, CURLOPT_USERAGENT, "zrythm-daw/" PACKAGE_VERSION);
#if 0
  curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);
#endif

  /* Create the form */
  curl_mime * mime = curl_mime_init (curl);

  /* add json */
  curl_mimepart * json_part = curl_mime_addpart (mime);
  curl_mime_name (json_part, "data");
  curl_mime_type (json_part, "application/json");
  curl_mime_data (json_part, data, CURL_ZERO_TERMINATED);

  /* add files */
  va_list args;
  va_start (args, error);
  while (true)
    {
      const char * mime_name = va_arg (args, const char *);
      if (!mime_name)
        break;
      const char * filepath = va_arg (args, const char *);
      const char * mimetype = va_arg (args, const char *);

      /* Fill in the file upload field */
      curl_mimepart * part = curl_mime_addpart (mime);
      curl_mime_name (part, mime_name);
      curl_mime_type (part, mimetype);
      curl_mime_filedata (part, filepath);
    }
  va_end (args);

  curl_easy_setopt (curl, CURLOPT_MIMEPOST, mime);

  res = curl_easy_perform (curl);
  if (res != CURLE_OK)
    {
      z_warning ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
      g_set_error (
        error, Z_UTILS_CURL_ERROR, Z_UTILS_CURL_ERROR_BAD_REQUEST,
        "curl_easy_perform() failed: %s", curl_easy_strerror (res));

      curl_easy_cleanup (curl);
      g_string_free (response, true);

      return -1;
    }

  char * response_str = g_string_free (response, false);
  long   http_code = 0;
  curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
#if 0
  z_debug (
    "[%ld]: %s", http_code, response_str);
#endif
  if (http_code >= 200 && http_code < 300)
    {
      z_debug ("success");
    }
  else
    {
      g_set_error (
        error, Z_UTILS_CURL_ERROR, Z_UTILS_CURL_ERROR_NON_2XX_RESPONSE,
        "POST failed: [%ld] %s", http_code, response_str);

      g_free (response_str);
      curl_easy_cleanup (curl);

      return -1;
    }
  g_free (response_str);

  z_info ("%s: done", __func__);

  curl_easy_cleanup (curl);
  curl_mime_free (mime);

  return 0;
}

typedef struct ZCurlTaskData
{
  const char * url;
  int          timeout_sec;
} ZCurlTaskData;

static void
get_page_contents_task (
  GTask *        task,
  gpointer       source_obj,
  gpointer       _task_data,
  GCancellable * cancellable)
{
  ZCurlTaskData * task_data = (ZCurlTaskData *) _task_data;
  GError *        err = NULL;
  char *          page =
    z_curl_get_page_contents (task_data->url, task_data->timeout_sec, &err);
  if (!page)
    {
      g_task_return_error (task, err);
      return;
    }
  g_task_return_pointer (task, page, g_free);
}

char *
z_curl_get_page_contents_finish (GAsyncResult * res, GError ** error)
{
  z_debug ("finished getting page contents");
  z_return_val_if_fail (g_task_is_valid (res, zrythm_app.get ()), nullptr);
  return (char *) g_task_propagate_pointer ((GTask *) res, error);
}

void
z_curl_get_page_contents_async (
  const char *        url,
  int                 timeout_sec,
  GAsyncReadyCallback callback,
  gpointer            callback_data)
{
  GTask * task =
    g_task_new (zrythm_app.get (), nullptr, callback, callback_data);
  ZCurlTaskData * task_data = object_new (ZCurlTaskData);
  task_data->url = url;
  task_data->timeout_sec = timeout_sec;
  g_task_set_task_data (task, task_data, (GDestroyNotify) free);
  g_task_run_in_thread (task, get_page_contents_task);
  g_object_unref (task);
}
