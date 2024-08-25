// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/exporter.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "project.h"
#include "utils/io.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  GENERIC_PROGRESS_DIALOG_WIDGET_TYPE)

static void
on_open_directory_clicked (ExportProgressDialogWidget * self)
{
  auto dir = io_get_dir (self->exporter->settings_.file_uri_);
  io_open_directory (dir.c_str ());
}

/**
 * Creates an export dialog widget and displays it.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  std::shared_ptr<Exporter>                        exporter,
  bool                                             autoclose,
  std::optional<ExportProgressDialogCloseCallback> close_callback,
  bool                                             show_open_dir_btn,
  bool                                             cancelable)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);

  ExportProgressDialogWidget * self = Z_EXPORT_PROGRESS_DIALOG_WIDGET (
    g_object_new (EXPORT_PROGRESS_DIALOG_WIDGET_TYPE, nullptr));

  GenericProgressDialogWidget * generic_progress_dialog =
    Z_GENERIC_PROGRESS_DIALOG_WIDGET (self);

  self->exporter = exporter;

  char * basename = g_path_get_basename (exporter->settings_.file_uri_.c_str ());
  auto exporting_msg = format_str (_ ("Exporting %s"), basename);
  g_free_and_null (basename);
  std::optional<GenericCallback> generic_close_callback;
  if (close_callback.has_value ())
    {
      generic_close_callback = [close_callback, exporter] () {
        close_callback.value () (exporter);
      };
    }
  generic_progress_dialog_widget_setup (
    generic_progress_dialog, _ ("Export Progress"),
    self->exporter->progress_info_.get (), exporting_msg.c_str (), autoclose,
    generic_close_callback, cancelable);

  self->show_open_dir_btn = show_open_dir_btn;

  if (show_open_dir_btn)
    {
      generic_progress_dialog_add_response (
        generic_progress_dialog, "open-directory", _ ("Open Directory"),
        [self] () { on_open_directory_clicked (self); }, true);
    }

  return self;
}

static void
export_progress_dialog_finalize (ExportProgressDialogWidget * self)
{
  self->exporter.~shared_ptr ();

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

  new (&self->exporter) std::shared_ptr<Exporter> ();
}
