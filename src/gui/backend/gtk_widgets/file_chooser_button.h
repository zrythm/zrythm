// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_FILE_CHOOSER_BUTTON_H__
#define __GUI_WIDGETS_FILE_CHOOSER_BUTTON_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define FILE_CHOOSER_BUTTON_WIDGET_TYPE (file_chooser_button_widget_get_type ())
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

  // GtkFileChooserNative * file_chooser_native;
} FileChooserButtonWidget;

void
file_chooser_button_widget_setup (
  FileChooserButtonWidget * self,
  GtkWindow *               parent,
  const char *              title,
  GtkFileChooserAction      action);

/**
 * This must be called at the end of the
 * user-provided response callback.
 */
void
file_chooser_button_widget_std_response (
  FileChooserButtonWidget * self,
  GtkNativeDialog *         dialog,
  gint                      response_id);

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
