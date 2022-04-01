/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_FIRST_RUN_DIALOG_H__
#define __GUI_WIDGETS_FIRST_RUN_DIALOG_H__

#include <gtk/gtk.h>

#define FIRST_RUN_DIALOG_WIDGET_TYPE \
  (first_run_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FirstRunDialogWidget, first_run_dialog_widget,
  Z, FIRST_RUN_DIALOG_WIDGET, GtkDialog)

#define FIRST_RUN_DIALOG_RESET_RESPONSE 450

typedef struct _FileChooserButtonWidget
  FileChooserButtonWidget;

typedef struct _FirstRunDialogWidget
{
  GtkDialog       parent_instance;

  GtkDropDown *   language_dropdown;
  FileChooserButtonWidget * fc_btn;

  GtkLabel *      lang_error_txt;
} FirstRunDialogWidget;

/**
 * Returns a new instance.
 */
FirstRunDialogWidget *
first_run_dialog_widget_new (
  GtkWindow * parent);

void
first_run_dialog_widget_reset (
  FirstRunDialogWidget * self);

void
first_run_dialog_widget_ok (
  FirstRunDialogWidget * self);

#endif
