// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/main_window.c"
#include "utils/error.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

GtkWindow *
error_handle_prv (GError * err, const char * format, ...)
{
  va_list args;
  va_start (args, format);

  GtkWindow * win = NULL;
  if (err)
    {
      char * tmp = g_strdup_vprintf (format, args);
      char * str =
        g_strdup_printf (_ ("%s\n---Backtrace---\n%s"), tmp, err->message);
      g_free (tmp);
      if (ZRYTHM_HAVE_UI)
        {
          win = ui_show_message_literal (_ ("Error"), str);
        }
      else
        {
          g_warning ("%s", str);
        }
      g_free (str);
      g_error_free (err);
    }
  else
    {
      g_critical ("programming error: err is null");
    }

  va_end (args);

  return win;
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

  g_propagate_prefixed_error (main_err, err, "%s", full_str);

  va_end (args);
}
