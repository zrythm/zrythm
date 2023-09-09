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
GtkDialog *
dialogs_get_overwrite_plugin_dialog (GtkWindow * parent)
{
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkDialog *    dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (
    _ ("Overwrite Plugin"), GTK_WINDOW (MAIN_WINDOW), flags, _ ("_OK"),
    GTK_RESPONSE_ACCEPT, _ ("_Cancel"), GTK_RESPONSE_REJECT, NULL));
  GtkWidget *    box = gtk_dialog_get_content_area (dialog);
  GtkWidget *    label = g_object_new (
    GTK_TYPE_LABEL, "label",
    _ ("A plugin already exists at the selected "
             "slot. Overwrite it?"),
    "margin-start", 8, "margin-end", 8, "margin-top", 4, "margin-bottom", 8,
    NULL);
  gtk_widget_set_visible (label, 1);
  gtk_box_append (GTK_BOX (box), label);
  return dialog;
}

GtkDialog *
dialogs_get_error_instantiating_plugin_dialog (GtkWindow * parent)
{
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *    dialog = gtk_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
    _ ("Error instantiating plugin. "
             "Please see log for details."));
  return GTK_DIALOG (dialog);
}
