// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

/* auto-generated */
#include "gui/widgets/dialogs/about_dialog.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <adwaita.h>
#include <glib/gi18n.h>

#include "src/translators.h"

/**
 * Creates and displays the about dialog.
 */
GtkWindow *
about_dialog_widget_new (GtkWindow * parent)
{
  const char * artists[] = {
    "Alexandros Theodotou <alex@zrythm.org>",
    "Daniel Peterson",
    "Carlos Han (C.K. Design) <https://twitter.com/karl_iaxd>",
    NULL
  };
  const char * authors[] = {
    "Alexandros Theodotou <alex@zrythm.org>", "Ryan Gonzalez",
    "Sascha Bast <sash@mischkonsum.org>", "Georg Krause", NULL
  };
  const char * documenters[] = {
    "Alexandros Theodotou <alex@zrythm.org>", NULL
  };
  const char * translators = TRANSLATORS_STR;

  char * version = zrythm_get_version (true);
  char * sys_nfo = zrythm_get_system_info ();

  /* quick hack - eventually reuse the XML from appdata.xml */
  char * release_notes = g_strdup_printf (
    "<p>%s</p>", CHANGELOG_TXT_FOR_ABOUT_DIALOG);

  AdwAboutWindow * dialog = g_object_new (
    ADW_TYPE_ABOUT_WINDOW, "artists", artists, "developers",
    authors, "copyright",
    "Copyright © " COPYRIGHT_YEARS " " COPYRIGHT_NAME
#if !defined(HAVE_CUSTOM_LOGO_AND_SPLASH) \
  || !defined(HAVE_CUSTOM_NAME)
    "\nZrythm and the Zrythm logo are trademarks of Alexandros Theodotou"
#endif
    ,
    "developer-name", COPYRIGHT_NAME, "documenters",
    documenters, "application-icon", "zrythm",
    /*"logo", pixbuf,*/
    "application-name", PROGRAM_NAME, "comments",
    _ ("a highly automated and intuitive digital audio workstation"),
    "debug-info", sys_nfo, "issue-url", ISSUE_TRACKER_URL,
    "license-type", GTK_LICENSE_AGPL_3_0, "translator-credits",
    translators, "release-notes", release_notes, "website",
    "https://www.zrythm.org", "version", version, NULL);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  g_free (version);
  g_free (sys_nfo);
  g_free (release_notes);

  return GTK_WINDOW (dialog);
}
