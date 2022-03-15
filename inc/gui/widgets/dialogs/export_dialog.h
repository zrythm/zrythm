/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
 * Export dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_DIALOG_H__

#include <gtk/gtk.h>

#define EXPORT_DIALOG_WIDGET_TYPE \
  (export_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportDialogWidget,
  export_dialog_widget,
  Z, EXPORT_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum ExportFilenamePattern
{
  EFP_APPEND_FORMAT,
  EFP_PREPEND_DATE_APPEND_FORMAT,
} ExportFilenamePattern;

/**
 * The export dialog.
 */
typedef struct _ExportDialogWidget
{
  GtkDialog            parent_instance;
  GtkButton *          cancel_button;
  GtkButton *          export_button;
  GtkEntry *           export_artist;
  GtkEntry *           export_title;
  GtkEntry *           export_genre;
  GtkComboBox *        filename_pattern;
  GtkToggleButton *    time_range_song;
  GtkToggleButton *    time_range_loop;
  GtkToggleButton *    time_range_custom;
  GtkComboBox *        format;
  GtkComboBox *        bit_depth;
  GtkCheckButton *     lanes_as_tracks;
  GtkToggleButton *    dither;
  GtkLabel *           output_label;

  GtkTreeView *        tracks_treeview;
  GtkTreeModel *       tracks_model;

  /** Underlying model of @ref
   * ExportDialogWidget.tracks_model. */
  GtkTreeStore *       tracks_store;

  GtkToggleButton *    mixdown_toggle;
  GtkToggleButton *    stems_toggle;
} ExportDialogWidget;

/**
 * Creates an export dialog widget and displays it.
 */
ExportDialogWidget *
export_dialog_widget_new (void);

/**
 * @}
 */

#endif
