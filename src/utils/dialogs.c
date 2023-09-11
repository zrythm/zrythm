// SPDX-FileCopyrightText: © 2018-2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/dialogs.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

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
