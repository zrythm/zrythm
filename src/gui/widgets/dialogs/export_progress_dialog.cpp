// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
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
#include "utils/objects.h"
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

ExportData *
export_data_new (GtkWidget * parent_owner, ExportSettings * info)
{
  ExportData * data = object_new (ExportData);
  data->info = info;
  data->parent_owner = parent_owner;
  data->state = object_new (EngineState);
  return data;
}

static void
export_data_free (ExportData * data)
{
  object_zero_and_free (data->state);
  object_free_w_func_and_null (g_ptr_array_unref, data->tracks);
  object_free_w_func_and_null (export_settings_free, data->info);
  ;
  object_zero_and_free (data);
}

static void
on_open_directory_clicked (ExportProgressDialogWidget * self)
{
  char * dir = io_get_dir (self->data->info->file_uri);
  io_open_directory (dir);
  g_free (dir);
}

/**
 * Creates an export dialog widget and displays it.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportData *                      data,
  bool                              autoclose,
  ExportProgressDialogCloseCallback close_callback,
  bool                              show_open_dir_btn,
  bool                              cancelable)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);

  ExportProgressDialogWidget * self = Z_EXPORT_PROGRESS_DIALOG_WIDGET (
    g_object_new (EXPORT_PROGRESS_DIALOG_WIDGET_TYPE, NULL));

  GenericProgressDialogWidget * generic_progress_dialog =
    Z_GENERIC_PROGRESS_DIALOG_WIDGET (self);

  self->data = data;

  char * basename = g_path_get_basename (data->info->file_uri);
  char * exporting_msg = g_strdup_printf (_ ("Exporting %s"), basename);
  g_free_and_null (basename);
  generic_progress_dialog_widget_setup (
    generic_progress_dialog, _ ("Export Progress"), data->info->progress_info,
    exporting_msg, autoclose, (GenericCallback) close_callback, self->data,
    cancelable);
  g_free_and_null (exporting_msg);

  self->show_open_dir_btn = show_open_dir_btn;

  if (show_open_dir_btn)
    {
      generic_progress_dialog_add_response (
        generic_progress_dialog, "open-directory", _ ("Open Directory"),
        (GenericCallback) on_open_directory_clicked, self, true);
    }

  return self;
}

static void
export_progress_dialog_finalize (ExportProgressDialogWidget * self)
{
  object_free_w_func_and_null (export_data_free, self->data);

  G_OBJECT_CLASS (export_progress_dialog_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
export_progress_dialog_widget_class_init (
  ExportProgressDialogWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) export_progress_dialog_finalize;
}

static void
export_progress_dialog_widget_init (ExportProgressDialogWidget * self)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);
}
