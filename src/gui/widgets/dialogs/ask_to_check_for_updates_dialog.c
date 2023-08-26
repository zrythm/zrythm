// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/widgets/dialogs/ask_to_check_for_updates_dialog.h"
#include "gui/widgets/main_window.h"
#include "settings/settings.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static void
response_cb (
  AdwMessageDialog * self,
  gchar *            response,
  gpointer           user_data)
{
  bool is_yes = string_is_equal (response, "yes");
  g_settings_set_boolean (
    S_P_GENERAL_UPDATES, "check-for-updates", is_yes);
  g_settings_set_boolean (
    S_GENERAL, "first-check-for-updates", false);

  if (is_yes)
    {
      zrythm_app_check_for_updates (zrythm_app);
    }
}

/**
 * Creates and displays the dialog asynchronously.
 */
void
ask_to_check_for_updates_dialog_run_async (GtkWindow * parent)
{
  AdwMessageDialog * dialog =
    ADW_MESSAGE_DIALOG (adw_message_dialog_new (
      parent, _ ("Check for Updates?"), NULL));
  adw_message_dialog_format_body (
    dialog,
    _ ("Do you want %s to check for updates on startup?"),
    PROGRAM_NAME);
  adw_message_dialog_add_responses (
    dialog, "yes", _ ("_Yes"), "no", _ ("_No"), NULL);
  gtk_window_set_modal (GTK_WINDOW (dialog), true);
  gtk_window_set_transient_for (
    GTK_WINDOW (dialog), GTK_WINDOW (MAIN_WINDOW));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "zrythm");
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "yes",
    ADW_RESPONSE_SUGGESTED);

  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "yes");
  adw_message_dialog_set_close_response (
    ADW_MESSAGE_DIALOG (dialog), "no");

  g_signal_connect (
    dialog, "response", G_CALLBACK (response_cb), dialog);

  gtk_window_present (GTK_WINDOW (dialog));
}
