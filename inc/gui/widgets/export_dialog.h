/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_EXPORT_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_DIALOG_H__

#include <gtk/gtk.h>

#define EXPORT_DIALOG_WIDGET_TYPE \
  (export_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (ExportDialogWidget,
                      export_dialog_widget,
                      Z,
                      EXPORT_DIALOG_WIDGET,
                      GtkDialog)

typedef struct _ExportDialogWidget
{
  GtkDialog            parent_instance;
  GtkButton *          cancel_button;
  GtkButton *          export_button;
  GtkEntry *           export_artist;
  GtkEntry *           export_genre;
  GtkComboBox *        filename_pattern;
  GtkListBox *         tracks;
  GtkToggleButton *    time_range_song;
  GtkToggleButton *    time_range_loop;
  GtkToggleButton *    time_range_custom;
  GtkComboBox *        format;
  GtkComboBox *        bit_depth;
  GtkCheckButton *     dither;
  GtkLabel *           output_label;
} ExportDialogWidget;

/**
 * Creates an export dialog widget and displays it.
 */
ExportDialogWidget *
export_dialog_widget_new ();

#endif
