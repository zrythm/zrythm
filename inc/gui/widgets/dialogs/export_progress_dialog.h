// clang-format off
// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Export progress dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__

#include "gui/widgets/dialogs/generic_progress_dialog.h"

#include "gtk_wrapper.h"

#define EXPORT_PROGRESS_DIALOG_WIDGET_TYPE \
  (export_progress_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  Z,
  EXPORT_PROGRESS_DIALOG_WIDGET,
  GenericProgressDialogWidget)

TYPEDEF_STRUCT (ExportSettings);
TYPEDEF_STRUCT (EngineState);

/**
 * Passed around when exporting asynchronously.
 */
typedef struct ExportData
{
  bool export_stems;

  /** Not owned by this instance. */
  GThread * thread;

  /** Owned by this instance. */
  GPtrArray * tracks;
  size_t      cur_track;

  /** Owned by this instance. */
  ExportSettings * info;

  /** Owned by this instance. */
  EngineState * state;

  /** Not owned by this instance. */
  GPtrArray * conns;

  /** Not owned by this instance. */
  GtkWidget * parent_owner;
} ExportData;

/**
 * @param info ExportData takes ownership of this object.
 */
ExportData *
export_data_new (GtkWidget * parent_owner, ExportSettings * info);

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

  /** Whether to show the open directory button or not. */
  bool show_open_dir_btn;

  ExportData * data;
} ExportProgressDialogWidget;

typedef void (*ExportProgressDialogCloseCallback) (ExportData * data);

/**
 * Creates an export dialog widget and displays it.
 *
 * @param data The dialog takes ownership of this object.
 */
ExportProgressDialogWidget *
export_progress_dialog_widget_new (
  ExportData *                      data,
  bool                              autoclose,
  ExportProgressDialogCloseCallback close_callback,
  bool                              show_open_dir_btn,
  bool                              cancelable);

/**
 * @}
 */

#endif
