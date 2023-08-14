// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Export dialog.
 */

#include <stdio.h>

#include "dsp/engine.h"
#include "dsp/exporter.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  GENERIC_PROGRESS_DIALOG_WIDGET_TYPE)

static void
on_open_directory_clicked (
  GtkButton *                  btn,
  ExportProgressDialogWidget * self)
{
  char * dir = io_get_dir (self->info->file_uri);
  io_open_directory (dir);
  g_free (dir);
}

/**
 * Creates an export dialog widget and displays it.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportSettings * info,
  bool             autoclose,
  bool             show_open_dir_btn,
  bool             cancelable)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);

  ExportProgressDialogWidget * self =
    g_object_new (EXPORT_PROGRESS_DIALOG_WIDGET_TYPE, NULL);

  GenericProgressDialogWidget * generic_progress_dialog =
    Z_GENERIC_PROGRESS_DIALOG_WIDGET (self);

  self->info = info;

  generic_progress_dialog_widget_setup (
    generic_progress_dialog, _ ("Export Progress"),
    info->progress_info, _ ("Exporting..."), autoclose,
    cancelable);

  self->show_open_dir_btn = show_open_dir_btn;

  if (show_open_dir_btn)
    {
      self->open_directory = GTK_BUTTON (
        gtk_button_new_with_label (_ ("Open Directory")));
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->open_directory),
        _ ("Opens the containing directory"));
      g_signal_connect (
        G_OBJECT (self->open_directory), "clicked",
        G_CALLBACK (on_open_directory_clicked), self);

      generic_progress_dialog_add_button (
        generic_progress_dialog, self->open_directory, true,
        true);
    }

  return self;
}

static void
export_progress_dialog_widget_class_init (
  ExportProgressDialogWidgetClass * _klass)
{
}

static void
export_progress_dialog_widget_init (
  ExportProgressDialogWidget * self)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);
}
