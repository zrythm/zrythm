/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/about_dialog.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Creates and displays the about dialog.
 */
GtkAboutDialog *
about_dialog_widget_new (
  GtkWindow * parent)
{
  GtkImage * img =
    GTK_IMAGE (
      resources_get_icon (ICON_TYPE_ZRYTHM,
                          "z-splash-png.png"));

  const char * artists[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      NULL
    };
  const char * authors[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      "Sascha Bast <sash@mischkonsum.org>",
      "Georg Krause",
      NULL
    };
  const char * documenters[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      NULL
    };
  const char * translators =
      "Alexandros Theodotou\n"
      "Nicolas Faure <sub26nico@laposte.net>\n"
      "Olivier Humbert <trebmuh@tuxfamily.org>\n"
      "Waui\n"
      "Allan Nordhøy <epost@anotheragency.no>\n"
      "WaldiS <admin@sto.ugu.pl>\n"
      "Silvério Santos\n"
      "Swann Martinet";

  char * version =
    zrythm_get_version (1);

  GtkAboutDialog * dialog =
    g_object_new (
      GTK_TYPE_ABOUT_DIALOG,
      "artists", artists,
      "authors", authors,
      "copyright", "Copyright (C) 2018-2019 Alexandros Theodotou",
      "documenters", documenters,
      /*"logo-icon-name", "z",*/
      "logo", gtk_image_get_pixbuf (img),
      "program-name", "Zrythm",
      "comments", _("a highly automated and intuitive digital audio workstation"),
      "license-type", GTK_LICENSE_AGPL_3_0,
      "translator-credits", translators,
      "website", "https://www.zrythm.org",
      "website-label", _("Website"),
      "version", version,
      NULL);
  gtk_window_set_transient_for (
    GTK_WINDOW (dialog),
    parent);
  gtk_widget_destroy (GTK_WIDGET (img));

  g_free (version);

  return dialog;
}
