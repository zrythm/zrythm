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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/file_chooser_button.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  FileChooserButtonWidget,
  file_chooser_button_widget,
  GTK_TYPE_BOX)

#if 0
GtkFileChooser *
file_chooser_button_widget_get_file_chooser (
  FileChooserButtonWidget * self)
{
  return GTK_FILE_CHOOSER (self->file_chooser_native);
}
#endif

/**
 * @param predefined_path Used if passed, otherwise the
 *   path is fetched from the file chooser.
 */
static void
update_btn_label (
  FileChooserButtonWidget * self,
  const char *              predefined_path,
  GtkFileChooser *          fc)
{
  if (predefined_path)
    {
      gtk_button_set_label (
        self->button, predefined_path);
    }
  else if (fc)
    {
      char * path = z_gtk_file_chooser_get_filename (
        GTK_FILE_CHOOSER (fc));
      gtk_button_set_label (
        self->button,
        path ? path : _ ("Select path"));
      g_debug ("updated label to %s", path);
      g_free_and_null (path);
    }
}

static void
on_std_response (
  GtkNativeDialog * dialog,
  gint              response_id,
  gpointer          user_data)
{
  FileChooserButtonWidget * self =
    Z_FILE_CHOOSER_BUTTON_WIDGET (user_data);

  g_debug (
    "std response - setting label and hiding dialog");

  update_btn_label (
    self, NULL, GTK_FILE_CHOOSER (dialog));
  gtk_native_dialog_destroy (
    GTK_NATIVE_DIALOG (dialog));
}

void
file_chooser_button_widget_set_response_callback (
  FileChooserButtonWidget * self,
  GCallback                 callback,
  gpointer                  user_data,
  GClosureNotify            destroy_notify)
{
  self->response_cb = callback;
  self->user_data = user_data;
  self->destroy_notify = destroy_notify;
}

static void
on_btn_clicked (
  GtkButton *               btn,
  FileChooserButtonWidget * self)
{
  GtkFileChooserNative * fc_native =
    GTK_FILE_CHOOSER_NATIVE (
      gtk_file_chooser_native_new (
        self->title, NULL, self->action, NULL, NULL));

  if (self->path)
    {
      z_gtk_file_chooser_set_file_from_path (
        GTK_FILE_CHOOSER (fc_native), self->path);
    }
  if (self->current_dir)
    {
      GFile * gfile =
        g_file_new_for_path (self->current_dir);
      gtk_file_chooser_set_current_folder (
        GTK_FILE_CHOOSER (fc_native), gfile, NULL);
      g_object_unref (gfile);
    }
  gtk_native_dialog_set_modal (
    GTK_NATIVE_DIALOG (fc_native), true);
  gtk_native_dialog_set_transient_for (
    GTK_NATIVE_DIALOG (fc_native), self->parent);
  gtk_native_dialog_show (
    GTK_NATIVE_DIALOG (fc_native));
  g_signal_connect (
    G_OBJECT (fc_native), "response",
    G_CALLBACK (on_std_response), self);
  if (self->response_cb)
    {
      if (self->destroy_notify)
        {
          g_signal_connect_data (
            G_OBJECT (fc_native), "response",
            G_CALLBACK (self->response_cb),
            self->user_data,
            (GClosureNotify) self->destroy_notify,
            G_CONNECT_AFTER);
        }
      else
        {
          g_signal_connect (
            G_OBJECT (fc_native), "response",
            self->response_cb, self->user_data);
        }
    }
}

void
file_chooser_button_widget_set_current_dir (
  FileChooserButtonWidget * self,
  const char *              dir)
{
  g_free_and_null (self->current_dir);
  self->current_dir = g_strdup (dir);
}

void
file_chooser_button_widget_set_path (
  FileChooserButtonWidget * self,
  const char *              path)
{
  g_debug ("setting path to %s", path);
  g_free_and_null (self->path);
  self->path = g_strdup (path);

  update_btn_label (self, path, NULL);
}

void
file_chooser_button_widget_setup (
  FileChooserButtonWidget * self,
  GtkWindow *               parent,
  const char *              title,
  GtkFileChooserAction      action)
{
  self->title = g_strdup (title);
  self->action = action;
  self->parent = parent;
}

FileChooserButtonWidget *
file_chooser_button_widget_new (
  GtkWindow *          parent,
  const char *         title,
  GtkFileChooserAction action)
{
  FileChooserButtonWidget * self = g_object_new (
    FILE_CHOOSER_BUTTON_WIDGET_TYPE, NULL);

  file_chooser_button_widget_setup (
    self, parent, title, action);

  return self;
}

static void
finalize (FileChooserButtonWidget * self)
{
  G_OBJECT_CLASS (
    file_chooser_button_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
file_chooser_button_widget_class_init (
  FileChooserButtonWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);

  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
file_chooser_button_widget_init (
  FileChooserButtonWidget * self)
{
  self->button = GTK_BUTTON (gtk_button_new ());
  gtk_box_append (
    GTK_BOX (self), GTK_WIDGET (self->button));
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->button), true);

  g_signal_connect (
    G_OBJECT (self->button), "clicked",
    G_CALLBACK (on_btn_clicked), self);

  /* add class */
  gtk_widget_add_css_class (
    GTK_WIDGET (self), "file-chooser-button");
}
