// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Export progress dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__

#include "common/dsp/exporter.h"
#include "gui/backend/gtk_widgets/dialogs/generic_progress_dialog.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define EXPORT_PROGRESS_DIALOG_WIDGET_TYPE \
  (export_progress_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  Z,
  EXPORT_PROGRESS_DIALOG_WIDGET,
  GenericProgressDialogWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The export dialog.
 */
using ExportProgressDialogWidget = struct _ExportProgressDialogWidget
{
  GenericProgressDialogWidget parent_instance;

  /** Whether to show the open directory button or not. */
  bool show_open_dir_btn;

  std::shared_ptr<Exporter> exporter;
};

using ExportProgressDialogCloseCallback =
  std::function<void (std::shared_ptr<Exporter>)>;

/**
 * Creates an export dialog widget and displays it.
 *
 * @param data The dialog takes ownership of this object.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  std::shared_ptr<Exporter>                        exporter,
  bool                                             autoclose,
  std::optional<ExportProgressDialogCloseCallback> close_callback,
  bool                                             show_open_dir_btn,
  bool                                             cancelable);

/**
 * @}
 */

#endif
