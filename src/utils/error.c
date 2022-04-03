/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/main_window.c"
#include "utils/error.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Only to be called by HANDLE_ERROR macro.
 */
void
error_handle_prv (
  GError *     err,
  const char * format,
  ...)
{
  va_list args;
  va_start (args, format);

  if (err)
    {
      char * tmp = g_strdup_vprintf (format, args);
      char * str = g_strdup_printf (
        _ ("%s\n---Backtrace---\n%s"), tmp,
        err->message);
      g_free (tmp);
      ui_show_message_printf (
        MAIN_WINDOW, GTK_MESSAGE_ERROR, true, "%s",
        str);
      g_free (str);
      g_error_free (err);
    }
  else
    {
      g_critical ("programming error: err is null");
    }

  va_end (args);
}

void
error_propagate_prefixed_prv (
  GError **    main_err,
  GError *     err,
  const char * format,
  ...)
{
  if (main_err == NULL && err == NULL)
    return;

  g_return_if_fail (main_err);
  g_return_if_fail (err);

  va_list args;
  va_start (args, format);

  /* separate errors by new line */
  char * tmp = g_strdup_vprintf (format, args);
  char   full_str[strlen (tmp) + 10];
  sprintf (full_str, "%s\n", tmp);
  g_free (tmp);

  g_propagate_prefixed_error (
    main_err, err, "%s", full_str);

  va_end (args);
}
