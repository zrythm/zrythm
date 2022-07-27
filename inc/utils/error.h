// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Error handling utils.
 */

#ifndef __UTILS_ERROR_H__
#define __UTILS_ERROR_H__

#include <glib.h>

/**
 * Only to be called by HANDLE_ERROR macro.
 */
void
error_handle_prv (GError * err, const char * format, ...)
  G_GNUC_PRINTF (2, 3);

/**
 * Shows a popup (or logs a warning if have no UI)
 * and frees the error.
 *
 * @note Only used for non-programming errors.
 */
#define HANDLE_ERROR(err, fmt, ...) \
  { \
    error_handle_prv (err, fmt, __VA_ARGS__); \
    err = NULL; \
  }

void
error_propagate_prefixed_prv (
  GError **    main_err,
  GError *     err,
  const char * format,
  ...) G_GNUC_PRINTF (3, 4);

#define PROPAGATE_PREFIXED_ERROR(main_err, err, fmt, ...) \
  { \
    error_propagate_prefixed_prv ( \
      main_err, err, fmt, __VA_ARGS__); \
    err = NULL; \
  }

#endif
