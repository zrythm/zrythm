/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/* auto-generated */
#include "src/translators.h"

#include "gui/widgets/dialogs/about_dialog.h"
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
  const char * artists[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      "Daniel Peterson",
      "Carlos Han (C.K. Design) <https://twitter.com/karl_iaxd>",
      NULL
    };
  const char * authors[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      "Ryan Gonzalez",
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
    TRANSLATORS_STR;

  char * version =
    zrythm_get_version (true);

  GtkAboutDialog * dialog =
    g_object_new (
      GTK_TYPE_ABOUT_DIALOG,
      "artists", artists,
      "authors", authors,
      "copyright",
      "Copyright © "
        COPYRIGHT_YEARS " " COPYRIGHT_NAME
#if !defined(HAVE_CUSTOM_LOGO_AND_SPLASH) || \
  !defined (HAVE_CUSTOM_NAME)
      "\nZrythm and the Zrythm logo are trademarks of Alexandros Theodotou"
#endif
      ,
      "documenters", documenters,
      "logo-icon-name", "zrythm-splash-png",
      /*"logo", pixbuf,*/
      "program-name", PROGRAM_NAME,
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

  g_free (version);

  return dialog;
}
