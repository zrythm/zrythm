/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_FILE_CHOOSER_BUTTON_H__
#define __GUI_WIDGETS_FILE_CHOOSER_BUTTON_H__

#include <gtk/gtk.h>

#define FILE_CHOOSER_BUTTON_WIDGET_TYPE \
  (file_chooser_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileChooserButtonWidget,
  file_chooser_button_widget,
  Z,
  FILE_CHOOSER_BUTTON_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _FileChooserButtonWidget
{
  GtkBox parent_instance;

  GtkButton * button;

  /* TODO free */
  char * title;

  char * current_dir;
  char * path;

  GtkFileChooserAction action;
  GtkWindow *          parent;

  GCallback      response_cb;
  gpointer       user_data;
  GClosureNotify destroy_notify;

  //GtkFileChooserNative * file_chooser_native;
} FileChooserButtonWidget;

void
file_chooser_button_widget_setup (
  FileChooserButtonWidget * self,
  GtkWindow *               parent,
  const char *              title,
  GtkFileChooserAction      action);

void
file_chooser_button_widget_set_response_callback (
  FileChooserButtonWidget * self,
  GCallback                 callback,
  gpointer                  user_data,
  GClosureNotify            destroy_notify);

void
file_chooser_button_widget_set_current_dir (
  FileChooserButtonWidget * self,
  const char *              dir);

void
file_chooser_button_widget_set_path (
  FileChooserButtonWidget * self,
  const char *              path);

FileChooserButtonWidget *
file_chooser_button_widget_new (
  GtkWindow *          parent,
  const char *         title,
  GtkFileChooserAction action);

#if 0
GtkFileChooser *
file_chooser_button_widget_get_file_chooser (
  FileChooserButtonWidget * self);
#endif

/**
 * @}
 */

#endif
