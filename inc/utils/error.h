/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Error handling utils.
 */

#ifndef __UTILS_ERROR_H__
#define __UTILS_ERROR_H__

#include <glib.h>

/* FIXME delete start */
typedef enum ErrorDomain
{
  ERROR_DOMAIN_GLAREA,
} ErrorDomain;

typedef enum ErrorCode
{
  ERR_PLUGIN_INSTANTIATION_FAILED = 1,
  ERR_OBJECT_IS_NULL,
  ERR_PORT_MAGIC_FAILED,
  ERR_FAILED_TO_LOAD_STATE_FROM_FILE,
  ERROR_SHADER_COMPILATION,
  ERROR_SHADER_LINK,
} ErrorCode;

/**
 * Return the message corresponding to the given
 * error code.
 */
const char *
error_code_get_message (
  ErrorCode err_code);
/* FIXME delete end */

/**
 * Only to be called by HANDLE_ERROR macro.
 */
void
error_handle_prv (
  GError *     err,
  const char * format,
  ...);

/**
 * Shows a popup (or logs a warning if have no UI)
 * and frees the error.
 *
 * @note Only used for non-programming errors.
 */
#define HANDLE_ERROR(err,fmt,...) \
  { \
    error_handle_prv (err, fmt, __VA_ARGS__); \
    err = NULL; \
  }

void
error_propagate_prefixed_prv (
  GError **    main_err,
  GError *     err,
  const char * format,
  ...);

#define PROPAGATE_PREFIXED_ERROR( \
  main_err,err,fmt,...) \
  { \
    error_propagate_prefixed_prv ( \
      main_err, err, fmt, __VA_ARGS__); \
    err = NULL; \
  }

#endif
