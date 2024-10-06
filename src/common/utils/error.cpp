// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/error.h"
#include "common/utils/ui.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

AdwDialog *
error_handle_prv (GError * err, const char * format, ...)
{
  va_list args;
  va_start (args, format);

  AdwDialog * win = NULL;
  if (err)
    {
      char * tmp = g_strdup_vprintf (format, args);
      char * str =
        g_strdup_printf ("%s\n- %s: -\n%s", tmp, _ ("Details"), err->message);
      g_free (tmp);
      if (ZRYTHM_HAVE_UI)
        {
          win = ui_show_message_literal (_ ("Error"), str);
        }
      else
        {
          z_warning ("{}", str);
        }
      g_free (str);
      g_error_free (err);
    }
  else
    {
      z_error ("programming error: err is null");
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
  if (main_err == NULL && err == nullptr)
    return;

  z_return_if_fail (main_err);
  z_return_if_fail (err);

  va_list args;
  va_start (args, format);

  /* separate errors by new line */
  char *            tmp = g_strdup_vprintf (format, args);
  std::vector<char> full_str;
  full_str.resize (strlen (tmp) + 10);
  sprintf (full_str.data (), "%s\n", tmp);
  g_free (tmp);

  g_propagate_prefixed_error (main_err, err, "%s", full_str.data ());

  va_end (args);
}
