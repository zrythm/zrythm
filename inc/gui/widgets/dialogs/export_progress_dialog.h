/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Export progress dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_PROGRESS_DIALOG_H__

#include <gtk/gtk.h>

#define EXPORT_PROGRESS_DIALOG_WIDGET_TYPE \
  (export_progress_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportProgressDialogWidget,
  export_progress_dialog_widget,
  Z, EXPORT_PROGRESS_DIALOG_WIDGET,
  GtkDialog)

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
  GtkDialog            parent_instance;

  GtkLabel *           label;
  GtkProgressBar *     progress_bar;
  GtkButton *          ok;
  GtkButton *          open_directory;
  GtkButton *          cancel;

  GtkButtonBox *       action_btn_box;

  /** Automatically close the progress dialog when
   * finished. */
  bool                 autoclose;

  /** Whether to show the open directory button or
   * not. */
  bool                 show_open_dir_btn;

  ExportSettings *     info;
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
