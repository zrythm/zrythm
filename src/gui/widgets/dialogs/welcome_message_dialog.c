/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/dialogs/welcome_message_dialog.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Creates and returns the welcome dialog.
 */
GtkDialog *
welcome_message_dialog_new (
  GtkWindow * parent)
{
  GString * gstr = g_string_new (NULL);

  g_string_append_printf (
    gstr, "<big><b>%s</b></big>\n\n",
    _("Welcome to the Zrythm DAW"));

  /* license info */
  g_string_append_printf (
    gstr,
    _("%sZrythm is free software%s: you can "
    "redistribute it and/or modify it under the "
    "terms of the GNU Affero General Public License "
    "as published by the Free Software Foundation, "
    "either version 3 of the License, or (at your "
    "option) any later version."),
    "<b>", "</b>");
  g_string_append (gstr, "\n");
  char * see_license_txt =
    g_strdup_printf (
      _("Zrythm is distributed in the hope that it "
      "will be useful, but WITHOUT ANY WARRANTY; "
      "without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR "
      "PURPOSE. See the %slicense%s for more "
      "details."),
      "<a href=\"" LICENSE_URL "\">", "</a>");
  g_string_append_printf (
    gstr, "%s\n\n", see_license_txt);
  g_free (see_license_txt);

#ifdef FLATPAK_BUILD
  g_string_append_printf (
    gstr, "<b>%s</b>: %s\n\n",
    _("Flatpak users"),
    _("PipeWire support is currently disabled due to "
    "a known bug and will be re-enabled after the "
    "GNOME runtime is updated. For the time being, "
    "please use the PulseAudio backend. Please note "
    "that no MIDI backend is currently usable."));
#endif

#if !defined (INSTALLER_VER) || defined (TRIAL_VER)
  char * donations =
    g_strdup_printf (
      _("%sZrythm relies on donations and purchases "
      "to sustain development%s. If you enjoy the "
      "software, please consider %sdonating%s or "
      "%sbuying an installer%s."),
      "<b>", "</b>",
      "<a href=\"" DONATION_URL  "\">", "</a>",
      "<a href=\"" PURCHASE_URL "\">", "</a>");
  g_string_append_printf (
    gstr, "%s\n\n", donations);
  g_free (donations);
#endif

  /* copyright line */
  g_string_append_printf (
    gstr, "%s",
    "Copyright © " COPYRIGHT_YEARS " "
    COPYRIGHT_NAME);

  /* trademark info */
#if !defined(HAVE_CUSTOM_NAME) || !defined(HAVE_CUSTOM_LOGO_AND_SPLASH)
  g_string_append_printf (
    gstr, "\n\n%s",
    _("Zrythm and the Zrythm logo are registered "
    "trademarks of Alexandros Theodotou in the "
    "United Kingdom."));
#endif

  char * str = g_string_free (gstr, false);

  GtkDialog * dialog =
    GTK_DIALOG (
      gtk_message_dialog_new_with_markup (
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        NULL));
  gtk_message_dialog_set_markup (
    GTK_MESSAGE_DIALOG (dialog), str);
  gtk_window_set_icon_name (
    GTK_WINDOW (dialog), "zrythm");
  gtk_window_set_title (
    GTK_WINDOW (dialog), _("Welcome"));

  GtkWidget * message_area =
    gtk_message_dialog_get_message_area (
      GTK_MESSAGE_DIALOG (dialog));
  GtkWidget * first_lbl =
    gtk_widget_get_first_child (message_area);
  gtk_label_set_justify (
    GTK_LABEL (first_lbl), GTK_JUSTIFY_CENTER);

  g_free (str);

  return dialog;
}