// clang-format off
// SPDX-FileCopyrightText: © 2018-2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/dialogs.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

/**
 * Creates and returns the overwrite plugin dialog.
 */
AdwMessageDialog *
dialogs_get_overwrite_plugin_dialog (GtkWindow * parent)
{
  AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
    parent, _ ("Overwrite Plugin?"),
    _ ("A plugin already exists at the selected slot. Overwrite it?")));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "overwrite",
    _ ("_Overwrite"), NULL);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "overwrite", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  return dialog;
}

AdwMessageDialog *
dialogs_get_basic_ok_message_dialog (GtkWindow * parent_window)
{
  GtkWidget * dialog = adw_message_dialog_new (parent_window, NULL, NULL);
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "ok", _ ("_OK"), NULL);
  adw_message_dialog_set_default_response (ADW_MESSAGE_DIALOG (dialog), "ok");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "ok");
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "zrythm");
  if (parent_window)
    {
      gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
      gtk_window_set_modal (GTK_WINDOW (dialog), true);
    }
  return ADW_MESSAGE_DIALOG (dialog);
}
