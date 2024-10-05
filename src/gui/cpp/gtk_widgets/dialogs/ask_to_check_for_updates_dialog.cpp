// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/utils/string.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/gtk_widgets/dialogs/ask_to_check_for_updates_dialog.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

static void
response_cb (AdwAlertDialog * self, gchar * response, gpointer user_data)
{
  bool is_yes = string_is_equal (response, "yes");
  g_settings_set_boolean (S_P_GENERAL_UPDATES, "check-for-updates", is_yes);
  g_settings_set_boolean (S_GENERAL, "first-check-for-updates", false);

  if (is_yes)
    {
      zrythm_app->check_for_updates ();
    }
}

void
ask_to_check_for_updates_dialog_run_async (GtkWidget * parent)
{
  AdwAlertDialog * dialog =
    ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Check for Updates?"), nullptr));
  adw_alert_dialog_format_body (
    dialog, _ ("Do you want %s to check for updates on startup?"), PROGRAM_NAME);
  adw_alert_dialog_add_responses (
    dialog, "yes", _ ("_Yes"), "no", _ ("_No"), nullptr);
  adw_alert_dialog_set_response_appearance (
    ADW_ALERT_DIALOG (dialog), "yes", ADW_RESPONSE_SUGGESTED);

  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "yes");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "no");

  g_signal_connect (dialog, "response", G_CALLBACK (response_cb), dialog);

  adw_alert_dialog_choose (
    dialog, GTK_WIDGET (parent), nullptr, nullptr, nullptr);
}
