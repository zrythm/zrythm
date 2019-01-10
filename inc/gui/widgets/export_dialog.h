/*
 * gui/widgets/export_dialog.h - Export audio dialog
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
  GtkButton            * cancel_button;
  GtkButton            * export_button;
  GtkComboBox          * pattern_cb;
  GtkListBox           * tracks_list;
  GtkButton            * arrangement_button;
  GtkBox               * start_box;
  GtkButton            * loop_button;
  GtkBox               * end_box;
  GtkComboBox          * format_cb;
  GtkCheckButton       * dither_check;
  GtkLabel             * output_label;
} ExportDialogWidget;

/**
 * Creates a export_dialog widget and displays it.
 */
ExportDialogWidget *
export_dialog_widget_new ();

#endif
