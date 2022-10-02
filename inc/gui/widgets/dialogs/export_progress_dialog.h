// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Export progress dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__

#include "gui/widgets/dialogs/generic_progress_dialog.h"

#include <gtk/gtk.h>

#define EXPORT_PROGRESS_DIALOG_WIDGET_TYPE \
  (export_progress_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  Z,
  EXPORT_PROGRESS_DIALOG_WIDGET,
  GenericProgressDialogWidget)

typedef struct ExportSettings ExportSettings;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The export dialog.
 */
typedef struct _ExportProgressDialogWidget
{
  GenericProgressDialogWidget parent_instance;

  GtkButton * open_directory;

  /** Whether to show the open directory button or
   * not. */
  bool show_open_dir_btn;

  ExportSettings * info;
} ExportProgressDialogWidget;

/**
 * Creates an export dialog widget and displays it.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportSettings * info,
  bool             autoclose,
  bool             show_open_dir_btn,
  bool             cancelable);

/**
 * @}
 */

#endif
