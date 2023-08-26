// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/widgets/dialogs/welcome_message_dialog.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
/**
 * Creates and returns the welcome dialog.
 */
AdwMessageDialog *
welcome_message_dialog_new (GtkWindow * parent)
{
  GString * gstr = g_string_new (NULL);

  g_string_append (gstr, "<big><b>%s</b></big>\n\n");

  char * getting_started_guide = g_strdup_printf (
    _ ("If this is your first time using Zrythm, "
       "we suggest going through the 'Getting "
       "Started' section in the %suser manual%s."),
    "<a href=\"" USER_MANUAL_URL "\">", "</a>");
  g_string_append_printf (
    gstr, "%s\n\n", getting_started_guide);

#if !defined(INSTALLER_VER) || defined(TRIAL_VER)
  char * donations = g_strdup_printf (
    _ ("%sZrythm relies on donations and purchases "
       "to sustain development%s. If you enjoy the "
       "software, please consider %sdonating%s or "
       "%sbuying an installer%s."),
    "<b>", "</b>", "<a href=\"" DONATION_URL "\">", "</a>",
    "<a href=\"" PURCHASE_URL "\">", "</a>");
  g_string_append_printf (gstr, "%s\n\n", donations);
  g_free (donations);
#endif

#ifdef FLATPAK_BUILD
  g_string_append_printf (
    gstr, "<b>%s</b>: %s\n\n", _ ("Flatpak users"),
    _ (
      "The PipeWire version of this Flatpak "
      "runtime has a known bug that may cause errors "
      "when starting Zrythm. "
      "We recommend setting a fixed buffer size "
      "for Zrythm in your PipeWire config to avoid "
      "this."));

  g_string_append_printf (
    gstr, "<b>%s</b>: %s\n\n", _ ("Flatpak limitation"),
    _ ("Only Flatpak-installed plugins are "
       "supported."));
#endif

  /* copyright line */
  g_string_append_printf (
    gstr, "%s", "© " COPYRIGHT_YEARS ", " COPYRIGHT_NAME ".");

  /* trademark info */
#if !defined(HAVE_CUSTOM_NAME) \
  || !defined(HAVE_CUSTOM_LOGO_AND_SPLASH)
  char * trademarks = g_strdup_printf (
    _ ("Zrythm and the Zrythm logo are "
       "%strademarks of Alexandros Theodotou%s."),
    "<a href=\"https://git.sr.ht/~alextee/zrythm/tree/master/item/TRADEMARKS.md\">",
    "</a>");
  g_string_append_printf (gstr, "\n\n%s", trademarks);
  g_free (trademarks);
#endif

  char * str = g_string_free (gstr, false);

  AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (
    adw_message_dialog_new (NULL, _ ("Welcome"), NULL));
  adw_message_dialog_format_body_markup (
    dialog, str, _ ("Welcome to the Zrythm DAW"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "zrythm");

  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "ok", _ ("_OK"), NULL);

  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "ok");
  adw_message_dialog_set_close_response (
    ADW_MESSAGE_DIALOG (dialog), "ok");

  g_free (str);

  return dialog;
}
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic pop
